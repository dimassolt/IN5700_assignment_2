#include <omnetpp.h>
using namespace omnetpp;

class Toc1 : public cSimpleModule {
  private:
    int dropsLeft = 0;

  protected:
    virtual void initialize() override {
        dropsLeft = par("dropTestAttempts");  // 3
    }

    virtual void handleMessage(cMessage *msg) override {
        const char* name = msg->getName();

        if (strcmp(name, "1- Hello") == 0) {
            // Reply to HELLO
            send(new cMessage("2- Ack from cloud to computer"), "out");
        }
        else if (strcmp(name, "3- Test Message") == 0) {
            if (dropsLeft > 0) {
                // Show the same yellow bubble text as in the video
                bubble("message lost");
                dropsLeft--;
                delete msg; // drop
                return;
            } else {
                send(new cMessage("4- Ack from cloud to computer"), "out");
            }
        }
        delete msg;
    }
};
Define_Module(Toc1);
