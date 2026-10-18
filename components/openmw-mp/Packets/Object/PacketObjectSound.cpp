#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectSound.hpp"

using namespace mwmp;

PacketObjectSound::PacketObjectSound(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_OBJECT_SOUND;
    hasCellData = true;
}

void PacketObjectSound::Packet(RakNet::BitStream *newBitstream, bool send)
{
    if (!PacketHeader(newBitstream, send))
        return;

    BaseObject baseObject;
    for (unsigned int i = 0; i < objectList->baseObjectCount; i++)
    {
        if (send)
            baseObject = objectList->baseObjects.at(i);

        RW(baseObject.isPlayer, send);

        if (baseObject.isPlayer)
            RW(baseObject.guid, send);
        else
            Object(baseObject, send);

        RW(baseObject.soundId, send, true);
        RW(baseObject.volume, send);
        RW(baseObject.pitch, send);

        if (!send)
            objectList->baseObjects.push_back(baseObject);
    }
}
