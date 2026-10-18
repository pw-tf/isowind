#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerAlly.hpp"

mwmp::PacketPlayerAlly::PacketPlayerAlly(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ALLY;
}

void mwmp::PacketPlayerAlly::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    uint32_t count;

    if (send)
        count = static_cast<uint32_t>(player->alliedPlayers.size());

    RW(count, send);

    if (!send)
    {
        player->alliedPlayers.clear();
        player->alliedPlayers.resize(count);
    }

    for (auto &&teamPlayerGuid : player->alliedPlayers)
    {
        RW(teamPlayerGuid, send, true);
    }
}
