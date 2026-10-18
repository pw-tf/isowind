#ifndef OPENMW_PACKETPLAYERINVENTORY_HPP
#define OPENMW_PACKETPLAYERINVENTORY_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerInventory : public PlayerPacket
    {
    public:
        PacketPlayerInventory(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERINVENTORY_HPP
