#ifndef OPENMW_PROCESSORACTORAI_HPP
#define OPENMW_PROCESSORACTORAI_HPP

#include "../ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorAI final: public ActorProcessor
    {
    public:
        ProcessorActorAI()
        {
            BPP_INIT(ID_ACTOR_AI);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s about %s", strPacketID.c_str(), actorList.cell.getShortDescription().c_str());

            Main::get().getCellController()->readAi(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORAI_HPP
