#ifndef OPENMW_PACKETPLAYERINPUT_HPP
#define OPENMW_PACKETPLAYERINPUT_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerInput : public PlayerPacket
    {
    public:
        PacketPlayerInput(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERINPUT_HPP
