package garbage_collection;

import garbage_collection.GarbageCollector;
import garbage_collection.GarbageCan;
import garbage_collection.CloudServer;
import garbage_collection.GarbageVisualizer;
import ned.DelayChannel;
import ned.IBidirectionalChannel;
import ned.IUnidirectionalChannel;
import ned.IdealChannel;


network GarbageCollectionSystem
{
    parameters:
        double canDelay @unit(s) = default(0.1s);
        double hostToCloudDelay @unit(s) = default(0.3s);
        double cloudToHostDelay @unit(s) = default(0.3s);
        double canToCloudDelay @unit(s) = default(0.15s);
        double cloudToCanDelay @unit(s) = default(0.15s);
        bool connectHostToCloud = default(false);
        bool connectCansToCloud = default(false);
        double cloudAckDelay @unit(s) = default(0.2s);
        @display("bgb=2000,800,#ECFFB3,#dfe6f0,2");
        @figure[roadOuterTop](type=rectangle; pos=140,140; size=1250,2; lineColor=#000000; lineWidth=1; fillColor=#000000; fillOpacity=1);
        @figure[roadOuterBottom](type=rectangle; pos=140,677; size=1250,2; lineColor=#000000; lineWidth=1; fillColor=#000000; fillOpacity=1);
        @figure[roadOuterLeft](type=rectangle; pos=140,140; size=2,540; lineColor=#000000; lineWidth=1; fillColor=#000000; fillOpacity=1);

        @figure[roadInnerTop](type=rectangle; pos=290,290; size=1100,2; lineColor=#000000; lineWidth=1; fillColor=#000000; fillOpacity=1);
        @figure[roadInnerBottom](type=rectangle; pos=290,527; size=1100,2; lineColor=#000000; lineWidth=1; fillColor=#000000; fillOpacity=1);
        @figure[roadInnerLeft](type=rectangle; pos=290,290; size=2,240; lineColor=#000000; lineWidth=1; fillColor=#000000; fillOpacity=1);

        @figure[infoHeading](type=text; pos=1450,100; text="No garbage solution"; font="Arial,40,b");
        @figure[smartSlowOutLabel](type=text; pos=1450,140; text="Slow connection from the smartphone to others (time it takes) ="; font="Arial,18");
        @figure[smartSlowOutValue](type=text; pos=1820,140; text=""; font="Arial,18");
        @figure[smartSlowInLabel](type=text; pos=1450,160; text="Slow connection from others to the smartphone (time it takes) ="; font="Arial,18");
        @figure[smartSlowInValue](type=text; pos=1820,160; text=""; font="Arial,18");
        @figure[smartFastOutLabel](type=text; pos=1450,180; text="Fast connection from the smartphone to others (time it takes) ="; font="Arial,18");
        @figure[smartFastOutValue](type=text; pos=1820,180; text=""; font="Arial,18");
        @figure[smartFastInLabel](type=text; pos=1450,200; text="Fast connection from others to the smartphone (time it takes) ="; font="Arial,18");
        @figure[smartFastInValue](type=text; pos=1820,200; text=""; font="Arial,18");
        @figure[canOutLabel](type=text; pos=1450,230; text="Connection from the can to others (time it takes) ="; font="Arial,18");
        @figure[canOutValue](type=text; pos=1750,230; text=""; font="Arial,18");
        @figure[canInLabel](type=text; pos=1450,250; text="Connection from others to the can (time it takes) ="; font="Arial,18");
        @figure[canInValue](type=text; pos=1750,250; text=""; font="Arial,18");
        @figure[anotherCanOutLabel](type=text; pos=1450,280; text="Connection from the anotherCan to others (time it takes) ="; font="Arial,18");
        @figure[anotherCanOutValue](type=text; pos=1790,280; text=""; font="Arial,18");
        @figure[anotherCanInLabel](type=text; pos=1450,300; text="Connection from others to the anotherCan (time it takes) ="; font="Arial,18");
        @figure[anotherCanInValue](type=text; pos=1790,300; text=""; font="Arial,18");
        @figure[cloudSlowOutLabel](type=text; pos=1450,330; text="Slow connection from the Cloud to others (time it takes) ="; font="Arial,18");
        @figure[cloudSlowOutValue](type=text; pos=1800,330; text=""; font="Arial,18");
        @figure[cloudSlowInLabel](type=text; pos=1450,350; text="Slow connection from others to the Cloud (time it takes) ="; font="Arial,18");
        @figure[cloudSlowInValue](type=text; pos=1800,350; text=""; font="Arial,18");
        @figure[cloudFastOutLabel](type=text; pos=1450,370; text="Fast connection from the Cloud to others (time it takes) ="; font="Arial,18");
        @figure[cloudFastOutValue](type=text; pos=1800,370; text=""; font="Arial,18");
        @figure[cloudFastInLabel](type=text; pos=1450,390; text="Fast connection from others to the Cloud (time it takes) ="; font="Arial,18");
        @figure[cloudFastInValue](type=text; pos=1800,390; text=""; font="Arial,18");
    @figure[hostCounters](type=text; pos=1025,230; text="GarbageCollector\nFast -> sent: 0, received: 0, lost: 0\nSlow -> sent: 0, received: 0, lost: 0"; font="Arial,18"; color=#0000ff; anchor=c);
    @figure[canCounters](type=text; pos=331,40; text="Can\nFast -> sent: 0, received: 0, lost: 0\nSlow -> sent: 0, received: 0, lost: 0"; font="Arial,18"; color=#0000ff; anchor=c);
    @figure[anotherCanCounters](type=text; pos=496,470; text="AnotherCan\nFast -> sent: 0, received: 0, lost: 0\nSlow -> sent: 0, received: 0, lost: 0"; font="Arial,18"; color=#0000ff; anchor=c);
    @figure[cloudCounters](type=text; pos=1283,260; text="Cloud\nFast -> sent: 0, received: 0, lost: 0\nSlow -> sent: 0, received: 0, lost: 0"; font="Arial,18"; color=#0000ff; anchor=c);

    types:
    submodules:
        host[1]: GarbageCollector {
            parameters:
                moveDuration = 0s;
                @display("p=1025,320;i=device/pocketpc;r=180");
        }
        can: GarbageCan {
            parameters:
                canId = 0;
                hasGarbage = false;
                responseDelay = parent.canDelay;
                @display("p=331,108;i=block/bucket;r=180");
        }
        anotherCan: GarbageCan {
            parameters:
                canId = 1;
                hasGarbage = false;
                responseDelay = parent.canDelay;
                @display("p=496,541;i=block/bucket;r=180");
        }
        cloud: CloudServer {
            parameters:
                ackDelay = parent.cloudAckDelay;
                @display("p=1283,378;i=misc/cloud_l;r=900");
        }
        visualizer: GarbageVisualizer {
            parameters:
                @display("p=1025,55;i=block/app;b=60,51,,#f0f4ff");
        }
    connections:
        host[0].outCan --> { delay = parent.canDelay; } --> can.in;
        can.out --> { delay = parent.canDelay; } --> host[0].inCan;
        host[0].outAnotherCan --> { delay = parent.canDelay; } --> anotherCan.in;
        anotherCan.out --> { delay = parent.canDelay; } --> host[0].inAnotherCan;

        if (connectHostToCloud) {
            host[0].outCloud --> { delay = parent.hostToCloudDelay; } --> cloud.inHost;
            cloud.outHost --> { delay = parent.cloudToHostDelay; } --> host[0].inCloud;
        }

        if (connectCansToCloud) {
            can.outCloud --> { delay = parent.canToCloudDelay; } --> cloud.inCan[0];
            cloud.outCan[0] --> { delay = parent.cloudToCanDelay; } --> can.inCloud;

            anotherCan.outCloud --> { delay = parent.canToCloudDelay; } --> cloud.inCan[1];
            cloud.outCan[1] --> { delay = parent.cloudToCanDelay; } --> anotherCan.inCloud;
        }
}
