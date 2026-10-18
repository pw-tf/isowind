#ifndef OPENMW_DEDICATEDPLAYER_HPP
#define OPENMW_DEDICATEDPLAYER_HPP

#include <components/esm3/custommarkerstate.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/openmw-mp/Base/BasePlayer.hpp>

#include "../mwclass/npc.hpp"

#include "../mwmechanics/aisequence.hpp"

#include "../mwworld/manualref.hpp"

#include <map>
#include <components/openmw-mp/RakNetIncludes.hpp>

namespace MWMechanics
{
    class Actor;
}

namespace mwmp
{
    class DedicatedPlayer : public BasePlayer
    {
        friend class PlayerList;

    public:

        void update(float dt);

        void move(float dt);
        void setBaseInfo();
        void setStatsDynamic();
        void setAnimFlags();
        void setAttributes();
        void setSkills();
        void setEquipment();
        void setShapeshift();
        void setCell();

        void playAnimation();
        void playSpeech();

        void equipItem(const ESM::RefId& itemId, bool noSound = false);
        void die();
        void resurrect();

        void addSpellsActive();
        void removeSpellsActive();
        void setSpellsActive();

        void updateMarker();
        void removeMarker();
        void enableMarker();

        void createReference(const ESM::RefId& recId);
        void deleteReference();

        MWWorld::Ptr getPtr();
        MWWorld::ManualRef* getRef();

        void setPtr(const MWWorld::Ptr& newPtr);
        void reloadPtr();

    private:

        DedicatedPlayer(RakNet::RakNetGUID guid);
        virtual ~DedicatedPlayer();

        MWWorld::ManualRef* reference;

        MWWorld::Ptr ptr;

        ESM::CustomMarker marker;
        bool markerEnabled = false;

        ESM::RefId previousRace;
        std::string previousCreatureRefId;
        bool previousDisplayCreatureName = false;

        ESM::RefId creatureRecordId;

        bool hasReceivedInitialEquipment;
        bool hasFinishedInitialTeleportation;
        bool isLevitationPurged = false;

        bool wasJumping;
    };
}
#endif //OPENMW_DEDICATEDPLAYER_HPP
