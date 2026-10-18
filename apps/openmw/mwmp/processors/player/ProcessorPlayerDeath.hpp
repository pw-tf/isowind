#ifndef OPENMW_PROCESSORPLAYERDEATH_HPP
#define OPENMW_PROCESSORPLAYERDEATH_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerDeath final: public PlayerProcessor
    {
    public:
        ProcessorPlayerDeath()
        {
            BPP_INIT(ID_PLAYER_DEATH)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_DEATH from server");

            if (isLocal())
            {
                LOG_APPEND(TimedLog::LOG_INFO, "- Packet was about me");

                static_cast<LocalPlayer*>(player)->die();
            }
            else if (player != 0)
            {
                LOG_APPEND(TimedLog::LOG_INFO, "- Packet was about %s", player->npc.mName.c_str());

                static_cast<DedicatedPlayer*>(player)->die();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERDEATH_HPP
