#ifndef OPENMW_PACKETOBJECTATTACH_HPP
#define OPENMW_PACKETOBJECTATTACH_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectAttach : public ObjectPacket
    {
    public:
        PacketObjectAttach(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTATTACH_HPP
