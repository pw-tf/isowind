#include <components/openmw-mp/NetworkMessages.hpp>
#include <iostream>
#include "PacketMasterAnnounce.hpp"
#include "ProxyMasterPacket.hpp"

using namespace mwmp;
using namespace RakNet;

PacketMasterAnnounce::PacketMasterAnnounce(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_MASTER_ANNOUNCE;
    orderChannel = CHANNEL_MASTER;
    reliability = RELIABLE_ORDERED_WITH_ACK_RECEIPT;
}

void PacketMasterAnnounce::Packet(BitStream *newBitstream, bool send)
{
    bs = newBitstream;
    if (send)
        bs->Write(packetID);

    RW(func, send);

    if (func == FUNCTION_ANNOUNCE)
        ProxyMasterPacket::addServer(this, *server, send);
}

void PacketMasterAnnounce::SetServer(QueryData *_server)
{
    server = _server;
}

void PacketMasterAnnounce::SetFunc(uint32_t _func)
{
    func = _func;
}

int PacketMasterAnnounce::GetFunc()
{
    return func;
}
