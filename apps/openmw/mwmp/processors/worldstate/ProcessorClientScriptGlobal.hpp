#ifndef OPENMW_PROCESSORCLIENTSCRIPTGLOBAL_HPP
#define OPENMW_PROCESSORCLIENTSCRIPTGLOBAL_HPP

#include "../WorldstateProcessor.hpp"

namespace mwmp
{
    class ProcessorClientScriptGlobal final: public WorldstateProcessor
    {
    public:
        ProcessorClientScriptGlobal()
        {
            BPP_INIT(ID_CLIENT_SCRIPT_GLOBAL)
        }

        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate)
        {
            mwmp::Main::get().getNetworking()->getWorldstate()->setClientGlobals();
        }
    };
}



#endif //OPENMW_PROCESSORCLIENTSCRIPTGLOBAL_HPP
