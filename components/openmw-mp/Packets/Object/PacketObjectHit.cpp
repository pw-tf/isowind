#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectHit.hpp"

using namespace mwmp;

PacketObjectHit::PacketObjectHit(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_HIT;
    hasCellData = true;
}

void PacketObjectHit::Packet(RakNet::BitStream *newBitstream, bool send)
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

        RW(baseObject.hittingActor.isPlayer, send);

        if (baseObject.hittingActor.isPlayer)
        {
            RW(baseObject.hittingActor.guid, send);
        }
        else
        {
            RW(baseObject.hittingActor.refId, send, true);
            RW(baseObject.hittingActor.refNum, send);
            RW(baseObject.hittingActor.mpNum, send);

            RW(baseObject.hittingActor.name, send);
        }

        RW(baseObject.hitAttack.success, send);

        if (baseObject.hitAttack.success)
        {
            RW(baseObject.hitAttack.damage, send);
            RW(baseObject.hitAttack.block, send);
            RW(baseObject.hitAttack.knockdown, send);
        }

        if (!send)
            objectList->baseObjects.push_back(baseObject);
    }
}
