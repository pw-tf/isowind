#ifndef OPENMW_PACKETPLAYERREPUTATION_HPP
#define OPENMW_PACKETPLAYERREPUTATION_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerReputation : public PlayerPacket
    {
    public:
        PacketPlayerReputation(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERREPUTATION_HPP
