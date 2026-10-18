#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include "PacketActorDeath.hpp"

using namespace mwmp;

PacketActorDeath::PacketActorDeath(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_DEATH;
}

void PacketActorDeath::Actor(BaseActor &actor, bool send)
{
    RW(actor.refId, send);

    RW(actor.deathState, send);
    RW(actor.isInstantDeath, send);
    RW(actor.killer.isPlayer, send);

    if (actor.killer.isPlayer)
    {
        RW(actor.killer.guid, send);
    }
    else
    {
        RW(actor.killer.refId, send, true);
        RW(actor.killer.refNum, send);
        RW(actor.killer.mpNum, send);

        RW(actor.killer.name, send, true);
    }
}
