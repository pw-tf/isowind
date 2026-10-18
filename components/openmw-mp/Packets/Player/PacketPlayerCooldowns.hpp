#ifndef OPENMW_PACKETPLAYERCOOLDOWNS_HPP
#define OPENMW_PACKETPLAYERCOOLDOWNS_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerCooldowns : public PlayerPacket
    {
    public:
        PacketPlayerCooldowns(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERCOOLDOWNS_HPP
