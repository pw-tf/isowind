#include "PacketWorldDestinationOverride.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

#include <components/openmw-mp/TimedLog.hpp>

using namespace mwmp;

PacketWorldDestinationOverride::PacketWorldDestinationOverride(RakNet::RakPeerInterface *peer) : WorldstatePacket(peer)
{
    packetID = ID_WORLD_DESTINATION_OVERRIDE;
    orderChannel = CHANNEL_WORLDSTATE;
}

void PacketWorldDestinationOverride::Packet(RakNet::BitStream *newBitstream, bool send)
{
    WorldstatePacket::Packet(newBitstream, send);

    uint32_t destinationCount;

    if (send)
        destinationCount = static_cast<uint32_t>(worldstate->destinationOverrides.size());

    RW(destinationCount, send);

    if (!send)
    {
        worldstate->destinationOverrides.clear();
    }

    std::string mapIndex;
    std::string mapValue;

    if (send)
    {
        for (auto &&destinationOverride : worldstate->destinationOverrides)
        {
            mapIndex = destinationOverride.first;
            mapValue = destinationOverride.second;
            RW(mapIndex, send, false);
            RW(mapValue, send, false);
        }
    }
    else
    {
        for (unsigned int n = 0; n < destinationCount; n++)
        {
            RW(mapIndex, send, false);
            RW(mapValue, send, false);
            worldstate->destinationOverrides[mapIndex] = mapValue;
        }
    }
}
