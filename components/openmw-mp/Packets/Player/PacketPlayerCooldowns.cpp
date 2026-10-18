#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCooldowns.hpp"

using namespace mwmp;

PacketPlayerCooldowns::PacketPlayerCooldowns(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_COOLDOWNS;
}

void PacketPlayerCooldowns::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    uint32_t count;

    if (send)
        count = static_cast<uint32_t>(player->cooldownChanges.size());

    RW(count, send);

    if (!send)
    {
        player->cooldownChanges.clear();
        player->cooldownChanges.resize(count);
    }

    for (auto &&spell : player->cooldownChanges)
    {
        RW(spell.id, send, true);
        RW(spell.startTimestampDay, send);
        RW(spell.startTimestampHour, send);
    }
}
