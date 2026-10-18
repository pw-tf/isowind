#ifndef OPENMW_PACKETOBJECTSOUND_HPP
#define OPENMW_PACKETOBJECTSOUND_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectSound : public ObjectPacket
    {
    public:
        PacketObjectSound(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTSOUND_HPP
