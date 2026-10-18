#ifndef OPENMW_PROCESSOROBJECTMISCELLANEOUS_HPP
#define OPENMW_PROCESSOROBJECTMISCELLANEOUS_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectMiscellaneous : public ObjectProcessor
    {
    public:
        ProcessorObjectMiscellaneous()
        {
            BPP_INIT(ID_OBJECT_MISCELLANEOUS)
        }

        void Do(ObjectPacket& packet, Player& player, BaseObjectList& objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            Script::Call<Script::CallbackIdentity("OnObjectMiscellaneous")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTMISCELLANEOUS_HPP
