#ifndef OPENMW_PROCESSORPLAYERCELLCHANGE_HPP
#define OPENMW_PROCESSORPLAYERCELLCHANGE_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerCellChange final: public PlayerProcessor
    {
    public:
        ProcessorPlayerCellChange()
        {
            BPP_INIT(ID_PLAYER_CELL_CHANGE)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_CELL_CHANGE from server about me");

                if (isRequest())
                    static_cast<LocalPlayer *>(player)->updateCell(true);
                else
                    static_cast<LocalPlayer *>(player)->setCell();
            }
            else if (player != 0)
                static_cast<DedicatedPlayer*>(player)->setCell();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERCELLCHANGE_HPP
