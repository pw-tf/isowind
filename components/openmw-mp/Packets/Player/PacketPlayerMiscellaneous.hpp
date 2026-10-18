#ifndef OPENMW_PACKETPLAYERMISCELLANEOUS_HPP
#define OPENMW_PACKETPLAYERMISCELLANEOUS_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerMiscellaneous : public PlayerPacket
    {
    public:
        PacketPlayerMiscellaneous(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERMISCELLANEOUS_HPP
