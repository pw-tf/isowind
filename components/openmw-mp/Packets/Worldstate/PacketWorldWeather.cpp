#include "PacketWorldWeather.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketWorldWeather::PacketWorldWeather(RakNet::RakPeerInterface *peer) : WorldstatePacket(peer)
{
    packetID = ID_WORLD_WEATHER;
    orderChannel = CHANNEL_WORLDSTATE;
}

void PacketWorldWeather::Packet(RakNet::BitStream *newBitstream, bool send)
{
    WorldstatePacket::Packet(newBitstream, send);

    RW(worldstate->forceWeather, send);
    RW(worldstate->weather.region, send, true);
    RW(worldstate->weather.currentWeather, send);
    RW(worldstate->weather.nextWeather, send);
    RW(worldstate->weather.queuedWeather, send);
    RW(worldstate->weather.transitionFactor, send);
}
