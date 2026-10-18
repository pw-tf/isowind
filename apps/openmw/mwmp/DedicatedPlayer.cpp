#include <algorithm>
#include <cmath>

#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/pathutil.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/activespells.hpp"
#include "../mwmechanics/aisequence.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/worldmodel.hpp"
#include "../mwworld/scene.hpp"

#include "DedicatedPlayer.hpp"
#include "Main.hpp"
#include "GUIController.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"
#include "RecordHelper.hpp"
#include "Ids.hpp"


using namespace mwmp;

DedicatedPlayer::DedicatedPlayer(RakNet::RakNetGUID guid) : BasePlayer(guid)
{
    reference = 0;
    attack.pressed = false;
    cast.pressed = false;

    creatureStats.mDead = false;
    // Give this new character a temporary high fatigue so it doesn't spawn on
    // the ground
    creatureStats.mDynamic[2].mBase = 1000;

    attack.instant = false;

    MWBase::World* world = MWBase::Environment::get().getWorld();

    cell = MWBase::Environment::get().getWorldModel()->getInterior(RecordHelper::getPlaceholderInteriorCellName())
        .getCell()->getEsm3();
    position.pos[0] = position.pos[1] = position.pos[2] = 0;

    npc = *world->getPlayerPtr().get<ESM::NPC>()->mBase;
    npc.mId = ESM::RefId();
    previousRace = npc.mRace;

    hasReceivedInitialEquipment = false;
    hasFinishedInitialTeleportation = false;

    isJumping = false;
    wasJumping = false;
}

DedicatedPlayer::~DedicatedPlayer()
{

}

void DedicatedPlayer::update(float dt)
{
    // Isowind: every frame, slow ones too (skipping them let the actor wander off on its last movement until the
    // framerate picked up, ending up somewhere else than for everyone else); the step toward the position from the
    // network is dt * 15 of the way, so dt is capped where that gets all the way there
    move(std::min(dt, 1.f / 15.f));
    setAnimFlags();

    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);

    MWMechanics::DynamicStat<float> value;

    if (creatureStats.mDead)
    {
        value.readState(creatureStats.mDynamic[0]);
        ptrCreatureStats->setHealth(value);
        return;
    }

    for (int i = 0; i < 3; ++i)
    {
        value.readState(creatureStats.mDynamic[i]);
        ptrCreatureStats->setDynamic(i, value);
    }

    if (ptrCreatureStats->isDead())
        MWBase::Environment::get().getMechanicsManager()->resurrect(ptr);

    ptrCreatureStats->setAttacked(false);

    ptrCreatureStats->getAiSequence().stopCombat();

    ptrCreatureStats->setAlarmed(false);
    ptrCreatureStats->setAiSetting(MWMechanics::AiSetting::Alarm, 0);
    ptrCreatureStats->setAiSetting(MWMechanics::AiSetting::Fight, 0);
    ptrCreatureStats->setAiSetting(MWMechanics::AiSetting::Flee, 0);
    ptrCreatureStats->setAiSetting(MWMechanics::AiSetting::Hello, 0);
}

