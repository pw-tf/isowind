#ifndef OPENMW_PROCESSORPLAYERSPELLSACTIVE_HPP
#define OPENMW_PROCESSORPLAYERSPELLSACTIVE_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerSpellsActive : public PlayerProcessor
    {
    public:
        ProcessorPlayerSpellsActive()
        {
            BPP_INIT(ID_PLAYER_SPELLS_ACTIVE)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerSpellsActive")>(player.getId());
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERSPELLSACTIVE_HPP
