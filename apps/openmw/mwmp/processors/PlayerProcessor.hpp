#ifndef OPENMW_PLAYERPROCESSOR_HPP
#define OPENMW_PLAYERPROCESSOR_HPP

#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include "../LocalPlayer.hpp"
#include "../DedicatedPlayer.hpp"
#include "../PlayerList.hpp"
#include "BaseClientPacketProcessor.hpp"

namespace mwmp
{
    class PlayerProcessor : public BasePacketProcessor<PlayerProcessor>, public BaseClientPacketProcessor
    {
    public:
        virtual void Do(PlayerPacket &packet, BasePlayer *player) = 0;

        static bool Process(RakNet::Packet &packet);

        virtual ~PlayerProcessor();
    };
}



#endif //OPENMW_PLAYERPROCESSOR_HPP
