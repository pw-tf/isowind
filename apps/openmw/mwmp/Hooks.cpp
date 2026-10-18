#include "Hooks.hpp"

#include <components/esm/util.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"

#include <components/misc/strings/algorithm.hpp>

#include "ActorList.hpp"
#include "Cell.hpp"
#include "CellController.hpp"
#include "DedicatedActor.hpp"
#include "DedicatedPlayer.hpp"
#include "LocalActor.hpp"

#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadweap.hpp>
#include "MechanicsHelper.hpp"

#include <osg/Quat>

#include <components/interpreter/context.hpp>

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/summoning.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldmodel.hpp"
#include "../mwworld/class.hpp"
#include "Ids.hpp"
#include "LocalPlayer.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "ObjectList.hpp"
#include "PlayerList.hpp"
#include "ScriptController.hpp"
#include "Worldstate.hpp"

namespace mwmp::hooks
{
    namespace
    {
        bool shouldSendProgress()
        {
            return isLoggedIn() && !ServerChange::isActive();
        }

        bool isLocalPlayerStats(const void* stats)
        {
            const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            return !player.isEmpty() && stats == &player.getClass().getNpcStats(player);
        }

        int sServerChanges = 0;

        // The object list to fill, reset, or nullptr when there is nothing to send (single player, or an object
        // outside the world, such as an item in a container)
        ObjectList* startObjectList(const MWWorld::Ptr& ptr)
        {
            if (!isMultiplayer() || ptr.isEmpty() || !ptr.isInCell() || ptr.getCell()->getCell()->isEsm4())
                return nullptr;
            ObjectList* objectList = Main::get().getNetworking()->getObjectList();
            objectList->reset();
            objectList->packetOrigin = CLIENT_GAMEPLAY;
            return objectList;
        }
    }

    bool isMultiplayer()
    {
        return Main::isInitialized();
    }

    bool isLoggedIn()
    {
        return isMultiplayer() && Main::get().getLocalPlayer()->isLoggedIn();
    }

    bool isLocalPlayer(const MWWorld::Ptr& ptr)
    {
        return !ptr.isEmpty() && ptr == MWBase::Environment::get().getWorld()->getPlayerPtr();
    }

    bool isDedicatedPlayer(const MWWorld::Ptr& ptr)
    {
        return isMultiplayer() && !ptr.isEmpty() && PlayerList::isDedicatedPlayer(ptr);
    }

    bool isLocalActor(const MWWorld::Ptr& ptr)
    {
        return isMultiplayer() && !ptr.isEmpty() && Main::get().getCellController()->isLocalActor(ptr);
    }

    bool isDedicatedActor(const MWWorld::Ptr& ptr)
    {
        return isMultiplayer() && !ptr.isEmpty() && Main::get().getCellController()->isDedicatedActor(ptr);
    }

    bool isLocallyControlled(const MWWorld::Ptr& actor)
    {
        return isLocalPlayer(actor) || isLocalActor(actor);
    }

    bool isRemotelyControlled(const MWWorld::Ptr& actor)
    {
        return isDedicatedPlayer(actor) || isDedicatedActor(actor);
    }

    bool requestActivation(const MWWorld::Ptr& object, const MWWorld::Ptr& actor)
    {
        if (!isMultiplayer() || !isLocallyControlled(actor))
            return false;
        // Before logging in there is nothing to ask; the activation then doesn't happen at all
        if (!isLoggedIn())
            return true;
        ObjectList* objectList = startObjectList(object);
        if (!objectList)
            return false;
        objectList->addObjectActivate(object, actor);
        objectList->sendObjectActivate();
        return true;
    }

