#ifndef OPENMW_PROCESSORWORLDREGIONAUTHORITY_HPP
#define OPENMW_PROCESSORWORLDREGIONAUTHORITY_HPP

#include <apps/openmw/mwbase/world.hpp>

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorWorldRegionAuthority final: public WorldstateProcessor
    {
    public:
        ProcessorWorldRegionAuthority()
        {
            BPP_INIT(ID_WORLD_REGION_AUTHORITY)
        }

        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate)
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();

            if (!worldstate.authorityRegion.empty()
                && world->getPlayerPtr().getCell()->getCell()->getRegion() == worldstate.authorityRegion)
            {
                LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s about %s", strPacketID.c_str(), worldstate.authorityRegion.c_str());

                if (isLocal())
                {
                    LOG_APPEND(TimedLog::LOG_INFO, "- The new region authority is me");
                    // There's a chance we've been made the region authority right after a teleportation that hasn't
                    // been registered in the WeatherManager yet, so make sure we update it
                    world->updateWeather(0);

                    world->setWeatherCreationState(true);
                    world->sendWeather();
                }
                else
                {
                    BasePlayer *player = PlayerList::getPlayer(guid);

                    if (player != 0)
                        LOG_APPEND(TimedLog::LOG_INFO, "- The new region authority is %s", player->npc.mName.c_str());

                    world->setWeatherCreationState(false);
                }
            }
        }
    };
}

#endif //OPENMW_PROCESSORWORLDREGIONAUTHORITY_HPP
