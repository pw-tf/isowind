#ifndef OPENMW_WORLDSTATEPROCESSOR_HPP
#define OPENMW_WORLDSTATEPROCESSOR_HPP

#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>
#include "BaseClientPacketProcessor.hpp"

namespace mwmp
{
    class WorldstateProcessor : public BasePacketProcessor<WorldstateProcessor>, public BaseClientPacketProcessor
    {
    public:
        virtual void Do(WorldstatePacket &packet, Worldstate &worldstate) = 0;

        static bool Process(RakNet::Packet &packet, Worldstate &worldstate);

        virtual ~WorldstateProcessor();
    };
}


#endif //OPENMW_WORLDSTATEPROCESSOR_HPP
