#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectRestock.hpp"

using namespace mwmp;

PacketObjectRestock::PacketObjectRestock(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_RESTOCK;
    hasCellData = true;
}
