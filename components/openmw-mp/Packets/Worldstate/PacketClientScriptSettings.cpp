#include "PacketClientScriptSettings.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketClientScriptSettings::PacketClientScriptSettings(RakNet::RakPeerInterface *peer) : WorldstatePacket(peer)
{
    packetID = ID_CLIENT_SCRIPT_SETTINGS;
    orderChannel = CHANNEL_WORLDSTATE;
}

void PacketClientScriptSettings::Packet(RakNet::BitStream *newBitstream, bool send)
{
    WorldstatePacket::Packet(newBitstream, send);

    uint32_t clientScriptsCount;

    if (send)
        clientScriptsCount = static_cast<uint32_t>(worldstate->synchronizedClientScriptIds.size());

    RW(clientScriptsCount, send);

    if (!send)
    {
        worldstate->synchronizedClientScriptIds.clear();
        worldstate->synchronizedClientScriptIds.resize(clientScriptsCount);
    }

    for (auto &&clientScriptId : worldstate->synchronizedClientScriptIds)
    {
        RW(clientScriptId, send, true);
    }

    uint32_t clientGlobalsCount;

    if (send)
        clientGlobalsCount = static_cast<uint32_t>(worldstate->synchronizedClientGlobalIds.size());

    RW(clientGlobalsCount, send);

    if (!send)
    {
        worldstate->synchronizedClientGlobalIds.clear();
        worldstate->synchronizedClientGlobalIds.resize(clientGlobalsCount);
    }

    for (auto &&clientGlobalId : worldstate->synchronizedClientGlobalIds)
    {
        RW(clientGlobalId, send, true);
    }
}
