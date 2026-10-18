#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectRotate.hpp"

using namespace mwmp;

PacketObjectRotate::PacketObjectRotate(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_ROTATE;
    hasCellData = true;
}

void PacketObjectRotate::Object(BaseObject &baseObject, bool send)
{
    ObjectPacket::Object(baseObject, send);
    RW(baseObject.position.rot[0], send);
    RW(baseObject.position.rot[1], send);
    RW(baseObject.position.rot[2], send);
}
