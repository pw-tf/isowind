#ifndef OPENMW_PROCESSORGAMESETTINGS_HPP
#define OPENMW_PROCESSORGAMESETTINGS_HPP

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwworld/worldimp.hpp"
#include "apps/openmw/mwgui/windowmanagerimp.hpp"

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorGameSettings final: public PlayerProcessor
    {
        const std::string GAME_SETTING_CATEGORY = "Game";
        const std::string VR_SETTING_CATEGORY = "VR";
    public:
        ProcessorGameSettings()
        {
            BPP_INIT(ID_GAME_SETTINGS)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            static const int initialLogLevel = TimedLog::GetLevel();

            if (isLocal())
            {
                LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_GAME_SETTINGS");
                LOG_APPEND(TimedLog::LOG_INFO, "- player %s rest in beds, %s rest in the wilderness, %s wait",
                    player->bedRestAllowed ? "can" : "cannot", player->wildernessRestAllowed ? "can" : "cannot",
                    player->waitAllowed ? "can" : "cannot");

                if (MWBase::Environment::get().getWindowManager()->isGuiMode())
                {
                    if (MWBase::Environment::get().getWindowManager()->isConsoleMode() && !player->consoleAllowed)
                        MWBase::Environment::get().getWindowManager()->popGuiMode();
                    else if (MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Rest &&
                        (!player->bedRestAllowed || !player->wildernessRestAllowed || !player->waitAllowed))
                        MWBase::Environment::get().getWindowManager()->popGuiMode();
                }

                if (player->enforcedLogLevel > -1)
                {
                    LOG_APPEND(TimedLog::LOG_INFO, "- server is enforcing log level %i", player->enforcedLogLevel);
                    TimedLog::SetLevel(player->enforcedLogLevel);
                }
                else if (initialLogLevel != TimedLog::GetLevel())
                {
                    LOG_APPEND(TimedLog::LOG_INFO, "- log level has been reset to initial value %i", initialLogLevel);
                    TimedLog::SetLevel(initialLogLevel);
                }

                // TODO: TES3MP let the server set the physics framerate; OpenMW 0.51 takes it from its settings only

                for (auto setting : player->gameSettings)
                {
                    // Ignore settings this version of the game doesn't have
                    try
                    {
                        Settings::Manager::setString(setting.first, GAME_SETTING_CATEGORY, setting.second);
                    }
                    catch (std::exception& e)
                    {
                        LOG_APPEND(TimedLog::LOG_INFO, "- ignored game setting %s: %s", setting.first.c_str(), e.what());
                    }
                }

                // Only read VR settings for players using a VR build
#ifdef USE_OPENXR
                for (auto setting : player->vrSettings)
                {
                    Settings::Manager::setString(setting.first, VR_SETTING_CATEGORY, setting.second);
                }
#endif
            }
        }
    };
}

#endif //OPENMW_PROCESSORGAMESETTINGS_HPP
