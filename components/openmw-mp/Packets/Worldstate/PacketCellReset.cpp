#include "PacketCellReset.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketCellReset::PacketCellReset(RakNet::RakPeerInterface *peer) : WorldstatePacket(peer)
{
    packetID = ID_CELL_RESET;
    orderChannel = CHANNEL_SYSTEM;
}

void PacketCellReset::Packet(RakNet::BitStream *newBitstream, bool send)
{
    WorldstatePacket::Packet(newBitstream, send);

    uint32_t cellCount;

    if (send)
        cellCount = static_cast<uint32_t>(worldstate->cellsToReset.size());

    RW(cellCount, send);

    if (!send)
    {
        worldstate->cellsToReset.clear();
        worldstate->cellsToReset.resize(cellCount);
    }

    for (auto &&cellToReset : worldstate->cellsToReset)
    {
        RW(cellToReset.mData, send, true);
        RW(cellToReset.mName, send, true);
    }
}
