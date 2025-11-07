#include <omnetpp.h>
#include <algorithm>
#include <array>
#include <deque>
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

const char *kQueryCommandFor(int canId)
{
    return canId == 0 ? "1-Is the can full?" : "4-Is the can full?";
}

const char *kCollectCommandFor(int canId)
{
    return canId == 0 ? "7-Collect garbage" : "9-Collect garbage";
}

const char *kCollectAckCommandFor(int canId)
{
    return canId == 0 ? "8-OK" : "10-OK";
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

class GarbageCollector : public cSimpleModule {
  private:
    static constexpr int kUnknownState = -1;

    cMessage *startEvent = nullptr;
    std::array<cMessage *, 2> retryEvents {nullptr, nullptr};
    std::array<int, 2> canStates {kUnknownState, kUnknownState};
    std::array<int, 2> attemptCounters {0, 0};
    std::array<bool, 2> awaitingCollectAck {false, false};
    std::array<bool, 2> collectSent {false, false};
    std::deque<int> collectQueue;
    bool pendingSecondCanQuery = false;

    bool hostSendsCollect = true;
    bool expectCloudAck = true;
    simtime_t retryInterval;
    int maxQueryAttempts = 4;
    bool inspectionComplete = false;

    std::string communicationMode;

    long sentHostFast = 0;
    long rcvdHostFast = 0;
    long sentHostSlow = 0;
    long rcvdHostSlow = 0;

    std::map<std::string, long> sentFastMessages;
    std::map<std::string, long> receivedFastMessages;
    std::map<std::string, long> sentSlowMessages;
    std::map<std::string, long> receivedSlowMessages;

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
            long other = 0;
            if (counterpart) {
                auto found = counterpart->find(command);
                if (found != counterpart->end())
                    other = found->second;
            }
            const long diff = counterpart ? (count - other) : 0;
            if (it != bucket.begin())
                oss << "\n";
            oss << "  " << command << " x" << count;
            if (counterpart) {
                if (other > 0)
                    oss << " (matched " << other << ")";
                if (diff > 0)
                    oss << " (pending " << diff << ")";
            }
        }
        return oss.str();
    }

    std::string formatStatusText() const
    {
        std::ostringstream oss;
        oss << "sentHostFast: " << sentHostFast
            << " rcvdHostFast: " << rcvdHostFast
            << " sentHostSlow: " << sentHostSlow
            << " rcvdHostSlow: " << rcvdHostSlow;
        return oss.str();
    }

    void updateHostCountersFigure()
    {
        if (!counterFigure)
            return;

        counterFigure->setVisible(true);
        const std::string text = formatStatusText();
        counterFigure->setText(text.c_str());
        if (cModule *parent = getParentModule()) {
            if (parent->hasPar("hostCountersText"))
                parent->par("hostCountersText").setStringValue(text.c_str());
        }
    }

    bool hasPendingCollectAck() const
    {
        return std::any_of(awaitingCollectAck.begin(), awaitingCollectAck.end(), [](bool awaiting) { return awaiting; });
    }

    void processCollectQueue()
    {
        if (!hostSendsCollect || !gate("outCloud")->isConnected())
            return;

        if (hasPendingCollectAck())
            return;

        while (!collectQueue.empty()) {
            int canId = collectQueue.front();
            collectQueue.pop_front();

            sendCollectRequest(canId);

            if (expectCloudAck)
                break;
        }
    }

    void enqueueCollect(int canId)
    {
        if (canId < 0 || canId >= static_cast<int>(collectSent.size()))
            return;

        if (collectSent[canId])
            return;

        collectSent[canId] = true;
        collectQueue.push_back(canId);
        processCollectQueue();
    }


    void recordHostFastSend(const char *command)
    {
        ++sentHostFast;
        noteMessage(sentFastMessages, command);
        updateHostCountersFigure();
    }

    void recordHostFastReceive(const char *command)
    {
        ++rcvdHostFast;
        noteMessage(receivedFastMessages, command);
        updateHostCountersFigure();
    }

    void recordHostSlowSend(const char *command)
    {
        ++sentHostSlow;
        noteMessage(sentSlowMessages, command);
        updateHostCountersFigure();
    }

    void recordHostSlowReceive(const char *command)
    {
        ++rcvdHostSlow;
        noteMessage(receivedSlowMessages, command);
        updateHostCountersFigure();
    }

    void ensureRetryEvent(int canId)
    {
        if (!retryEvents[canId]) {
            std::ostringstream name;
            name << "retry-can" << canId;
            retryEvents[canId] = new cMessage(name.str().c_str());
        }
    }

    void scheduleQuery(int canId, simtime_t when)
    {
        ensureRetryEvent(canId);
        cancelEvent(retryEvents[canId]);
        scheduleAt(std::max(simTime(), when), retryEvents[canId]);
    }

    void attemptQuery(int canId)
    {
        if (canId < 0 || canId >= static_cast<int>(canStates.size()))
            throw cRuntimeError("Invalid can id %d", canId);

        if (canStates[canId] != kUnknownState)
            return;

        attemptCounters[canId]++;
        if (attemptCounters[canId] > maxQueryAttempts) {
            EV_WARN << "Reached max query attempts for can " << canId << " without response" << endl;
            return;
        }

        auto *query = new GarbagePacket("Is the can full?");
        query->setCommand(kQueryCommandFor(canId));
        query->setCanId(canId);
        query->setIsFull(false);
        query->setTravelTime(0);
        const std::string attemptNote = "attempt=" + std::to_string(attemptCounters[canId]);
        query->setNote(attemptNote.c_str());

        if (canId == 0) {
            recordHostFastSend(query->getCommand());
            send(query, "outCan");
        }
        else {
            recordHostFastSend(query->getCommand());
            send(query, "outAnotherCan");
        }

        EV_INFO << "Sent query attempt " << attemptCounters[canId] << " to can " << canId << endl;

        if (canStates[canId] == kUnknownState && attemptCounters[canId] < maxQueryAttempts)
            scheduleQuery(canId, simTime() + retryInterval);
    }

    void handleStatus(GarbagePacket *pkt)
    {
        const int canId = pkt->getCanId();
        if (canId < 0 || canId >= static_cast<int>(canStates.size())) {
            EV_WARN << "Received status for unknown can " << canId << endl;
            return;
        }

        const bool isFull = pkt->isFull();
        const bool firstObservation = (canStates[canId] == kUnknownState);
        canStates[canId] = isFull ? 1 : 0;

        EV_INFO << "Can " << canId << " reported " << (isFull ? "full" : "empty")
                << " after " << attemptCounters[canId] << " attempts" << endl;

        ensureRetryEvent(canId);
        if (retryEvents[canId]->isScheduled())
            cancelEvent(retryEvents[canId]);

        if (isFull && hostSendsCollect && gate("outCloud")->isConnected())
            enqueueCollect(canId);

        if (firstObservation && canId == 0 && attemptCounters[1] == 0) {
            const bool shouldDeferSecondQuery = isFull && hostSendsCollect && expectCloudAck
                && gate("outCloud")->isConnected();
            if (shouldDeferSecondQuery) {
                pendingSecondCanQuery = true;
            }
            else {
                scheduleQuery(1, simTime() + retryInterval);
            }
        }

        finalizeInspection();
    }

    bool allCansKnown() const
    {
        return std::find(canStates.begin(), canStates.end(), kUnknownState) == canStates.end();
    }

    void finalizeInspection()
    {
        if (inspectionComplete || !allCansKnown())
            return;

        inspectionComplete = true;

        EV_INFO << "Inspection complete: can0="
                << (canStates[0] == 1 ? "full" : "empty")
                << ", can1=" << (canStates[1] == 1 ? "full" : "empty") << endl;

        if (!hostSendsCollect || !gate("outCloud")->isConnected())
            return;

        for (int canId = 0; canId < static_cast<int>(canStates.size()); ++canId) {
            if (canStates[canId] == 1)
                enqueueCollect(canId);
        }
    }

    void sendCollectRequest(int canId)
    {
        collectSent[canId] = true;

        auto *collect = new GarbagePacket("Collect garbage");
        collect->setCommand(kCollectCommandFor(canId));
        collect->setCanId(canId);
        collect->setIsFull(true);
        collect->setTravelTime(0);
        collect->setNote(communicationMode.c_str());
        recordHostSlowSend(collect->getCommand());
        send(collect, "outCloud");
        incrementParentCounter(this, "hostCollectCount");
        EV_INFO << "Sent collect request for can " << canId << " to the cloud" << endl;

        if (expectCloudAck)
            awaitingCollectAck[canId] = true;
    }

    void handleCloudAck(GarbagePacket *pkt)
    {
        const int canId = pkt->getCanId();
        if (canId < 0 || canId >= static_cast<int>(awaitingCollectAck.size())) {
            EV_WARN << "Cloud acknowledgement missing can id" << endl;
            return;
        }

        awaitingCollectAck[canId] = false;
        EV_INFO << "Cloud acknowledgement received for can " << canId
                << ": " << (pkt->getNote() ? pkt->getNote() : "") << endl;
        incrementParentCounter(this, "hostCollectAckCount");
    processCollectQueue();

        if (pendingSecondCanQuery && canId == 0 && attemptCounters[1] == 0) {
            pendingSecondCanQuery = false;
            scheduleQuery(1, simTime() + retryInterval);
        }
    }

  protected:
    void initialize() override
    {
        communicationMode = par("communicationMode").stdstringValue();
        retryInterval = par("queryRetryInterval");
        maxQueryAttempts = par("maxQueryAttempts");
        hostSendsCollect = par("hostSendsCollect");
        expectCloudAck = par("expectCloudAck");

        if (communicationMode == "GarbageInTheCansAndSlow") {
            hostSendsCollect = true;
            expectCloudAck = true;
        }
        else if (communicationMode == "GarbageInTheCansAndFast") {
            hostSendsCollect = false;
            expectCloudAck = false;
        }

        if (hasPar("reportToCloud")) {
            const bool legacyReportToCloud = par("reportToCloud");
            if (legacyReportToCloud && !hostSendsCollect) {
                EV_WARN << "Legacy parameter reportToCloud=true detected; enabling hostSendsCollect for backward compatibility" << endl;
                hostSendsCollect = true;
            }
        }

        counterFigure = requireTextFigure(this, "hostCounters");
        updateHostCountersFigure();

        startEvent = new cMessage("startEvent");
        scheduleAt(simTime(), startEvent);
    }

    void handleMessage(cMessage *msg) override
    {
        if (msg == startEvent) {
            EV_INFO << "Collector starting inspection (mode=" << communicationMode
                    << ", hostSendsCollect=" << (hostSendsCollect ? "true" : "false")
                    << ")" << endl;
            scheduleQuery(0, simTime());
            return;
        }

        for (int canId = 0; canId < static_cast<int>(retryEvents.size()); ++canId) {
            if (msg == retryEvents[canId]) {
                attemptQuery(canId);
                return;
            }
        }

        auto *pkt = check_and_cast<GarbagePacket *>(msg);
        const std::string command = pkt->getCommand();
        if (cGate *arrivalGate = pkt->getArrivalGate()) {
            const char *base = arrivalGate->getBaseName();
            if (strcmp(base, "inCan") == 0 || strcmp(base, "inAnotherCan") == 0)
                recordHostFastReceive(command.c_str());
            else if (strcmp(base, "inCloud") == 0)
                recordHostSlowReceive(command.c_str());
        }

        if (command == "2-NO" || command == "3-YES" || command == "5-NO" || command == "6-YES") {
            handleStatus(pkt);
        }
        else if (command == kCollectAckCommandFor(0) || command == kCollectAckCommandFor(1)) {
            handleCloudAck(pkt);
        }
        else {
            EV_WARN << "Collector received unexpected command '" << command << "'" << endl;
        }
        delete pkt;
    }

    void finish() override
    {
        for (bool awaiting : awaitingCollectAck) {
            if (awaiting)
                EV_WARN << "Collector finished without receiving all cloud acknowledgements" << endl;
        }

        setParentIntParameter(this, "hostCan0Attempts", attemptCounters[0]);
        setParentIntParameter(this, "hostCan1Attempts", attemptCounters[1]);
    }

    void refreshDisplay() const override
    {
        const_cast<GarbageCollector *>(this)->updateHostCountersFigure();
    }

    ~GarbageCollector() override
    {
        cancelAndDelete(startEvent);
        for (auto &evt : retryEvents) {
            if (evt) {
                cancelAndDelete(evt);
                evt = nullptr;
            }
        }
    }
};

Define_Module(GarbageCollector);
