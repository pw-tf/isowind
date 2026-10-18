#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectPlace.hpp"

using namespace mwmp;

PacketObjectPlace::PacketObjectPlace(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_PLACE;
    hasCellData = true;
}

void PacketObjectPlace::Object(BaseObject &baseObject, bool send)
{
    ObjectPacket::Object(baseObject, send);
    RW(baseObject.count, send);
    RW(baseObject.charge, send);
    RW(baseObject.enchantmentCharge, send);
    RW(baseObject.soul, send, true);
    RW(baseObject.goldValue, send);
    RW(baseObject.position, send);
    RW(baseObject.droppedByPlayer, send);
    RW(baseObject.hasContainer, send);
}
