#ifndef OPENMW_PROCESSORPLACEHOLDER_HPP
#define OPENMW_PROCESSORPLACEHOLDER_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerPlaceholder : public PlayerProcessor
    {
    public:
        ProcessorPlayerPlaceholder()
        {
            BPP_INIT(ID_WORLD_KILL_COUNT)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());
        }
    };
}

#endif //OPENMW_PROCESSORPLACEHOLDER_HPP
