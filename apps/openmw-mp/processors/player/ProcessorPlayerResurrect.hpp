#ifndef OPENMW_PROCESSORPLAYERRESURRECT_HPP
#define OPENMW_PROCESSORPLAYERRESURRECT_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerResurrect : public PlayerProcessor
    {
    public:
        ProcessorPlayerResurrect()
        {
            BPP_INIT(ID_PLAYER_RESURRECT)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            player.creatureStats.mDead = false;

            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnPlayerResurrect")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERRESURRECT_HPP
