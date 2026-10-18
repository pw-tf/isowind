#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/Utils.hpp>

#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/strings/algorithm.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/scene.hpp"

#include "../mwmechanics/activespells.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/damagesourcetype.hpp"
#include "../mwmechanics/levelledlist.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"

#include "MechanicsHelper.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"
#include "PlayerList.hpp"
#include "ObjectList.hpp"
#include "CellController.hpp"
#include "Ids.hpp"

using namespace mwmp;

namespace
{
    // The weapon attack type the attack animation stands for (ESM::Weapon::AT_*).
    int getAttackType(const std::string& attackAnimation)
    {
        if (attackAnimation.find("slash") != std::string::npos)
            return ESM::Weapon::AT_Slash;
        if (attackAnimation.find("thrust") != std::string::npos)
            return ESM::Weapon::AT_Thrust;
        return ESM::Weapon::AT_Chop;
    }
}

osg::Vec3f MechanicsHelper::getLinearInterpolation(osg::Vec3f start, osg::Vec3f end, float percent)
{
    osg::Vec3f position(percent, percent, percent);

    return (start + osg::componentMultiply(position, (end - start)));
}

ESM::Position MechanicsHelper::getPositionFromVector(osg::Vec3f vector)
{
    ESM::Position position;
    position.pos[0] = vector.x();
    position.pos[1] = vector.y();
    position.pos[2] = vector.z();

    return position;
}

// Inspired by similar code in mwclass\creaturelevlist.cpp
//
// TODO: Add handling of scaling based on leveled list's assigned scale
void MechanicsHelper::spawnLeveledCreatures(MWWorld::CellStore* cellStore)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    mwmp::ObjectList *objectList = mwmp::Main::get().getNetworking()->getObjectList();
    objectList->reset();
    objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;

    // Pick the creatures first: the cell can't change while it's being visited
    std::vector<std::pair<ESM::RefId, ESM::Position>> spawns;
    cellStore->forEachType<ESM::CreatureLevList>([&](const MWWorld::Ptr& ptr) {
        ESM::RefId id = MWMechanics::getLevelledItem(ptr.get<ESM::CreatureLevList>()->mBase, true, world->getPrng());
        if (!id.empty())
            spawns.emplace_back(id, ptr.getCellRef().getPosition());
        return true;
    });

    for (const auto& [id, position] : spawns)
    {
        MWWorld::ManualRef manualRef(world->getStore(), id);
        manualRef.getPtr().getCellRef().setPosition(position);
        MWWorld::Ptr placed = world->placeObject(manualRef.getPtr(), cellStore, position);
        objectList->addObjectSpawn(placed);
        world->deleteObject(placed);
    }

    if (!spawns.empty())
        objectList->sendObjectSpawn();
}

bool MechanicsHelper::respawnActor(const MWWorld::Ptr& ptr)
{
    if (ptr.isEmpty() || !ptr.getClass().isActor() || ptr.getCell()->getState() != MWWorld::CellStore::State_Loaded
        || MWBase::Environment::get().getWorldScene()->isCellActive(*ptr.getCell()))
        return false;

    // What Npc::respawn and Creature::respawn do, whatever the Respawn flag and time since death (its local
    // script comes back when the cell becomes active)
    MWBase::World* world = MWBase::Environment::get().getWorld();
    if (ptr.getCellRef().getCount() == 0)
        ptr.getCellRef().setCount(1);
    MWBase::Environment::get().getWindowManager()->onDeleteCustomData(ptr);
    ptr.getRefData().setCustomData(nullptr);

    const ESM::Position& start = ptr.getCellRef().getPosition();
    MWWorld::Ptr moved = world->moveObject(ptr, ptr.getCell()->getOriginCell(ptr), start.asVec3());
    world->rotateObject(moved, start.asRotationVec3(), MWBase::RotationFlag_none);
    return true;
}

