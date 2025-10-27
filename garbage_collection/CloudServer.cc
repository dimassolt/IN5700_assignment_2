#include <omnetpp.h>
#include <cstring>
#include "messages_m.h"

using namespace omnetpp;
using namespace garbage_collection;

class CloudServer : public cSimpleModule {
  private:
    simtime_t ackDelay;

  protected:
    virtual void initialize() override {
        ackDelay = par("ackDelay");
    }

    virtual void handleMessage(cMessage *msg) override {
        auto *pkt = check_and_cast<GarbagePacket *>(msg);
        const char *command = pkt->getCommand();

        if (strcmp(command, "summary") == 0) {
            bubble("Summary received");
            auto *ack = new GarbagePacket("cloud-ack");
            ack->setCommand("cloud-ack");
            ack->setNote("Cloud stored report");
            sendDelayed(ack, ackDelay, "out");
        }

        delete pkt;
    }
};
Define_Module(CloudServer);
