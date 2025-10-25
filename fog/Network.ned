package fog;

network FogNetwork
{
    parameters:
        double propDelay @unit(s) = default(100ms);  // video uses +100ms ticks
    submodules:
        computerNode: Tic1   { @display("p=100,100"); }
        cloudNode:    Toc1   { @display("p=540,100"); }
    connections allowunconnected:
        computerNode.out --> { delay = parent.propDelay; } --> cloudNode.in;
        cloudNode.out    --> { delay = parent.propDelay; } --> computerNode.in;
}
