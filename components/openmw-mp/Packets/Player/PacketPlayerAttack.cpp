#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerAttack.hpp"

using namespace mwmp;

PacketPlayerAttack::PacketPlayerAttack(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ATTACK;
}

void PacketPlayerAttack::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->attack.target.isPlayer, send);

    if (player->attack.target.isPlayer)
    {
        RW(player->attack.target.guid, send);
    }
    else
    {
        RW(player->attack.target.refId, send, true);
        RW(player->attack.target.refNum, send);
        RW(player->attack.target.mpNum, send);
    }

    RW(player->attack.type, send);

    RW(player->attack.pressed, send);
    RW(player->attack.success, send);

    RW(player->attack.isHit, send);

    if (player->attack.type == mwmp::Attack::MELEE)
    {
        RW(player->attack.attackAnimation, send, true);
    }
    else if (player->attack.type == mwmp::Attack::RANGED)
    {
        RW(player->attack.attackStrength, send);
        RW(player->attack.rangedWeaponId, send, true);
        RW(player->attack.rangedAmmoId, send, true);

        RW(player->attack.projectileOrigin.origin[0], send);
        RW(player->attack.projectileOrigin.origin[1], send);
        RW(player->attack.projectileOrigin.origin[2], send);
        RW(player->attack.projectileOrigin.orientation[0], send);
        RW(player->attack.projectileOrigin.orientation[1], send);
        RW(player->attack.projectileOrigin.orientation[2], send);
        RW(player->attack.projectileOrigin.orientation[3], send);
    }

    if (player->attack.isHit)
    {
        RW(player->attack.damage, send);
        RW(player->attack.block, send);
        RW(player->attack.knockdown, send);
        RW(player->attack.applyWeaponEnchantment, send);

        if (player->attack.type == mwmp::Attack::RANGED)
            RW(player->attack.applyAmmoEnchantment, send);

        RW(player->attack.hitPosition.pos[0], send);
        RW(player->attack.hitPosition.pos[1], send);
        RW(player->attack.hitPosition.pos[2], send);
    }
}
