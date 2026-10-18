#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCellState.hpp"


mwmp::PacketPlayerCellState::PacketPlayerCellState(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CELL_STATE;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketPlayerCellState::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    uint32_t count;

    if (send)
        count = static_cast<uint32_t>(player->cellStateChanges.size());

    RW(count, send);

    if (!send)
    {
        player->cellStateChanges.clear();
        player->cellStateChanges.resize(count);
    }

    for (auto &&cellState : player->cellStateChanges)
    {
        RW(cellState.type, send);
        RW(cellState.cell.mData, send, true);
        RW(cellState.cell.mName, send, true);
    }
}
