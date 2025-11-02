#include <omnetpp.h>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include "messages_m.h"

using namespace omnetpp;
using namespace garbage_collection;

namespace {
const char *kCollectAckCommandFor(int canId)
{
    return canId == 0 ? "8-OK" : "10-OK";
}

}

class CloudServer : public cSimpleModule {
  private:
    simtime_t ackDelay;
    std::map<int, bool> latestStatuses;

    long sentFastCount = 0;
    long rcvdFastCount = 0;
    long sentSlowCount = 0;
    long rcvdSlowCount = 0;

    cTextFigure *counterFigure = nullptr;
    bool displayCounters = true;

    static bool moduleHasFastCloudTraffic(cModule *module)
    {
        if (!module)
            return false;
        bool report = module->hasPar("reportStatusToCloud") && module->par("reportStatusToCloud").boolValue();
        bool sendCollect = module->hasPar("sendCollectToCloud") && module->par("sendCollectToCloud").boolValue();
        return (report || sendCollect) && module->hasGate("outCloud") && module->gate("outCloud")->isConnected();
    }

    bool shouldDisplayCounters() const
    {
        cModule *parent = getParentModule();
        auto *host = parent->getSubmodule("host", 0);
        bool hostUsesCloud = host && host->hasGate("outCloud") && host->gate("outCloud")->isConnected()
            && host->hasPar("hostSendsCollect") && host->par("hostSendsCollect").boolValue();
        bool canUsesCloud = moduleHasFastCloudTraffic(parent->getSubmodule("can"));
        bool anotherUsesCloud = moduleHasFastCloudTraffic(parent->getSubmodule("anotherCan"));
        return hostUsesCloud || canUsesCloud || anotherUsesCloud;
    }

    void updateCounterFigure()
    {
        if (!counterFigure || !displayCounters)
            return;

        std::ostringstream oss;
        oss << "Cloud\n"
            << "sentCloudFast: " << sentFastCount << "\n"
            << "rcvdCloudFast: " << rcvdFastCount << "\n"
            << "sentCloudSlow: " << sentSlowCount << "\n"
            << "rcvdCloudSlow: " << rcvdSlowCount;
        counterFigure->setText(oss.str().c_str());
    }

    void recordFastSend()
    {
        ++sentFastCount;
        updateCounterFigure();
    }

    void recordFastReceive()
    {
        ++rcvdFastCount;
        updateCounterFigure();
    }

    void recordSlowSend()
    {
        ++sentSlowCount;
        updateCounterFigure();
    }

    void recordSlowReceive()
    {
        ++rcvdSlowCount;
        updateCounterFigure();
    }

    void sendAck(GarbagePacket *ack, cGate *arrivalGate)
    {
        if (arrivalGate) {
            const char *baseName = arrivalGate->getBaseName();
            if (strcmp(baseName, "inHost") == 0 && gate("outHost")->isConnected()) {
                recordSlowSend();
                sendDelayed(ack, ackDelay, "outHost");
                return;
            }
            if (strcmp(baseName, "inCan") == 0) {
                int index = arrivalGate->getIndex();
                if (index >= 0 && index < gateSize("outCan") && gate("outCan", index)->isConnected()) {
                    recordFastSend();
                    sendDelayed(ack, ackDelay, "outCan", index);
                    return;
                }
            }
        }

        if (gate("outHost")->isConnected()) {
            recordSlowSend();
            sendDelayed(ack, ackDelay, "outHost");
            return;
        }
        for (int i = 0; i < gateSize("outCan"); ++i) {
            if (gate("outCan", i)->isConnected()) {
                recordFastSend();
                sendDelayed(ack, ackDelay, "outCan", i);
                return;
            }
        }

        delete ack;
    }

    bool isStatusCommand(const char *command) const
    {
        return strcmp(command, "2-NO") == 0 || strcmp(command, "3-YES") == 0
            || strcmp(command, "5-NO") == 0 || strcmp(command, "6-YES") == 0;
    }

    bool isCollectCommand(const char *command) const
    {
        return strcmp(command, "7-Collect garbage") == 0 || strcmp(command, "9-Collect garbage") == 0;
    }

  protected:
    void initialize() override
    {
        ackDelay = par("ackDelay");
        counterFigure = dynamic_cast<cTextFigure *>(getParentModule()->getCanvas()->getFigure("cloudCounters"));
        displayCounters = shouldDisplayCounters();
        if (counterFigure) {
            counterFigure->setVisible(displayCounters);
            updateCounterFigure();
        }
    }

    void handleMessage(cMessage *msg) override
    {
        auto *pkt = check_and_cast<GarbagePacket *>(msg);
        const char *command = pkt->getCommand();
        cGate *arrivalGate = pkt->getArrivalGate();

        if (arrivalGate) {
            const char *baseName = arrivalGate->getBaseName();
            if (strcmp(baseName, "inHost") == 0)
                recordSlowReceive();
            else if (strcmp(baseName, "inCan") == 0)
                recordFastReceive();
        }

        if (isStatusCommand(command)) {
            latestStatuses[pkt->getCanId()] = pkt->isFull();
            EV_INFO << "Cloud recorded status from can " << pkt->getCanId()
                    << " => " << (pkt->isFull() ? "full" : "empty") << endl;
        }
        else if (isCollectCommand(command)) {
            const int canId = pkt->getCanId();
            EV_INFO << "Cloud received collect request for can " << canId
                    << " (note=" << (pkt->getNote() ? pkt->getNote() : "") << ")" << endl;

            auto *ack = new GarbagePacket("collect-ack");
            ack->setCommand(kCollectAckCommandFor(canId));
            ack->setCanId(canId);
            ack->setIsFull(false);
            ack->setTravelTime(SIMTIME_DBL(ackDelay));
            ack->setNote("collect-confirmed");
            sendAck(ack, arrivalGate);
        }
        else if (strcmp(command, "cloud-ack") == 0) {
            EV_INFO << "Cloud relayed acknowledgement received: "
                    << (pkt->getNote() ? pkt->getNote() : "") << endl;
        }
        else {
            EV_WARN << "Cloud received unknown command '" << command << "'" << endl;
        }

        delete pkt;
    }
};
Define_Module(CloudServer);
