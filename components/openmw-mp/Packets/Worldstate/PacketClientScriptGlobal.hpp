#ifndef OPENMW_PACKETCLIENTSCRIPTGLOBAL_HPP
#define OPENMW_PACKETCLIENTSCRIPTGLOBAL_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketClientScriptGlobal: public WorldstatePacket
    {
    public:
        PacketClientScriptGlobal(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETCLIENTSCRIPTGLOBAL_HPP
