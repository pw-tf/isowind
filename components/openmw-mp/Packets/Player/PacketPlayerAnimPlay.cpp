#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerAnimPlay.hpp"

mwmp::PacketPlayerAnimPlay::PacketPlayerAnimPlay(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ANIM_PLAY;
}

void mwmp::PacketPlayerAnimPlay::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->animation.groupname, send);
    RW(player->animation.mode, send);
    RW(player->animation.count, send);
    RW(player->animation.persist, send);
}
