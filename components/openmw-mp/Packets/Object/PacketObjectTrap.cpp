#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectTrap.hpp"

using namespace mwmp;

PacketObjectTrap::PacketObjectTrap(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_TRAP;
    hasCellData = true;
}

void PacketObjectTrap::Object(BaseObject &baseObject, bool send)
{
    ObjectPacket::Object(baseObject, send);
    RW(baseObject.isDisarmed, send);

    if (!baseObject.isDisarmed)
        RW(baseObject.position, send);
}
