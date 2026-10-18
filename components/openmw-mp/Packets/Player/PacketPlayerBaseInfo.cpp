#include "PacketPlayerBaseInfo.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerBaseInfo::PacketPlayerBaseInfo(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_BASEINFO;
}

void PacketPlayerBaseInfo::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->npc.mName, send, true);
    RW(player->npc.mModel, send, true);
    RW(player->npc.mRace, send, true);
    RW(player->npc.mHair, send, true);
    RW(player->npc.mHead, send, true);

    RW(player->npc.mFlags, send);

    RW(player->birthsign, send, true);

    RW(player->resetStats, send);
}
