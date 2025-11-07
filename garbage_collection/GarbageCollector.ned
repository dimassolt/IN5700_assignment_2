package garbage_collection;

simple GarbageCollector
{
    parameters:
        bool reportToCloud @deprecated("Unused; kept for backward compatibility") = default(false);
        string communicationMode = default("default");
        double queryRetryInterval @unit(s) = default(0.4s);
        int maxQueryAttempts = default(4);
        bool hostSendsCollect = default(true);
        bool expectCloudAck = default(true);
        @display("i=device/palm,,0");
    gates:
        input inCan;
        input inAnotherCan;
        input inCloud;
        output outCan;
        output outAnotherCan;
        output outCloud;
}
