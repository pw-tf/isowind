#ifndef OPENMW_PACKETOBJECTDELETE_HPP
#define OPENMW_PACKETOBJECTDELETE_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectDelete : public ObjectPacket
    {
    public:
        PacketObjectDelete(RakNet::RakPeerInterface *peer);
    };
}

#endif //OPENMW_PACKETOBJECTDELETE_HPP
