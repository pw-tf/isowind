#include "PacketPlayerDeath.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerDeath::PacketPlayerDeath(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_DEATH;
}

void PacketPlayerDeath::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->deathState, send);
    RW(player->killer.isPlayer, send);

    if (player->killer.isPlayer)
    {
        RW(player->killer.guid, send);
    }
    else
    {
        RW(player->killer.refId, send, true);
        RW(player->killer.refNum, send);
        RW(player->killer.mpNum, send);

        RW(player->killer.name, send, true);
    }
}
