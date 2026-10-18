#ifndef OPENMW_PROCESSORPLAYERSTATSDYNAMIC_HPP
#define OPENMW_PROCESSORPLAYERSTATSDYNAMIC_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerStatsDynamic final: public PlayerProcessor
    {
    public:
        ProcessorPlayerStatsDynamic()
        {
            BPP_INIT(ID_PLAYER_STATS_DYNAMIC)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (isRequest())
                    static_cast<LocalPlayer *>(player)->updateStatsDynamic(true);
                else
                    static_cast<LocalPlayer *>(player)->setDynamicStats();
            }
            else if (player != 0)
            {
                static_cast<DedicatedPlayer*>(player)->setStatsDynamic();
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERSTATSDYNAMIC_HPP