void DedicatedPlayer::move(float dt)
{
    if (!reference) return;

    ESM::Position refPos = ptr.getRefData().getPosition();
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const int maxInterpolationDistance = 80;

    // Apply interpolation only if the position hasn't changed too much from last time
    bool shouldInterpolate =
            std::abs(position.pos[0] - refPos.pos[0]) < maxInterpolationDistance &&
            std::abs(position.pos[1] - refPos.pos[1]) < maxInterpolationDistance &&
            std::abs(position.pos[2] - refPos.pos[2]) < maxInterpolationDistance;

    if (shouldInterpolate)
    {
        static const int timeMultiplier = 15;
        osg::Vec3f lerp = MechanicsHelper::getLinearInterpolation(refPos.asVec3(), position.asVec3(), dt * timeMultiplier);

        ptr = world->moveObject(ptr, lerp);
    }
    else
        ptr = world->moveObject(ptr, position.asVec3());

    world->rotateObject(ptr, osg::Vec3f(position.rot[0], 0, position.rot[2]));

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

void DedicatedPlayer::setBaseInfo()
{
    // Use the previous race if the new one doesn't exist
    if (!RecordHelper::doesRecordIdExist<ESM::Race>(npc.mRace))
        npc.mRace = previousRace;

    if (!reference)
    {
        npc.mId = RecordHelper::createRecord(npc)->mId;
        createReference(npc.mId);
    }
    else
    {
        RecordHelper::overrideRecord(npc);
        reloadPtr();
    }

    // Only set equipment if the player isn't disguised as a creature
    if (ptr.getType() == ESM::NPC::sRecordId)
        setEquipment();

    previousRace = npc.mRace;
}

void DedicatedPlayer::setStatsDynamic()
{
    MWMechanics::CreatureStats* ptrCreatureStats = &getPtr().getClass().getCreatureStats(getPtr());
    MWMechanics::DynamicStat<float> value;

    for (int i = 0; i < 3; ++i)
    {
        value.readState(creatureStats.mDynamic[i]);
        ptrCreatureStats->setDynamic(i, value);
    }
}

void DedicatedPlayer::setAnimFlags()
{
    using namespace MWMechanics;

    MWBase::World *world = MWBase::Environment::get().getWorld();

    // Until we figure out a better workaround for disabling player gravity,
    // simply cast Levitate over and over on a player that's supposed to be flying
    if (!isFlying && !hasTcl && !isLevitationPurged)
    {
        ptr.getClass().getCreatureStats(ptr).getActiveSpells().purgeEffect(ptr, ESM::MagicEffect::Levitate);
        isLevitationPurged = true;
    }
    else if ((isFlying || hasTcl) && !world->isFlying(ptr))
    {
        MWMechanics::CastSpell levitationCast(ptr, ptr);
        levitationCast.mHitPosition = ptr.getRefData().getPosition().asVec3();
        levitationCast.mAlwaysSucceed = true;
        levitationCast.cast(refId("Levitate"));
        isLevitationPurged = false;
    }

    if (isJumping && !wasJumping)
    {
        world->setOnGround(ptr, false);
        wasJumping = true;
    }
    else if (wasJumping && !isJumping)
    {
        world->setOnGround(ptr, true);
        wasJumping = false;
    }

    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);

    ptrCreatureStats->setDrawState(static_cast<MWMechanics::DrawState>(drawState));

    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_Run, (movementFlags & CreatureStats::Flag_Run) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_Sneak, (movementFlags & CreatureStats::Flag_Sneak) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_ForceJump, (movementFlags & CreatureStats::Flag_ForceJump) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_ForceMoveJump, (movementFlags & CreatureStats::Flag_ForceMoveJump) != 0);
}

void DedicatedPlayer::setAttributes()
{
    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);
    MWMechanics::AttributeValue attributeValue;

    for (int i = 0; i < ESM::Attribute::Length; ++i)
    {
        attributeValue.readState(creatureStats.mAttributes[i]);
        ptrCreatureStats->setAttribute(attributeId(i), attributeValue);
    }
}

void DedicatedPlayer::setSkills()
{
    // Go no further if the player is disguised as a creature
    if (ptr.getType() != ESM::NPC::sRecordId) return;

    MWMechanics::NpcStats *ptrNpcStats = &ptr.getClass().getNpcStats(ptr);
    MWMechanics::SkillValue skillValue;

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        skillValue.readState(npcStats.mSkills[i]);
        ptrNpcStats->setSkill(skillId(i), skillValue);
    }
}

void DedicatedPlayer::setEquipment()
{
    // Go no further if the player is disguised as a creature
    if (!ptr.getClass().hasInventoryStore(ptr)) return;

    bool equippedSomething = false;

    MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
    {
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);

        const std::string &packetRefId = equipmentItems[slot].refId;
        bool equal = false;

        if (it != invStore.end())
        {
            const ESM::RefId ptrItemId = it->getCellRef().getRefId();

            if (!isSameId(ptrItemId, packetRefId)) // if other item is now equipped
            {
                MWWorld::ContainerStore &store = ptr.getClass().getContainerStore(ptr);

                // Remove the items that are no longer equipped, except for throwing weapons and ranged weapon ammo that
                // have just run out but still need to be kept briefly so they can be used in attacks about to be released
                bool shouldRemove = true;

                if (attack.type == mwmp::Attack::RANGED && packetRefId.empty() && !attack.pressed)
                {
                    if (slot == MWWorld::InventoryStore::Slot_CarriedRight && isSameId(ptrItemId, attack.rangedWeaponId))
                        shouldRemove = false;
                    else if (slot == MWWorld::InventoryStore::Slot_Ammunition && isSameId(ptrItemId, attack.rangedAmmoId))
                        shouldRemove = false;
                }

                if (shouldRemove)
                {
                    store.remove(ptrItemId, store.count(ptrItemId));
                }
            }
            else
                equal = true;
        }

        if (packetRefId.empty() || equal)
            continue;

        const int count = equipmentItems[slot].count;
        try
        {
            ptr.getClass().getContainerStore(ptr).add(refId(packetRefId), count, false);
        }
        catch (std::exception&)
        {
            LOG_APPEND(TimedLog::LOG_INFO, "- Ignored invalid equipment item %s", packetRefId.c_str());
            continue;
        }
        // Equip items silently if this is the first time equipment is being set for this character
        equipItem(refId(packetRefId), !hasReceivedInitialEquipment);
        equippedSomething = true;
    }

    // Only track the initial equipment as received if at least one item has been equipped
    if (equippedSomething)
        hasReceivedInitialEquipment = true;
}

