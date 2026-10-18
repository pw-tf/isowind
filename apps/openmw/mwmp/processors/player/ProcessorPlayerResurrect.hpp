#ifndef OPENMW_PROCESSORPLAYERRESURRECT_HPP
#define OPENMW_PROCESSORPLAYERRESURRECT_HPP

#include "../PlayerProcessor.hpp"

#include "apps/openmw/mwmechanics/mechanicsmanagerimp.hpp"

#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/Networking.hpp"

namespace mwmp
{
    class ProcessorPlayerResurrect final: public PlayerProcessor
    {
    public:
        ProcessorPlayerResurrect()
        {
            BPP_INIT(ID_PLAYER_RESURRECT)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_RESURRECT from server");
            
            if (isLocal())
            {
                LOG_APPEND(TimedLog::LOG_INFO, "- Packet was about me with resurrectType of %i", player->resurrectType);

                static_cast<LocalPlayer*>(player)->resurrect();
            }
            else if (player != 0)
            {
                LOG_APPEND(TimedLog::LOG_INFO, "- Packet was about %s", player->npc.mName.c_str());

                static_cast<DedicatedPlayer*>(player)->resurrect();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERRESURRECT_HPP
