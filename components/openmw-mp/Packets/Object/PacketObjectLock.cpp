#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectLock.hpp"

using namespace mwmp;

PacketObjectLock::PacketObjectLock(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_LOCK;
    hasCellData = true;
}

void PacketObjectLock::Object(BaseObject &baseObject, bool send)
{
    ObjectPacket::Object(baseObject, send);
    RW(baseObject.lockLevel, send);
}
