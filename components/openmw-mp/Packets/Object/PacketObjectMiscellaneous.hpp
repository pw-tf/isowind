#ifndef OPENMW_PACKETOBJECTMISCELLANEOUS_HPP
#define OPENMW_PACKETOBJECTMISCELLANEOUS_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectMiscellaneous : public ObjectPacket
    {
    public:
        PacketObjectMiscellaneous(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &obj, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTMISCELLANEOUS_HPP