bool MechanicsHelper::isUsingRangedWeapon(const MWWorld::Ptr& ptr)
{
    if (ptr.getClass().hasInventoryStore(ptr))
    {
        MWWorld::InventoryStore &inventoryStore = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator weaponSlot = inventoryStore.getSlot(
            MWWorld::InventoryStore::Slot_CarriedRight);

        if (weaponSlot != inventoryStore.end() && weaponSlot->getType() == ESM::Weapon::sRecordId)
        {
            const ESM::Weapon* weaponRecord = weaponSlot->get<ESM::Weapon>()->mBase;

            if (weaponRecord->mData.mType >= ESM::Weapon::MarksmanBow)
                return true;
        }
    }

    return false;
}

Attack *MechanicsHelper::getLocalAttack(const MWWorld::Ptr& ptr)
{
    if (ptr == MWMechanics::getPlayer())
        return &mwmp::Main::get().getLocalPlayer()->attack;
    else if (mwmp::Main::get().getCellController()->isLocalActor(ptr))
        return &mwmp::Main::get().getCellController()->getLocalActor(ptr)->attack;

    return nullptr;
}

Attack *MechanicsHelper::getDedicatedAttack(const MWWorld::Ptr& ptr)
{
    if (mwmp::PlayerList::isDedicatedPlayer(ptr))
        return &mwmp::PlayerList::getPlayer(ptr)->attack;
    else if (mwmp::Main::get().getCellController()->isDedicatedActor(ptr))
        return &mwmp::Main::get().getCellController()->getDedicatedActor(ptr)->attack;

    return nullptr;
}

Cast *MechanicsHelper::getLocalCast(const MWWorld::Ptr& ptr)
{
    if (ptr == MWMechanics::getPlayer())
        return &mwmp::Main::get().getLocalPlayer()->cast;
    else if (mwmp::Main::get().getCellController()->isLocalActor(ptr))
        return &mwmp::Main::get().getCellController()->getLocalActor(ptr)->cast;

    return nullptr;
}

Cast *MechanicsHelper::getDedicatedCast(const MWWorld::Ptr& ptr)
{
    if (mwmp::PlayerList::isDedicatedPlayer(ptr))
        return &mwmp::PlayerList::getPlayer(ptr)->cast;
    else if (mwmp::Main::get().getCellController()->isDedicatedActor(ptr))
        return &mwmp::Main::get().getCellController()->getDedicatedActor(ptr)->cast;

    return nullptr;
}

MWWorld::Ptr MechanicsHelper::getPlayerPtr(const Target& target)
{
    if (target.guid == mwmp::Main::get().getLocalPlayer()->guid)
    {
        return MWMechanics::getPlayer();
    }
    else
    {
        mwmp::DedicatedPlayer* dedicatedPlayer = mwmp::PlayerList::getPlayer(target.guid);

        if (dedicatedPlayer != nullptr)
        {
            return dedicatedPlayer->getPtr();
        }
    }

    return MWWorld::Ptr();
}

MWWorld::Ptr MechanicsHelper::getTargetPtr(const mwmp::Target& target)
{
    if (target.isPlayer)
        return getPlayerPtr(target);

    auto controller = mwmp::Main::get().getCellController();
    if (controller->isLocalActor(target.refNum, target.mpNum))
        return controller->getLocalActor(target.refNum, target.mpNum)->getPtr();
    else if (controller->isDedicatedActor(target.refNum, target.mpNum))
        return controller->getDedicatedActor(target.refNum, target.mpNum)->getPtr();

    return MWWorld::Ptr();
}

mwmp::Item MechanicsHelper::getItem(const MWWorld::Ptr& itemPtr, int count)
{
    mwmp::Item item;

    if (itemPtr.getClass().isGold(itemPtr))
        item.refId = idString(MWWorld::ContainerStore::sGoldId);
    else
        item.refId = idString(itemPtr.getCellRef().getRefId());

    item.count = count;
    item.charge = itemPtr.getCellRef().getCharge();
    item.enchantmentCharge = itemPtr.getCellRef().getEnchantmentCharge();
    item.soul = idString(itemPtr.getCellRef().getSoul());

    return item;
}

