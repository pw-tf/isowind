#ifndef OPENMW_PROCESSORWORLDWEATHER_HPP
#define OPENMW_PROCESSORWORLDWEATHER_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorWorldWeather final: public WorldstateProcessor
    {
    public:
        ProcessorWorldWeather()
        {
            BPP_INIT(ID_WORLD_WEATHER)
        }

        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate)
        {
            worldstate.setWeather();
        }
    };
}

#endif //OPENMW_PROCESSORWORLDWEATHER_HPP
