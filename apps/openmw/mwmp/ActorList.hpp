#ifndef OPENMW_ACTORLIST_HPP
#define OPENMW_ACTORLIST_HPP

#include <components/openmw-mp/Base/BaseActor.hpp>
#include "../mwworld/cellstore.hpp"
#include <components/openmw-mp/RakNetIncludes.hpp>

#include "LocalActor.hpp"

namespace mwmp
{
    class Networking;
    class ActorList : public BaseActorList
    {
    public:

        ActorList();
        virtual ~ActorList();

        void reset();
        void addActor(BaseActor baseActor);

        void addPositionActor(BaseActor baseActor);
        void addAnimFlagsActor(BaseActor baseActor);
        void addAnimPlayActor(BaseActor baseActor);
        void addSpeechActor(BaseActor baseActor);
        void addStatsDynamicActor(BaseActor baseActor);
        void addDeathActor(BaseActor baseActor);
        void addEquipmentActor(BaseActor baseActor);
        void addAiActor(BaseActor baseActor);
        void addAiActor(const MWWorld::Ptr& actorPtr, const MWWorld::Ptr& targetPtr, unsigned int aiAction);
        void addAttackActor(BaseActor baseActor);
        void addAttackActor(const MWWorld::Ptr& actorPtr, const mwmp::Attack &attack);
        void addCastActor(BaseActor baseActor);
        void addCellChangeActor(BaseActor baseActor);

        void sendPositionActors();
        void sendAnimFlagsActors();
        void sendAnimPlayActors();
        void sendSpeechActors();
        void sendStatsDynamicActors();
        void sendDeathActors();
        void sendEquipmentActors();
        void sendAiActors();
        void sendAttackActors();
        void sendCastActors();
        void sendCellChangeActors();

        void sendActorsInCell(MWWorld::CellStore* cellStore);

    private:
        Networking *getNetworking();

        std::vector<BaseActor> positionActors;
        std::vector<BaseActor> animFlagsActors;
        std::vector<BaseActor> animPlayActors;
        std::vector<BaseActor> speechActors;
        std::vector<BaseActor> statsDynamicActors;
        std::vector<BaseActor> deathActors;
        std::vector<BaseActor> equipmentActors;
        std::vector<BaseActor> aiActors;
        std::vector<BaseActor> attackActors;
        std::vector<BaseActor> castActors;
        std::vector<BaseActor> cellChangeActors;
    };
}

#endif //OPENMW_ACTORLIST_HPP