mwmp::Target MechanicsHelper::getTarget(const MWWorld::Ptr& ptr)
{
    mwmp::Target target;
    clearTarget(target);

    if (!ptr.isEmpty())
    {
        if (ptr == MWMechanics::getPlayer())
        {
            target.isPlayer = true;
            target.guid = mwmp::Main::get().getLocalPlayer()->guid;
        }
        else if (mwmp::PlayerList::isDedicatedPlayer(ptr))
        {
            target.isPlayer = true;
            target.guid = mwmp::PlayerList::getPlayer(ptr)->guid;
        }
        else
        {
            const MWWorld::CellRef &ptrRef = ptr.getCellRef();

            target.isPlayer = false;
            target.refId = idString(ptrRef.getRefId());
            target.refNum = ptrRef.getMpRefNum();
            target.mpNum = ptrRef.getMpNum();
            target.name = ptr.getClass().getName(ptr);
        }
    }

    return target;
}

void MechanicsHelper::clearTarget(mwmp::Target& target)
{
    target.isPlayer = false;
    target.refId.clear();
    target.refNum = -1;
    target.mpNum = -1;

    target.name.clear();
}

bool MechanicsHelper::isEmptyTarget(const mwmp::Target& target)
{
    if (target.isPlayer == false && target.refId.empty())
        return true;

    return false;
}

void MechanicsHelper::assignAttackTarget(Attack* attack, const MWWorld::Ptr& target)
{
    if (target == MWMechanics::getPlayer())
    {
        attack->target.isPlayer = true;
        attack->target.guid = mwmp::Main::get().getLocalPlayer()->guid;
    }
    else if (mwmp::PlayerList::isDedicatedPlayer(target))
    {
        attack->target.isPlayer = true;
        attack->target.guid = mwmp::PlayerList::getPlayer(target)->guid;
    }
    else
    {
        const MWWorld::CellRef &targetRef = target.getCellRef();

        attack->target.isPlayer = false;
        attack->target.refId = idString(targetRef.getRefId());
        attack->target.refNum = targetRef.getMpRefNum();
        attack->target.mpNum = targetRef.getMpNum();
    }
}

void MechanicsHelper::resetAttack(Attack* attack)
{
    attack->isHit = false;
    attack->success = false;
    attack->knockdown = false;
    attack->block = false;
    attack->applyWeaponEnchantment = false;
    attack->applyAmmoEnchantment = false;
    attack->hitPosition.pos[0] = attack->hitPosition.pos[1] = attack->hitPosition.pos[2] = 0;
    attack->target.guid = RakNet::RakNetGUID();
    attack->target.refId.clear();
    attack->target.refNum = 0;
    attack->target.mpNum = 0;
}

void MechanicsHelper::resetCast(Cast* cast)
{
    cast->isHit = false;
    cast->success = false;
    cast->target.guid = RakNet::RakNetGUID();
    cast->target.refId.clear();
    cast->target.refNum = 0;
    cast->target.mpNum = 0;
}

bool MechanicsHelper::getSpellSuccess(const std::string& spellId, const MWWorld::Ptr& caster)
{
    return Misc::Rng::roll0to99(MWBase::Environment::get().getWorld()->getPrng())
        < MWMechanics::getSpellSuccessChance(refId(spellId), caster, nullptr, true, false);
}

bool MechanicsHelper::isTeamMember(const MWWorld::Ptr& playerChecked, const MWWorld::Ptr& playerWithTeam)
{
    bool isTeamMember = false;
    bool playerCheckedIsLocal = playerChecked == MWMechanics::getPlayer();
    bool playerCheckedIsDedicated = !playerCheckedIsLocal ? mwmp::PlayerList::isDedicatedPlayer(playerChecked) : false;
    bool playerWithTeamIsLocal = !playerCheckedIsLocal ? playerWithTeam == MWMechanics::getPlayer() : false;
    bool playerWithTeamIsDedicated = !playerWithTeamIsLocal ? mwmp::PlayerList::isDedicatedPlayer(playerWithTeam) : false;

    if (playerCheckedIsLocal || playerCheckedIsDedicated)
    {
        if (playerWithTeamIsLocal || playerWithTeamIsDedicated)
        {
            RakNet::RakNetGUID playerCheckedGuid;

            if (playerCheckedIsLocal)
                playerCheckedGuid = mwmp::Main::get().getLocalPlayer()->guid;
            else
                playerCheckedGuid = PlayerList::getPlayer(playerChecked)->guid;

            if (playerWithTeamIsLocal)
                isTeamMember = Utils::vectorContains(mwmp::Main::get().getLocalPlayer()->alliedPlayers, playerCheckedGuid);
            else
                isTeamMember = Utils::vectorContains(PlayerList::getPlayer(playerWithTeam)->alliedPlayers, playerCheckedGuid);
        }
    }

    return isTeamMember;
}

