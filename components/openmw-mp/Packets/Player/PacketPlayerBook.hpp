#ifndef OPENMW_PACKETPLAYERBOOK_HPP
#define OPENMW_PACKETPLAYERBOOK_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerBook : public PlayerPacket
    {
    public:
        PacketPlayerBook(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERBOOK_HPP
