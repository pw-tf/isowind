#ifndef OPENMW_PROCESSORCLIENTSCRIPTSETTINGS_HPP
#define OPENMW_PROCESSORCLIENTSCRIPTSETTINGS_HPP


#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwbase/environment.hpp>
#include "../WorldstateProcessor.hpp"

namespace mwmp
{
    class ProcessorClientScriptSettings final: public WorldstateProcessor
    {
    public:
        ProcessorClientScriptSettings()
        {
            BPP_INIT(ID_CLIENT_SCRIPT_SETTINGS)
        }

        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_CLIENT_SCRIPT_SETTINGS making us send packets for the following globals:");
            std::string debugMessage = "";

            for (const auto &globalId : worldstate.synchronizedClientGlobalIds)
            {
                if (TimedLog::GetLevel() <= TimedLog::LOG_INFO)
                {
                    if (!debugMessage.empty())
                        debugMessage += ", ";

                    debugMessage += globalId;
                }
            }

            LOG_APPEND(TimedLog::LOG_INFO, "- %s", debugMessage.c_str());
        }
    };
}



#endif //OPENMW_PROCESSORCLIENTSCRIPTSETTINGS_HPP