void MechanicsHelper::processAttack(Attack attack, const MWWorld::Ptr& attacker)
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Processing attack from %s of type %i",
        std::string(attacker.getClass().getName(attacker)).c_str(), attack.type);

    LOG_APPEND(TimedLog::LOG_VERBOSE, "- pressed: %s", attack.pressed ? "true" : "false");

    if (!attack.pressed)
    {
        LOG_APPEND(TimedLog::LOG_VERBOSE, "- success: %s", attack.success ? "true" : "false");

        if (attack.success)
            LOG_APPEND(TimedLog::LOG_VERBOSE, "- damage: %f", attack.damage);
    }
    else
    {
        if (attack.type == attack.MELEE)
            LOG_APPEND(TimedLog::LOG_VERBOSE, "- animation: %s", attack.attackAnimation.c_str());
    }

    // The attack animation (chop, slash, thrust, shoot) is played as on the attacker's client: with AI off for remote
    // actors, the character controller takes it from their stats
    if (attack.pressed && !attack.attackAnimation.empty())
        attacker.getClass().getCreatureStats(attacker).setAttackType(attack.attackAnimation);
    attacker.getClass().getCreatureStats(attacker).setAttackingOrSpell(attack.pressed);

    MWWorld::Ptr victim = getTargetPtr(attack.target);

    if (attack.isHit)
    {
        bool isRanged = attack.type == attack.RANGED;

        MWWorld::Ptr weaponPtr;
        MWWorld::Ptr ammoPtr;

        bool usedTempRangedWeapon = false;
        bool usedTempRangedAmmo = false;

        // Get the attacker's current weapon
        //
        // Note: if using hand-to-hand, the weapon is equal to inv.end()
        if (attacker.getClass().hasInventoryStore(attacker))
        {
            MWWorld::InventoryStore &inventoryStore = attacker.getClass().getInventoryStore(attacker);
            MWWorld::ContainerStoreIterator weaponSlot = inventoryStore.getSlot(
                MWWorld::InventoryStore::Slot_CarriedRight);

            weaponPtr = weaponSlot != inventoryStore.end() ? *weaponSlot : MWWorld::Ptr();

            // Is the currently selected weapon different from the one recorded for this ranged attack?
            // If so, try to find the correct one in the attacker's inventory and use it here. If it
            // no longer exists, add it back temporarily.
            if (isRanged)
            {
                if (weaponPtr.isEmpty() || !(weaponPtr.getCellRef().getRefId() == attack.rangedWeaponId))
                {
                    weaponPtr = inventoryStore.search(refId(attack.rangedWeaponId));

                    if (weaponPtr.isEmpty())
                    {
                        weaponPtr = *inventoryStore.MWWorld::ContainerStore::add(refId(attack.rangedWeaponId), 1, false);
                        usedTempRangedWeapon = true;
                    }
                }

                if (!attack.rangedAmmoId.empty())
                {
                    MWWorld::ContainerStoreIterator ammoSlot = inventoryStore.getSlot(
                        MWWorld::InventoryStore::Slot_Ammunition);
                    ammoPtr = ammoSlot != inventoryStore.end() ? *ammoSlot : MWWorld::Ptr();

                    if (ammoPtr.isEmpty() || !(ammoPtr.getCellRef().getRefId() == attack.rangedAmmoId))
                    {
                        ammoPtr = inventoryStore.search(refId(attack.rangedAmmoId));

                        if (ammoPtr.isEmpty())
                        {
                            ammoPtr = *inventoryStore.MWWorld::ContainerStore::add(refId(attack.rangedAmmoId), 1, false);
                            usedTempRangedAmmo = true;
                        }
                    }
                }
            }

            if (!weaponPtr.isEmpty() && weaponPtr.getType() != ESM::Weapon::sRecordId)
                weaponPtr = MWWorld::Ptr();
        }

        if (!weaponPtr.isEmpty())
        {
            LOG_APPEND(TimedLog::LOG_VERBOSE, "- weapon: %s\n- isRanged: %s\n- applyWeaponEnchantment: %s\n- applyAmmoEnchantment: %s",
                idString(weaponPtr.getCellRef().getRefId()).c_str(), isRanged ? "true" : "false",
                attack.applyWeaponEnchantment ? "true" : "false", attack.applyAmmoEnchantment ? "true" : "false");

            if (attack.applyWeaponEnchantment)
            {
                MWMechanics::CastSpell cast(attacker, victim, isRanged);
                cast.mHitPosition = attack.hitPosition.asVec3();
                cast.cast(weaponPtr, false);
            }

            if (isRanged && !ammoPtr.isEmpty() && attack.applyAmmoEnchantment)
            {
                MWMechanics::CastSpell cast(attacker, victim, isRanged);
                cast.mHitPosition = attack.hitPosition.asVec3();
                cast.cast(ammoPtr, false);
            }
        }

        if (!victim.isEmpty())
        {
            bool isHealthDamage = true;
            float damage = attack.damage;

            if (weaponPtr.isEmpty())
            {
                if (attacker.getClass().isBipedal(attacker))
                {
                    MWMechanics::CreatureStats &victimStats = victim.getClass().getCreatureStats(victim);
                    isHealthDamage = victimStats.isParalyzed() || victimStats.getKnockedDown();
                }
            }

            if (!isRanged && MWMechanics::blockMeleeAttack(attacker, victim, weaponPtr, damage, 1))
                damage = 0;

            // The same path a hit made here takes (OpenMW's Lua combat handler applies it)
            MWBase::Environment::get().getLuaManager()->onHit(attacker, victim, weaponPtr, ammoPtr,
                getAttackType(attack.attackAnimation), attack.attackStrength, damage, isHealthDamage,
                attack.hitPosition.asVec3(), attack.success,
                isRanged ? MWMechanics::DamageSourceType::Ranged : MWMechanics::DamageSourceType::Melee);
        }

        // Remove temporary items that may have been added above for ranged attacks
        if (isRanged && attacker.getClass().hasInventoryStore(attacker))
        {
            MWWorld::InventoryStore &inventoryStore = attacker.getClass().getInventoryStore(attacker);

            if (usedTempRangedWeapon)
                inventoryStore.remove(weaponPtr, 1);

            if (usedTempRangedAmmo)
                inventoryStore.remove(ammoPtr, 1);
        }
    }
}

