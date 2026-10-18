#ifndef OPENMW_PROCESSORPLAYERREPUTATION_HPP
#define OPENMW_PROCESSORPLAYERREPUTATION_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerReputation : public PlayerProcessor
    {
    public:
        ProcessorPlayerReputation()
        {
            BPP_INIT(ID_PLAYER_REPUTATION)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            Script::Call<Script::CallbackIdentity("OnPlayerReputation")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERREPUTATION_HPP
