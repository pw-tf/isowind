#ifndef OPENMW_PACKETOBJECTRESTOCK_HPP
#define OPENMW_PACKETOBJECTRESTOCK_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketObjectRestock : public ObjectPacket
    {
    public:
        PacketObjectRestock(RakNet::RakPeerInterface *peer);
    };
}

#endif //OPENMW_PACKETOBJECTRESTOCK_HPP