void DedicatedPlayer::setShapeshift()
{
    MWBase::World* world = MWBase::Environment::get().getWorld();

    bool isNpc = false;

    if (reference)
        isNpc = ptr.getType() == ESM::NPC::sRecordId;

    if (creatureRefId != previousCreatureRefId || displayCreatureName != previousDisplayCreatureName)
    {
        if (!creatureRefId.empty() && RecordHelper::doesRecordIdExist<ESM::Creature>(creatureRefId))
        {
            deleteReference();

            const ESM::Creature* tmpCreature = world->getStore().get<ESM::Creature>().search(refId(creatureRefId));
            creature = *tmpCreature;
            creature.mScript = ESM::RefId();
            if (!displayCreatureName)
                creature.mName = npc.mName;
            LOG_APPEND(TimedLog::LOG_INFO, "- %s is disguised as %s", npc.mName.c_str(), creatureRefId.c_str());

            // Is this our first time creating a creature record id for this player? If so, keep it around
            // and reuse it
            if (creatureRecordId.empty())
            {
                creature.mId = creatureRecordId = RecordHelper::createRecord(creature)->mId;
                LOG_APPEND(TimedLog::LOG_INFO, "- Creating new creature record %s", idString(creatureRecordId).c_str());
            }
            else
            {
                creature.mId = creatureRecordId;
                RecordHelper::overrideRecord(creature);
            }

            LOG_APPEND(TimedLog::LOG_INFO, "- Creating reference for %s", idString(creature.mId).c_str());
            createReference(creature.mId);
        }
        // This player was already a creature, but the new creature refId was empty or
        // invalid, so we'll turn this player into their NPC self again as a result
        else if (!isNpc)
        {
            if (reference)
            {
                deleteReference();
            }

            RecordHelper::overrideRecord(npc);
            createReference(npc.mId);
            reloadPtr();
        }

        previousCreatureRefId = creatureRefId;
        previousDisplayCreatureName = displayCreatureName;
    }

    if (ptr.getType() == ESM::NPC::sRecordId)
    {
        MWBase::Environment::get().getMechanicsManager()->setWerewolf(ptr, isWerewolf);

        if (!isWerewolf)
            setEquipment();
    }

    MWBase::Environment::get().getWorld()->scaleObject(ptr, scale);
}

void DedicatedPlayer::setCell()
{
    // Prevent cell update when reference doesn't exist
    if (!reference) return;

    MWBase::World *world = MWBase::Environment::get().getWorld();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Server says DedicatedPlayer %s moved to %s",
        npc.mName.c_str(), cell.getShortDescription().c_str());

    MWWorld::CellStore *cellStore = Main::get().getCellController()->getCellStore(cell);

    if (!cellStore)
    {
        LOG_APPEND(TimedLog::LOG_INFO, "%s", "- Cell doesn't exist on this client");
        world->disable(getPtr());
        return;
    }
    else
        world->enable(getPtr());

    // Make sure the Ptr's dynamic stats and anim flags are up-to-date, so it doesn't show up
    // knocked down or in a jump loop when it shouldn't
    setStatsDynamic();
    setAnimFlags();

    // Allow this player's reference to move across a cell now that a manual cell
    // update has been called
    setPtr(world->moveObject(ptr, cellStore, position.asVec3()));

    LOG_APPEND(TimedLog::LOG_INFO, "- Now in %s (%s), %s, %s, at %.0f %.0f %.0f",
        ptr.getCell()->getCell()->getEsm3().getShortDescription().c_str(),
        MWBase::Environment::get().getWorldScene()->isCellActive(*ptr.getCell()) ? "active" : "inactive",
        ptr.getRefData().isEnabled() ? "enabled" : "disabled",
        ptr.getRefData().getBaseNode() ? "in the scene" : "not in the scene",
        ptr.getRefData().getPosition().pos[0], ptr.getRefData().getPosition().pos[1],
        ptr.getRefData().getPosition().pos[2]);

    // Remove the marker entirely if this player has moved to an interior that is inactive for us
    if (!cell.isExterior() && !Main::get().getCellController()->isActiveWorldCell(cell))
        removeMarker();
    // Otherwise, update their marker so the player shows up in the right cell on the world map
    else
    {
        enableMarker();
    }

    // If this player is now in a cell that we are the local authority over, we should send them all
    // NPC data in that cell
    if (Main::get().getCellController()->hasLocalAuthority(cell))
        Main::get().getCellController()->getCell(cell)->updateLocal(true);

    // If this player is a new player or is now in a region that we are the weather authority over,
    // or is a new player, we should send our latest weather data to the server
    if (world->getWeatherCreationState())
    {
        if (!hasFinishedInitialTeleportation
            || getPtr().getCell()->getCell()->getRegion() == world->getPlayerPtr().getCell()->getCell()->getRegion())
        {
            world->sendWeather();
        }
    }

    hasFinishedInitialTeleportation = true;
}

