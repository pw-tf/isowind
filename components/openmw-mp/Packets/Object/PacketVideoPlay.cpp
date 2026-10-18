#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketVideoPlay.hpp"

using namespace mwmp;

PacketVideoPlay::PacketVideoPlay(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_VIDEO_PLAY;
}

void PacketVideoPlay::Object(BaseObject &baseObject, bool send)
{
    RW(baseObject.videoFilename, send, true);
    RW(baseObject.allowSkipping, send);
}
