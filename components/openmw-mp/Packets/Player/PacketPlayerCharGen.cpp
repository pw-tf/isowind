#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCharGen.hpp"

mwmp::PacketPlayerCharGen::PacketPlayerCharGen(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CHARGEN;
}

void mwmp::PacketPlayerCharGen::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->charGenState, send);

}
