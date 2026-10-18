#ifndef OPENMW_PACKETWORLDKILLCOUNT_HPP
#define OPENMW_PACKETWORLDKILLCOUNT_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketWorldKillCount: public WorldstatePacket
    {
    public:
        PacketWorldKillCount(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETWORLDKILLCOUNT_HPP
