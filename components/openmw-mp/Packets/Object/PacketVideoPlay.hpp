#ifndef OPENMW_PACKETVIDEOPLAY_HPP
#define OPENMW_PACKETVIDEOPLAY_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketVideoPlay : public ObjectPacket
    {
    public:
        PacketVideoPlay(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &obj, bool send);
    };
}

#endif //OPENMW_PACKETVIDEOPLAY_HPP
