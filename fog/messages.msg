namespace fog;

message AppPacket
{
    int type = 0;  // 1=HELLO, 2=ACK, 3=TEST
    int seq = 0;   // attempt counter for TEST
}