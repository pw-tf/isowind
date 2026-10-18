#ifndef OPENMW_PLAYERPACKET_HPP
#define OPENMW_PLAYERPACKET_HPP

#include <string>
#include <components/openmw-mp/RakNetIncludes.hpp>
#include <BitStream.h>
#include <PacketPriority.h>
#include <components/openmw-mp/Base/BasePlayer.hpp>

#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PlayerPacket : public BasePacket
    {
    public:
        PlayerPacket(RakNet::RakPeerInterface *peer);

        ~PlayerPacket();

        void setPlayer(BasePlayer *newPlayer);
        BasePlayer *getPlayer();

    protected:
        BasePlayer *player;

    };
}

#endif //OPENMW_PLAYERPACKET_HPP
