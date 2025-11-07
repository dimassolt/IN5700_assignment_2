#include <omnetpp.h>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include "messages_m.h"

using namespace omnetpp;
using namespace garbage_collection;

namespace {
cTextFigure *requireTextFigure(cModule *module, const char *name)
{
    if (!module)
        throw cRuntimeError("Null module while searching for figure '%s'", name);

    cCanvas *canvas = module->getParentModule()->getCanvas();
    if (!canvas)
        throw cRuntimeError("%s: canvas unavailable while searching for figure '%s'", module->getFullPath().c_str(), name);

    std::function<cTextFigure *(cFigure *)> search = [&](cFigure *figure) -> cTextFigure * {
        if (!figure)
            return nullptr;
        if (strcmp(figure->getName(), name) == 0)
            return dynamic_cast<cTextFigure *>(figure);
        for (int i = 0; i < figure->getNumFigures(); ++i) {
            if (auto *child = search(figure->getFigure(i)))
                return child;
        }
        return nullptr;
    };

    if (auto *result = search(canvas->getRootFigure()))
        return result;

    throw cRuntimeError("%s: unable to locate text figure '%s'", module->getFullPath().c_str(), name);
}

const char *kStatusCommandFor(int canId, bool isFull)
{
    if (canId == 0)
        return isFull ? "3-YES" : "2-NO";
    return isFull ? "6-YES" : "5-NO";
}

const char *kCollectCommandFor(int canId)
{
    return canId == 0 ? "7-Collect garbage" : "9-Collect garbage";
}

void incrementParentCounter(cModule *module, const char *parName)
{
    if (!module)
        return;
    if (cModule *parent = module->getParentModule()) {
        if (parent->hasPar(parName)) {
            const int current = parent->par(parName).intValue();
            parent->par(parName).setIntValue(current + 1);
        }
    }
}

void setParentIntParameter(cModule *module, const char *parName, int value)
{
    if (!module)
        return;
    if (cModule *parent = module->getParentModule()) {
        if (parent->hasPar(parName))
            parent->par(parName).setIntValue(value);
    }
}
}

class GarbageCan : public cSimpleModule {
  private:
    bool hasGarbage = false;
    int canId = 0;
    simtime_t responseDelay;
    bool reportStatusToCloud = false;
    bool sendCollectToCloud = false;
    int lostQueryCount = 3;
    simtime_t collectDispatchDelay;

    int lostQueriesSeen = 0;
    bool collectDispatched = false;

    long sentFastTotal = 0;
    long rcvdFastTotal = 0;
    long lostFastTotal = 0;

    std::map<std::string, long> sentFastMessages;
    std::map<std::string, long> receivedFastMessages;
    std::map<std::string, long> lostFastMessages;

    cTextFigure *counterFigure = nullptr;

    static void noteMessage(std::map<std::string, long> &bucket, const char *command)
    {
        const char *label = (command && *command) ? command : "<unknown>";
        bucket[label]++;
    }

