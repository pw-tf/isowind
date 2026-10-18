#include <cmath>
#include <components/settings/values.hpp>
#include <components/vfs/pathutil.hpp>
#include "Ids.hpp"
#include <components/openmw-mp/TimedLog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwdialogue/dialoguemanagerimp.hpp"

#include "../mwmechanics/aiactivate.hpp"
#include "../mwmechanics/aicombat.hpp"
#include "../mwmechanics/aiescort.hpp"
#include "../mwmechanics/aifollow.hpp"
#include "../mwmechanics/aitravel.hpp"
#include "../mwmechanics/aiwander.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/worldimp.hpp"

#include "DedicatedActor.hpp"
#include "Main.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"

using namespace mwmp;

DedicatedActor::DedicatedActor()
{
    drawState = static_cast<char>(MWMechanics::DrawState::Nothing);
    movementFlags = 0;
    animation.groupname = "";
    sound = "";

    hasPositionData = false;
    hasStatsDynamicData = false;
    hasReceivedInitialEquipment = false;
    hasChangedCell = true;

    attack.pressed = false;
    cast.pressed = false;
}

DedicatedActor::~DedicatedActor()
{

}

void DedicatedActor::update(float dt)
{
    // Only move and set anim flags if the framerate isn't too low
    if (dt < 0.1)
    {
        move(dt);
        setAnimFlags();
    }

    setStatsDynamic();
}

void DedicatedActor::setCell(MWWorld::CellStore *cellStore)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    ptr = world->moveObject(ptr, cellStore, position.asVec3());
    setMovementSettings();

    hasChangedCell = true;
}

void DedicatedActor::move(float dt)
{
    ESM::Position refPos = ptr.getRefData().getPosition();
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const int maxInterpolationDistance = 40;

    // Apply interpolation only if the position hasn't changed too much from last time
    bool shouldInterpolate = std::abs(position.pos[0] - refPos.pos[0]) < maxInterpolationDistance && std::abs(position.pos[1] - refPos.pos[1]) < maxInterpolationDistance && std::abs(position.pos[2] - refPos.pos[2]) < maxInterpolationDistance;

    // Don't apply linear interpolation if the DedicatedActor has just gone through a cell change, because
    // the interpolated position will be invalid, causing a slight hopping glitch
    if (shouldInterpolate && !hasChangedCell)
    {
        static const int timeMultiplier = 15;
        osg::Vec3f lerp = MechanicsHelper::getLinearInterpolation(refPos.asVec3(), position.asVec3(), dt * timeMultiplier);
        ptr = world->moveObject(ptr, lerp);
    }
    else
    {
        setPosition();
        hasChangedCell = false;
    }

    setMovementSettings();
    world->rotateObject(ptr, position.asRotationVec3());
}

void DedicatedActor::setMovementSettings()
{
    MWMechanics::Movement *move = &ptr.getClass().getMovementSettings(ptr);
    move->mPosition[0] = direction.pos[0];
    move->mPosition[1] = direction.pos[1];
    move->mPosition[2] = direction.pos[2];

    // Make sure the values are valid, or we'll get an infinite error loop
    if (!std::isnan(direction.rot[0]) && !std::isnan(direction.rot[1]) && !std::isnan(direction.rot[2]))
    {
        move->mRotation[0] = direction.rot[0];
        move->mRotation[1] = direction.rot[1];
        move->mRotation[2] = direction.rot[2];
    }
}

void DedicatedActor::setPosition()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    ptr = world->moveObject(ptr, position.asVec3());
}

void DedicatedActor::setAnimFlags()
{
    using namespace MWMechanics;

    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);

    ptrCreatureStats->setDrawState(static_cast<MWMechanics::DrawState>(drawState));

    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_Run, (movementFlags & CreatureStats::Flag_Run) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_Sneak, (movementFlags & CreatureStats::Flag_Sneak) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_ForceJump, (movementFlags & CreatureStats::Flag_ForceJump) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_ForceMoveJump, (movementFlags & CreatureStats::Flag_ForceMoveJump) != 0);
}

void DedicatedActor::setStatsDynamic()
{
    // Only set dynamic stats if we have received at least one packet about them
    if (!hasStatsDynamicData) return;

    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);
    MWMechanics::DynamicStat<float> value;

    // Resurrect this Actor if it's not supposed to be dead according to its authority
    if (creatureStats.mDynamic[0].mCurrent > 0)
        MWBase::Environment::get().getMechanicsManager()->resurrect(ptr);

    for (int i = 0; i < 3; ++i)
    {
        value.readState(creatureStats.mDynamic[i]);
        ptrCreatureStats->setDynamic(i, value);
    }
}

