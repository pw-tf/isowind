#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectMiscellaneous.hpp"

using namespace mwmp;

PacketObjectMiscellaneous::PacketObjectMiscellaneous(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_MISCELLANEOUS;
    hasCellData = true;
}

void PacketObjectMiscellaneous::Object(BaseObject &baseObject, bool send)
{
    ObjectPacket::Object(baseObject, send);
    RW(baseObject.goldPool, send);
    RW(baseObject.lastGoldRestockHour, send);
    RW(baseObject.lastGoldRestockDay, send);
}
