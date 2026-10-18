#ifndef OPENMW_OBJECTPACKET_HPP
#define OPENMW_OBJECTPACKET_HPP

#include <string>
#include <components/openmw-mp/RakNetIncludes.hpp>
#include <BitStream.h>
#include <PacketPriority.h>
#include <components/openmw-mp/Base/BaseObject.hpp>

#include <components/openmw-mp/Packets/BasePacket.hpp>


namespace mwmp
{
    class ObjectPacket : public BasePacket
    {
    public:
        ObjectPacket(RakNet::RakPeerInterface *peer);

        ~ObjectPacket();

        void setObjectList(BaseObjectList *newObjectList);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);

    protected:
        virtual void Object(BaseObject &baseObject, bool send);
        bool PacketHeader(RakNet::BitStream *newBitstream, bool send);
        BaseObjectList *objectList;
        static const int maxObjects = 3000;
        bool hasCellData;
    };
}

#endif //OPENMW_OBJECTPACKET_HPP
