#include <omnetpp.h>
#include <string>
#include "messages_m.h"

using namespace omnetpp;
using namespace garbage_collection;

class GarbageCollector : public cSimpleModule {
  private:
    cMessage *startEvent = nullptr;

    void sendQuery(int canId) {
        auto *query = new GarbagePacket("garbage-query");
        query->setCommand("query");
        query->setCanId(canId);
        query->setIsFull(false);
        query->setTravelTime(0);
        if (canId == 0)
            send(query, "outCan");
        else
            send(query, "outAnotherCan");
    }

  protected:
    virtual void handleMessage(cMessage *msg) override {
        if (msg == startEvent) {
            EV_INFO << "Collector polling cans" << endl;
            sendQuery(0);
            return;
        }

        auto *pkt = check_and_cast<GarbagePacket *>(msg);
        const std::string command = pkt->getCommand();

        if (command == "status") {
            int canId = pkt->getCanId();
            bool isFull = pkt->isFull();

            if (isFull)
                EV_INFO << "Can " << canId << " is full!" << endl;
            else
                EV_INFO << "Can " << canId << " is empty" << endl;

            if (canId == 0) {
                sendQuery(1);
            }
            else if (canId == 1) {
                EV_INFO << "Inspection complete" << endl;
            }
        }
        delete pkt;
    }

    virtual void initialize() override {
        startEvent = new cMessage("startEvent");
        EV_INFO << "Collector ready – querying cans" << endl;
        scheduleAt(simTime(), startEvent);
    }

    virtual ~GarbageCollector() {
        cancelAndDelete(startEvent);
    }
};
Define_Module(GarbageCollector);
