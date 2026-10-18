#ifndef OPENMW_PROCESSORPLAYERCOOLDOWNS_HPP
#define OPENMW_PROCESSORPLAYERCOOLDOWNS_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerCooldowns : public PlayerProcessor
    {
    public:
        ProcessorPlayerCooldowns()
        {
            BPP_INIT(ID_PLAYER_COOLDOWNS)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerCooldowns")>(player.getId());
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERCOOLDOWNS_HPP
