#ifndef OPENMW_PACKETDOORDESTINATION_HPP
#define OPENMW_PACKETDOORDESTINATION_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketDoorDestination : public ObjectPacket
    {
    public:
        PacketDoorDestination(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETDOORDESTINATION_HPP
