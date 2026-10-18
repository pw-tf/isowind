#ifndef OPENMW_PROCESSOROBJECTSOUND_HPP
#define OPENMW_PROCESSOROBJECTSOUND_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectSound : public ObjectProcessor
    {
    public:
        ProcessorObjectSound()
        {
            BPP_INIT(ID_OBJECT_SOUND)
        }

        void Do(ObjectPacket &packet, Player &player, BaseObjectList &objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            Script::Call<Script::CallbackIdentity("OnObjectSound")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTSOUND_HPP
