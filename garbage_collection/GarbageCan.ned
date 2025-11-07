package garbage_collection;


// Smart garbage can that answers status queries, tracks retry counts, and can escalate collection requests to the cloud.
 
simple GarbageCan
{
    parameters:
        bool hasGarbage = default(false);
        int canId = default(0);
        double responseDelay @unit(s) = default(0.1s);
        bool reportStatusToCloud = default(false);
        bool sendCollectToCloud = default(false);
        int lostQueryCount = default(3);
        double collectDispatchDelay @unit(s) = default(0.05s);
        @display("i=block/bucket,,0");
    gates:
        input in;
        input inCloud;
        output out;
        output outCloud;
}