void MechanicsHelper::processCast(Cast cast, const MWWorld::Ptr& caster)
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Processing cast from %s of type %i",
        std::string(caster.getClass().getName(caster)).c_str(), cast.type);

    LOG_APPEND(TimedLog::LOG_VERBOSE, "- pressed: %s", cast.pressed ? "true" : "false");

    if (!cast.pressed)
    {
        LOG_APPEND(TimedLog::LOG_VERBOSE, "- success: %s", cast.success ? "true" : "false");
    }

    MWMechanics::CreatureStats &casterStats = caster.getClass().getCreatureStats(caster);
    casterStats.setAttackingOrSpell(cast.pressed);

    if (cast.type == cast.REGULAR)
    {
        casterStats.getSpells().setSelectedSpell(refId(cast.spellId));

        if (cast.success)
            MWBase::Environment::get().getWorld()->castSpell(caster);

        LOG_APPEND(TimedLog::LOG_VERBOSE, "- spellId: %s", cast.spellId.c_str());
    }
    else if (cast.type == cast.ITEM)
    {
        casterStats.getSpells().setSelectedSpell(ESM::RefId());

        MWWorld::InventoryStore& inventoryStore = caster.getClass().getInventoryStore(caster);

        MWWorld::ContainerStoreIterator it = inventoryStore.begin();
        for (; it != inventoryStore.end(); ++it)
        {
            if (it->getCellRef().getRefId() == cast.itemId)
                break;
        }

        // Add the item if it's missing
        if (it == inventoryStore.end())
            it = inventoryStore.MWWorld::ContainerStore::add(refId(cast.itemId), 1, false);

        inventoryStore.setSelectedEnchantItem(it);
        LOG_APPEND(TimedLog::LOG_VERBOSE, "- itemId: %s", cast.itemId.c_str());
        MWBase::Environment::get().getWorld()->castSpell(caster);
        inventoryStore.setSelectedEnchantItem(inventoryStore.end());
    }
}

