#ifndef OPENMW_PACKETCONSOLECOMMAND_HPP
#define OPENMW_PACKETCONSOLECOMMAND_HPP

#include <components/openmw-mp/Packets/Object/ObjectPacket.hpp>

namespace mwmp
{
    class PacketConsoleCommand : public ObjectPacket
    {
    public:
        PacketConsoleCommand(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETCONSOLECOMMAND_HPP
