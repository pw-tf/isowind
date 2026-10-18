#ifndef OPENMW_PROCESSORACTORCAST_HPP
#define OPENMW_PROCESSORACTORCAST_HPP

#include "../ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorCast : public ActorProcessor
    {
    public:
        ProcessorActorCast()
        {
            BPP_INIT(ID_ACTOR_CAST)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr && *serverCell->getAuthority() == actorList.guid)
                serverCell->sendToLoaded(&packet, &actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORCAST_HPP