void DedicatedPlayer::playAnimation()
{
    MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(getPtr(),
        animation.groupname, animation.mode, animation.count, animation.persist);
}

void DedicatedPlayer::playSpeech()
{
    MWBase::Environment::get().getSoundManager()->say(getPtr(), VFS::Path::Normalized(sound));

    if (Settings::gui().mSubtitles)
        MWBase::Environment::get().getWindowManager()->messageBox(MechanicsHelper::getVoiceCaption(sound),
            MWGui::ShowInDialogueMode_Never);
}

void DedicatedPlayer::equipItem(const ESM::RefId& itemId, bool noSound)
{
    for (const auto& itemPtr : ptr.getClass().getInventoryStore(ptr))
    {
        if (itemPtr.getCellRef().getRefId() == itemId)
        {
            std::unique_ptr<MWWorld::Action> action = itemPtr.getClass().use(itemPtr);
            action->execute(ptr, noSound);
            break;
        }
    }
}

void DedicatedPlayer::die()
{
    MWMechanics::DynamicStat<float> health;
    creatureStats.mDead = true;
    health.readState(creatureStats.mDynamic[0]);
    health.setCurrent(0);
    health.writeState(creatureStats.mDynamic[0]);

    ptr.getClass().getCreatureStats(ptr).setHealth(health);
}

void DedicatedPlayer::resurrect()
{
    creatureStats.mDead = false;
    if (creatureStats.mDynamic[0].mMod < 1)
        creatureStats.mDynamic[0].mMod = 1;
    creatureStats.mDynamic[0].mCurrent = creatureStats.mDynamic[0].mMod;

    MWBase::Environment::get().getMechanicsManager()->resurrect(getPtr());

    MWMechanics::DynamicStat<float> health;
    health.readState(creatureStats.mDynamic[0]);
    getPtr().getClass().getCreatureStats(getPtr()).setHealth(health);
}

void DedicatedPlayer::addSpellsActive()
{
    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
    {
        MechanicsHelper::createSpellGfx(getPtr(), activeSpell.params.mEffects);
        MechanicsHelper::addActiveSpell(getPtr(), activeSpell);
    }
}

void DedicatedPlayer::removeSpellsActive()
{
    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
        MechanicsHelper::removeActiveSpell(getPtr(), activeSpell);
}

void DedicatedPlayer::setSpellsActive()
{
    MWMechanics::ActiveSpells& activeSpells = getPtr().getClass().getCreatureStats(getPtr()).getActiveSpells();
    activeSpells.clear(getPtr());

    // Proceed by adding spells active: they were running already, so quietly
    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
        MechanicsHelper::addActiveSpell(getPtr(), activeSpell, true);
}

void DedicatedPlayer::updateMarker()
{
    if (!markerEnabled)
    {
        return;
    }

    GUIController* gui = Main::get().getGUIController();

    if (gui->mPlayerMarkers.contains(marker))
    {
        gui->mPlayerMarkers.deleteMarker(marker);
        marker = gui->createMarker(guid);
        gui->mPlayerMarkers.addMarker(marker);
    }
    else
    {
        gui->mPlayerMarkers.addMarker(marker, true);
    }
}

void DedicatedPlayer::enableMarker()
{
    markerEnabled = true;
    updateMarker();
}

void DedicatedPlayer::removeMarker()
{
    if (!markerEnabled)
        return;

    markerEnabled = false;
    GUIController* gui = Main::get().getGUIController();

    if (gui->mPlayerMarkers.contains(marker))
    {
        Main::get().getGUIController()->mPlayerMarkers.deleteMarker(marker);
    }
}

void DedicatedPlayer::createReference(const ESM::RefId& recId)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    reference = new MWWorld::ManualRef(world->getStore(), recId, 1);

    LOG_APPEND(TimedLog::LOG_INFO, "- Creating new reference pointer for %s", npc.mName.c_str());

    ptr = world->placeObject(reference->getPtr(), Main::get().getCellController()->getCellStore(cell), position);

    ESM::CustomMarker mEditingMarker = Main::get().getGUIController()->createMarker(guid);
    marker = mEditingMarker;
    enableMarker();
}

void DedicatedPlayer::deleteReference()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    LOG_APPEND(TimedLog::LOG_INFO, "- Deleting reference");
    world->deleteObject(ptr);
    delete reference;
    reference = nullptr;
}

MWWorld::Ptr DedicatedPlayer::getPtr()
{
    return ptr;
}

MWWorld::ManualRef *DedicatedPlayer::getRef()
{
    return reference;
}

void DedicatedPlayer::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;
}

void DedicatedPlayer::reloadPtr()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    world->disable(ptr);
    world->enable(ptr);
}
