#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectState.hpp"

using namespace mwmp;

PacketObjectState::PacketObjectState(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_STATE;
    hasCellData = true;
}

void PacketObjectState::Object(BaseObject &baseObject, bool send)
{
    ObjectPacket::Object(baseObject, send);
    RW(baseObject.objectState, send);
}
