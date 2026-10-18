#ifndef OPENMW_PROCESSOROBJECTRESTOCK_HPP
#define OPENMW_PROCESSOROBJECTRESTOCK_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectRestock : public ObjectProcessor
    {
    public:
        ProcessorObjectRestock()
        {
            BPP_INIT(ID_OBJECT_RESTOCK)
        }

        void Do(ObjectPacket &packet, Player &player, BaseObjectList &objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            Script::Call<Script::CallbackIdentity("OnObjectRestock")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTRESTOCK_HPP
