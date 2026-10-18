#ifndef OPENMW_PROCESSORPLAYERREPUTATION_HPP
#define OPENMW_PROCESSORPLAYERREPUTATION_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerReputation final: public PlayerProcessor
    {
    public:
        ProcessorPlayerReputation()
        {
            BPP_INIT(ID_PLAYER_REPUTATION)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isRequest())
            {
                static_cast<LocalPlayer *>(player)->updateReputation(true);
            }
            else if (player != 0)
            {
                static_cast<LocalPlayer *>(player)->setReputation();
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERREPUTATION_HPP
