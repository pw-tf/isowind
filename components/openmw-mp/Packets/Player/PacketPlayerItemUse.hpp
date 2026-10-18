#ifndef OPENMW_PACKETPLAYERITEMUSE_HPP
#define OPENMW_PACKETPLAYERITEMUSE_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerItemUse : public PlayerPacket
    {
    public:
        PacketPlayerItemUse(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERITEMUSE_HPP
