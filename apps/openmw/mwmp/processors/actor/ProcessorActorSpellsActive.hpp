#ifndef OPENMW_PROCESSORACTORSPELLSACTIVE_HPP
#define OPENMW_PROCESSORACTORSPELLSACTIVE_HPP

#include "../ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorSpellsActive final: public ActorProcessor
    {
    public:
        ProcessorActorSpellsActive()
        {
            BPP_INIT(ID_ACTOR_SPELLS_ACTIVE);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            Main::get().getCellController()->readSpellsActive(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORSPELLSACTIVE_HPP
