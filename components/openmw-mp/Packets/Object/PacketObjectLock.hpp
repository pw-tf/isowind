#ifndef OPENMW_PACKETOBJECTLOCK_HPP
#define OPENMW_PACKETOBJECTLOCK_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectLock : public ObjectPacket
    {
    public:
        PacketObjectLock(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTLOCK_HPP
