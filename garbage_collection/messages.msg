namespace garbage_collection;

packet GarbagePacket
{
    string command;
    int canId = -1;
    bool isFull = false;
    double travelTime = 0;
    string note;
}
