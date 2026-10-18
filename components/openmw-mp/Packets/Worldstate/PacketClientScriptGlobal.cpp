#include "PacketClientScriptGlobal.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketClientScriptGlobal::PacketClientScriptGlobal(RakNet::RakPeerInterface *peer) : WorldstatePacket(peer)
{
    packetID = ID_CLIENT_SCRIPT_GLOBAL;
    orderChannel = CHANNEL_WORLDSTATE;
}

void PacketClientScriptGlobal::Packet(RakNet::BitStream *newBitstream, bool send)
{
    WorldstatePacket::Packet(newBitstream, send);

    uint32_t clientGlobalsCount;

    if (send)
        clientGlobalsCount = static_cast<uint32_t>(worldstate->clientGlobals.size());

    RW(clientGlobalsCount, send);

    if (!send)
    {
        worldstate->clientGlobals.clear();
        worldstate->clientGlobals.resize(clientGlobalsCount);
    }

    for (auto &&clientGlobal : worldstate->clientGlobals)
    {
        RW(clientGlobal.id, send, true);
        RW(clientGlobal.variableType, send);

        if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::SHORT || clientGlobal.variableType == mwmp::VARIABLE_TYPE::LONG)
            RW(clientGlobal.intValue, send);
        else if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::FLOAT)
            RW(clientGlobal.floatValue, send);
    }
}
