#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketActorAuthority.hpp"

using namespace mwmp;

PacketActorAuthority::PacketActorAuthority(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_AUTHORITY;
}

void PacketActorAuthority::Packet(RakNet::BitStream *newBitstream, bool send)
{
    BasePacket::Packet(newBitstream, send);

    RW(actorList->cell.mData, send, true);
    RW(actorList->cell.mName, send, true);
}
