#ifndef OPENMW_PACKETPLAYERCLASS_HPP
#define OPENMW_PACKETPLAYERCLASS_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerClass : public PlayerPacket
    {
    public:
        PacketPlayerClass(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERCLASS_HPP
