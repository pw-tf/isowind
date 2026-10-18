#ifndef OPENMW_PROCESSORPLAYERBEHAVIOR_HPP
#define OPENMW_PROCESSORPLAYERBEHAVIOR_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerBehavior final: public PlayerProcessor
    {
    public:
        ProcessorPlayerBehavior()
        {
            BPP_INIT(ID_PLAYER_BEHAVIOR)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                //static_cast<LocalPlayer *>(player)->setBehavior();
            }
            else if (player != 0)
            {
                //static_cast<DedicatedPlayer *>(player)->setBehavior();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERBEHAVIOR_HPP