void MechanicsHelper::createSpellGfx(const MWWorld::Ptr& targetPtr, const std::vector<ESM::ActiveEffect>& mEffects)
{
    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
    for (const ESM::ActiveEffect& effect : mEffects)
    {
        const ESM::MagicEffect* magicEffect = store.get<ESM::MagicEffect>().search(effect.mEffectId);
        if (magicEffect)
            MWMechanics::playEffects(targetPtr, *magicEffect, true);
    }
}

void MechanicsHelper::addActiveSpell(const MWWorld::Ptr& actor, const mwmp::ActiveSpell& activeSpell, bool restored)
{
    ESM::ActiveSpells::ActiveSpellParams params = activeSpell.params;
    params.mSourceSpellId = refId(activeSpell.id);
    params.mFlags = activeSpell.isStackingSpell
        ? static_cast<ESM::ActiveSpells::Flags>(ESM::ActiveSpells::Flag_Temporary | ESM::ActiveSpells::Flag_Stackable)
        : ESM::ActiveSpells::Flag_Temporary;
    if (restored)
        params.mFlags = static_cast<ESM::ActiveSpells::Flags>(params.mFlags | ESM::ActiveSpells::Flag_Restored);
    params.mWorsenings = -1;

    MWWorld::Ptr caster = getTargetPtr(activeSpell.caster);
    if (!caster.isEmpty())
        params.mCaster = caster.getCellRef().getRefNum();

    // Don't check whether the spell exists: effects from potions come here too
    actor.getClass().getCreatureStats(actor).getActiveSpells().addSpell(params);
}

void MechanicsHelper::removeActiveSpell(const MWWorld::Ptr& actor, const mwmp::ActiveSpell& activeSpell)
{
    MWMechanics::ActiveSpells& activeSpells = actor.getClass().getCreatureStats(actor).getActiveSpells();
    const ESM::RefId id = refId(activeSpell.id);

    if (!activeSpell.isStackingSpell)
    {
        activeSpells.removeEffectsBySourceSpellId(actor, id);
        return;
    }

    // TES3MP told stacked copies apart by when they were cast, which OpenMW 0.51 doesn't keep; they arrive in the
    // order they were cast on every client, so remove the oldest copy
    for (const auto& params : activeSpells)
    {
        if (params.getSourceSpellId() == id)
        {
            activeSpells.removeEffectsByActiveSpellId(actor, params.getActiveSpellId());
            return;
        }
    }

    LOG_APPEND(TimedLog::LOG_INFO, "-- spell %s is not active", activeSpell.id.c_str());
}

bool MechanicsHelper::isStackingSpell(const std::string& id)
{
    return !MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(refId(id));
}

