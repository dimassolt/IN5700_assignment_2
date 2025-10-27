package garbage_collection;

import garbage_collection.GarbageCollector;
import garbage_collection.GarbageCan;
import garbage_collection.CloudServer;
import garbage_collection.GarbageVisualizer;

network GarbageCollectionSystem
{
    parameters:
        double canDelay @unit(s) = default(0.1s);
        double cloudDelay @unit(s) = default(0.2s);
        @display("bgb=1120,400,white,#dfe6f0");
    @figure[activityZone](type=rectangle; pos=140,60; size=520,260; lineColor=#000000; lineWidth=2; fillColor=#ffffff; fillOpacity=0);
    @figure[upperLane](type=rectangle; pos=140,136; size=520,8; lineColor=#000000; lineWidth=1; lineOpacity=0; fillColor=#000000; fillOpacity=1);
    @figure[lowerLane](type=rectangle; pos=140,256; size=520,8; lineColor=#000000; lineWidth=1; lineOpacity=0; fillColor=#000000; fillOpacity=1);
    @figure[binZone](type=rectangle; pos=160,110; size=240,200; lineColor=#000000; lineWidth=2; fillColor=#ffffff; fillOpacity=0);
    @figure[infoHeading](type=text; pos=700,100; text="No garbage solution"; font="Arial,16,b");
    @figure[smartSlowOutLabel](type=text; pos=700,140; text="Slow connection from the smartphone to others (time it takes) =");
    @figure[smartSlowOutValue](type=text; pos=1060,140; text="");
    @figure[smartSlowInLabel](type=text; pos=700,160; text="Slow connection from others to the smartphone (time it takes) =");
    @figure[smartSlowInValue](type=text; pos=1060,160; text="");
    @figure[smartFastOutLabel](type=text; pos=700,180; text="Fast connection from the smartphone to others (time it takes) =");
    @figure[smartFastOutValue](type=text; pos=1060,180; text="");
    @figure[smartFastInLabel](type=text; pos=700,200; text="Fast connection from others to the smartphone (time it takes) =");
    @figure[smartFastInValue](type=text; pos=1060,200; text="");
    @figure[canOutLabel](type=text; pos=700,230; text="Connection from the can to others (time it takes) =");
    @figure[canOutValue](type=text; pos=1060,230; text="");
    @figure[canInLabel](type=text; pos=700,250; text="Connection from others to the can (time it takes) =");
    @figure[canInValue](type=text; pos=1060,250; text="");
    @figure[anotherCanOutLabel](type=text; pos=700,280; text="Connection from the anotherCan to others (time it takes) =");
    @figure[anotherCanOutValue](type=text; pos=1060,280; text="");
    @figure[anotherCanInLabel](type=text; pos=700,300; text="Connection from others to the anotherCan (time it takes) =");
    @figure[anotherCanInValue](type=text; pos=1060,300; text="");
    @figure[cloudSlowOutLabel](type=text; pos=700,330; text="Slow connection from the Cloud to others (time it takes) =");
    @figure[cloudSlowOutValue](type=text; pos=1060,330; text="");
    @figure[cloudSlowInLabel](type=text; pos=700,350; text="Slow connection from others to the Cloud (time it takes) =");
    @figure[cloudSlowInValue](type=text; pos=1060,350; text="");
    @figure[cloudFastOutLabel](type=text; pos=700,370; text="Fast connection from the Cloud to others (time it takes) =");
    @figure[cloudFastOutValue](type=text; pos=1060,370; text="");
    @figure[cloudFastInLabel](type=text; pos=700,390; text="Fast connection from others to the Cloud (time it takes) =");
    @figure[cloudFastInValue](type=text; pos=1060,390; text="");
    submodules:
        host[1]: GarbageCollector {
            parameters:
                moveDuration = 0.6s;
                @display("p=540,120;i=device/smartphone;r=280");
        }
        can: GarbageCan {
            parameters:
                canId = 0;
                hasGarbage = false;
                responseDelay = parent.canDelay;
                @display("p=220,120;i=device/bin;r=180");
        }
        anotherCan: GarbageCan {
            parameters:
                canId = 1;
                hasGarbage = false;
                responseDelay = parent.canDelay;
                @display("p=220,260;i=device/bin;r=180");
        }
        cloud: CloudServer {
            parameters:
                ackDelay = parent.cloudDelay;
                @display("p=560,220;i=misc/cloud_l;r=260");
        }
        visualizer: GarbageVisualizer {
            parameters:
                @display("p=750,199;i=block/table;b=320,320,,#f0f4ff");
        }
    connections allowunconnected:
        host[0].outCan --> {  delay = parent.canDelay; } --> can.in;
        can.out --> {  delay = parent.canDelay; } --> host[0].inCan;
        host[0].outAnotherCan --> {  delay = parent.canDelay; } --> anotherCan.in;
        anotherCan.out --> {  delay = parent.canDelay; } --> host[0].inAnotherCan;
}
