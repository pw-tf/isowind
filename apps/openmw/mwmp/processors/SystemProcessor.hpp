#ifndef OPENMW_SYSTEMPROCESSOR_HPP
#define OPENMW_SYSTEMPROCESSOR_HPP

#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/System/SystemPacket.hpp>
#include "../LocalSystem.hpp"
#include "BaseClientPacketProcessor.hpp"

namespace mwmp
{
    class SystemProcessor : public BasePacketProcessor<SystemProcessor>, public BaseClientPacketProcessor
    {
    public:
        virtual void Do(SystemPacket &packet, BaseSystem *system) = 0;

        static bool Process(RakNet::Packet &packet);

        virtual ~SystemProcessor();
    };
}



#endif //OPENMW_SYSTEMPROCESSOR_HPP
