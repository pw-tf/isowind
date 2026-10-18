#ifndef OPENMW_PROCESSORWORLDDESTINATIONOVERRIDE_HPP
#define OPENMW_PROCESSORWORLDDESTINATIONOVERRIDE_HPP


#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwbase/environment.hpp>
#include "../WorldstateProcessor.hpp"

namespace mwmp
{
    class ProcessorWorldDestinationOverride final: public WorldstateProcessor
    {
    public:
        ProcessorWorldDestinationOverride()
        {
            BPP_INIT(ID_WORLD_DESTINATION_OVERRIDE)
        }

        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_WORLD_DESTINATION_OVERRIDE with the following overrides:");

            for (auto iterator : worldstate.destinationOverrides)
            {
                LOG_APPEND(TimedLog::LOG_INFO, "- %s now leads to %s", iterator.first.c_str(), iterator.second.c_str());
            }
        }
    };
}



#endif //OPENMW_PROCESSORWORLDDESTINATIONOVERRIDE_HPP
