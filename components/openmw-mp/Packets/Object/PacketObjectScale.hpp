#ifndef OPENMW_PACKETOBJECTSCALE_HPP
#define OPENMW_PACKETOBJECTSCALE_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectScale : public ObjectPacket
    {
    public:
        PacketObjectScale(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTSCALE_HPP
