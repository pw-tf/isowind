#ifndef OPENMW_PROCESSORSYSTEMHANDSHAKE_HPP
#define OPENMW_PROCESSORSYSTEMHANDSHAKE_HPP

#include <components/openmw-mp/Base/BaseSystem.hpp>

#include "apps/openmw/mwmp/Main.hpp"

#include "../SystemProcessor.hpp"

namespace mwmp
{
    class ProcessorSystemHandshake final: public SystemProcessor
    {
    public:
        ProcessorSystemHandshake()
        {
            BPP_INIT(ID_SYSTEM_HANDSHAKE)
        }

        virtual void Do(SystemPacket &packet, BaseSystem *system)
        {
            packet.setSystem(Main::get().getLocalSystem());
            packet.Send(serverAddr);
        }
    };
}

#endif //OPENMW_PROCESSORSYSTEMHANDSHAKE_HPP
