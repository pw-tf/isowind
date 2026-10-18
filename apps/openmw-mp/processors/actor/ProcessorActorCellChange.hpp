#ifndef OPENMW_PROCESSORACTORCELLCHANGE_HPP
#define OPENMW_PROCESSORACTORCELLCHANGE_HPP

#include "../ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorCellChange : public ActorProcessor
    {
    public:
        ProcessorActorCellChange()
        {
            BPP_INIT(ID_ACTOR_CELL_CHANGE)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            bool isAccepted = false;
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
            {
                bool isFollowerCellChange = false;

                // TODO: Move this check on the Lua side
                for (unsigned int i = 0; i < actorList.count; i++)
                {
                    if (actorList.baseActors.at(i).isFollowerCellChange)
                    {
                        isFollowerCellChange = true;
                        break;
                    }
                }

                // If the cell is loaded, only accept regular cell changes from a cell's authority, but accept follower
                // cell changes from other players
                if (*serverCell->getAuthority() == actorList.guid || isFollowerCellChange)
                {
                    serverCell->removeActors(&actorList);
                    isAccepted = true;
                }
            }
            // If the cell isn't loaded, the packet must be from dialogue or a script, so accept it
            else
            {
                isAccepted = true;
            }

            if (isAccepted)
            {
                Script::Call<Script::CallbackIdentity("OnActorCellChange")>(player.getId(), actorList.cell.getShortDescription().c_str());

                // Send this to everyone
                packet.Send(true);
            }
        }
    };
}

#endif //OPENMW_PROCESSORACTORCELLCHANGE_HPP
