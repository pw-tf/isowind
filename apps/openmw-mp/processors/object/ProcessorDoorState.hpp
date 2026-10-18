#ifndef OPENMW_PROCESSORDOORSTATE_HPP
#define OPENMW_PROCESSORDOORSTATE_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorDoorState : public ObjectProcessor
    {
    public:
        ProcessorDoorState()
        {
            BPP_INIT(ID_DOOR_STATE)
        }

        void Do(ObjectPacket &packet, Player &player, BaseObjectList &objectList) override
        {
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnDoorState")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORDOORSTATE_HPP
