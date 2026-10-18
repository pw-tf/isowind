#ifndef OPENMW_PACKETALLY_HPP
#define OPENMW_PACKETALLY_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerAlly : public PlayerPacket
    {
    public:
        PacketPlayerAlly(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETALLY_HPP
