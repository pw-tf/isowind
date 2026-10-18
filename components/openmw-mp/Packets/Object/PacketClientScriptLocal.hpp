#ifndef OPENMW_PACKETCLIENTSCRIPTLOCAL_HPP
#define OPENMW_PACKETCLIENTSCRIPTLOCAL_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketClientScriptLocal : public ObjectPacket
    {
    public:
        PacketClientScriptLocal(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &obj, bool send);
    };
}

#endif //OPENMW_PACKETCLIENTSCRIPTLOCAL_HPP
