#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCellChange.hpp"


mwmp::PacketPlayerCellChange::PacketPlayerCellChange(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CELL_CHANGE;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketPlayerCellChange::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->cell.mData, send, true);
    RW(player->cell.mName, send, true);

    RW(player->previousCellPosition.pos, send, true);

    RW(player->isChangingRegion, send);

    if (player->isChangingRegion)
        RW(player->cell.mRegion, send, true);
}
