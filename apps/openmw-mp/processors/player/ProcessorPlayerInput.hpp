#ifndef OPENMW_PROCESSORPLAYERINPUT_HPP
#define OPENMW_PROCESSORPLAYERINPUT_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerInput : public PlayerProcessor
    {
    public:
        ProcessorPlayerInput()
        {
            BPP_INIT(ID_PLAYER_INPUT)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerInput")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERINPUT_HPP
