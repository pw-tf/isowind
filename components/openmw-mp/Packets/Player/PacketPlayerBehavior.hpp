#ifndef OPENMW_PACKETPLAYERBEHAVIOR_HPP
#define OPENMW_PACKETPLAYERBEHAVIOR_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerBehavior : public PlayerPacket
    {
    public:
        PacketPlayerBehavior(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERBEHAVIOR_HPP
