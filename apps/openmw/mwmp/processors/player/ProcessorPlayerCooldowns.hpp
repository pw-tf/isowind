#ifndef OPENMW_PROCESSORPLAYERCOOLDOWNS_HPP
#define OPENMW_PROCESSORPLAYERCOOLDOWNS_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerCooldowns final: public PlayerProcessor
    {
    public:
        ProcessorPlayerCooldowns()
        {
            BPP_INIT(ID_PLAYER_COOLDOWNS)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (!isLocal()) return;

            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_COOLDOWNS about LocalPlayer from server");

            LocalPlayer &localPlayer = static_cast<LocalPlayer&>(*player);
            localPlayer.setCooldowns();
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERCOOLDOWNS_HPP
