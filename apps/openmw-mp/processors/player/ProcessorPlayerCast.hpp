#ifndef OPENMW_PROCESSORPLAYERCAST_HPP
#define OPENMW_PROCESSORPLAYERCAST_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerCast : public PlayerProcessor
    {
        PlayerPacketController *playerController;
    public:
        ProcessorPlayerCast()
        {
            BPP_INIT(ID_PLAYER_CAST)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            if (!player.creatureStats.mDead)
            {
                player.sendToLoaded(&packet);
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERCAST_HPP
