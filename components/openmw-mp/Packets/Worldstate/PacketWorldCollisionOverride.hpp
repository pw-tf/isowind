#ifndef OPENMW_PACKETWORLDCOLLISIONOVERRIDE_HPP
#define OPENMW_PACKETWORLDCOLLISIONOVERRIDE_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>

namespace mwmp
{
    class PacketWorldCollisionOverride : public WorldstatePacket
    {
    public:
        PacketWorldCollisionOverride(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETWORLDCOLLISIONOVERRIDE_HPP
