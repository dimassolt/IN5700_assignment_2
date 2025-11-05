#include <omnetpp.h>
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
    long lostFastCount = 0;
    long lostSlowCount = 0;

    cTextFigure *counterFigure = nullptr;
    bool displayCounters = true;

    std::string formatStatusText() const
    {
        std::ostringstream oss;
        oss << "Cloud\n"
            << "Fast -> sent: " << sentFastCount
            << ", received: " << rcvdFastCount
            << ", lost: " << lostFastCount << "\n"
            << "Slow -> sent: " << sentSlowCount
            << ", received: " << rcvdSlowCount
            << ", lost: " << lostSlowCount;
        return oss.str();
    }

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
        if (!counterFigure)
            return;

        counterFigure->setVisible(displayCounters);
        if (!displayCounters)
            return;

        const std::string text = formatStatusText();
        counterFigure->setText(text.c_str());
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

    void recordLostFast()
    {
        ++lostFastCount;
        updateCounterFigure();
    }

    void recordLostSlow()
    {
        ++lostSlowCount;
        updateCounterFigure();
    }

    void sendAck(GarbagePacket *ack, cGate *arrivalGate)
    {
        const bool arrivedFromHost = arrivalGate && strcmp(arrivalGate->getBaseName(), "inHost") == 0;
        const bool arrivedFromCan = arrivalGate && strcmp(arrivalGate->getBaseName(), "inCan") == 0;

        auto trySendSlow = [&]() -> bool {
            if (!gate("outHost")->isConnected())
                return false;
            recordSlowSend();
            sendDelayed(ack, ackDelay, "outHost");
            return true;
        };

        auto trySendFastToIndex = [&](int index) -> bool {
            if (index < 0 || index >= gateSize("outCan") || !gate("outCan", index)->isConnected())
                return false;
            recordFastSend();
            sendDelayed(ack, ackDelay, "outCan", index);
            return true;
        };

        bool delivered = false;

        if (arrivedFromHost)
            delivered = trySendSlow();

        if (!delivered && arrivedFromCan)
            delivered = trySendFastToIndex(arrivalGate->getIndex());

        if (!delivered)
            delivered = trySendSlow();

        if (!delivered) {
            for (int i = 0; i < gateSize("outCan") && !delivered; ++i)
                delivered = trySendFastToIndex(i);
        }

        if (!delivered) {
            if (arrivedFromHost)
                recordLostSlow();
            else
                recordLostFast();
            delete ack;
        }
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
        counterFigure = requireTextFigure(this, "cloudCounters");
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

    void refreshDisplay() const override
    {
        const_cast<CloudServer *>(this)->updateCounterFigure();
    }
};
Define_Module(CloudServer);
