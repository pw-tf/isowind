#ifndef OPENMW_PROCESSOROBJECTPLACE_HPP
#define OPENMW_PROCESSOROBJECTPLACE_HPP

#include "../ObjectProcessor.hpp"
#include <apps/openmw-mp/Networking.hpp>

namespace mwmp
{
    class ProcessorObjectPlace : public ObjectProcessor
    {
    public:
        ProcessorObjectPlace()
        {
            BPP_INIT(ID_OBJECT_PLACE)
        }

        void Do(ObjectPacket &packet, Player &player, BaseObjectList &objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            for (unsigned int i = 0; i < objectList.baseObjectCount; i++)
            {
                objectList.baseObjects.at(i).mpNum = mwmp::Networking::getPtr()->incrementMpNum();
            }

            Script::Call<Script::CallbackIdentity("OnObjectPlace")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTPLACE_HPP
