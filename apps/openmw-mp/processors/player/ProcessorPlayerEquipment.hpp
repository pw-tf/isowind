#ifndef OPENMW_PROCESSORPLAYEREQUIPMENT_HPP
#define OPENMW_PROCESSORPLAYEREQUIPMENT_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerEquipment : public PlayerProcessor
    {
    public:
        ProcessorPlayerEquipment()
        {
            BPP_INIT(ID_PLAYER_EQUIPMENT)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            player.sendToLoaded(&packet);

            Script::Call<Script::CallbackIdentity("OnPlayerEquipment")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYEREQUIPMENT_HPP
