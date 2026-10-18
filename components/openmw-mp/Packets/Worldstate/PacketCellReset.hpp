#ifndef OPENMW_PACKETCELLRESET_HPP
#define OPENMW_PACKETCELLRESET_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketCellReset: public WorldstatePacket
    {
    public:
        PacketCellReset(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETCELLRESET_HPP
