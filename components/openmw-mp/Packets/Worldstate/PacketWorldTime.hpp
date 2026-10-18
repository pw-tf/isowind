#ifndef OPENMW_PACKETWORLDTIME_HPP
#define OPENMW_PACKETWORLDTIME_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>

namespace mwmp
{
    class PacketWorldTime : public WorldstatePacket
    {
    public:
        PacketWorldTime(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETWORLDTIME_HPP
