#ifndef OPENMW_PACKETOBJECTSPAWN_HPP
#define OPENMW_PACKETOBJECTSPAWN_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectSpawn : public ObjectPacket
    {
    public:
        PacketObjectSpawn(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTSPAWN_HPP
