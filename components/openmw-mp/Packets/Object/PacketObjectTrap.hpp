#ifndef OPENMW_PACKETOBJECTTRAP_HPP
#define OPENMW_PACKETOBJECTTRAP_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectTrap : public ObjectPacket
    {
    public:
        PacketObjectTrap(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTTRAP_HPP
