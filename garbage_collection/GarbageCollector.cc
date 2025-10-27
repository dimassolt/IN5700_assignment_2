#include <omnetpp.h>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>
#include <string>
#include "messages_m.h"

using namespace omnetpp;
using namespace garbage_collection;

namespace {
struct Point { double x; double y; };
}

class GarbageCollector : public cSimpleModule {
  private:
    cMessage *startEvent = nullptr;
    cMessage *moveEvent = nullptr;
    std::vector<Point> path;
    enum class MoveStage { None, ToTop, ToBottom, ToFinal };
    MoveStage pendingStage = MoveStage::None;
    size_t currentIndex = 0;

  private:
    void setPosition(size_t index) {
        if (index >= path.size())
            return;
        const auto &p = path[index];
        std::string sx = std::to_string(p.x);
        std::string sy = std::to_string(p.y);
        getDisplayString().setTagArg("p", 0, sx.c_str());
        getDisplayString().setTagArg("p", 1, sy.c_str());
    }

    void loadPathFromParameter() {
        static const std::array<Point, 4> fallbackPath{{
            Point{540, 120},   // start near top-right
            Point{220, 120},   // first can
            Point{220, 260},   // another can
            Point{460, 260}    // final stop in lower corridor
        }};

        path.assign(fallbackPath.begin(), fallbackPath.end());

        cXMLElement *movement = par("turtleScript").xmlValue();
        if (!movement) {
            EV_WARN << "No turtleScript configured; using fallback path" << endl;
            return;
        }

        std::array<bool, 4> overridden{};
        for (cXMLElement *child = movement->getFirstChild(); child != nullptr; child = child->getNextSibling()) {
            if (strcmp(child->getTagName(), "waypoint") != 0)
                continue;

            int index = -1;
            if (const char *role = child->getAttribute("role")) {
                if (strcmp(role, "start") == 0)
                    index = 0;
                else if (strcmp(role, "can") == 0)
                    index = 1;
                else if (strcmp(role, "anotherCan") == 0)
                    index = 2;
                else if (strcmp(role, "final") == 0)
                    index = 3;
            }

            if (index < 0) {
                if (const char *order = child->getAttribute("order")) {
                    try {
                        index = std::stoi(order);
                    }
                    catch (...) {
                        index = -1;
                    }
                }
            }

            if (index < 0 || static_cast<size_t>(index) >= path.size()) {
                EV_WARN << "Ignoring waypoint with invalid role/order" << endl;
                continue;
            }

            const char *xAttr = child->getAttribute("x");
            const char *yAttr = child->getAttribute("y");
            if (!xAttr || !yAttr) {
                EV_WARN << "Waypoint missing coordinates" << endl;
                continue;
            }

            try {
                path[index] = {std::stod(xAttr), std::stod(yAttr)};
                overridden[index] = true;
            }
            catch (const std::exception &e) {
                EV_WARN << "Invalid waypoint coordinates: " << e.what() << endl;
            }
        }

        if (!std::all_of(overridden.begin(), overridden.end(), [](bool v) { return v; })) {
            EV_INFO << "Turtle script missing some waypoints; supplemented with defaults" << endl;
        }
    }

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

    void scheduleMove(MoveStage stage, const char *logNote) {
        if (moveEvent->isScheduled())
            cancelEvent(moveEvent);
        pendingStage = stage;
        if (logNote && *logNote)
            EV_INFO << logNote << endl;
        simtime_t delay = par("moveDuration");
        scheduleAt(simTime() + delay, moveEvent);
    }

  protected:
    virtual void initialize() override {
        startEvent = new cMessage("startEvent");
        moveEvent = new cMessage("moveEvent");

        loadPathFromParameter();
        setPosition(0);
        currentIndex = 0;
        EV_INFO << "Collector ready – heading to can" << endl;
        scheduleAt(simTime(), startEvent);
    }

    virtual void handleMessage(cMessage *msg) override {
        if (msg == startEvent) {
            scheduleMove(MoveStage::ToTop, "Collector moving from right to left");
            return;
        }

        if (msg == moveEvent) {
            switch (pendingStage) {
                case MoveStage::ToTop:
                    currentIndex = 1;
                    setPosition(currentIndex);
                    EV_INFO << "Collector stopped at can" << endl;
                    sendQuery(0);
                    break;
                case MoveStage::ToBottom:
                    currentIndex = 2;
                    setPosition(currentIndex);
                    EV_INFO << "Collector stopped at anotherCan" << endl;
                    sendQuery(1);
                    break;
                case MoveStage::ToFinal:
                    currentIndex = 3;
                    setPosition(currentIndex);
                    EV_INFO << "Collector returning to base" << endl;
                    endSimulation();
                    break;
                case MoveStage::None:
                    break;
            }
            pendingStage = MoveStage::None;
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
                scheduleMove(MoveStage::ToBottom, "Collector sliding down to anotherCan");
            }
            else if (canId == 1) {
                scheduleMove(MoveStage::ToFinal, "Collector moving back to the right");
            }
        }
        delete pkt;
    }

    virtual ~GarbageCollector() {
        cancelAndDelete(startEvent);
        cancelAndDelete(moveEvent);
    }
};
Define_Module(GarbageCollector);
