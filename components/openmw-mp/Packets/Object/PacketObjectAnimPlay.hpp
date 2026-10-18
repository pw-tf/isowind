#ifndef OPENMW_PACKETOBJECTANIMPLAY_HPP
#define OPENMW_PACKETOBJECTANIMPLAY_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectAnimPlay : public ObjectPacket
    {
    public:
        PacketObjectAnimPlay(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTANIMPLAY_HPP
