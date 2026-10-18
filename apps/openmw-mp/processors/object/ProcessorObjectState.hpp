#ifndef OPENMW_PROCESSOROBJECTSTATE_HPP
#define OPENMW_PROCESSOROBJECTSTATE_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectState : public ObjectProcessor
    {
    public:
        ProcessorObjectState()
        {
            BPP_INIT(ID_OBJECT_STATE)
        }

        void Do(ObjectPacket &packet, Player &player, BaseObjectList &objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());
            
            Script::Call<Script::CallbackIdentity("OnObjectState")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTSTATE_HPP
