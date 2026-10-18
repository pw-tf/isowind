#ifndef OPENMW_OBJECTPACKETCONTROLLER_HPP
#define OPENMW_OBJECTPACKETCONTROLLER_HPP


#include <RakPeerInterface.h>
#include "../Packets/Object/ObjectPacket.hpp"
#include <unordered_map>
#include <memory>

namespace mwmp
{
    class ObjectPacketController
    {
    public:
        ObjectPacketController(RakNet::RakPeerInterface *peer);
        ObjectPacket *GetPacket(RakNet::MessageID id);
        void SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream);

        bool ContainsPacket(RakNet::MessageID id);

        typedef std::unordered_map<unsigned char, std::unique_ptr<ObjectPacket> > packets_t;
    private:
        packets_t packets;
    };
}

#endif //OPENMW_OBJECTPACKETCONTROLLER_HPP
