#ifndef OPENMW_PACKETDOORSTATE_HPP
#define OPENMW_PACKETDOORSTATE_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketDoorState : public ObjectPacket
    {
    public:
        PacketDoorState(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETDOORSTATE_HPP
