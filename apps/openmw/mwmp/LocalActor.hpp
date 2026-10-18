#ifndef OPENMW_LOCALACTOR_HPP
#define OPENMW_LOCALACTOR_HPP

#include <components/openmw-mp/Base/BaseActor.hpp>
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/activespells.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/timestamp.hpp"

namespace mwmp
{
    class LocalActor : public BaseActor
    {
    public:

        LocalActor();
        virtual ~LocalActor();

        void update(bool forceUpdate);

        void updateCell();
        void updatePosition(bool forceUpdate);
        void updateAnimFlags(bool forceUpdate);
        void updateAnimPlay();
        void updateSpeech();
        void updateStatsDynamic(bool forceUpdate);
        void updateEquipment(bool forceUpdate, bool sendImmediately = false);
        void updateAttackOrCast();

        void sendEquipment();
        void sendSpellsActiveAddition(const MWMechanics::ActiveSpells::ActiveSpellParams& params);
        void sendSpellsActiveRemoval(const ESM::RefId& id, bool isStackingSpell);
        void sendDeath(char newDeathState);

        MWWorld::Ptr getPtr();
        void setPtr(const MWWorld::Ptr& newPtr);

        bool hasSentData;

    private:
        MWWorld::Ptr ptr;

        bool posWasChanged;
        bool equipmentChanged;

        bool wasRunning;
        bool wasSneaking;
        bool wasForceJumping;
        bool wasForceMoveJumping;

        bool wasJumping;
        bool wasFlying;

        MWMechanics::DrawState lastDrawState;

        MWMechanics::DynamicStat<float> oldHealth;
        MWMechanics::DynamicStat<float> oldMagicka;
        MWMechanics::DynamicStat<float> oldFatigue;
    };
}

#endif //OPENMW_LOCALACTOR_HPP
