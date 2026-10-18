#ifndef OPENMW_PROCESSORWORLDMAP_HPP
#define OPENMW_PROCESSORWORLDMAP_HPP

#include "../WorldstateProcessor.hpp"

namespace mwmp
{
    class ProcessorWorldMap final: public WorldstateProcessor
    {
    public:
        ProcessorWorldMap()
        {
            BPP_INIT(ID_WORLD_MAP)
        }

        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate)
        {
            worldstate.setMapExplored();
        }
    };
}

#endif //OPENMW_PROCESSORWORLDMAP_HPP
