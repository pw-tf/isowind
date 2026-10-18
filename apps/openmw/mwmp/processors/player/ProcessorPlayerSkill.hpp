#ifndef OPENMW_PROCESSORPLAYERSKILL_HPP
#define OPENMW_PROCESSORPLAYERSKILL_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerSkill final: public PlayerProcessor
    {
    public:
        ProcessorPlayerSkill()
        {
            BPP_INIT(ID_PLAYER_SKILL)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (isRequest())
                    static_cast<LocalPlayer *>(player)->updateSkills(true);
                else
                    static_cast<LocalPlayer *>(player)->setSkills();
            }
            else if (player != 0)
            {
                static_cast<DedicatedPlayer *>(player)->setSkills();
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERSKILL_HPP
