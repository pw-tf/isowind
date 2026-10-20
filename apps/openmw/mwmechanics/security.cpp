#include "security.hpp"

#include <components/misc/rng.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmp/Hooks.hpp"

#include "actorutil.hpp"
#include "creaturestats.hpp"
#include "npcstats.hpp"
#include "spellutil.hpp"

namespace MWMechanics
{

    Security::Security(const MWWorld::Ptr& actor)
        : mActor(actor)
    {
        CreatureStats& creatureStats = actor.getClass().getCreatureStats(actor);
        mAgility = creatureStats.getAttribute(ESM::Attribute::Agility).getModified();
        mLuck = creatureStats.getAttribute(ESM::Attribute::Luck).getModified();
        mSecuritySkill = static_cast<float>(actor.getClass().getSkill(actor, ESM::Skill::Security));
        mFatigueTerm = creatureStats.getFatigueTerm();
    }

    void Security::pickLock(const MWWorld::Ptr& lock, const MWWorld::Ptr& lockpick, std::string_view& resultMessage,
        std::string_view& resultSound)
    {
        // Isowind multiplayer: another player's (or a remote actor's) attempt is their own client's to make; here their
        // swing would otherwise pick what this player is looking at
        if (mwmp::hooks::isRemotelyControlled(mActor))
            return;

        // If it's unlocked or can not be unlocked back out immediately. Note that we're not strictly speaking checking
        // if the ref is locked, lock levels <= 0 can exist but they cannot be picked
        if (lock.getCellRef().getLockLevel() <= 0 || !lock.getClass().hasToolTip(lock))
            return;

        int uses = lockpick.getClass().getItemHealth(lockpick);
        if (uses == 0)
            return;

        int lockStrength = lock.getCellRef().getLockLevel();

        float pickQuality = lockpick.get<ESM::Lockpick>()->mBase->mData.mQuality;

        float fPickLockMult = MWBase::Environment::get()
                                  .getESMStore()
                                  ->get<ESM::GameSetting>()
                                  .find("fPickLockMult")
                                  ->mValue.getFloat();

        float x = 0.2f * mAgility + 0.1f * mLuck + mSecuritySkill;
        x *= pickQuality * mFatigueTerm;
        x += fPickLockMult * lockStrength;

        MWBase::Environment::get().getMechanicsManager()->unlockAttempted(mActor, lock);

        resultSound = "Open Lock Fail";
        if (x <= 0)
            resultMessage = "#{sLockImpossible}";
        else
        {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            if (Misc::Rng::roll0to99(prng) <= x)
            {
                // Isowind multiplayer: the server unlocks it, for everyone
                if (mwmp::hooks::isMultiplayer())
                    mwmp::hooks::sendObjectLock(lock, 0);
                else
                    lock.getCellRef().unlock();
                resultMessage = "#{sLockSuccess}";
                resultSound = "Open Lock";
                mActor.getClass().skillUsageSucceeded(mActor, ESM::Skill::Security, ESM::Skill::Security_PickLock);
            }
            else
                resultMessage = "#{sLockFail}";
        }

        lockpick.getCellRef().setCharge(--uses);
        if (!uses)
            lockpick.getContainerStore()->remove(lockpick, 1);
    }

