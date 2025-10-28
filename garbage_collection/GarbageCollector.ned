package garbage_collection;

simple GarbageCollector
{
    parameters:
        double moveDuration @unit(s) = default(1s);
        double moveUpdateInterval @unit(s) = default(0.05s);
        xml turtleScript = default(xml("<movement/>"));
        @display("i=device/palm,,0");
    gates:
        input inCan;
        input inAnotherCan;
        output outCan;
        output outAnotherCan;
}
