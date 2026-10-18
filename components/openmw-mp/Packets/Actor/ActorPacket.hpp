#ifndef OPENMW_ACTORPACKET_HPP
#define OPENMW_ACTORPACKET_HPP

#include <string>
#include <components/openmw-mp/RakNetIncludes.hpp>
#include <BitStream.h>
#include <PacketPriority.h>
#include <components/openmw-mp/Base/BaseActor.hpp>

#include <components/openmw-mp/Packets/BasePacket.hpp>


namespace mwmp
{
    class ActorPacket : public BasePacket
    {
    public:
        ActorPacket(RakNet::RakPeerInterface *peer);

        ~ActorPacket();

        void setActorList(BaseActorList *newActorList);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    protected:
        bool PacketHeader(RakNet::BitStream *newBitstream, bool send);
        virtual void Actor(BaseActor &actor, bool send);
        BaseActorList *actorList;
        static const int maxActors = 3000;
    };
}

#endif //OPENMW_ACTORPACKET_HPP
