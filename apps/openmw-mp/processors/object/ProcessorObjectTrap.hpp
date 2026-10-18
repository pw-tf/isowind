#ifndef OPENMW_PROCESSOROBJECTTRAP_HPP
#define OPENMW_PROCESSOROBJECTTRAP_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectTrap : public ObjectProcessor
    {
    public:
        ProcessorObjectTrap()
        {
            BPP_INIT(ID_OBJECT_TRAP)
        }

        void Do(ObjectPacket &packet, Player &player, BaseObjectList &objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());
            
            Script::Call<Script::CallbackIdentity("OnObjectTrap")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTTRAP_HPP
