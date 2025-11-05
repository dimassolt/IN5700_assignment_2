#include <omnetpp.h>
#include <cstring>
#include <functional>
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

    long sentFastCount = 0;
    long rcvdFastCount = 0;
    long lostFastCount = 0;

    cTextFigure *counterFigure = nullptr;
    const char *counterLabel = "Can";

    std::string formatStatusText() const
    {
        std::ostringstream oss;
        oss << counterLabel << "\n"
            << "Fast -> sent: " << sentFastCount
            << ", received: " << rcvdFastCount
            << ", lost: " << lostFastCount << "\n"
            << "Slow -> sent: 0, received: 0, lost: 0";
        return oss.str();
    }

    bool isQueryCommand(const char *command) const
    {
        return strcmp(command, "1-Is the can full?") == 0 || strcmp(command, "4-Is the can full?") == 0;
    }

    void recordSentFast()
    {
        ++sentFastCount;
        updateCounterFigure();
    }

    void recordRcvdFast()
    {
        ++rcvdFastCount;
        updateCounterFigure();
    }

    void recordLostFast()
    {
        ++lostFastCount;
        updateCounterFigure();
    }

    void updateCounterFigure()
    {
        if (!counterFigure)
            return;

        counterFigure->setVisible(true);
        const std::string text = formatStatusText();
        counterFigure->setText(text.c_str());
    }

    void dispatchStatus()
    {
        auto *reply = new GarbagePacket(hasGarbage ? "Yes" : "No");
        reply->setCommand(kStatusCommandFor(canId, hasGarbage));
        reply->setCanId(canId);
        reply->setIsFull(hasGarbage);
        reply->setTravelTime(SIMTIME_DBL(responseDelay));

        recordSentFast();
        sendDelayed(reply, responseDelay, "out");

        if (reportStatusToCloud && gate("outCloud")->isConnected()) {
            auto *cloudReport = reply->dup();
            cloudReport->setName("garbage-status-cloud");
            cloudReport->setNote("direct-report");
            recordSentFast();
            sendDelayed(cloudReport, responseDelay, "outCloud");
        }
    }

    void dispatchCollectIfNeeded()
    {
        if (!sendCollectToCloud || collectDispatched || !hasGarbage || !gate("outCloud")->isConnected())
            return;

        auto *collect = new GarbagePacket("collect-request");
        collect->setCommand(kCollectCommandFor(canId));
        collect->setCanId(canId);
        collect->setIsFull(true);
        collect->setTravelTime(SIMTIME_DBL(collectDispatchDelay));
        collect->setNote("fog-direct");
        recordSentFast();
        sendDelayed(collect, collectDispatchDelay, "outCloud");
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

        counterFigure = requireTextFigure(this, canId == 0 ? "canCounters" : "anotherCanCounters");
        counterLabel = (canId == 0) ? "Can" : "AnotherCan";
        updateCounterFigure();
    }

    void handleMessage(cMessage *msg) override
    {
        auto *pkt = check_and_cast<GarbagePacket *>(msg);
        const char *command = pkt->getCommand();

        if (isQueryCommand(command)) {
            recordRcvdFast();
            if (lostQueriesSeen < lostQueryCount) {
                ++lostQueriesSeen;
                EV_INFO << "GarbageCan " << canId << " dropping query attempt " << lostQueriesSeen << endl;
                bubble("Lost Message");
                recordLostFast();
                delete pkt;
                return;
            }

            EV_INFO << "GarbageCan " << canId << " processing query command" << endl;
            dispatchStatus();
            dispatchCollectIfNeeded();
        }
        else if (strcmp(command, "8-OK") == 0 || strcmp(command, "10-OK") == 0) {
            recordRcvdFast();
            EV_INFO << "Cloud acknowledged collect request for can " << canId
                    << ": " << (pkt->getNote() ? pkt->getNote() : "") << endl;
        }
        else if (strcmp(command, "cloud-ack") == 0) {
            recordRcvdFast();
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
};
Define_Module(GarbageCan);
