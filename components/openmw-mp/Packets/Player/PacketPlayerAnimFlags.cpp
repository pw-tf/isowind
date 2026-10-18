#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerAnimFlags.hpp"

mwmp::PacketPlayerAnimFlags::PacketPlayerAnimFlags(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ANIM_FLAGS;
}

void mwmp::PacketPlayerAnimFlags::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->movementFlags, send);
    RW(player->drawState, send);
    RW(player->isJumping, send);
    RW(player->isFlying, send);
    RW(player->hasTcl, send);
}