    void Security::probeTrap(const MWWorld::Ptr& trap, const MWWorld::Ptr& probe, std::string_view& resultMessage,
        std::string_view& resultSound)
    {
        // Isowind multiplayer: as for lockpicks, another player's attempt is their own client's
        if (mwmp::hooks::isRemotelyControlled(mActor))
            return;

        if (trap.getCellRef().getTrap().empty())
            return;

        int uses = probe.getClass().getItemHealth(probe);
        if (uses == 0)
            return;

        float probeQuality = probe.get<ESM::Probe>()->mBase->mData.mQuality;

        const ESM::Spell* trapSpell
            = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().find(trap.getCellRef().getTrap());
        int trapSpellPoints = MWMechanics::calcSpellCost(*trapSpell);

        float fTrapCostMult = MWBase::Environment::get()
                                  .getESMStore()
                                  ->get<ESM::GameSetting>()
                                  .find("fTrapCostMult")
                                  ->mValue.getFloat();

        float x = 0.2f * mAgility + 0.1f * mLuck + mSecuritySkill;
        x += fTrapCostMult * trapSpellPoints;
        x *= probeQuality * mFatigueTerm;

        MWBase::Environment::get().getMechanicsManager()->unlockAttempted(mActor, trap);

        resultSound = "Disarm Trap Fail";
        if (x <= 0)
            resultMessage = "#{sTrapImpossible}";
        else
        {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            if (Misc::Rng::roll0to99(prng) <= x)
            {
                trap.getCellRef().setTrap(ESM::RefId());

                resultSound = "Disarm Trap";
                resultMessage = "#{sTrapSuccess}";
                mActor.getClass().skillUsageSucceeded(mActor, ESM::Skill::Security, ESM::Skill::Security_DisarmTrap);
            }
            else
                resultMessage = "#{sTrapFail}";
        }

        probe.getCellRef().setCharge(--uses);
        if (!uses)
            probe.getContainerStore()->remove(probe, 1);
    }

    MWWorld::Ptr chooseLockpick(const MWWorld::Ptr& actor)
    {
        MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);
        MWWorld::ContainerStoreIterator held = store.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if (held != store.end() && held->getType() == ESM::Lockpick::sRecordId
            && held->getClass().getItemHealth(*held) > 0)
            return *held;

        // The best; of equally good ones the most worn, so that a whole stack isn't split for the one that is used
        MWWorld::Ptr best;
        float bestQuality = 0.f;
        int bestUses = 0;
        for (auto it = store.begin(MWWorld::ContainerStore::Type_Lockpick); it != store.end(); ++it)
        {
            const int uses = it->getClass().getItemHealth(*it);
            if (uses <= 0)
                continue;
            const float quality = it->get<ESM::Lockpick>()->mBase->mData.mQuality;
            if (best.isEmpty() || quality > bestQuality || (quality == bestQuality && uses < bestUses))
            {
                best = *it;
                bestQuality = quality;
                bestUses = uses;
            }
        }
        return best;
    }

    std::unique_ptr<MWWorld::Action> pickLockOnActivation(const MWWorld::Ptr& lock, const MWWorld::Ptr& actor)
    {
        // NPCs don't pick locks, and a werewolf can't hold a lockpick
        if (actor != getPlayer() || actor.getClass().getNpcStats(actor).isWerewolf())
            return nullptr;
        // What Security::pickLock backs out of, checked first so that no stack of lockpicks is split for nothing
        if (lock.getCellRef().getLockLevel() <= 0 || !lock.getClass().hasToolTip(lock))
            return nullptr;
        MWWorld::Ptr lockpick = chooseLockpick(actor);
        if (lockpick.isEmpty())
            return nullptr;

        // Isowind multiplayer: the inventory goes to the server once afterwards, with the pick's wear (or without
        // the pick, used up), instead of item by item as the stack is split
        mwmp::hooks::beginBatchedItemRemovals();
        // Only the one pick wears (one that is held is on its own already)
        actor.getClass().getContainerStore(actor).unstack(lockpick);
        std::string_view resultMessage, resultSound;
        Security(actor).pickLock(lock, lockpick, resultMessage, resultSound);
        mwmp::hooks::endBatchedItemRemovals();

        if (resultMessage.empty())
            return nullptr;
        // The result shown and heard as after a swing of the lockpick: the message, and the sound at the lock
        std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::FailedAction>(resultMessage, lock);
        if (!resultSound.empty())
            action->setSound(ESM::RefId::stringRefId(resultSound));
        return action;
    }
}