    void sendObjectSound(const MWWorld::Ptr& ptr, const ESM::RefId& soundId, float volume, float pitch)
    {
        if (soundId.empty())
            return;
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectSound(ptr, idString(soundId), volume, pitch);
            objectList->sendObjectSound();
        }
    }

    void sendObjectDelete(const MWWorld::Ptr& ptr)
    {
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectGeneric(ptr);
            objectList->sendObjectDelete();
        }
    }

    void sendObjectLock(const MWWorld::Ptr& ptr, int lockLevel)
    {
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectLock(ptr, lockLevel);
            objectList->sendObjectLock();
        }
    }

    void sendObjectTrap(const MWWorld::Ptr& ptr, const ESM::Position& pos, bool isDisarmed)
    {
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectTrap(ptr, pos, isDisarmed);
            objectList->sendObjectTrap();
        }
    }

    void sendObjectState(const MWWorld::Ptr& ptr, bool enabled)
    {
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectState(ptr, enabled);
            objectList->sendObjectState();
        }
    }

    void sendObjectScale(const MWWorld::Ptr& ptr, float scale)
    {
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectScale(ptr, scale);
            objectList->sendObjectScale();
        }
    }

    void sendObjectAnimPlay(const MWWorld::Ptr& ptr, const std::string& group, int mode)
    {
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectAnimPlay(ptr, group, mode);
            objectList->sendObjectAnimPlay();
        }
    }

    void sendDoorState(const MWWorld::Ptr& door, MWWorld::DoorState state)
    {
        if (ObjectList* objectList = startObjectList(door))
        {
            objectList->addDoorState(door, state);
            objectList->sendDoorState();
        }
    }

    void sendObjectPlace(const MWWorld::Ptr& ptr, bool droppedByPlayer)
    {
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectPlace(ptr, droppedByPlayer);
            objectList->sendObjectPlace();
        }
    }

    bool requestItemUse(const MWWorld::Ptr& item, bool usingItemMagic, int drawState)
    {
        if (!isMultiplayer() || item.isEmpty())
            return false;
        if (isLoggedIn())
            Main::get().getLocalPlayer()->sendItemUse(item, usingItemMagic, static_cast<char>(drawState));
        return true;
    }

    void sendSelectedSpell(const ESM::RefId& spellId)
    {
        if (isLoggedIn())
            Main::get().getLocalPlayer()->sendSelectedSpell(spellId);
    }

    void sendContainerChange(const MWWorld::Ptr& container, int action, int subAction,
        const std::vector<std::pair<MWWorld::Ptr, int>>& items)
    {
        if (items.empty())
            return;
        ObjectList* objectList = startObjectList(container);
        if (!objectList)
            return;
        objectList->cell = container.getCell()->getCell()->getEsm3();
        objectList->action = static_cast<unsigned char>(action);
        objectList->containerSubAction = static_cast<unsigned char>(subAction);
        BaseObject baseObject = objectList->getBaseObjectFromPtr(container);
        for (const auto& [item, count] : items)
        {
            // An item put in: count is how many; an item taken out: how many there are, and how many are taken
            if (action == BaseObjectList::ADD)
                objectList->addContainerItem(baseObject, item, count, 0);
            else
                objectList->addContainerItem(baseObject, item, item.getCellRef().getCount(), count);
        }
        objectList->addBaseObject(baseObject);
        objectList->sendContainer();
    }

    bool isCurrentContainer(const MWWorld::Ptr& container)
    {
        if (!isMultiplayer() || container.isEmpty())
            return false;
        const CurrentContainer& current = Main::get().getLocalPlayer()->currentContainer;
        return current.refNum == container.getCellRef().getMpRefNum()
            && current.mpNum == container.getCellRef().getMpNum();
    }

    bool denyRemoteCellChange(const MWWorld::Ptr& ptr, const MWWorld::CellStore& newCell)
    {
        if (!isMultiplayer() || newCell.getCell()->isEsm4())
            return false;
        CellController* cellController = Main::get().getCellController();
        const ESM::Cell& cell = newCell.getCell()->getEsm3();
        if (DedicatedPlayer* player = PlayerList::getPlayer(ptr))
            return !cellController->isSameCell(player->cell, cell);
        if (cellController->isDedicatedActor(ptr))
            return !cellController->isSameCell(cellController->getDedicatedActor(ptr)->cell, cell);
        return false;
    }

    void updatePtr(const MWWorld::Ptr& oldPtr, const MWWorld::Ptr& newPtr)
    {
        if (!isMultiplayer() || oldPtr == newPtr)
            return;
        CellController* cellController = Main::get().getCellController();
        if (cellController->isLocalActor(oldPtr))
            cellController->getLocalActor(oldPtr)->setPtr(newPtr);
        else if (cellController->isDedicatedActor(oldPtr))
            cellController->getDedicatedActor(oldPtr)->setPtr(newPtr);
        else if (DedicatedPlayer* player = PlayerList::getPlayer(oldPtr))
            player->setPtr(newPtr);
    }

    ServerChange::ServerChange()
    {
        ++sServerChanges;
    }

    ServerChange::~ServerChange()
    {
        --sServerChanges;
    }

    bool ServerChange::isActive()
    {
        return sServerChanges > 0;
    }

    mwmp::Attack* getLocalAttack(const MWWorld::Ptr& actor)
    {
        return isMultiplayer() && !actor.isEmpty() ? MechanicsHelper::getLocalAttack(actor) : nullptr;
    }

    mwmp::Attack* getDedicatedAttack(const MWWorld::Ptr& actor)
    {
        return isMultiplayer() && !actor.isEmpty() ? MechanicsHelper::getDedicatedAttack(actor) : nullptr;
    }

    mwmp::Cast* getLocalCast(const MWWorld::Ptr& actor)
    {
        return isMultiplayer() && !actor.isEmpty() ? MechanicsHelper::getLocalCast(actor) : nullptr;
    }

    mwmp::Cast* getDedicatedCast(const MWWorld::Ptr& actor)
    {
        return isMultiplayer() && !actor.isEmpty() ? MechanicsHelper::getDedicatedCast(actor) : nullptr;
    }

    void updateLocalAttackState(const MWWorld::Ptr& actor)
    {
        mwmp::Attack* attack = getLocalAttack(actor);
        if (!attack)
            return;
        const MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        if (stats.getDrawState() != MWMechanics::DrawState::Weapon)
            return;
        const bool pressed = stats.getAttackingOrSpell();
        if (attack->pressed == pressed)
            return;

        MechanicsHelper::resetAttack(attack);
        attack->type = MechanicsHelper::isUsingRangedWeapon(actor) ? mwmp::Attack::RANGED : mwmp::Attack::MELEE;
        attack->pressed = pressed;
        // A ranged attack being released is sent when the projectile leaves (onProjectileReleased), with the final
        // attack strength that decides its speed
        if (attack->type == mwmp::Attack::MELEE || pressed)
            attack->shouldSend = true;
    }

    void recordAttackAnimation(const MWWorld::Ptr& actor, const std::string& attackType)
    {
        if (mwmp::Attack* attack = getLocalAttack(actor))
            attack->attackAnimation = attackType;
    }

    void prepareLocalHit(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, const osg::Vec3f& hitPosition,
        bool success, bool appliedEnchantment)
    {
        mwmp::Attack* attack = getLocalAttack(attacker);
        if (!attack)
            return;
        attack->type = mwmp::Attack::MELEE;
        attack->isHit = true;
        attack->success = success;
        attack->applyWeaponEnchantment = appliedEnchantment;
        attack->hitPosition = MechanicsHelper::getPositionFromVector(hitPosition);
        MechanicsHelper::assignAttackTarget(attack, victim);
    }

    void finishLocalMiss(const MWWorld::Ptr& attacker)
    {
        mwmp::Attack* attack = getLocalAttack(attacker);
        if (!attack)
            return;
        attack->pressed = false;
        attack->isHit = false;
        attack->success = false;
        attack->shouldSend = true;
    }

    void finishLocalHit(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, float damage, bool success)
    {
        mwmp::Attack* attack = getLocalAttack(attacker);
        if (!attack || !attack->isHit)
            return;
        attack->pressed = false;
        attack->success = success;
        attack->damage = success ? damage : 0.f;
        attack->knockdown = success && victim.getClass().getCreatureStats(victim).getKnockedDown();
        MechanicsHelper::assignAttackTarget(attack, victim);
        attack->shouldSend = true;

        // The server hears of the hit too, for its scripts
        if (ObjectList* objectList = startObjectList(victim))
        {
            objectList->addObjectHit(victim, attacker, *attack);
            objectList->sendObjectHit();
        }

        if (isLocalPlayer(victim))
            Main::get().getLocalPlayer()->updateStatsDynamic(true);
    }

    bool decideKnockdown(const MWWorld::Ptr& attacker, bool localDecision)
    {
        if (mwmp::Attack* attack = getDedicatedAttack(attacker))
            return attack->knockdown;
        return localDecision;
    }

    bool decideBlock(const MWWorld::Ptr& attacker, bool localDecision)
    {
        if (mwmp::Attack* attack = getDedicatedAttack(attacker))
            return attack->block;
        if (mwmp::Attack* attack = getLocalAttack(attacker))
            attack->block = localDecision;
        return localDecision;
    }

    void recordKiller(const MWWorld::Ptr& victim, const MWWorld::Ptr& killer)
    {
        if (!isMultiplayer() || victim.isEmpty())
            return;
        const mwmp::Target target = killer.isEmpty() ? mwmp::Target() : MechanicsHelper::getTarget(killer);
        if (isLocalPlayer(victim))
            Main::get().getLocalPlayer()->killer = target;
        else if (isLocalActor(victim))
            Main::get().getCellController()->getLocalActor(victim)->killer = target;
    }

    void onProjectileReleased(const MWWorld::Ptr& actor, const MWWorld::Ptr& weapon, const MWWorld::Ptr& ammo,
        float& attackStrength, osg::Vec3f& launchPos, osg::Quat& orient)
    {
        if (mwmp::Attack* attack = getLocalAttack(actor))
        {
            attack->type = mwmp::Attack::RANGED;
            attack->pressed = false;
            attack->attackStrength = attackStrength;
            attack->rangedWeaponId = weapon.isEmpty() ? std::string() : idString(weapon.getCellRef().getRefId());
            attack->rangedAmmoId = ammo.isEmpty() || ammo == weapon ? std::string() : idString(ammo.getCellRef().getRefId());
            attack->projectileOrigin.origin[0] = launchPos.x();
            attack->projectileOrigin.origin[1] = launchPos.y();
            attack->projectileOrigin.origin[2] = launchPos.z();
            attack->projectileOrigin.orientation[0] = static_cast<float>(orient.x());
            attack->projectileOrigin.orientation[1] = static_cast<float>(orient.y());
            attack->projectileOrigin.orientation[2] = static_cast<float>(orient.z());
            attack->projectileOrigin.orientation[3] = static_cast<float>(orient.w());
            attack->shouldSend = true;
        }
        else if (mwmp::Attack* attack = getDedicatedAttack(actor))
        {
            attackStrength = attack->attackStrength;
            const auto& origin = attack->projectileOrigin;
            launchPos = osg::Vec3f(origin.origin[0], origin.origin[1], origin.origin[2]);
            orient = osg::Quat(origin.orientation[0], origin.orientation[1], origin.orientation[2], origin.orientation[3]);
        }
    }

    bool onProjectileHit(const MWWorld::Ptr& attacker, bool hasVictim, const osg::Vec3f& hitPosition,
        bool success, bool appliedEnchantment)
    {
        if (isRemotelyControlled(attacker))
            return false;
        mwmp::Attack* attack = getLocalAttack(attacker);
        if (!attack)
            return true;
        attack->type = mwmp::Attack::RANGED;
        attack->isHit = true;
        attack->success = success;
        attack->applyAmmoEnchantment = appliedEnchantment;
        attack->hitPosition = MechanicsHelper::getPositionFromVector(hitPosition);
        // Without a victim no damage comes to say how it went: send it now
        if (!hasVictim)
            attack->shouldSend = true;
        return true;
    }

    void onDeathAnimation(const MWWorld::Ptr& actor, int deathState, float& startpoint)
    {
        if (!isMultiplayer())
            return;
        const bool finished = actor.getClass().getCreatureStats(actor).isDeathAnimationFinished();
        if (finished && (isLocalActor(actor) || isDedicatedActor(actor)))
            startpoint = 1.f;
        if (isLocalPlayer(actor))
            Main::get().getLocalPlayer()->sendDeath(static_cast<char>(deathState));
        else if (!finished && isLocalActor(actor))
            Main::get().getCellController()->getLocalActor(actor)->sendDeath(static_cast<char>(deathState));
    }

    void onDeathAnimationFinished(const MWWorld::Ptr& actor)
    {
        if (isLocalActor(actor))
            Main::get().getCellController()->getLocalActor(actor)->creatureStats.mDeathAnimationFinished = true;
    }

    void recordAnimationPlay(const MWWorld::Ptr& actor, std::string_view group, int mode, int count, bool persist)
    {
        if (!isLocalActor(actor))
            return;
        LocalActor* localActor = Main::get().getCellController()->getLocalActor(actor);
        localActor->animation.groupname = std::string(group);
        localActor->animation.mode = mode;
        localActor->animation.count = count;
        localActor->animation.persist = persist;
    }

    void sendFactionExpulsion(const ESM::RefId& factionId, bool expelled)
    {
        if (isLoggedIn())
            Main::get().getLocalPlayer()->sendFactionExpulsionState(factionId, expelled);
    }

    void onArrestAttempt()
    {
        if (!isMultiplayer())
            return;
        LocalPlayer* localPlayer = Main::get().getLocalPlayer();
        localPlayer->diedSinceArrestAttempt = false;
        localPlayer->closeInventoryWindows();
    }

    bool hasDiedSinceArrestAttempt()
    {
        return isMultiplayer() && Main::get().getLocalPlayer()->diedSinceArrestAttempt;
    }

    bool isAlliedPlayer(const MWWorld::Ptr& actor, const MWWorld::Ptr& other)
    {
        if (!isMultiplayer() || actor.isEmpty() || other.isEmpty() || actor == other)
            return false;
        const auto contains = [](const std::vector<RakNet::RakNetGUID>& guids, const RakNet::RakNetGUID& guid) {
            return std::find(guids.begin(), guids.end(), guid) != guids.end();
        };
        LocalPlayer* localPlayer = Main::get().getLocalPlayer();
        if (isLocalPlayer(actor))
        {
            DedicatedPlayer* otherPlayer = PlayerList::getPlayer(other);
            return otherPlayer && contains(localPlayer->alliedPlayers, otherPlayer->guid);
        }
        DedicatedPlayer* player = PlayerList::getPlayer(actor);
        if (!player)
            return false;
        if (isLocalPlayer(other))
            return contains(player->alliedPlayers, localPlayer->guid);
        DedicatedPlayer* otherPlayer = PlayerList::getPlayer(other);
        return otherPlayer && contains(player->alliedPlayers, otherPlayer->guid);
    }

    namespace
    {
        bool sPvp = false;
        bool sMmo = false;

        bool isPlayer(const MWWorld::Ptr& ptr)
        {
            return isLocalPlayer(ptr) || isDedicatedPlayer(ptr);
        }
    }

    bool applyServerRule(const std::string& name, const std::string& value)
    {
        if (name == "isowind pvp")
            sPvp = value == "true";
        else if (name == "isowind mode")
            sMmo = value == "mmo";
        else
            return false;
        return true;
    }

    bool isPvpBlocked(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim)
    {
        return isMultiplayer() && !sPvp && !attacker.isEmpty() && !victim.isEmpty() && attacker != victim
            && isPlayer(attacker) && isPlayer(victim);
    }

    bool isMmoMode()
    {
        return isMultiplayer() && sMmo;
    }

    void sendSummonRemoval(const MWWorld::Ptr& creature)
    {
        if (!isLoggedIn() || creature.isEmpty())
            return;
        // The local player's own summons, or those in cells this client has authority over
        bool isOurs = isLocalActor(creature);
        if (!isOurs)
        {
            const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            for (const auto& [_, refNum] : player.getClass().getCreatureStats(player).getSummonedCreatureMap())
                isOurs = isOurs || refNum == creature.getCellRef().getRefNum();
        }
        if (isOurs)
            sendObjectDelete(creature);
    }

    void onLocalCastStart(const MWWorld::Ptr& caster, const ESM::RefId& spellId, bool isMagicItem)
    {
        mwmp::Cast* cast = getLocalCast(caster);
        if (!cast || spellId.empty())
            return;
        MechanicsHelper::resetCast(cast);
        cast->type = isMagicItem ? mwmp::Cast::ITEM : mwmp::Cast::REGULAR;
        cast->spellId = isMagicItem ? std::string() : idString(spellId);
        cast->pressed = true;
        cast->shouldSend = true;
    }

    void onLocalItemCast(const MWWorld::Ptr& caster, const MWWorld::Ptr& item)
    {
        mwmp::Cast* cast = getLocalCast(caster);
        if (!cast || item.isEmpty())
            return;
        MechanicsHelper::resetCast(cast);
        cast->type = mwmp::Cast::ITEM;
        cast->itemId = idString(item.getCellRef().getRefId());
        cast->success = true;
        cast->shouldSend = true;
    }

    bool decideSpellSuccess(const MWWorld::Ptr& caster, bool localRoll)
    {
        if (mwmp::Cast* cast = getDedicatedCast(caster))
        {
            cast->pressed = false;
            return cast->success;
        }
        if (mwmp::Cast* cast = getLocalCast(caster))
        {
            cast->success = localRoll;
            cast->pressed = false;
            cast->shouldSend = true;
        }
        return localRoll;
    }

    bool isCastPaidHere(const MWWorld::Ptr& caster)
    {
        return !isRemotelyControlled(caster);
    }

    void onMagicBoltLaunched(const MWWorld::Ptr& caster, osg::Vec3f& pos, osg::Quat& orient)
    {
        if (mwmp::Cast* cast = getLocalCast(caster))
        {
            cast->hasProjectile = true;
            cast->projectileOrigin.origin[0] = pos.x();
            cast->projectileOrigin.origin[1] = pos.y();
            cast->projectileOrigin.origin[2] = pos.z();
            cast->projectileOrigin.orientation[0] = static_cast<float>(orient.x());
            cast->projectileOrigin.orientation[1] = static_cast<float>(orient.y());
            cast->projectileOrigin.orientation[2] = static_cast<float>(orient.z());
            cast->projectileOrigin.orientation[3] = static_cast<float>(orient.w());
        }
        else if (mwmp::Cast* cast = getDedicatedCast(caster))
        {
            if (!cast->hasProjectile)
                return;
            const auto& origin = cast->projectileOrigin;
            pos = osg::Vec3f(origin.origin[0], origin.origin[1], origin.origin[2]);
            orient = osg::Quat(origin.orientation[0], origin.orientation[1], origin.orientation[2], origin.orientation[3]);
        }
    }

    bool requestSpellLock(const MWWorld::Ptr& caster, const MWWorld::Ptr& target, int lockLevel)
    {
        if (!isMultiplayer())
            return false;
        // Every client carries out a remote caster's spell too: only the caster's own client asks
        if (!isRemotelyControlled(caster))
            sendObjectLock(target, lockLevel);
        return true;
    }

    void sendMarkLocation(const MWWorld::Ptr& player)
    {
        if (isLoggedIn() && isLocalPlayer(player) && player.isInCell() && !player.getCell()->getCell()->isEsm4())
            Main::get().getLocalPlayer()->sendMarkLocation(
                player.getCell()->getCell()->getEsm3(), player.getRefData().getPosition());
    }

    ESM::RefNum requestSummon(const ESM::RefId& effectId, const MWWorld::Ptr& summoner,
        const ESM::RefId& spellId, float duration)
    {
        if (!isMultiplayer())
            return MWMechanics::summonCreature(effectId, summoner);
        // A remote summoner's creature comes from the server, asked for by its own client
        if (isRemotelyControlled(summoner))
            return ESM::RefNum();

        // Placed here only to find a spot for it, then handed to the server to spawn for everyone
        const ESM::RefNum placedNum = MWMechanics::summonCreature(effectId, summoner);
        auto& creatureMap = summoner.getClass().getCreatureStats(summoner).getSummonedCreatureMap();
        for (auto it = creatureMap.begin(); it != creatureMap.end(); ++it)
        {
            if (it->first == effectId && it->second == placedNum)
            {
                it->second = ESM::RefNum();
                break;
            }
        }
        const MWWorld::Ptr placed = placedNum.isSet()
            ? MWBase::Environment::get().getWorldModel()->getPtr(placedNum)
            : MWWorld::Ptr();
        if (placed.isEmpty())
            return ESM::RefNum();
        if (ObjectList* objectList = startObjectList(placed))
        {
            objectList->addObjectSpawn(placed, summoner, idString(spellId),
                ESM::MagicEffect::refIdToIndex(effectId), duration);
            objectList->sendObjectSpawn();
        }
        MWBase::Environment::get().getWorld()->deleteObject(placed);
        return ESM::RefNum();
    }

    void linkSummon(const MWWorld::Ptr& summoner, const ESM::RefId& effectId, ESM::RefNum creature)
    {
        MWMechanics::CreatureStats& stats = summoner.getClass().getCreatureStats(summoner);
        auto& creatureMap = stats.getSummonedCreatureMap();
        bool linked = false;
        for (auto it = creatureMap.begin(); it != creatureMap.end(); ++it)
        {
            if (it->first == effectId && !it->second.isSet())
            {
                it->second = creature;
                linked = true;
                break;
            }
        }
        if (!linked)
            creatureMap.emplace(effectId, creature);
        stats.getActiveSpells().linkSummon(effectId, creature);
    }

    void onActiveSpellAdded(const MWWorld::Ptr& actor, const MWMechanics::ActiveSpells::ActiveSpellParams& params)
    {
        if (!isLoggedIn() || params.isFromServer() || !params.hasFlag(ESM::ActiveSpells::Flag_Temporary))
            return;
        if (isLocalPlayer(actor))
            Main::get().getLocalPlayer()->sendSpellsActiveAddition(params);
        else if (isLocalActor(actor))
            Main::get().getCellController()->getLocalActor(actor)->sendSpellsActiveAddition(params);
    }

    void onActiveSpellExpired(const MWWorld::Ptr& actor, const MWMechanics::ActiveSpells::ActiveSpellParams& params)
    {
        if (!isLoggedIn() || !params.hasFlag(ESM::ActiveSpells::Flag_Temporary))
            return;
        const bool stacking = params.hasFlag(ESM::ActiveSpells::Flag_Stackable);
        if (isLocalPlayer(actor))
            Main::get().getLocalPlayer()->sendSpellsActiveRemoval(params.getSourceSpellId(), stacking);
        else if (isLocalActor(actor))
            Main::get().getCellController()->getLocalActor(actor)->sendSpellsActiveRemoval(
                params.getSourceSpellId(), stacking);
    }

    void sendSpellbookChange(const MWWorld::Ptr& actor, const ESM::RefId& spellId, bool added)
    {
        if (isLoggedIn() && isLocalPlayer(actor))
            Main::get().getLocalPlayer()->sendSpellChange(
                spellId, added ? SpellbookChanges::ADD : SpellbookChanges::REMOVE);
    }

    void sendPowerUse(const MWWorld::Ptr& actor, const ESM::RefId& spellId)
    {
        if (!isLoggedIn() || !isLocalPlayer(actor))
            return;
        const MWWorld::TimeStamp now = MWBase::Environment::get().getWorld()->getTimeStamp();
        Main::get().getLocalPlayer()->sendCooldownChange(spellId, now.getDay(), now.getHour());
    }

    void onPlayerItemAdded(const MWWorld::Ptr& container, const MWWorld::Ptr& item, int count)
    {
        if (!isLoggedIn() || !isLocalPlayer(container) || count <= 0)
            return;
        LocalPlayer* localPlayer = Main::get().getLocalPlayer();
        if (!localPlayer->avoidSendingInventoryPackets)
            localPlayer->sendItemChange(item, count, InventoryChanges::ADD);
    }

    void onPlayerItemRemoved(const MWWorld::Ptr& container, const MWWorld::Ptr& item, int count)
    {
        if (!isLoggedIn() || !isLocalPlayer(container) || count <= 0)
            return;
        LocalPlayer* localPlayer = Main::get().getLocalPlayer();
        if (!localPlayer->avoidSendingInventoryPackets)
            localPlayer->sendItemChange(item, count, InventoryChanges::REMOVE);
    }

    void onJournalEntry(const ESM::RefId& quest, int index, const MWWorld::Ptr& actor)
    {
        if (shouldSendProgress())
            Main::get().getLocalPlayer()->sendJournalEntry(quest, index, actor);
    }

    void onJournalIndex(const ESM::RefId& quest, int index)
    {
        if (shouldSendProgress())
            Main::get().getLocalPlayer()->sendJournalIndex(quest, index);
    }

    void onTopicLearned(const ESM::RefId& topic)
    {
        if (shouldSendProgress())
            Main::get().getLocalPlayer()->sendTopic(topic);
    }

    void onFactionRank(const void* stats, const ESM::RefId& faction, int rank)
    {
        if (shouldSendProgress() && isLocalPlayerStats(stats))
            Main::get().getLocalPlayer()->sendFactionRank(faction, rank);
    }

    void onFactionExpulsion(const void* stats, const ESM::RefId& faction, bool expelled)
    {
        if (shouldSendProgress() && isLocalPlayerStats(stats))
            Main::get().getLocalPlayer()->sendFactionExpulsionState(faction, expelled);
    }

    void onFactionReputation(const void* stats, const ESM::RefId& faction, int reputation)
    {
        if (shouldSendProgress() && isLocalPlayerStats(stats))
            Main::get().getLocalPlayer()->sendFactionReputation(faction, reputation);
    }

    void onSpellbookChange(const void* spells, const ESM::RefId& spellId, bool added)
    {
        if (!shouldSendProgress())
            return;
        const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (!player.isEmpty() && spells == &player.getClass().getCreatureStats(player).getSpells())
            Main::get().getLocalPlayer()->sendSpellChange(
                spellId, added ? SpellbookChanges::ADD : SpellbookChanges::REMOVE);
    }

    bool sendPotionRecord(const ESM::Potion& potion, int count)
    {
        if (!isMultiplayer())
            return false;
        if (count > 0)
            Main::get().getNetworking()->getWorldstate()->sendPotionRecord(&potion, static_cast<unsigned int>(count));
        return true;
    }

    bool sendEnchantmentRecord(const ESM::Enchantment& enchantment, int itemCount)
    {
        if (!isMultiplayer())
            return false;
        Main::get().getNetworking()->getWorldstate()->sendEnchantmentRecord(&enchantment);
        Main::get().getLocalPlayer()->storeLastEnchantmentQuantity(static_cast<unsigned int>(itemCount));
        return true;
    }

    bool sendEnchantedItemRecord(const ESM::Armor* armor, const ESM::Book* book, const ESM::Clothing* clothing,
        const ESM::Weapon* weapon, const ESM::RefId& baseId)
    {
        if (!isMultiplayer())
            return false;
        Worldstate* worldstate = Main::get().getNetworking()->getWorldstate();
        const std::string base = idString(baseId);
        if (armor)
            worldstate->sendArmorRecord(armor, base);
        else if (book)
            worldstate->sendBookRecord(book, base);
        else if (clothing)
            worldstate->sendClothingRecord(clothing, base);
        else if (weapon)
            worldstate->sendWeaponRecord(weapon, base, Main::get().getLocalPlayer()->lastEnchantmentQuantity);
        return true;
    }

    bool sendSpellRecord(const ESM::Spell& spell)
    {
        if (!isMultiplayer())
            return false;
        Main::get().getNetworking()->getWorldstate()->sendSpellRecord(&spell);
        return true;
    }

    void sendPlayerInventory()
    {
        if (isLoggedIn())
            Main::get().getLocalPlayer()->sendInventory();
    }

    void beginBatchedItemRemovals()
    {
        if (isMultiplayer())
            Main::get().getLocalPlayer()->avoidSendingInventoryPackets = true;
    }

    void endBatchedItemRemovals()
    {
        if (!isMultiplayer())
            return;
        // The inventory as it is now, all at once
        LocalPlayer* localPlayer = Main::get().getLocalPlayer();
        localPlayer->avoidSendingInventoryPackets = false;
        if (isLoggedIn())
            localPlayer->sendInventory();
    }

    namespace
    {
        // An object list for a change a script makes, telling the server which script it was; nullptr when there is
        // nothing to send (single player, not logged in, an object without a network identity)
        ObjectList* startScriptObjectList(const Interpreter::Context& context, const MWWorld::Ptr& ptr)
        {
            if (!isLoggedIn() || ptr.isEmpty() || !ptr.isInCell() || ptr.getCell()->getCell()->isEsm4())
                return nullptr;
            if (ptr.getCellRef().getMpRefNum() == 0 && ptr.getCellRef().getMpNum() == 0
                && !PlayerList::isDedicatedPlayer(ptr))
                return nullptr;
            ObjectList* objectList = Main::get().getNetworking()->getObjectList();
            objectList->reset();
            objectList->packetOrigin
                = static_cast<unsigned char>(ScriptController::getPacketOriginFromContextType(context.getContextType()));
            objectList->originClientScript = context.getCurrentScriptName();
            return objectList;
        }

        bool isFromConsoleOrDialogue(const Interpreter::Context& context)
        {
            return context.getContextType() == Interpreter::Context::CONSOLE
                || context.getContextType() == Interpreter::Context::DIALOGUE;
        }
    }

    bool requestScriptObjectState(const Interpreter::Context& context, const MWWorld::Ptr& ptr, bool enabled)
    {
        if (!isMultiplayer())
            return false;
        ObjectList* objectList = startScriptObjectList(context, ptr);
        if (!objectList)
            return false;
        // A local script runs every frame: only ask once (the server kicks clients that keep asking)
        const auto state = enabled ? MWWorld::RefData::StateCommunication::Enabled
                                   : MWWorld::RefData::StateCommunication::Disabled;
        if (isFromConsoleOrDialogue(context) || ptr.getRefData().getLastCommunicatedState() != state)
        {
            ptr.getRefData().setLastCommunicatedState(state);
            objectList->addObjectState(ptr, enabled);
            objectList->sendObjectState();
        }
        return true;
    }

    bool requestScriptLock(const Interpreter::Context& context, const MWWorld::Ptr& ptr, int lockLevel)
    {
        if (!isMultiplayer())
            return false;
        ObjectList* objectList = startScriptObjectList(context, ptr);
        if (!objectList)
            return false;
        if (ptr.getCellRef().getLockLevel() != lockLevel)
        {
            objectList->addObjectLock(ptr, lockLevel);
            objectList->sendObjectLock();
        }
        return true;
    }

    bool requestScriptDelete(const Interpreter::Context& context, const MWWorld::Ptr& ptr)
    {
        if (!isMultiplayer())
            return false;
        ObjectList* objectList = startScriptObjectList(context, ptr);
        if (!objectList)
            return false;
        if (ptr.getRefData().getLastCommunicatedState() != MWWorld::RefData::StateCommunication::Deleted)
        {
            ptr.getRefData().setLastCommunicatedState(MWWorld::RefData::StateCommunication::Deleted);
            objectList->addObjectGeneric(ptr);
            objectList->sendObjectDelete();
        }
        return true;
    }

    void sendScriptVideo(const Interpreter::Context& context, const std::string& name, bool allowSkipping)
    {
        if (!isLoggedIn())
            return;
        ObjectList* objectList = Main::get().getNetworking()->getObjectList();
        objectList->reset();
        objectList->packetOrigin
            = static_cast<unsigned char>(ScriptController::getPacketOriginFromContextType(context.getContextType()));
        objectList->originClientScript = context.getCurrentScriptName();
        objectList->addVideoPlay(name, allowSkipping);
        objectList->sendVideoPlay();
    }

    void onScriptLocalChanged(const Interpreter::Context& context, bool sendPackets, const MWWorld::Ptr& ptr,
        int index, int value, bool isLong)
    {
        if (!sendPackets || ServerChange::isActive())
            return;
        if (ObjectList* objectList = startScriptObjectList(context, ptr))
        {
            objectList->addClientScriptLocal(ptr, index, value, isLong ? VARIABLE_TYPE::LONG : VARIABLE_TYPE::SHORT);
            objectList->sendClientScriptLocal();
        }
    }

    void onScriptLocalFloatChanged(
        const Interpreter::Context& context, bool sendPackets, const MWWorld::Ptr& ptr, int index, float value)
    {
        if (!sendPackets || ServerChange::isActive())
            return;
        if (ObjectList* objectList = startScriptObjectList(context, ptr))
        {
            objectList->addClientScriptLocal(ptr, index, value);
            objectList->sendClientScriptLocal();
        }
    }

    void onScriptGlobalChanged(bool sendPackets, std::string_view name, int value, bool isLong)
    {
        if (!isLoggedIn() || ServerChange::isActive())
            return;
        const std::string id(name);
        if (sendPackets || Main::isValidPacketGlobal(id))
            Main::get().getNetworking()->getWorldstate()->sendClientGlobal(
                id, value, isLong ? VARIABLE_TYPE::LONG : VARIABLE_TYPE::SHORT);
    }

    void onScriptGlobalFloatChanged(bool sendPackets, std::string_view name, float value)
    {
        if (!isLoggedIn() || ServerChange::isActive())
            return;
        const std::string id(name);
        if (sendPackets || Main::isValidPacketGlobal(id))
            Main::get().getNetworking()->getWorldstate()->sendClientGlobal(id, value);
    }

    void onScriptMemberShortChanged(
        const Interpreter::Context& context, bool sendPackets, const ESM::RefId& scriptId, int index, int value)
    {
        if (!sendPackets || !isLoggedIn() || ServerChange::isActive())
            return;
        ObjectList* objectList = Main::get().getNetworking()->getObjectList();
        objectList->reset();
        objectList->packetOrigin
            = static_cast<unsigned char>(ScriptController::getPacketOriginFromContextType(context.getContextType()));
        objectList->originClientScript = context.getCurrentScriptName();
        objectList->addScriptMemberShort(idString(scriptId), index, value);
        objectList->sendScriptMemberShort();
    }

    bool requestScriptScale(const Interpreter::Context& context, const MWWorld::Ptr& ptr, float scale)
    {
        if (!isMultiplayer())
            return false;
        if (isLocalPlayer(ptr) || isDedicatedPlayer(ptr))
        {
            MWBase::Environment::get().getWindowManager()->messageBox(
                "Only the server can change the scale of players in multiplayer.");
            return true;
        }
        ObjectList* objectList = startScriptObjectList(context, ptr);
        if (!objectList)
            return false;
        if (ptr.getCellRef().getScale() != scale)
        {
            objectList->addObjectScale(ptr, scale);
            objectList->sendObjectScale();
        }
        return true;
    }

    bool requestScriptPlacement(const Interpreter::Context& context, const MWWorld::Ptr& placed)
    {
        if (!isLoggedIn() || placed.isEmpty() || !placed.isInCell() || placed.getCell()->getCell()->isEsm4())
            return false;
        ObjectList* objectList = Main::get().getNetworking()->getObjectList();
        objectList->reset();
        objectList->packetOrigin
            = static_cast<unsigned char>(ScriptController::getPacketOriginFromContextType(context.getContextType()));
        objectList->originClientScript = context.getCurrentScriptName();
        if (placed.getClass().isActor())
        {
            objectList->addObjectSpawn(placed);
            objectList->sendObjectSpawn();
        }
        else
        {
            objectList->addObjectPlace(placed);
            objectList->sendObjectPlace();
        }
        MWBase::Environment::get().getWorld()->deleteObject(placed);
        return true;
    }

    void sendActorCellChange(const MWWorld::Ptr& actor, const ESM::Cell& originalCell, bool isFollower)
    {
        if (!isLoggedIn() || !actor.getClass().isActor() || isLocalPlayer(actor) || isDedicatedPlayer(actor)
            || !actor.isInCell() || actor.getCell()->getCell()->isEsm4())
            return;
        const ESM::Cell& newCell = actor.getCell()->getCell()->getEsm3();
        CellController* cellController = Main::get().getCellController();
        if (cellController->isSameCell(originalCell, newCell))
            return;

        BaseActor baseActor;
        baseActor.refNum = actor.getCellRef().getMpRefNum();
        baseActor.mpNum = actor.getCellRef().getMpNum();
        baseActor.cell = newCell;
        baseActor.position = actor.getRefData().getPosition();
        baseActor.isFollowerCellChange = true;

        ActorList* actorList = Main::get().getNetworking()->getActorList();
        actorList->reset();
        actorList->cell = originalCell;
        actorList->addCellChangeActor(baseActor);
        actorList->sendCellChangeActors();

        // A follower stays the player's follower, whoever has authority over the cell it arrives in
        if (isFollower)
        {
            actorList->reset();
            actorList->cell = newCell;
            actorList->addAiActor(actor, MWBase::Environment::get().getWorld()->getPlayerPtr(), BaseActorList::FOLLOW);
            actorList->sendAiActors();
        }
    }

    void allowRemoteCellChange(const MWWorld::Ptr& actor, const MWWorld::CellStore& newCell)
    {
        if (!isDedicatedActor(actor) || newCell.getCell()->isEsm4())
            return;
        Main::get().getCellController()->getDedicatedActor(actor)->cell = newCell.getCell()->getEsm3();
    }

    bool sendScriptContainerChange(
        const Interpreter::Context& context, const MWWorld::Ptr& ptr, const ESM::RefId& itemId, int count, bool added)
    {
        if (!isLoggedIn() || isLocalPlayer(ptr) || isDedicatedPlayer(ptr) || !ptr.isInCell()
            || ptr.getCell()->getCell()->isEsm4() || count <= 0)
            return false;
        ObjectList* objectList = Main::get().getNetworking()->getObjectList();
        objectList->reset();
        objectList->packetOrigin
            = static_cast<unsigned char>(ScriptController::getPacketOriginFromContextType(context.getContextType()));
        objectList->originClientScript = context.getCurrentScriptName();
        objectList->cell = ptr.getCell()->getCell()->getEsm3();
        objectList->action = added ? BaseObjectList::ADD : BaseObjectList::REMOVE;
        objectList->containerSubAction = BaseObjectList::NONE;
        BaseObject baseObject = objectList->getBaseObjectFromPtr(ptr);
        if (added)
            objectList->addContainerItem(baseObject, idString(itemId), count, 0);
        else
            objectList->addContainerItem(baseObject, idString(itemId),
                ptr.getClass().getContainerStore(ptr).count(itemId), count);
        objectList->addBaseObject(baseObject);
        objectList->sendContainer();
        // Scripts and dialogue change it here too (some mods would loop otherwise); console commands only through
        // the server
        return context.getContextType() == Interpreter::Context::CONSOLE;
    }

    void sendScriptAi(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, int aiAction)
    {
        if (!isLoggedIn() || actor.isEmpty() || target.isEmpty() || !actor.isInCell()
            || actor.getCell()->getCell()->isEsm4())
            return;
        ActorList* actorList = Main::get().getNetworking()->getActorList();
        actorList->reset();
        actorList->cell = actor.getCell()->getCell()->getEsm3();
        actorList->addAiActor(actor, target, static_cast<unsigned int>(aiAction));
        actorList->sendAiActors();
    }

    void sendScriptAnimation(const Interpreter::Context& context, const MWWorld::Ptr& ptr, const std::string& group,
        int mode)
    {
        if (ptr.isEmpty() || !isScriptSynchronized(ptr.getClass().getScript(ptr)))
            return;
        if (ObjectList* objectList = startScriptObjectList(context, ptr))
        {
            objectList->addObjectAnimPlay(ptr, group, mode);
            objectList->sendObjectAnimPlay();
        }
    }

    void sendScriptMusic(const Interpreter::Context& context, const std::string& music)
    {
        if (!isLoggedIn() || ServerChange::isActive())
            return;
        ObjectList* objectList = Main::get().getNetworking()->getObjectList();
        objectList->reset();
        objectList->packetOrigin
            = static_cast<unsigned char>(ScriptController::getPacketOriginFromContextType(context.getContextType()));
        objectList->originClientScript = context.getCurrentScriptName();
        objectList->addMusicPlay(music);
        objectList->sendMusicPlay();
    }

    void setNoClip(bool noClip)
    {
        if (isMultiplayer())
            Main::get().getLocalPlayer()->hasTcl = noClip;
    }

    void sendWerewolfState(const MWWorld::Ptr& actor, bool werewolf)
    {
        if (isLoggedIn() && isLocalPlayer(actor) && !ServerChange::isActive())
            Main::get().getLocalPlayer()->sendWerewolfState(werewolf);
    }

    bool mayRestInBed()
    {
        if (!isMultiplayer())
            return true;
        LocalPlayer* localPlayer = Main::get().getLocalPlayer();
        if (!localPlayer->bedRestAllowed)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("You are not allowed to rest in beds.");
            return false;
        }
        localPlayer->isUsingBed = true;
        return true;
    }

    bool isScriptSynchronized(const ESM::RefId& scriptId)
    {
        return isMultiplayer() && Main::isValidPacketScript(idString(scriptId));
    }

    void sendConsoleCommand(const std::string& command, const MWWorld::Ptr& target)
    {
        if (!isLoggedIn() || ServerChange::isActive())
            return;
        ObjectList* objectList = Main::get().getNetworking()->getObjectList();
        objectList->reset();
        objectList->packetOrigin = CLIENT_CONSOLE;
        objectList->consoleCommand = command;
        if (target.isEmpty() || !target.isInCell() || target.getCell()->getCell()->isEsm4())
            objectList->cell = Main::get().getLocalPlayer()->cell;
        else
            objectList->addObjectGeneric(target);
        objectList->sendConsoleCommand();
    }

    void onCellRefsChanged(const MWWorld::CellStore& cell)
    {
        if (!isMultiplayer() || cell.getCell()->isEsm4())
            return;
        CellController* cellController = Main::get().getCellController();
        const ESM::Cell& esmCell = cell.getCell()->getEsm3();
        if (!cellController->hasLocalAuthority(esmCell))
            return;
        if (Cell* mpCell = cellController->getCell(esmCell))
            mpCell->shouldInitializeActors = true;
    }

    std::vector<OtherPlayer> getOtherPlayers()
    {
        std::vector<OtherPlayer> players;
        if (!isLoggedIn())
            return players;
        for (DedicatedPlayer* player : PlayerList::getPlayers())
        {
            if (player->npc.mName.empty())
                continue;
            OtherPlayer& other = players.emplace_back();
            other.name = player->npc.mName;
            other.exterior = player->cell.isExterior();
            other.cellName = player->cell.mName;
            // Where they are drawn, if they are near; otherwise the last position heard of
            const MWWorld::Ptr ptr = player->getPtr();
            const ESM::Position& pos
                = !ptr.isEmpty() && ptr.isInCell() ? ptr.getRefData().getPosition() : player->position;
            other.x = pos.pos[0];
            other.y = pos.pos[1];
            other.yaw = pos.rot[2];
        }
        return players;
    }

    bool hasActorCollision(const MWWorld::Ptr& actor)
    {
        if (!isMultiplayer() || isLocalPlayer(actor))
            return true;
        const BaseWorldstate* worldstate = Main::get().getNetworking()->getWorldstate();
        return isDedicatedPlayer(actor) ? worldstate->hasPlayerCollision : worldstate->hasActorCollision;
    }

    ESM::RefId getDoorDestination(const ESM::RefId& destination)
    {
        if (!isMultiplayer() || !destination.is<ESM::StringRefId>())
            return destination;
        const std::string name = destination.getRefIdString();
        for (const auto& [original, replacement] : Main::get().getNetworking()->getWorldstate()->destinationOverrides)
        {
            if (Misc::StringUtils::ciEqual(original, name))
                return ESM::RefId::stringRefId(replacement);
        }
        return destination;
    }

    bool hasEnforcedCollision(const MWWorld::Ptr& ptr, bool& asActor)
    {
        if (!isMultiplayer())
            return false;
        const BaseWorldstate* worldstate = Main::get().getNetworking()->getWorldstate();
        asActor = worldstate->useActorCollisionForPlacedObjects;
        if (worldstate->hasPlacedObjectCollision)
            return true;
        for (const std::string& refId : worldstate->enforcedCollisionRefIds)
        {
            if (isSameId(ptr.getCellRef().getRefId(), refId))
                return true;
        }
        return false;
    }

    void sendObjectSpawn(const MWWorld::Ptr& ptr)
    {
        if (ObjectList* objectList = startObjectList(ptr))
        {
            objectList->addObjectSpawn(ptr);
            objectList->sendObjectSpawn();
        }
    }

    bool claimExploredMapTile(int cellX, int cellY)
    {
        if (!isLoggedIn())
            return false;
        mwmp::Worldstate* worldstate = Main::get().getNetworking()->getWorldstate();
        if (worldstate->containsExploredMapTile(cellX, cellY))
            return false;
        worldstate->markExploredMapTile(cellX, cellY);
        return true;
    }

    void sendExploredMapTile(int cellX, int cellY, const std::vector<char>& png)
    {
        Main::get().getNetworking()->getWorldstate()->sendMapExplored(cellX, cellY, png);
    }
}
