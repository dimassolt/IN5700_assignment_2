package garbage_collection;

simple CloudServer
{
    parameters:
        double ackDelay @unit(s) = default(0.2s);
        @display("i=misc/cloud_l");
    gates:
        input inHost;
        output outHost;
        input inCan[2];
        output outCan[2];
}
