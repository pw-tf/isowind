#ifndef OPENMW_PROCESSOROBJECTHIT_HPP
#define OPENMW_PROCESSOROBJECTHIT_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectHit : public ObjectProcessor
    {
    public:
        ProcessorObjectHit()
        {
            BPP_INIT(ID_OBJECT_HIT)
        }

        void Do(ObjectPacket &packet, Player &player, BaseObjectList &objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            Script::Call<Script::CallbackIdentity("OnObjectHit")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTHIT_HPP
