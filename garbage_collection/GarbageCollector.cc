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

    struct MovementState {
        MoveStage stage = MoveStage::None;
        bool active = false;
        size_t fromIndex = 0;
        size_t toIndex = 0;
        simtime_t startTime;
        simtime_t endTime;
    } movement;

    size_t currentIndex = 0;
    simtime_t moveUpdateInterval;

  private:
    void setPosition(const Point &p) {
        std::string sx = std::to_string(p.x);
        std::string sy = std::to_string(p.y);
        getDisplayString().setTagArg("p", 0, sx.c_str());
        getDisplayString().setTagArg("p", 1, sy.c_str());
    }

    void setPosition(size_t index) {
        if (index >= path.size())
            return;
        setPosition(path[index]);
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

    size_t targetIndexForStage(MoveStage stage) const {
        switch (stage) {
            case MoveStage::ToTop:
                return 1;
            case MoveStage::ToBottom:
                return 2;
            case MoveStage::ToFinal:
                return 3;
            case MoveStage::None:
            default:
                return currentIndex;
        }
    }

    Point interpolate(const Point &from, const Point &to, double fraction) const {
        fraction = std::clamp(fraction, 0.0, 1.0);
        return Point{from.x + (to.x - from.x) * fraction,
                     from.y + (to.y - from.y) * fraction};
    }

    void startMovement(MoveStage stage, const char *logNote) {
        if (stage == MoveStage::None)
            return;

        if (moveEvent->isScheduled())
            cancelEvent(moveEvent);

        size_t targetIndex = targetIndexForStage(stage);
        if (targetIndex >= path.size()) {
            EV_WARN << "Target index out of bounds; skipping movement" << endl;
            return;
        }

        movement.stage = stage;
        movement.active = true;
        movement.fromIndex = currentIndex;
        movement.toIndex = targetIndex;
        movement.startTime = simTime();
        simtime_t duration = par("moveDuration");
        movement.endTime = movement.startTime + duration;

        if (logNote && *logNote)
            EV_INFO << logNote << endl;

        if (duration <= SIMTIME_ZERO) {
            scheduleAt(simTime(), moveEvent);
        }
        else {
            scheduleAt(simTime(), moveEvent);
        }
    }

    void finishMovement() {
        MoveStage stage = movement.stage;
        movement.active = false;
        movement.stage = MoveStage::None;
        movement.fromIndex = movement.toIndex;
        currentIndex = movement.toIndex;
        setPosition(currentIndex);

        switch (stage) {
            case MoveStage::ToTop:
                EV_INFO << "Collector stopped at can" << endl;
                sendQuery(0);
                break;
            case MoveStage::ToBottom:
                EV_INFO << "Collector stopped at anotherCan" << endl;
                sendQuery(1);
                break;
            case MoveStage::ToFinal:
                EV_INFO << "Collector returning to base" << endl;
                endSimulation();
                break;
            case MoveStage::None:
                break;
        }
    }

  protected:
    void loadPathFromParameter() {
        static const std::array<Point, 4> fallbackPath{ {
            Point{1025, 233},  // start below visualizer
            Point{331,  215},  // can
            Point{496,  541},  // another can
            Point{1025, 560}   // final stop in lower lane
        } };

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

    virtual void handleMessage(cMessage *msg) override {
        if (msg == startEvent) {
            startMovement(MoveStage::ToTop, "Collector moving from right to left");
            return;
        }

        if (msg == moveEvent) {
            if (!movement.active) {
                return;
            }

            simtime_t now = simTime();
            simtime_t duration = movement.endTime - movement.startTime;
            double fraction = duration <= SIMTIME_ZERO ? 1.0 : ((now - movement.startTime) / duration).dbl();

            if (fraction >= 1.0) {
                finishMovement();
            }
            else {
                const Point &from = path[movement.fromIndex];
                const Point &to = path[movement.toIndex];
                Point pos = interpolate(from, to, fraction);
                setPosition(pos);

                simtime_t interval = moveUpdateInterval;
                if (interval <= SIMTIME_ZERO)
                    interval = SimTime(0.05);

                simtime_t next = now + interval;
                if (next > movement.endTime)
                    next = movement.endTime;
                scheduleAt(next, moveEvent);
            }
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
                startMovement(MoveStage::ToBottom, "Collector sliding down to anotherCan");
            }
            else if (canId == 1) {
                startMovement(MoveStage::ToFinal, "Collector moving back to the right");
            }
        }
        delete pkt;
    }

    virtual void initialize() override {
        startEvent = new cMessage("startEvent");
        moveEvent = new cMessage("moveEvent");

        loadPathFromParameter();
        setPosition(0);
        currentIndex = 0;

        moveUpdateInterval = par("moveUpdateInterval");
        if (moveUpdateInterval <= SIMTIME_ZERO)
            moveUpdateInterval = SimTime(0.05);

        EV_INFO << "Collector ready – heading to can" << endl;
        scheduleAt(simTime(), startEvent);
    }

    virtual ~GarbageCollector() {
        cancelAndDelete(startEvent);
        cancelAndDelete(moveEvent);
    }
};
Define_Module(GarbageCollector);
