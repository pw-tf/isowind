#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketSystemHandshake.hpp"

using namespace mwmp;

PacketSystemHandshake::PacketSystemHandshake(RakNet::RakPeerInterface *peer) : SystemPacket(peer)
{
    packetID = ID_SYSTEM_HANDSHAKE;
    orderChannel = CHANNEL_SYSTEM;
}

void PacketSystemHandshake::Packet(RakNet::BitStream *newBitstream, bool send)
{
    SystemPacket::Packet(newBitstream, send);

    if (!RW(system->playerName, send, true, maxNameLength) ||
        !RW(system->serverPassword, send, true, maxPasswordLength))
    {
        packetValid = false;
        return;
    }
}
