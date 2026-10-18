#ifndef OPENMW_PACKETPLAYEREQUIPMENT_HPP
#define OPENMW_PACKETPLAYEREQUIPMENT_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerEquipment : public PlayerPacket
    {
    public:
        PacketPlayerEquipment(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
        void ExchangeItemInformation(Item &item, bool send);
    };
}

#endif //OPENMW_PACKETPLAYEREQUIPMENT_HPP
