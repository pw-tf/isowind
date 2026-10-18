#ifndef OPENMW_PROCESSORPLAYERINPUT_HPP
#define OPENMW_PROCESSORPLAYERINPUT_HPP

#include "apps/openmw/mwmp/Main.hpp"
#include "../PlayerProcessor.hpp"
#include "apps/openmw/mwmp/MechanicsHelper.hpp"

namespace mwmp
{
    class ProcessorPlayerInput final: public PlayerProcessor
    {
    public:
        ProcessorPlayerInput()
        {
            BPP_INIT(ID_PLAYER_INPUT)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            //if (player != 0)
            //    MechanicsHelper::processInteraction(player->interaction, static_cast<DedicatedPlayer*>(player)->getPtr());
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERINPUT_HPP
