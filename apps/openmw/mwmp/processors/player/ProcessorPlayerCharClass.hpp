#ifndef OPENMW_PROCESSORPLAYERCHARCLASS_HPP
#define OPENMW_PROCESSORPLAYERCHARCLASS_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerCharClass final: public PlayerProcessor
    {
    public:
        ProcessorPlayerCharClass()
        {
            BPP_INIT(ID_PLAYER_CHARCLASS)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (isRequest())
                    static_cast<LocalPlayer *>(player)->sendClass();
                else
                    static_cast<LocalPlayer *>(player)->setClass();
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERCHARCLASS_HPP
