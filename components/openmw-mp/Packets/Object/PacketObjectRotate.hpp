#ifndef OPENMW_PACKETOBJECTROTATE_HPP
#define OPENMW_PACKETOBJECTROTATE_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectRotate : public ObjectPacket
    {
    public:
        PacketObjectRotate(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTROTATE_HPP
