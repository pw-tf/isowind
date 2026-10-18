#include "PacketWorldCollisionOverride.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketWorldCollisionOverride::PacketWorldCollisionOverride(RakNet::RakPeerInterface *peer) : WorldstatePacket(peer)
{
    packetID = ID_WORLD_COLLISION_OVERRIDE;
    orderChannel = CHANNEL_WORLDSTATE;
}

void PacketWorldCollisionOverride::Packet(RakNet::BitStream *newBitstream, bool send)
{
    WorldstatePacket::Packet(newBitstream, send);

    RW(worldstate->hasPlayerCollision, send);
    RW(worldstate->hasActorCollision, send);
    RW(worldstate->hasPlacedObjectCollision, send);
    RW(worldstate->useActorCollisionForPlacedObjects, send);

    uint32_t enforcedCollisionCount;

    if (send)
        enforcedCollisionCount = static_cast<uint32_t>(worldstate->enforcedCollisionRefIds.size());

    RW(enforcedCollisionCount, send);

    if (!send)
    {
        worldstate->enforcedCollisionRefIds.clear();
        worldstate->enforcedCollisionRefIds.resize(enforcedCollisionCount);
    }

    for (auto &&enforcedCollisionRefId : worldstate->enforcedCollisionRefIds)
    {
        RW(enforcedCollisionRefId, send, true);
    }
}
