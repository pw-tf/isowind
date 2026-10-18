#ifndef OPENMW_ACTORPROCESSOR_HPP
#define OPENMW_ACTORPROCESSOR_HPP

#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>
#include "../ObjectList.hpp"
#include "../ActorList.hpp"
#include "BaseClientPacketProcessor.hpp"

namespace mwmp
{
    class ActorProcessor : public BasePacketProcessor<ActorProcessor>, public BaseClientPacketProcessor
    {
    public:
        virtual void Do(ActorPacket &packet, ActorList &actorList) = 0;

        static bool Process(RakNet::Packet &packet, ActorList &actorList);

        virtual ~ActorProcessor();
    };
}


#endif //OPENMW_ACTORPROCESSOR_HPP
