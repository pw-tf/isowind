#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include "PacketActorCast.hpp"

using namespace mwmp;

PacketActorCast::PacketActorCast(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_CAST;
}

void PacketActorCast::Actor(BaseActor &actor, bool send)
{
    RW(actor.cast.target.isPlayer, send);

    if (actor.cast.target.isPlayer)
    {
        RW(actor.cast.target.guid, send);
    }
    else
    {
        RW(actor.cast.target.refId, send, true);
        RW(actor.cast.target.refNum, send);
        RW(actor.cast.target.mpNum, send);
    }

    RW(actor.cast.type, send);

    if (actor.cast.type == mwmp::Cast::ITEM)
        RW(actor.cast.itemId, send, true);
    else
    {
        RW(actor.cast.pressed, send);
        RW(actor.cast.success, send);

        RW(actor.cast.instant, send);
        RW(actor.cast.spellId, send, true);
    }

    RW(actor.cast.hasProjectile, send);

    if (actor.cast.hasProjectile)
    {
        RW(actor.cast.projectileOrigin.origin[0], send);
        RW(actor.cast.projectileOrigin.origin[1], send);
        RW(actor.cast.projectileOrigin.origin[2], send);
        RW(actor.cast.projectileOrigin.orientation[0], send);
        RW(actor.cast.projectileOrigin.orientation[1], send);
        RW(actor.cast.projectileOrigin.orientation[2], send);
        RW(actor.cast.projectileOrigin.orientation[3], send);
    }
}
