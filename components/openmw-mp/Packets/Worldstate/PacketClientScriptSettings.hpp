#ifndef OPENMW_PACKETCLIENTSCRIPTSETTINGS_HPP
#define OPENMW_PACKETCLIENTSCRIPTSETTINGS_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>

namespace mwmp
{
    class PacketClientScriptSettings : public WorldstatePacket
    {
    public:
        PacketClientScriptSettings(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETCLIENTSCRIPTSETTINGS_HPP
