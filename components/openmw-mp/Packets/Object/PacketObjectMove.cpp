#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectMove.hpp"

using namespace mwmp;

PacketObjectMove::PacketObjectMove(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_MOVE;
    hasCellData = true;
}

void PacketObjectMove::Object(BaseObject &baseObject, bool send)
{
    ObjectPacket::Object(baseObject, send);
    RW(baseObject.position.pos, send);
}
