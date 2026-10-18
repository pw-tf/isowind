#include "PacketPlayerPosition.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerPosition::PacketPlayerPosition(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_POSITION;
    priority = MEDIUM_PRIORITY;
    //reliability = UNRELIABLE_SEQUENCED;
}

void PacketPlayerPosition::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->position, send, 1);
    RW(player->direction, send, 1);
}