void DedicatedActor::setEquipment()
{
    if (!ptr.getClass().hasInventoryStore(ptr))
        return;

    MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);

    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
    {
        int count = equipmentItems[slot].count;

        // If we've somehow received a corrupted item with a count lower than 0, ignore it
        if (count < 0) continue;

        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);

        const std::string &packetRefId = equipmentItems[slot].refId;
        int packetCharge = equipmentItems[slot].charge;
        bool equal = false;

        if (it != invStore.end())
        {
            if (!isSameId(it->getCellRef().getRefId(), packetRefId)) // if other item equiped
                invStore.unequipSlot(slot);
            else
                equal = true;
        }

        if (packetRefId.empty() || equal)
            continue;

        if (!hasItem(packetRefId, packetCharge))
        {
            try
            {
                ptr.getClass().getContainerStore(ptr).add(mwmp::refId(packetRefId), count, false);
            }
            catch (std::exception&)
            {
                LOG_APPEND(TimedLog::LOG_INFO, "- Ignored invalid equipment item %s", packetRefId.c_str());
                continue;
            }
        }

        // Equip items silently if this is the first time equipment is being set for this character
        equipItem(packetRefId, packetCharge, !hasReceivedInitialEquipment);
    }

    hasReceivedInitialEquipment = true;
}

void DedicatedActor::setAi()
{
    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);
    ptrCreatureStats->setAiSetting(MWMechanics::AiSetting::Fight, 0);

    LOG_APPEND(TimedLog::LOG_VERBOSE, "- actor cellRef: %s %i-%i",
        mwmp::idString(ptr.getCellRef().getRefId()).c_str(), ptr.getCellRef().getMpRefNum(), ptr.getCellRef().getMpNum());

    if (aiAction == mwmp::BaseActorList::CANCEL)
    {
        LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Cancelling AI sequence");

        ptrCreatureStats->getAiSequence().clear();
    }
    else if (aiAction == mwmp::BaseActorList::TRAVEL)
    {
        LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Travelling to %f, %f, %f",
            aiCoordinates.pos[0], aiCoordinates.pos[1], aiCoordinates.pos[2]);

        MWMechanics::AiTravel package(aiCoordinates.pos[0], aiCoordinates.pos[1], aiCoordinates.pos[2], false);
        ptrCreatureStats->getAiSequence().stack(package, ptr, true);
    }
    else if (aiAction == mwmp::BaseActorList::WANDER)
    {
        LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Wandering for distance %i and duration %i, repetition is %s",
            aiDistance, aiDuration, aiShouldRepeat ? "true" : "false");

        std::vector<unsigned char> idleList;

        MWMechanics::AiWander package(aiDistance, aiDuration, -1, idleList, aiShouldRepeat);
        ptrCreatureStats->getAiSequence().stack(package, ptr, true);
    }
    else if (hasAiTarget)
    {
        MWWorld::Ptr targetPtr;

        if (aiTarget.isPlayer)
        {
            targetPtr = MechanicsHelper::getPlayerPtr(aiTarget);

            LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Has player target %s",
                std::string(targetPtr.getClass().getName(targetPtr)).c_str());
        }
        else
        {
            if (mwmp::Main::get().getCellController()->isLocalActor(aiTarget.refNum, aiTarget.mpNum))
                targetPtr = mwmp::Main::get().getCellController()->getLocalActor(aiTarget.refNum, aiTarget.mpNum)->getPtr();
            else if (mwmp::Main::get().getCellController()->isDedicatedActor(aiTarget.refNum, aiTarget.mpNum))
                targetPtr = mwmp::Main::get().getCellController()->getDedicatedActor(aiTarget.refNum, aiTarget.mpNum)->getPtr();
            else if (aiAction == mwmp::BaseActorList::ACTIVATE)
                targetPtr = mwmp::Main::get().getCellController()->searchPtrViaUniqueIndex(aiTarget.refNum, aiTarget.mpNum);

            if (!targetPtr.isEmpty())
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Has actor target %s %i-%i",
                    mwmp::idString(targetPtr.getCellRef().getRefId()).c_str(), aiTarget.refNum, aiTarget.mpNum);
            }
            else
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Has invalid actor target %i-%i",
                    aiTarget.refNum, aiTarget.mpNum);
            }

        }

        if (!targetPtr.isEmpty())
        {
            if (aiAction == mwmp::BaseActorList::ACTIVATE)
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Activating target");

                // TES3MP's AiActivate took the exact object; OpenMW 0.51's takes an id and finds the nearest one
                MWMechanics::AiActivate package(targetPtr.getCellRef().getRefId(), false);
                ptrCreatureStats->getAiSequence().stack(package, ptr, true);
            }

            if (aiAction == mwmp::BaseActorList::COMBAT)
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Starting combat with target");

                MWMechanics::AiCombat package(targetPtr);
                ptrCreatureStats->getAiSequence().stack(package, ptr, true);
            }
            else if (aiAction == mwmp::BaseActorList::ESCORT)
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Being escorted by target, for duration %i, to coordinates %f, %f, %f",
                    aiDuration, aiCoordinates.pos[0], aiCoordinates.pos[1], aiCoordinates.pos[2]);

                MWMechanics::AiEscort package(targetPtr.getCellRef().getRefNum(), {}, aiDuration,
                    aiCoordinates.pos[0], aiCoordinates.pos[1], aiCoordinates.pos[2], false);
                ptrCreatureStats->getAiSequence().stack(package, ptr, true);
            }
            else if (aiAction == mwmp::BaseActorList::FOLLOW)
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "-- Following target");

                MWMechanics::AiFollow package(targetPtr);
                package.allowAnyDistance(true);
                ptrCreatureStats->getAiSequence().stack(package, ptr, true);
            }
        }
    }
}

