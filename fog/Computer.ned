package fog;

simple Tic1
{
    parameters:
        double timeout @unit(s) = default(1s);
        @display("i=device/laptop");
    gates: input in; output out;
}
