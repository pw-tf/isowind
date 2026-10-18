#ifndef OPENMW_PROCESSORWORLDKILLCOUNT_HPP
#define OPENMW_PROCESSORWORLDKILLCOUNT_HPP

#include "../WorldstateProcessor.hpp"

namespace mwmp
{
    class ProcessorWorldKillCount : public WorldstateProcessor
    {
    public:
        ProcessorWorldKillCount()
        {
            BPP_INIT(ID_WORLD_KILL_COUNT)
        }

        void Do(WorldstatePacket &packet, Player &player, BaseWorldstate &worldstate) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnWorldKillCount")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORWORLDKILLCOUNT_HPP
