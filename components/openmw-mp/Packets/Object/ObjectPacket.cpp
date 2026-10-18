#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "ObjectPacket.hpp"

using namespace mwmp;

ObjectPacket::ObjectPacket(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    hasCellData = false;
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    orderChannel = CHANNEL_OBJECT;
    this->peer = peer;
}

ObjectPacket::~ObjectPacket()
{

}

void ObjectPacket::setObjectList(BaseObjectList *newObjectList)
{
    objectList = newObjectList;
    guid = objectList->guid;
}

void ObjectPacket::Packet(RakNet::BitStream *newBitstream, bool send)
{
    if (!PacketHeader(newBitstream, send))
        return;

    BaseObject baseObject;
    for (unsigned int i = 0; i < objectList->baseObjectCount; i++)
    {
        if (send)
            baseObject = objectList->baseObjects.at(i);

        Object(baseObject, send);

        if (!send)
            objectList->baseObjects.push_back(baseObject);
    }
}

bool ObjectPacket::PacketHeader(RakNet::BitStream *newBitstream, bool send)
{
    BasePacket::Packet(newBitstream, send);

    RW(objectList->packetOrigin, send);

    if (objectList->packetOrigin == mwmp::CLIENT_SCRIPT_LOCAL || objectList->packetOrigin == mwmp::CLIENT_SCRIPT_GLOBAL)
        RW(objectList->originClientScript, send, true);

    if (send)
        objectList->baseObjectCount = (unsigned int)(objectList->baseObjects.size());
    else
        objectList->baseObjects.clear();

    RW(objectList->baseObjectCount, send);

    if (objectList->baseObjectCount > maxObjects)
    {
        objectList->isValid = false;
        return false;
    }

    if (hasCellData)
    {
        RW(objectList->cell.mData, send, true);
        RW(objectList->cell.mName, send, true);
    }

    return true;
}

void ObjectPacket::Object(BaseObject &baseObject, bool send)
{
    RW(baseObject.refId, send, true);
    RW(baseObject.refNum, send);
    RW(baseObject.mpNum, send);
}
