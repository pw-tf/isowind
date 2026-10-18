#ifndef OPENMW_PACKETACTORSPELLSACTIVE_HPP
#define OPENMW_PACKETACTORSPELLSACTIVE_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorSpellsActive : public ActorPacket
    {
    public:
        PacketActorSpellsActive(RakNet::RakPeerInterface *peer);

        virtual void Actor(BaseActor &actor, bool send);

    protected:
        static const int maxEffects = 20;
    };
}

#endif //OPENMW_PACKETACTORSPELLSACTIVE_HPP
