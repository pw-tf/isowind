#ifndef OPENMW_PACKETWORLDREGIONAUTHORITY_HPP
#define OPENMW_PACKETWORLDREGIONAUTHORITY_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>

namespace mwmp
{
    class PacketWorldRegionAuthority : public WorldstatePacket
    {
    public:
        PacketWorldRegionAuthority(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETWORLDREGIONAUTHORITY_HPP
