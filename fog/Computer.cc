#include <omnetpp.h>
using namespace omnetpp;

class Tic1 : public cSimpleModule {
  private:
    cMessage *timeoutEvent = nullptr;
    int testAttempts = 0;
    simtime_t timeout;

  protected:
    virtual void initialize() override {
        timeout = par("timeout");      // default 1s
        timeoutEvent = new cMessage("timeoutEvent");

        // 1) Send "1- Hello"
        send(new cMessage("1- Hello"), "out");
    }

    void sendTest() {
        testAttempts++;
        auto *m = new cMessage("3- Test Message");
        send(m, "out");
        scheduleAt(simTime() + timeout, timeoutEvent);  // retransmit timer
    }

    virtual void handleMessage(cMessage *msg) override {
        if (msg == timeoutEvent) {
            // Timeout -> resend TEST
            sendTest();
            return;
        }

        // ACKs are just cMessage identified by their name
        const char* name = msg->getName();

        if (strcmp(name, "2- Ack from cloud to computer") == 0) {
            // HELLO got ACK -> start TEST sequence
            sendTest();
        }
        else if (strcmp(name, "4- Ack from cloud to computer") == 0) {
            // Final ACK for TEST
            if (timeoutEvent->isScheduled()) cancelEvent(timeoutEvent);
            // Stop exactly like the video
            endSimulation();
        }
        delete msg;
    }

    virtual ~Tic1() {
        cancelAndDelete(timeoutEvent);
    }
};
Define_Module(Tic1);
