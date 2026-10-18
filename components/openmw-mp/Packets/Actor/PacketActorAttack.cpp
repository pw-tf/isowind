#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include "PacketActorAttack.hpp"

using namespace mwmp;

PacketActorAttack::PacketActorAttack(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_ATTACK;
}

void PacketActorAttack::Actor(BaseActor &actor, bool send)
{
    RW(actor.attack.target.isPlayer, send);

    if (actor.attack.target.isPlayer)
    {
        RW(actor.attack.target.guid, send);
    }
    else
    {
        RW(actor.attack.target.refId, send, true);
        RW(actor.attack.target.refNum, send);
        RW(actor.attack.target.mpNum, send);
    }

    RW(actor.attack.type, send);

    RW(actor.attack.pressed, send);
    RW(actor.attack.success, send);

    RW(actor.attack.isHit, send);

    if (actor.attack.type == mwmp::Attack::MELEE)
    {
        RW(actor.attack.attackAnimation, send, true);
    }
    else if (actor.attack.type == mwmp::Attack::RANGED)
    {
        RW(actor.attack.attackStrength, send);
        RW(actor.attack.rangedWeaponId, send, true);
        RW(actor.attack.rangedAmmoId, send, true);

        RW(actor.attack.projectileOrigin.origin[0], send);
        RW(actor.attack.projectileOrigin.origin[1], send);
        RW(actor.attack.projectileOrigin.origin[2], send);
        RW(actor.attack.projectileOrigin.orientation[0], send);
        RW(actor.attack.projectileOrigin.orientation[1], send);
        RW(actor.attack.projectileOrigin.orientation[2], send);
        RW(actor.attack.projectileOrigin.orientation[3], send);
    }

    if (actor.attack.isHit)
    {
        RW(actor.attack.damage, send);
        RW(actor.attack.block, send);
        RW(actor.attack.knockdown, send);
        RW(actor.attack.applyWeaponEnchantment, send);

        if (actor.attack.type == mwmp::Attack::RANGED)
            RW(actor.attack.applyAmmoEnchantment, send);

        RW(actor.attack.hitPosition.pos[0], send);
        RW(actor.attack.hitPosition.pos[1], send);
        RW(actor.attack.hitPosition.pos[2], send);
    }
}
