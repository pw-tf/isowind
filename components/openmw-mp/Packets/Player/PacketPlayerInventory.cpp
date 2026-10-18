#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerInventory.hpp"

using namespace mwmp;

PacketPlayerInventory::PacketPlayerInventory(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_INVENTORY;
}

void PacketPlayerInventory::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->inventoryChanges.action, send);

    uint32_t count;

    if (send)
        count = static_cast<uint32_t>(player->inventoryChanges.items.size());

    RW(count, send);

    if (!send)
    {
        player->inventoryChanges.items.clear();
        player->inventoryChanges.items.resize(count);
    }

    for (auto &&item : player->inventoryChanges.items)
    {
        RW(item.refId, send, true);
        RW(item.count, send);
        RW(item.charge, send);
        RW(item.enchantmentCharge, send);
        RW(item.soul, send, true);
    }
}
