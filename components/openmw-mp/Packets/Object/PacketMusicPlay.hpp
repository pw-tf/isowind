#ifndef OPENMW_PACKETMUSICPLAY_HPP
#define OPENMW_PACKETMUSICPLAY_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketMusicPlay : public ObjectPacket
    {
    public:
        PacketMusicPlay(RakNet::RakPeerInterface *peer);

        virtual void Object(BaseObject &baseObject, bool send);
    };
}

#endif //OPENMW_PACKETMUSICPLAY_HPP