bool MechanicsHelper::doesEffectListContainEffect(const ESM::EffectList& effectList, const ESM::RefId& effectId,
    const ESM::RefId& attributeId, const ESM::RefId& skillId)
{
    for (const auto &effect : effectList.mList)
    {
        if (effect.mData.mEffectID == effectId)
        {
            if (attributeId.empty() || effect.mData.mAttribute == attributeId)
            {
                if (skillId.empty() || effect.mData.mSkill == skillId)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void MechanicsHelper::unequipItemsByEffect(const MWWorld::Ptr& ptr, int enchantmentType, const ESM::RefId& effectId,
    const ESM::RefId& attributeId, const ESM::RefId& skillId)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::InventoryStore &ptrInventory = ptr.getClass().getInventoryStore(ptr);

    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
    {
        if (ptrInventory.getSlot(slot) != ptrInventory.end())
        {
            MWWorld::ConstContainerStoreIterator itemIterator = ptrInventory.getSlot(slot);
            ESM::RefId enchantmentId = itemIterator->getClass().getEnchantment(*itemIterator);

            if (!enchantmentId.empty())
            {
                const ESM::Enchantment* enchantment = world->getStore().get<ESM::Enchantment>().search(enchantmentId);

                if (enchantment && enchantment->mData.mType == enchantmentType
                    && doesEffectListContainEffect(enchantment->mEffects, effectId, attributeId, skillId))
                    ptrInventory.unequipSlot(slot);
            }
        }
    }
}

void MechanicsHelper::setGoldPool(const MWWorld::Ptr& merchant, int goldPool)
{
    MWMechanics::CreatureStats& stats = merchant.getClass().getCreatureStats(merchant);
    if (!mwmp::Main::isInitialized())
    {
        stats.setGoldPool(goldPool);
        return;
    }

    mwmp::ObjectList* objectList = mwmp::Main::get().getNetworking()->getObjectList();
    objectList->reset();
    objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;
    objectList->addObjectMiscellaneous(merchant, std::max(goldPool, 0), stats.getLastRestockTime().getHour(),
        stats.getLastRestockTime().getDay());
    objectList->sendObjectMiscellaneous();
}

void MechanicsHelper::restockGoldPool(const MWWorld::Ptr& merchant, int goldPool)
{
    MWMechanics::CreatureStats& stats = merchant.getClass().getCreatureStats(merchant);
    const MWWorld::TimeStamp now = MWBase::Environment::get().getWorld()->getTimeStamp();
    if (!mwmp::Main::isInitialized())
    {
        stats.setGoldPool(goldPool);
        stats.setLastRestockTime(now);
        return;
    }

    mwmp::ObjectList* objectList = mwmp::Main::get().getNetworking()->getObjectList();
    objectList->reset();
    objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;
    objectList->addObjectMiscellaneous(merchant, std::max(goldPool, 0), now.getHour(), now.getDay());
    objectList->sendObjectMiscellaneous();
}

MWWorld::Ptr MechanicsHelper::getItemPtrFromStore(const mwmp::Item& item, MWWorld::ContainerStore& store)
{
    MWWorld::Ptr closestPtr;

    for (MWWorld::ContainerStoreIterator storeIterator = store.begin(); storeIterator != store.end(); ++storeIterator)
    {
        // Enchantment charges are often in the process of refilling themselves, so don't check for them here
        if (storeIterator->getCellRef().getRefId() == item.refId &&
            item.count == storeIterator->getCellRef().getCount() &&
            item.charge == storeIterator->getCellRef().getCharge() &&
            isSameId(storeIterator->getCellRef().getSoul(), item.soul))
        {
            // If we have no closestPtr, set it to the Ptr corresponding to this storeIterator; otherwise, make
            // sure the storeIterator's enchantmentCharge is closer to our goal than that of the previous closestPtr
            if (closestPtr.isEmpty() || abs(storeIterator->getCellRef().getEnchantmentCharge() - item.enchantmentCharge) <
                abs(closestPtr.getCellRef().getEnchantmentCharge() - item.enchantmentCharge))
            {
                closestPtr = *storeIterator;
            }
        }
    }

    return closestPtr;
}

std::string MechanicsHelper::getVoiceCaption(const std::string& sound)
{
    for (const ESM::Dialogue& dialogue : MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>())
    {
        if (dialogue.mType != ESM::Dialogue::Voice)
            continue;

        for (const ESM::DialInfo& info : dialogue.mInfo)
        {
            if (!info.mSound.empty() && Misc::StringUtils::ciEqual(sound, info.mSound))
                return info.mResponse;
        }
    }

    return "???";
}
