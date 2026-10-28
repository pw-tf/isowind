#include "PacketPlayerMiscellaneous.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerMiscellaneous::PacketPlayerMiscellaneous(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_MISCELLANEOUS;
}

void PacketPlayerMiscellaneous::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->miscellaneousChangeType, send);

    if (player->miscellaneousChangeType == mwmp::MISCELLANEOUS_CHANGE_TYPE::MARK_LOCATION)
    {
        RW(player->markCell.mData, send, true);
        RW(player->markCell.mName, send, true);

        RW(player->markPosition.pos, send);
        RW(player->markPosition.rot[0], send);
        RW(player->markPosition.rot[2], send);
    }
    else if (player->miscellaneousChangeType == mwmp::MISCELLANEOUS_CHANGE_TYPE::SELECTED_SPELL)
        RW(player->selectedSpellId, send, true);
    else if (player->miscellaneousChangeType == mwmp::MISCELLANEOUS_CHANGE_TYPE::PORTABLE_LINKED)
        RW(player->portableId, send, true);
    else if (player->miscellaneousChangeType == mwmp::MISCELLANEOUS_CHANGE_TYPE::PORTABLE_CHARACTER)
    {
        RW(player->portableId, send, true);
        RW(player->portablePlayed, send);
    }
    else if (player->miscellaneousChangeType == mwmp::MISCELLANEOUS_CHANGE_TYPE::PORTABLE_DATA)
    {
        RW(player->portableChunk, send);
        RW(player->portableChunkCount, send);
        RW(player->portableData, send);
    }
}
