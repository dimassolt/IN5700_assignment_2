#include <omnetpp.h>
#include <cstring>
#include "messages_m.h"

using namespace omnetpp;
using namespace garbage_collection;

class GarbageCan : public cSimpleModule {
  private:
    bool hasGarbage = false;
    int canId = 0;
    simtime_t responseDelay;

  protected:
    virtual void initialize() override {
        hasGarbage = par("hasGarbage");
        canId = par("canId");
        responseDelay = par("responseDelay");
    }

    virtual void handleMessage(cMessage *msg) override {
        auto *pkt = check_and_cast<GarbagePacket *>(msg);
        const char *command = pkt->getCommand();

        if (strcmp(command, "query") == 0) {
            auto *reply = new GarbagePacket("garbage-status");
            reply->setCommand("status");
            reply->setCanId(canId);
            reply->setIsFull(hasGarbage);
            reply->setTravelTime(SIMTIME_DBL(responseDelay));

            if (!hasGarbage)
                bubble("I'm empty");
            else
                bubble("I'm full");

            sendDelayed(reply, responseDelay, "out");
        }

        delete pkt;
    }
};
Define_Module(GarbageCan);
