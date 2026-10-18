#ifndef OPENMW_PROCESSORACTORSPELLSACTIVE_HPP
#define OPENMW_PROCESSORACTORSPELLSACTIVE_HPP

#include "../ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorSpellsActive : public ActorProcessor
    {
    public:
        ProcessorActorSpellsActive()
        {
            BPP_INIT(ID_ACTOR_SPELLS_ACTIVE)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr && *serverCell->getAuthority() == actorList.guid)
            {
                Script::Call<Script::CallbackIdentity("OnActorSpellsActive")>(player.getId(), actorList.cell.getShortDescription().c_str());

                serverCell->sendToLoaded(&packet, &actorList);
            }
        }
    };
}

#endif //OPENMW_PROCESSORACTORSPELLSACTIVE_HPP
