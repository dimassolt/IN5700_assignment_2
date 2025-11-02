#include <omnetpp.h>
#include <cstring>
#include <sstream>
#include <string>
#include "messages_m.h"

using namespace omnetpp;
using namespace garbage_collection;

namespace {
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

        std::ostringstream oss;
        oss << counterLabel << "\n"
            << "sent" << counterLabel << "Fast: " << sentFastCount << "\n"
            << "rcvd" << counterLabel << "Fast: " << rcvdFastCount << "\n"
            << "numberOfLost" << counterLabel << "Msgs: " << lostFastCount;
        counterFigure->setText(oss.str().c_str());
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

        counterFigure = dynamic_cast<cTextFigure *>(getParentModule()->getCanvas()->getFigure(canId == 0 ? "canCounters" : "anotherCanCounters"));
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
};
Define_Module(GarbageCan);
