#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketWorldRegionAuthority.hpp"

mwmp::PacketWorldRegionAuthority::PacketWorldRegionAuthority(RakNet::RakPeerInterface *peer) : WorldstatePacket(peer)
{
    packetID = ID_WORLD_REGION_AUTHORITY;
    // Make sure the priority is lower than PlayerCellChange's, so it doesn't get sent before it
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketWorldRegionAuthority::Packet(RakNet::BitStream *newBitstream, bool send)
{
    WorldstatePacket::Packet(newBitstream, send);

    RW(worldstate->authorityRegion, send, true);
}
