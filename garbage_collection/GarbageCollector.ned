package garbage_collection;

simple GarbageCollector
{
    parameters:
        double moveDuration @unit(s) = default(1s);
        xml turtleScript = default(xml("<movement/>"));
        @display("i=device/smartphone");
    gates:
        input inCan;
        input inAnotherCan;
        output outCan;
        output outAnotherCan;
}
