#ifndef OPENMW_PACKETWORLDWEATHER_HPP
#define OPENMW_PACKETWORLDWEATHER_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>

namespace mwmp
{
    class PacketWorldWeather : public WorldstatePacket
    {
    public:
        PacketWorldWeather(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
    };
}

#endif //OPENMW_PACKETWORLDWEATHER_HPP
