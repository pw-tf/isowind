#include "PacketPlayerStatsDynamic.hpp"

#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerStatsDynamic::PacketPlayerStatsDynamic(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_STATS_DYNAMIC;
}

void PacketPlayerStatsDynamic::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->exchangeFullInfo, send);

    if (player->exchangeFullInfo)
    {
        RW(player->creatureStats.mDynamic, send);
    }
    else
    {
        uint32_t count;

        if (send)
            count = static_cast<uint32_t>(player->statsDynamicIndexChanges.size());

        RW(count, send);

        if (!send)
        {
            player->statsDynamicIndexChanges.clear();
            player->statsDynamicIndexChanges.resize(count);
        }

        for (auto &&statsDynamicIndex : player->statsDynamicIndexChanges)
        {
            RW(statsDynamicIndex, send);
            if (statsDynamicIndex >= 3)
            {
                packetValid = false;
                return;
            }
            RW(player->creatureStats.mDynamic[statsDynamicIndex], send);
        }
    }
}
