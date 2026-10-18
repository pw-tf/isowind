#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketMasterUpdate.hpp"
#include "ProxyMasterPacket.hpp"

using namespace mwmp;
using namespace RakNet;

PacketMasterUpdate::PacketMasterUpdate(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_MASTER_UPDATE;
    orderChannel = CHANNEL_MASTER;
    reliability = RELIABLE_ORDERED_WITH_ACK_RECEIPT;
}

void PacketMasterUpdate::Packet(RakNet::BitStream *newBitstream, bool send)
{
    bs = newBitstream;
    if (send)
        bs->Write(packetID);

    std::string addr = server->first.ToString(false);
    uint16_t port = server->first.GetPort();

    RW(addr, send);
    RW(port, send);

    if (!send)
        server->first = SystemAddress(addr.c_str(), port);

    ProxyMasterPacket::addServer(this, server->second, send);

}

void PacketMasterUpdate::SetServer(std::pair<RakNet::SystemAddress, QueryData> *serverPair)
{
    server = serverPair;
}
