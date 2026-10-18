#ifndef OPENMW_PROCESSORPLAYERATTACK_HPP
#define OPENMW_PROCESSORPLAYERATTACK_HPP

#include "apps/openmw/mwmp/Main.hpp"
#include "../PlayerProcessor.hpp"
#include "apps/openmw/mwmp/MechanicsHelper.hpp"

namespace mwmp
{
    class ProcessorPlayerAttack final: public PlayerProcessor
    {
    public:
        ProcessorPlayerAttack()
        {
            BPP_INIT(ID_PLAYER_ATTACK)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (!isLocal() && player != 0)
            {
                DedicatedPlayer& dedicatedPlayer = static_cast<DedicatedPlayer&>(*player);
                MWWorld::Ptr playerPtr = dedicatedPlayer.getPtr();
                MWBase::World* world = MWBase::Environment::get().getWorld();
                playerPtr = world->moveObject(playerPtr, dedicatedPlayer.position.asVec3());
                dedicatedPlayer.setPtr(playerPtr);
                world->rotateObject(playerPtr, osg::Vec3f(dedicatedPlayer.position.rot[0], 0, dedicatedPlayer.position.rot[2]));
                MechanicsHelper::processAttack(player->attack, playerPtr);
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERATTACK_HPP
