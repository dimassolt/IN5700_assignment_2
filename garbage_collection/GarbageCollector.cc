#include <omnetpp.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <sstream>
#include <string>
#include "messages_m.h"

using namespace omnetpp;
using namespace garbage_collection;

namespace {
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

}

class GarbageCollector : public cSimpleModule {
  private:
    static constexpr int kUnknownState = -1;

    cMessage *startEvent = nullptr;
    std::array<cMessage *, 2> retryEvents {nullptr, nullptr};
    std::array<int, 2> canStates {kUnknownState, kUnknownState};
    std::array<int, 2> attemptCounters {0, 0};
    std::array<bool, 2> awaitingCollectAck {false, false};

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

    cTextFigure *counterFigure = nullptr;

    void updateHostCountersFigure()
    {
        if (!counterFigure)
            return;

        std::ostringstream oss;
        oss << "GarbageCollector\n"
            << "sentHostFast: " << sentHostFast << "\n"
            << "rcvdHostFast: " << rcvdHostFast << "\n"
            << "sentHostSlow: " << sentHostSlow << "\n"
            << "rcvdHostSlow: " << rcvdHostSlow;
        counterFigure->setText(oss.str().c_str());
    }

    void recordHostFastSend()
    {
        ++sentHostFast;
        updateHostCountersFigure();
    }

    void recordHostFastReceive()
    {
        ++rcvdHostFast;
        updateHostCountersFigure();
    }

    void recordHostSlowSend()
    {
        ++sentHostSlow;
        updateHostCountersFigure();
    }

    void recordHostSlowReceive()
    {
        ++rcvdHostSlow;
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
            recordHostFastSend();
            send(query, "outCan");
        }
        else {
            recordHostFastSend();
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

        if (firstObservation && canId == 0 && attemptCounters[1] == 0)
            scheduleQuery(1, simTime() + retryInterval);

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
                sendCollectRequest(canId);
        }
    }

    void sendCollectRequest(int canId)
    {
        auto *collect = new GarbagePacket("collect-request");
        collect->setCommand(kCollectCommandFor(canId));
        collect->setCanId(canId);
        collect->setIsFull(true);
        collect->setTravelTime(0);
        collect->setNote(communicationMode.c_str());
        recordHostSlowSend();
        send(collect, "outCloud");
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
    }

  protected:
    void initialize() override
    {
        communicationMode = par("communicationMode").stdstringValue();
        retryInterval = par("queryRetryInterval");
        maxQueryAttempts = par("maxQueryAttempts");
        hostSendsCollect = par("hostSendsCollect");
        expectCloudAck = par("expectCloudAck");

        counterFigure = dynamic_cast<cTextFigure *>(getParentModule()->getCanvas()->getFigure("hostCounters"));
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
        if (cGate *arrivalGate = pkt->getArrivalGate()) {
            const char *base = arrivalGate->getBaseName();
            if (strcmp(base, "inCan") == 0 || strcmp(base, "inAnotherCan") == 0)
                recordHostFastReceive();
            else if (strcmp(base, "inCloud") == 0)
                recordHostSlowReceive();
        }
        const std::string command = pkt->getCommand();

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
