#ifndef OPENMW_PROCESSORPLAYERITEMUSE_HPP
#define OPENMW_PROCESSORPLAYERITEMUSE_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerItemUse : public PlayerProcessor
    {
    public:
        ProcessorPlayerItemUse()
        {
            BPP_INIT(ID_PLAYER_ITEM_USE)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerItemUse")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERITEMUSE_HPP
