#ifndef OPENMW_PROCESSORCLIENTSCRIPTLOCAL_HPP
#define OPENMW_PROCESSORCLIENTSCRIPTLOCAL_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorClientScriptLocal : public ObjectProcessor
    {
    public:
        ProcessorClientScriptLocal()
        {
            BPP_INIT(ID_CLIENT_SCRIPT_LOCAL)
        }

        void Do(ObjectPacket& packet, Player& player, BaseObjectList& objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            Script::Call<Script::CallbackIdentity("OnClientScriptLocal")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORCLIENTSCRIPTLOCAL_HPP
