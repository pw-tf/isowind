#ifndef OPENMW_PROCESSORCLIENTSCRIPTGLOBAL_HPP
#define OPENMW_PROCESSORCLIENTSCRIPTGLOBAL_HPP

#include "../WorldstateProcessor.hpp"

namespace mwmp
{
    class ProcessorClientScriptGlobal : public WorldstateProcessor
    {
    public:
        ProcessorClientScriptGlobal()
        {
            BPP_INIT(ID_CLIENT_SCRIPT_GLOBAL)
        }

        void Do(WorldstatePacket &packet, Player &player, BaseWorldstate &worldstate) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnClientScriptGlobal")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORCLIENTSCRIPTGLOBAL_HPP
