#ifndef OPENMW_PROCESSORCELLRESET_HPP
#define OPENMW_PROCESSORCELLRESET_HPP

#include "../WorldstateProcessor.hpp"
#include <apps/openmw/mwworld/player.hpp>

namespace mwmp
{
    class ProcessorCellReset final: public WorldstateProcessor
    {
    public:
        ProcessorCellReset()
        {
            BPP_INIT(ID_CELL_RESET)
        }

        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_CELL_RESET");

            CellController* cellController = Main::get().getCellController();

            mwmp::Main::get().getNetworking()->getWorldstate()->resetCells(&worldstate.cellsToReset);
        }
    };
}

#endif //OPENMW_PROCESSORCELLRESET_HPP
