#include "PacketPlayerAttribute.hpp"

#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerAttribute::PacketPlayerAttribute(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ATTRIBUTE;
}

void PacketPlayerAttribute::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->exchangeFullInfo, send);

    if (player->exchangeFullInfo)
    {
        RW(player->creatureStats.mAttributes, send);
        RW(player->npcStats.mSkillIncrease, send);
    }
    else
    {
        uint32_t count;

        if (send)
            count = static_cast<uint32_t>(player->attributeIndexChanges.size());

        RW(count, send);

        if (!send)
        {
            player->attributeIndexChanges.clear();
            player->attributeIndexChanges.resize(count);
        }

        for (auto &&attributeIndex : player->attributeIndexChanges)
        {
            RW(attributeIndex, send);

            if (attributeIndex >= 8)
            {
                packetValid = false;
                return;
            }

            RW(player->creatureStats.mAttributes[attributeIndex], send);
            RW(player->npcStats.mSkillIncrease[attributeIndex], send);
        }
    }
}
