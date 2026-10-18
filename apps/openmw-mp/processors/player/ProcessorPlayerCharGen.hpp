#ifndef OPENMW_PROCESSORPLAYERCHARGEN_HPP
#define OPENMW_PROCESSORPLAYERCHARGEN_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerCharGen : public PlayerProcessor
    {
    public:
        ProcessorPlayerCharGen()
        {
            BPP_INIT(ID_PLAYER_CHARGEN)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            if (player.charGenState.currentStage == player.charGenState.endStage)
                Script::Call<Script::CallbackIdentity("OnPlayerEndCharGen")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERCHARGEN_HPP
