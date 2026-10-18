#ifndef OPENMW_PROCESSORACTORCAST_HPP
#define OPENMW_PROCESSORACTORCAST_HPP

#include "../ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorCast final: public ActorProcessor
    {
    public:
        ProcessorActorCast()
        {
            BPP_INIT(ID_ACTOR_CAST);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            Main::get().getCellController()->readCast(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORCAST_HPP
