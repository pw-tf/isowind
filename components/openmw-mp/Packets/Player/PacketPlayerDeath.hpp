#ifndef OPENMW_PACKETPLAYERDEATH_HPP
#define OPENMW_PACKETPLAYERDEATH_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerDeath: public PlayerPacket
    {
    public:
        PacketPlayerDeath(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERDEATH_HPP
