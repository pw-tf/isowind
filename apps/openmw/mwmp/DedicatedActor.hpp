#ifndef OPENMW_DEDICATEDACTOR_HPP
#define OPENMW_DEDICATEDACTOR_HPP

#include <components/openmw-mp/Base/BaseActor.hpp>
#include "../mwmechanics/aisequence.hpp"
#include "../mwworld/manualref.hpp"

namespace mwmp
{
    class DedicatedActor : public BaseActor
    {
    public:

        DedicatedActor();
        virtual ~DedicatedActor();

        void update(float dt);
        void move(float dt);
        void setCell(MWWorld::CellStore *cellStore);
        void setMovementSettings();
        void setPosition();
        void setAnimFlags();
        void setStatsDynamic();
        void setEquipment();
        void setAi();
        void playAnimation();
        void playSound();

        bool hasItem(const std::string& itemId, int charge);
        void equipItem(const std::string& itemId, int charge, bool noSound = false);

        void addSpellsActive();
        void removeSpellsActive();
        void setSpellsActive();

        MWWorld::Ptr getPtr();
        void setPtr(const MWWorld::Ptr& newPtr);
        void reloadPtr();

    private:
        MWWorld::Ptr ptr;

        bool hasReceivedInitialEquipment;
        bool hasChangedCell;
    };
}

#endif //OPENMW_DEDICATEDACTOR_HPP
