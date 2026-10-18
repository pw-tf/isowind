#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectActivate.hpp"

using namespace mwmp;

PacketObjectActivate::PacketObjectActivate(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_ACTIVATE;
    hasCellData = true;
}

void PacketObjectActivate::Packet(RakNet::BitStream *newBitstream, bool send)
{
    if (!PacketHeader(newBitstream, send))
        return;

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

        RW(baseObject.activatingActor.isPlayer, send);

        if (baseObject.activatingActor.isPlayer)
        {
            RW(baseObject.activatingActor.guid, send);
        }
        else
        {
            RW(baseObject.activatingActor.refId, send, true);
            RW(baseObject.activatingActor.refNum, send);
            RW(baseObject.activatingActor.mpNum, send);

            RW(baseObject.activatingActor.name, send);
        }

        if (!send)
            objectList->baseObjects.push_back(baseObject);
    }
}
