package garbage_collection;

simple CloudServer
{
    parameters:
        double ackDelay @unit(s) = default(0.2s);
        @display("i=misc/cloud_l");
    gates:
        input in;
        output out;
}
