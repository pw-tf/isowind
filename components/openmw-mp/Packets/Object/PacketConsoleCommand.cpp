#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketConsoleCommand.hpp"

using namespace mwmp;

PacketConsoleCommand::PacketConsoleCommand(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_CONSOLE_COMMAND;
    hasCellData = true;
}

void PacketConsoleCommand::Packet(RakNet::BitStream *newBitstream, bool send)
{
    if (!PacketHeader(newBitstream, send))
        return;

    RW(objectList->consoleCommand, send, true);

    BaseObject baseObject;
    for (unsigned int i = 0; i < objectList->baseObjectCount; i++)
    {
        if (send)
            baseObject = objectList->baseObjects.at(i);

        RW(baseObject.isPlayer, send);

        if (baseObject.isPlayer)
            RW(baseObject.guid, send);
        else
            Object(baseObject, send);

        if (!send)
            objectList->baseObjects.push_back(baseObject);
    }
}
