#ifndef OPENMW_PROCESSORWORLDCOLLISIONOVERRIDE_HPP
#define OPENMW_PROCESSORWORLDCOLLISIONOVERRIDE_HPP


#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwbase/environment.hpp>
#include "../WorldstateProcessor.hpp"

namespace mwmp
{
    class ProcessorWorldCollisionOverride final: public WorldstateProcessor
    {
    public:
        ProcessorWorldCollisionOverride()
        {
            BPP_INIT(ID_WORLD_COLLISION_OVERRIDE)
        }

        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate)
        {
            // Placeholder
        }
    };
}



#endif //OPENMW_PROCESSORWORLDCOLLISIONOVERRIDE_HPP
