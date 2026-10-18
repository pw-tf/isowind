#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerTopic.hpp"

using namespace mwmp;

PacketPlayerTopic::PacketPlayerTopic(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_TOPIC;
}

void PacketPlayerTopic::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    uint32_t count;

    if (send)
        count = static_cast<uint32_t>(player->topicChanges.size());

    RW(count, send);

    if (!send)
    {
        player->topicChanges.clear();
        player->topicChanges.resize(count);
    }

    for (auto &&topic : player->topicChanges)
    {
        RW(topic.topicId, send, true);
    }
}
