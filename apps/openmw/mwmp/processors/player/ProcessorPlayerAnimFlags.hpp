#ifndef OPENMW_PROCESSORPLAYERANIMFLAGS_HPP
#define OPENMW_PROCESSORPLAYERANIMFLAGS_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerAnimFlags final: public PlayerProcessor
    {
    public:
        ProcessorPlayerAnimFlags()
        {
            BPP_INIT(ID_PLAYER_ANIM_FLAGS)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (isRequest())
                    static_cast<LocalPlayer *>(player)->updateAnimFlags(true);
            }
            else if (player != 0)
                static_cast<DedicatedPlayer *>(player)->setAnimFlags();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERANIMFLAGS_HPP