    static std::string describeMessages(const std::map<std::string, long> &bucket, const std::map<std::string, long> *counterpart = nullptr)
    {
        if (bucket.empty())
            return "  none";

        std::ostringstream oss;
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            const std::string &command = it->first;
            const long count = it->second;
            long counterpartCount = 0;
            if (counterpart) {
                auto found = counterpart->find(command);
                if (found != counterpart->end())
                    counterpartCount = found->second;
            }
            const long pending = counterpart ? (count - counterpartCount) : 0;
            if (it != bucket.begin())
                oss << "\n";
            oss << "  " << command << " x" << count;
            if (counterpart && counterpartCount > 0)
                oss << " (matched " << counterpartCount << ")";
            if (counterpart && pending > 0)
                oss << " (pending " << pending << ")";
        }
        return oss.str();
    }

    std::string formatStatusText() const
    {
        std::ostringstream oss;
        const std::string prefix = (canId == 0) ? "Can" : "AnotherCan";
        oss << "sent" << prefix << "Fast: " << sentFastTotal
            << " rcvd" << prefix << "Fast: " << rcvdFastTotal
            << " numberOfLost" << prefix << "Msgs: " << lostFastTotal;
        return oss.str();
    }

    bool isQueryCommand(const char *command) const
    {
        return strcmp(command, "1-Is the can full?") == 0 || strcmp(command, "4-Is the can full?") == 0;
    }

    void recordSentFast(const char *command)
    {
        ++sentFastTotal;
        noteMessage(sentFastMessages, command);
        updateCounterFigure();
    }

    void recordRcvdFast(const char *command)
    {
        ++rcvdFastTotal;
        noteMessage(receivedFastMessages, command);
        updateCounterFigure();
    }

    void recordLostFast(const char *command)
    {
        ++lostFastTotal;
        noteMessage(lostFastMessages, command);
        updateCounterFigure();
    }

    void updateCounterFigure()
    {
        if (!counterFigure)
            return;

        counterFigure->setVisible(true);
        const std::string text = formatStatusText();
        counterFigure->setText(text.c_str());
        if (cModule *parent = getParentModule()) {
            const char *parName = (canId == 0) ? "canCountersText" : "anotherCanCountersText";
            if (parent->hasPar(parName))
                parent->par(parName).setStringValue(text.c_str());
        }
    }

    void dispatchStatus()
    {
        auto *reply = new GarbagePacket(hasGarbage ? "Yes" : "No");
        reply->setCommand(kStatusCommandFor(canId, hasGarbage));
        reply->setCanId(canId);
        reply->setIsFull(hasGarbage);
        reply->setTravelTime(SIMTIME_DBL(responseDelay));

        recordSentFast(reply->getCommand());
        sendDelayed(reply, responseDelay, "out");

        if (reportStatusToCloud && gate("outCloud")->isConnected()) {
            auto *cloudReport = reply->dup();
            cloudReport->setName("garbage-status-cloud");
            cloudReport->setNote("direct-report");
            recordSentFast(cloudReport->getCommand());
            sendDelayed(cloudReport, responseDelay, "outCloud");
        }
    }

    void dispatchCollectIfNeeded()
    {
        if (!sendCollectToCloud || collectDispatched || !hasGarbage || !gate("outCloud")->isConnected())
            return;

        auto *collect = new GarbagePacket("Collect can garbage");
        collect->setCommand(kCollectCommandFor(canId));
        collect->setCanId(canId);
        collect->setIsFull(true);
        collect->setTravelTime(SIMTIME_DBL(collectDispatchDelay));
        collect->setNote("fog-direct");
        recordSentFast(collect->getCommand());
        sendDelayed(collect, collectDispatchDelay, "outCloud");
        incrementParentCounter(this, canId == 0 ? "canCollectCount" : "anotherCanCollectCount");
        collectDispatched = true;
        EV_INFO << "Can " << canId << " dispatched collect request to cloud" << endl;
    }

  protected:
    void initialize() override
    {
        hasGarbage = par("hasGarbage");
        canId = par("canId");
        responseDelay = par("responseDelay");
        reportStatusToCloud = par("reportStatusToCloud");
        sendCollectToCloud = par("sendCollectToCloud");
        lostQueryCount = par("lostQueryCount");
        collectDispatchDelay = par("collectDispatchDelay");

        std::string communicationMode;
        if (cModule *parent = getParentModule()) {
            if (parent->hasPar("communicationMode"))
                communicationMode = parent->par("communicationMode").stdstringValue();
        }
        if (communicationMode == "GarbageInTheCansAndSlow" || communicationMode == "GarbageInTheCansAndFast")
            hasGarbage = true;
        if (communicationMode == "GarbageInTheCansAndFast")
            sendCollectToCloud = true;
        else if (communicationMode == "GarbageInTheCansAndSlow")
            sendCollectToCloud = false;

        counterFigure = requireTextFigure(this, canId == 0 ? "canCounters" : "anotherCanCounters");
        updateCounterFigure();
    }

    void handleMessage(cMessage *msg) override
    {
        auto *pkt = check_and_cast<GarbagePacket *>(msg);
        const char *command = pkt->getCommand();

        if (isQueryCommand(command)) {
            if (lostQueriesSeen < lostQueryCount) {
                ++lostQueriesSeen;
                EV_INFO << "GarbageCan " << canId << " dropping query attempt " << lostQueriesSeen << endl;
                bubble("Lost Message");
                recordLostFast(command);
                delete pkt;
                return;
            }
            else {
                recordRcvdFast(command);
            }

            EV_INFO << "GarbageCan " << canId << " processing query command" << endl;
            dispatchStatus();
            dispatchCollectIfNeeded();
        }
        else if (strcmp(command, "8-OK") == 0 || strcmp(command, "10-OK") == 0) {
            recordRcvdFast(command);
            EV_INFO << "Cloud acknowledged collect request for can " << canId
                    << ": " << (pkt->getNote() ? pkt->getNote() : "") << endl;
            incrementParentCounter(this, canId == 0 ? "canCollectAckCount" : "anotherCanCollectAckCount");
        }
        else if (strcmp(command, "cloud-ack") == 0) {
            recordRcvdFast(command);
            EV_INFO << "Cloud acknowledged status for can " << canId
                    << ": " << (pkt->getNote() ? pkt->getNote() : "") << endl;
        }
        else {
            EV_WARN << "GarbageCan " << canId << " received unknown command '" << command << "'" << endl;
        }

        delete pkt;
    }

    void refreshDisplay() const override
    {
        const_cast<GarbageCan *>(this)->updateCounterFigure();
    }

    void finish() override
    {
        setParentIntParameter(this,
            canId == 0 ? "canLostQueriesFinal" : "anotherCanLostQueriesFinal",
            lostQueriesSeen);
    }
};
Define_Module(GarbageCan);
