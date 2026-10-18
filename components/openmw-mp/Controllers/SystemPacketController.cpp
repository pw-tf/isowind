#include "../Packets/System/PacketSystemHandshake.hpp"

#include "SystemPacketController.hpp"

template <typename T>
inline void AddPacket(mwmp::SystemPacketController::packets_t *packets, RakNet::RakPeerInterface *peer)
{
    T *packet = new T(peer);
    typedef mwmp::SystemPacketController::packets_t::value_type value_t;
    packets->insert(value_t(packet->GetPacketID(), value_t::second_type(packet)));
}

mwmp::SystemPacketController::SystemPacketController(RakNet::RakPeerInterface *peer)
{
    AddPacket<PacketSystemHandshake>(&packets, peer);
}


mwmp::SystemPacket *mwmp::SystemPacketController::GetPacket(RakNet::MessageID id)
{
    return packets[(unsigned char)id].get();
}

void mwmp::SystemPacketController::SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    for(const auto &packet : packets)
        packet.second->SetStreams(inStream, outStream);
}

bool mwmp::SystemPacketController::ContainsPacket(RakNet::MessageID id)
{
    for(const auto &packet : packets)
    {
        if (packet.first == id)
            return true;
    }
    return false;
}
