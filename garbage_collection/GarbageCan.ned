package garbage_collection;

simple GarbageCan
{
    parameters:
        bool hasGarbage = default(false);
        int canId = default(0);
        double responseDelay @unit(s) = default(0.1s);
        @display("i=device/bin");
    gates:
        input in;
        output out;
}
