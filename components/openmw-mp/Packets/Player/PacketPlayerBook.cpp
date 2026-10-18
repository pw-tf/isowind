#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerBook.hpp"

using namespace mwmp;

PacketPlayerBook::PacketPlayerBook(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_BOOK;
}

void PacketPlayerBook::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    uint32_t count;

    if (send)
        count = static_cast<uint32_t>(player->bookChanges.size());

    RW(count, send);

    if (!send)
    {
        player->bookChanges.clear();
        player->bookChanges.resize(count);
    }

    for (auto &&book : player->bookChanges)
    {
        RW(book.bookId, send, true);
    }
}
