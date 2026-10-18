#ifndef OPENMW_PROCESSORPLAYERMISCELLANEOUS_HPP
#define OPENMW_PROCESSORPLAYERMISCELLANEOUS_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerMiscellaneous : public PlayerProcessor
    {
    public:
        ProcessorPlayerMiscellaneous()
        {
            BPP_INIT(ID_PLAYER_MISCELLANEOUS)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerMiscellaneous")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERMISCELLANEOUS_HPP