void DedicatedActor::playAnimation()
{
    if (!animation.groupname.empty())
    {
        MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptr,
            animation.groupname, animation.mode, animation.count, animation.persist);

        animation.groupname.clear();
    }
}

void DedicatedActor::playSound()
{
    if (!sound.empty())
    {
        MWBase::Environment::get().getSoundManager()->say(ptr, VFS::Path::Normalized(sound));

        if (Settings::gui().mSubtitles)
            MWBase::Environment::get().getWindowManager()->messageBox(MechanicsHelper::getVoiceCaption(sound),
                MWGui::ShowInDialogueMode_Never);

        sound.clear();
    }
}

bool DedicatedActor::hasItem(const std::string& itemId, int charge)
{
    for (const auto &itemPtr : ptr.getClass().getInventoryStore(ptr))
    {
        if (isSameId(itemPtr.getCellRef().getRefId(), itemId) && itemPtr.getCellRef().getCharge() == charge)
            return true;
    }

    return false;
}

void DedicatedActor::equipItem(const std::string& itemId, int charge, bool noSound)
{
    for (const auto &itemPtr : ptr.getClass().getInventoryStore(ptr))
    {
        if (isSameId(itemPtr.getCellRef().getRefId(), itemId) && itemPtr.getCellRef().getCharge() == charge)
        {
            std::unique_ptr<MWWorld::Action> action = itemPtr.getClass().use(itemPtr);
            action->execute(ptr, noSound);
            break;
        }
    }
}

void DedicatedActor::addSpellsActive()
{
    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
    {
        MechanicsHelper::createSpellGfx(getPtr(), activeSpell.params.mEffects);
        MechanicsHelper::addActiveSpell(getPtr(), activeSpell);
    }
}

void DedicatedActor::removeSpellsActive()
{
    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
        MechanicsHelper::removeActiveSpell(getPtr(), activeSpell);
}

void DedicatedActor::setSpellsActive()
{
    MWMechanics::ActiveSpells& activeSpells = getPtr().getClass().getCreatureStats(getPtr()).getActiveSpells();
    activeSpells.clear(getPtr());

    // Proceed by adding spells active: they were running already, so quietly
    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
        MechanicsHelper::addActiveSpell(getPtr(), activeSpell, true);
}

MWWorld::Ptr DedicatedActor::getPtr()
{
    return ptr;
}

void DedicatedActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;

    refId = idString(ptr.getCellRef().getRefId());
    refNum = ptr.getCellRef().getMpRefNum();
    mpNum = ptr.getCellRef().getMpNum();

    position = ptr.getRefData().getPosition();
    drawState = static_cast<char>(ptr.getClass().getCreatureStats(ptr).getDrawState());
}

void DedicatedActor::reloadPtr()
{
    MWBase::World* world = MWBase::Environment::get().getWorld();
    world->disable(ptr);
    world->enable(ptr);
}
