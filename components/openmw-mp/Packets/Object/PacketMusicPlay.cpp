#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketMusicPlay.hpp"

using namespace mwmp;

PacketMusicPlay::PacketMusicPlay(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_MUSIC_PLAY;
}

void PacketMusicPlay::Object(BaseObject &baseObject, bool send)
{
    RW(baseObject.musicFilename, send);
}
