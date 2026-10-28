#include <components/esm3/esmwriter.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/Utils.hpp>

#include <algorithm>

#include <components/esm/util.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/pathutil.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwclass/creature.hpp"
#include "../mwclass/npc.hpp"

#include "../mwdialogue/dialoguemanagerimp.hpp"

#include "../mwgui/inventorywindow.hpp"
#include "../mwgui/windowmanagerimp.hpp"

#include "../mwinput/inputmanagerimp.hpp"

#include "../mwmechanics/activespells.hpp"
#include "../mwmechanics/aitravel.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwscript/scriptmanagerimp.hpp"

#include "../mwstate/statemanagerimp.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldimp.hpp"
#include "../mwworld/worldmodel.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/globals.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spells.hpp"

#include "LocalPlayer.hpp"
#include "Hooks.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "PlayerList.hpp"
#include "CellController.hpp"
#include "GUIController.hpp"
#include "MechanicsHelper.hpp"
#include "RecordHelper.hpp"
#include "Ids.hpp"
#include "PortableCharacter.hpp"

using namespace mwmp;

std::map<std::string, int> storedItemRemovals;

LocalPlayer::LocalPlayer()
{
    deathTime = time(0);
    receivedCharacter = false;

    charGenState.currentStage = 0;
    charGenState.endStage = 1;
    charGenState.isFinished = false;

    ignorePosPacket = false;
    ignoreJailTeleportation = false;
    ignoreJailSkillIncreases = false;
    
    attack.shouldSend = false;
    attack.instant = false;
    attack.pressed = false;

    cast.shouldSend = false;
    cast.instant = false;
    cast.pressed = false;

    killer.isPlayer = false;
    killer.refId = "";
    killer.name = "";

    isChangingRegion = false;

    jailProgressText = "";
    jailEndText = "";

    isUsingBed = false;
    avoidSendingInventoryPackets = false;
    isReceivingQuickKeys = false;
    isPlayingAnimation = false;
    diedSinceArrestAttempt = false;
}

LocalPlayer::~LocalPlayer()
{

}

Networking *LocalPlayer::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}

MWWorld::Ptr LocalPlayer::getPlayerPtr()
{
    return MWBase::Environment::get().getWorld()->getPlayerPtr();
}

void LocalPlayer::update()
{
    static float updateTimer = 0;
    const float timeoutSec = 0.015;

    if ((updateTimer += MWBase::Environment::get().getFrameDuration()) >= timeoutSec)
    {
        updateTimer = 0;
        // Cells loaded or unloaded since the last update (stored by MWWorld::Scene), so the server knows what this
        // client needs to hear about (updateCell also sends them, before a cell change)
        if (!cellStateChanges.empty())
        {
            sendCellStates();
            clearCellStates();
        }

        updateCell();
        updatePosition();
        updateAnimFlags();
        updateAttackOrCast();
        updateEquipment();
        updateStatsDynamic();
        updateAttributes();
        updateSkills();
        updateLevel();
        updateBounty();
        updateReputation();
    }
}

bool LocalPlayer::processCharGen()
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();

    // If we haven't finished CharGen and we're in a menu, it must be
    // one of the CharGen menus, so go no further until it's closed
    if (windowManager->isGuiMode() && !charGenState.isFinished)
    {
        return false;
    }

    // If the current stage of CharGen is not the last one,
    // move to the next one
    else if (charGenState.currentStage < charGenState.endStage)
    {
        switch (charGenState.currentStage)
        {
        case 0:
        {
            // A character from single-player keeps its name: no asking for one
            if (!Main::getCharacterSave().empty())
            {
                std::string name = PortableCharacter::readName(Main::getCharacterSave());
                if (!name.empty())
                {
                    MWBase::Environment::get().getMechanicsManager()->setPlayerName(name.substr(0, 31));
                    charGenState.currentStage = 1;
                    return false;
                }
            }
            windowManager->pushGuiMode(MWGui::GM_Name);
            break;
        }
        case 1:
            windowManager->pushGuiMode(MWGui::GM_Race);
            break;
        case 2:
            windowManager->pushGuiMode(MWGui::GM_Class);
            break;
        case 3:
            windowManager->pushGuiMode(MWGui::GM_Birth);
            break;
        default:
            windowManager->pushGuiMode(MWGui::GM_Review);
            break;
        }
        getNetworking()->getPlayerPacket(ID_PLAYER_CHARGEN)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_CHARGEN)->Send();

        return false;
    }

    // If we've reached the last stage of CharGen, send the
    // corresponding packets and mark CharGen as finished
    else if (!charGenState.isFinished)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr ptrPlayer = world->getPlayerPtr();
        npc = *ptrPlayer.get<ESM::NPC>()->mBase;
        birthsign = idString(world->getPlayer().getBirthSign());

        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_BASEINFO to server with my CharGen info");
        getNetworking()->getPlayerPacket(ID_PLAYER_BASEINFO)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_BASEINFO)->Send();

        // Send stats packets if this is the 2nd round of CharGen that
        // only happens for new characters
        if (charGenState.endStage != 1)
        {
            updateStatsDynamic(true);
            updateAttributes(true);
            updateSkills(true);
            updateLevel(true);
            sendClass();
            sendSpellbook();
            getNetworking()->getPlayerPacket(ID_PLAYER_CHARGEN)->setPlayer(this);
            getNetworking()->getPlayerPacket(ID_PLAYER_CHARGEN)->Send();
            // Isowind: the server only hears of items as they come and go, so tell it what a new character starts
            // with (or it's missing after the next login)
            sendInventory();
        }

        // Mark character generation as finished until overridden by a new ID_PLAYER_CHARGEN packet
        charGenState.isFinished = true;
    }

    return true;
}

void LocalPlayer::waitInVoid()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    ESM::Position pos;
    pos.pos[0] = pos.pos[1] = pos.pos[2] = 0;
    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
    world->getPlayer().setTeleported(true);
    world->changeToInteriorCell(RecordHelper::getPlaceholderInteriorCellName(), pos, false);
    // Collision off: nothing to stand on there, so no falling either
    if (!world->toggleCollisionMode())
        waitingInVoid = true;
    else
        world->toggleCollisionMode();
}

bool LocalPlayer::isLoggedIn()
{
    if (charGenState.isFinished && (charGenState.endStage > 1 || receivedCharacter))
        return true;

    return false;
}

void LocalPlayer::updateStatsDynamic(bool forceUpdate)
{
    if (statsDynamicIndexChanges.size() > 0)
        statsDynamicIndexChanges.clear();

    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
    MWMechanics::DynamicStat<float> health(ptrCreatureStats->getHealth());
    MWMechanics::DynamicStat<float> magicka(ptrCreatureStats->getMagicka());
    MWMechanics::DynamicStat<float> fatigue(ptrCreatureStats->getFatigue());

    static MWMechanics::DynamicStat<float> oldHealth(ptrCreatureStats->getHealth());
    static MWMechanics::DynamicStat<float> oldMagicka(ptrCreatureStats->getMagicka());
    static MWMechanics::DynamicStat<float> oldFatigue(ptrCreatureStats->getFatigue());


    // Update stats when they become 0 or they have changed enough
    auto needUpdate = [](MWMechanics::DynamicStat<float> &oldVal, MWMechanics::DynamicStat<float> &newVal, int limit) {
        return oldVal != newVal && (newVal.getCurrent() == 0 || oldVal.getCurrent() == 0
                                    || abs(oldVal.getCurrent() - newVal.getCurrent()) >= limit);
    };

    if (forceUpdate || needUpdate(oldHealth, health, 2))
        statsDynamicIndexChanges.push_back(0);

    if (forceUpdate || needUpdate(oldMagicka, magicka, 4))
        statsDynamicIndexChanges.push_back(1);

    if (forceUpdate || needUpdate(oldFatigue, fatigue, 4))
        statsDynamicIndexChanges.push_back(2);

    if (forceUpdate || statsDynamicIndexChanges.size() > 0)
    {
        oldHealth = health;
        oldMagicka = magicka;
        oldFatigue = fatigue;

        health.writeState(creatureStats.mDynamic[0]);
        magicka.writeState(creatureStats.mDynamic[1]);
        fatigue.writeState(creatureStats.mDynamic[2]);

        creatureStats.mDead = ptrCreatureStats->isDead();

        exchangeFullInfo = false;
        getNetworking()->getPlayerPacket(ID_PLAYER_STATS_DYNAMIC)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_STATS_DYNAMIC)->Send();
    }
}

void LocalPlayer::updateAttributes(bool forceUpdate)
{
    // Only send attributes if we are not a werewolf, or they will be
    // overwritten by the werewolf ones
    if (isWerewolf) return;

    if (attributeIndexChanges.size() > 0)
        attributeIndexChanges.clear();

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    for (int i = 0; i < ESM::Attribute::Length; ++i)
    {
        const ESM::Attribute::AttributeID id = attributeId(i);
        const MWMechanics::AttributeValue &attribute = ptrNpcStats.getAttribute(id);
        const int skillIncrease = ptrNpcStats.getSkillIncreasesForAttribute(id);

        if (attribute.getBase() != creatureStats.mAttributes[i].mBase ||
            attribute.getModifier() != creatureStats.mAttributes[i].mMod ||
            attribute.getDamage() != creatureStats.mAttributes[i].mDamage ||
            skillIncrease != npcStats.mSkillIncrease[i] ||
            forceUpdate)
        {
            attributeIndexChanges.push_back(i);
            attribute.writeState(creatureStats.mAttributes[i]);
            npcStats.mSkillIncrease[i] = skillIncrease;
        }
    }

    if (attributeIndexChanges.size() > 0)
    {
        exchangeFullInfo = false;
        getNetworking()->getPlayerPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_ATTRIBUTE)->Send();
    }
}

void LocalPlayer::updateSkills(bool forceUpdate)
{
    // Only send skills if we are not a werewolf, or they will be
    // overwritten by the werewolf ones
    if (isWerewolf) return;

    if (skillIndexChanges.size() > 0)
        skillIndexChanges.clear();

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        const MWMechanics::SkillValue &skill = ptrNpcStats.getSkill(skillId(i));

        // Update a skill if its base value has changed at all or its progress has changed enough
        if (skill.getBase() != npcStats.mSkills[i].mBase ||
            skill.getModifier() != npcStats.mSkills[i].mMod ||
            skill.getDamage() != npcStats.mSkills[i].mDamage ||
            abs(skill.getProgress() - npcStats.mSkills[i].mProgress) > 0.75 ||
            forceUpdate)
        {
            skillIndexChanges.push_back(i);
            skill.writeState(npcStats.mSkills[i]);
        }
    }

    if (skillIndexChanges.size() > 0)
    {
        exchangeFullInfo = false;
        getNetworking()->getPlayerPacket(ID_PLAYER_SKILL)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_SKILL)->Send();
    }
}

void LocalPlayer::updateLevel(bool forceUpdate)
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    if (ptrNpcStats.getLevel() != creatureStats.mLevel ||
        ptrNpcStats.getLevelProgress() != npcStats.mLevelProgress ||
        forceUpdate)
    {
        creatureStats.mLevel = ptrNpcStats.getLevel();
        npcStats.mLevelProgress = ptrNpcStats.getLevelProgress();
        getNetworking()->getPlayerPacket(ID_PLAYER_LEVEL)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_LEVEL)->Send();
    }
}

void LocalPlayer::updateBounty(bool forceUpdate)
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    if (ptrNpcStats.getBounty() != npcStats.mBounty || forceUpdate)
    {
        npcStats.mBounty = ptrNpcStats.getBounty();
        getNetworking()->getPlayerPacket(ID_PLAYER_BOUNTY)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_BOUNTY)->Send();
    }
}

void LocalPlayer::updateReputation(bool forceUpdate)
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    if (ptrNpcStats.getReputation() != npcStats.mReputation || forceUpdate)
    {
        npcStats.mReputation = ptrNpcStats.getReputation();
        getNetworking()->getPlayerPacket(ID_PLAYER_REPUTATION)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_REPUTATION)->Send();
    }
}

void LocalPlayer::updatePosition(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    static bool posWasChanged = false;
    static bool isJumping = false;
    static bool sentJumpEnd = true;
    static float oldRot[2] = {0};

    position = ptrPlayer.getRefData().getPosition();

    bool posIsChanging = (direction.pos[0] != 0 || direction.pos[1] != 0 ||
        direction.rot[0] != 0 || direction.rot[1] != 0 || direction.rot[2] != 0);

    // Isowind: also when something else moved the player (pushed out of another actor, knocked back, sliding down a
    // slope, a teleport that physics then adjusts): the others would keep seeing them where they last walked to
    static osg::Vec3f lastSentPosition;
    if (!posIsChanging && (position.asVec3() - lastSentPosition).length2() > 4.f)
        posIsChanging = true;

    // Animations can change a player's position without actually creating directional movement,
    // so update positions accordingly
    if (!posIsChanging && isPlayingAnimation)
    {
        if (MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(ptrPlayer, animation.groupname))
            posIsChanging = true;
        else
            isPlayingAnimation = false;
    }

    if (forceUpdate || posIsChanging || posWasChanged)
    {
        oldRot[0] = position.rot[0];
        oldRot[1] = position.rot[2];

        posWasChanged = posIsChanging;

        if (!isJumping && !world->isOnGround(ptrPlayer) && !world->isFlying(ptrPlayer))
            isJumping = true;

        getNetworking()->getPlayerPacket(ID_PLAYER_POSITION)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_POSITION)->Send();
        lastSentPosition = position.asVec3();
    }
    else if (isJumping && world->isOnGround(ptrPlayer))
    {
        isJumping = false;
        sentJumpEnd = false;
    }
    // Packet with jump end position has to be sent one tick after above check
    else if (!sentJumpEnd)
    {
        sentJumpEnd = true;
        position = ptrPlayer.getRefData().getPosition();
        getNetworking()->getPlayerPacket(ID_PLAYER_POSITION)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_POSITION)->Send();
        lastSentPosition = position.asVec3();
    }
}

void LocalPlayer::updateCell(bool forceUpdate)
{
    const ESM::Cell *ptrCell = &MWBase::Environment::get().getWorld()->getPlayerPtr().getCell()->getCell()->getEsm3();

    // If the LocalPlayer's Ptr cell is different from the LocalPlayer's packet cell, proceed
    if (forceUpdate || !Main::get().getCellController()->isSameCell(*ptrCell, cell))
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_CELL_CHANGE about LocalPlayer to server");

        LOG_APPEND(TimedLog::LOG_INFO, "- Moved from %s to %s", cell.getShortDescription().c_str(),
                   ptrCell->getShortDescription().c_str());

        if (cell.mRegion != ptrCell->mRegion)
        {
            LOG_APPEND(TimedLog::LOG_INFO, "- Changed region from %s to %s",
                cell.mRegion.empty() ? "none" : idString(cell.mRegion).c_str(),
                ptrCell->mRegion.empty() ? "none" : idString(ptrCell->mRegion).c_str());

            isChangingRegion = true;
        }

        cell = *ptrCell;
        previousCellPosition = position;

        // Make sure the position is updated before a cell packet is sent, or else
        // cell change events in server scripts will have the wrong player position
        updatePosition(true);

        // The cells loaded for this one first: on a cell change, the server exchanges equipment, stats and so on
        // with the players it knows have the cell loaded
        if (!cellStateChanges.empty())
        {
            sendCellStates();
            clearCellStates();
        }

        getNetworking()->getPlayerPacket(ID_PLAYER_CELL_CHANGE)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_CELL_CHANGE)->Send();

        isChangingRegion = false;

        // If this is an interior cell, are there any other players in it? If so,
        // enable their markers
        if (!ptrCell->isExterior())
        {
            mwmp::PlayerList::enableMarkers(*ptrCell);
        }
    }
}

void LocalPlayer::updateEquipment(bool forceUpdate)
{
    if (equipmentIndexChanges.size() > 0)
        equipmentIndexChanges.clear();

    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWWorld::InventoryStore &invStore = ptrPlayer.getClass().getInventoryStore(ptrPlayer);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
    {
        auto &item = equipmentItems[slot];
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);

        if (it != invStore.end())
        {
            MWWorld::CellRef &cellRef = it->getCellRef();

            if (!(cellRef.getRefId() == item.refId) ||
                cellRef.getCharge() != item.charge ||
                Utils::compareFloats(cellRef.getEnchantmentCharge(), item.enchantmentCharge, 1.0f) == false ||
                it->getCellRef().getCount() != item.count ||
                forceUpdate)
            {
                equipmentIndexChanges.push_back(slot);

                item.refId = idString(it->getCellRef().getRefId());
                item.count = it->getCellRef().getCount();
                item.charge = it->getCellRef().getCharge();
                item.enchantmentCharge = it->getCellRef().getEnchantmentCharge();
            }
        }
        else if (!item.refId.empty())
        {
            equipmentIndexChanges.push_back(slot);
            item.refId = "";
            item.count = 0;
            item.charge = -1;
            item.enchantmentCharge = -1;
        }
    }

    if (equipmentIndexChanges.size() > 0)
    {
        exchangeFullInfo = false;
        getNetworking()->getPlayerPacket(ID_PLAYER_EQUIPMENT)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_EQUIPMENT)->Send();
    }
}

void LocalPlayer::updateInventory(bool forceUpdate)
{
    static bool invChanged = false;

    if (forceUpdate)
        invChanged = true;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::InventoryStore &ptrInventory = ptrPlayer.getClass().getInventoryStore(ptrPlayer);
    mwmp::Item item;

    auto setItem = [](Item &item, const MWWorld::Ptr &iter) {
        if (isLocalOnlyId(iter.getCellRef().getRefId()))
            return true;
        item.refId = idString(iter.getCellRef().getRefId());
        item.count = iter.getCellRef().getCount();
        item.charge = iter.getCellRef().getCharge();
        item.enchantmentCharge = iter.getCellRef().getEnchantmentCharge();
        item.soul = idString(iter.getCellRef().getSoul());

        return false;
    };

    if (!invChanged)
    {
        for (const auto &itemOld : inventoryChanges.items)
        {
            auto result = ptrInventory.begin();
            for (; result != ptrInventory.end(); ++result)
            {
                if(setItem(item, *result))
                    continue;

                if (item == itemOld)
                    break;
            }
            if (result == ptrInventory.end())
            {
                invChanged = true;
                break;
            }
        }
    }

    if (!invChanged)
    {
        for (const auto &iter : ptrInventory)
        {
            if(setItem(item, iter))
                continue;

            auto items = inventoryChanges.items;

            if (find(items.begin(), items.end(), item) == items.end())
            {
                invChanged = true;
                break;
            }
        }
    }

    if (!invChanged)
        return;

    invChanged = false;

    sendInventory();
}

void LocalPlayer::updateAttackOrCast()
{
    if (attack.shouldSend)
    {
        getNetworking()->getPlayerPacket(ID_PLAYER_ATTACK)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_ATTACK)->Send();

        attack.shouldSend = false;
    }
    else if (cast.shouldSend)
    {
        getNetworking()->getPlayerPacket(ID_PLAYER_CAST)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_CAST)->Send();

        cast.shouldSend = false;
        cast.hasProjectile = false;
    }
}

void LocalPlayer::updateAnimFlags(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::NpcStats ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);
    using namespace MWMechanics;

    static bool wasRunning = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Run);
    static bool wasSneaking = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    static bool wasForceJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    static bool wasForceMoveJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);

    bool isRunning = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Run);
    bool isSneaking = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    bool isForceJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    bool isForceMoveJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);
    
    isFlying = world->isFlying(ptrPlayer);
    isJumping = !world->isOnGround(ptrPlayer) && !isFlying;

    // We need to send a new packet at the end of jumping, flying and TCL-ing too,
    // so keep track of what we were doing last frame
    static bool wasJumping = false;
    static bool wasFlying = false;
    static bool hadTcl = false;

    drawState = static_cast<char>(ptrPlayer.getClass().getNpcStats(ptrPlayer).getDrawState());
    static char lastDrawState = drawState;

    if (wasRunning != isRunning ||
        wasSneaking != isSneaking || wasForceJumping != isForceJumping ||
        wasForceMoveJumping != isForceMoveJumping || lastDrawState != drawState ||
        wasJumping || isJumping || wasFlying != isFlying || hadTcl != hasTcl ||
        forceUpdate)
    {
        wasSneaking = isSneaking;
        wasRunning = isRunning;
        wasForceJumping = isForceJumping;
        wasForceMoveJumping = isForceMoveJumping;
        lastDrawState = drawState;
        
        wasJumping = isJumping;
        wasFlying = isFlying;
        hadTcl = hasTcl;

        movementFlags = 0;

#define __SETFLAG(flag, value) (value) ? (movementFlags | flag) : (movementFlags & ~flag)

        movementFlags = __SETFLAG(CreatureStats::Flag_Sneak, isSneaking);
        movementFlags = __SETFLAG(CreatureStats::Flag_Run, isRunning);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, isForceJumping);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, isJumping);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceMoveJump, isForceMoveJumping);

#undef __SETFLAG

        if (isJumping)
            updatePosition(true); // fix position after jump;

        getNetworking()->getPlayerPacket(ID_PLAYER_ANIM_FLAGS)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_ANIM_FLAGS)->Send();
    }
}

void LocalPlayer::addItems()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWWorld::ESMStore &esmStore = MWBase::Environment::get().getWorld()->getStore();
    MWWorld::ContainerStore &ptrStore = ptrPlayer.getClass().getContainerStore(ptrPlayer);

    for (const auto &item : inventoryChanges.items)
    {
        try
        {
            MWWorld::ManualRef itemRef(esmStore, refId(item.refId), item.count);
            MWWorld::Ptr itemPtr = itemRef.getPtr();

            // Skip bound items
            if (MWBase::Environment::get().getMechanicsManager()->isBoundItem(itemPtr))
                continue;

            if (item.charge != -1)
                itemPtr.getCellRef().setCharge(item.charge);

            if (item.enchantmentCharge != -1)
                itemPtr.getCellRef().setEnchantmentCharge(item.enchantmentCharge);

            if (!item.soul.empty())
                itemPtr.getCellRef().setSoul(refId(item.soul));

            LOG_APPEND(TimedLog::LOG_INFO, "- Adding inventory item %s with count %i", item.refId.c_str(), item.count);

            ptrStore.add(itemPtr, item.count);
        }
        catch (std::exception&)
        {
            LOG_APPEND(TimedLog::LOG_INFO, "- Ignored addition of invalid inventory item %s", item.refId.c_str());
        }
    }

    updateInventoryWindow();
}

void LocalPlayer::addSpells()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    for (const auto &spell : spellbookChanges.spells)
        // Only add spells that are ensured to exist
        if (MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spell.mId))
            ptrSpells.add(spell.mId);
        else
            LOG_APPEND(TimedLog::LOG_INFO, "- Ignored addition of invalid spell %s", idString(spell.mId).c_str());
}

void LocalPlayer::addSpellsActive()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
        MechanicsHelper::addActiveSpell(ptrPlayer, activeSpell);
}

void LocalPlayer::addJournalItems()
{
    // What the server says isn't sent back to it
    hooks::ServerChange serverChange;

    for (const auto &journalItem : journalChanges)
    {
        MWWorld::Ptr ptrFound;

        if (journalItem.type == JournalItem::ENTRY)
        {
            LOG_APPEND(TimedLog::LOG_VERBOSE, "- type: ENTRY, quest: %s, index: %i, actorRefId: %s",
                journalItem.quest.c_str(), journalItem.index, journalItem.actorRefId.c_str());

            ptrFound = MWBase::Environment::get().getWorld()->searchPtr(refId(journalItem.actorRefId), false);

            if (ptrFound.isEmpty())
                ptrFound = getPlayerPtr();
        }
        else
        {
            LOG_APPEND(TimedLog::LOG_VERBOSE, "- type: INDEX, quest: %s, index: %i",
                journalItem.quest.c_str(), journalItem.index);
        }

        try
        {
            if (journalItem.type == JournalItem::ENTRY)
            {
                if (journalItem.hasTimestamp)
                {
                    MWBase::Environment::get().getJournal()->addEntry(refId(journalItem.quest), journalItem.index, ptrFound,
                        journalItem.timestamp.daysPassed, journalItem.timestamp.month, journalItem.timestamp.day);
                }
                else
                {
                    MWBase::Environment::get().getJournal()->addEntry(refId(journalItem.quest), journalItem.index, ptrFound);
                }
            }
            else
                MWBase::Environment::get().getJournal()->setJournalIndex(refId(journalItem.quest), journalItem.index);
        }
        catch (std::exception&)
        {
            LOG_APPEND(TimedLog::LOG_INFO, "- Ignored addition of invalid journal quest %s", journalItem.quest.c_str());
        }
    }
}

void LocalPlayer::addTopics()
{
    // What the server says isn't sent back to it
    hooks::ServerChange serverChange;

    auto &env = MWBase::Environment::get();
    for (const auto &topic : topicChanges)
    {
        // TODO: TES3MP translated topics from English for translated versions of Morrowind; OpenMW 0.51's
        // translation data can't do that, so all players need the same language for now
        env.getDialogueManager()->addTopic(refId(topic.topicId));

        if (env.getWindowManager()->containsMode(MWGui::GM_Dialogue))
            env.getDialogueManager()->updateActorKnownTopics();
    }
}

void LocalPlayer::removeItems()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::ContainerStore &ptrStore = ptrPlayer.getClass().getContainerStore(ptrPlayer);

    for (const auto &item : inventoryChanges.items)
    {
        ptrStore.remove(refId(item.refId), item.count);

        LOG_APPEND(TimedLog::LOG_INFO, "- Removing inventory item %s with count %i", item.refId.c_str(), item.count);
    }
}

void LocalPlayer::removeSpells()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();
    for (const auto &spell : spellbookChanges.spells)
    {
        ptrSpells.remove(spell.mId);
        if (spell.mId == wm->getSelectedSpell())
            wm->unsetSelectedSpell();
    }
}

void LocalPlayer::removeSpellsActive()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
    {
        LOG_APPEND(TimedLog::LOG_INFO, "- removing %sstacking active spell %s", activeSpell.isStackingSpell ? "" : "non-", activeSpell.id.c_str());
        MechanicsHelper::removeActiveSpell(ptrPlayer, activeSpell);
    }
}

void LocalPlayer::die()
{
    creatureStats.mDead = true;

    MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
    MWMechanics::DynamicStat<float> health = playerPtr.getClass().getCreatureStats(playerPtr).getHealth();
    health.setCurrent(0);
    playerPtr.getClass().getCreatureStats(playerPtr).setHealth(health);

    Main::get().getNetworking()->getPlayerPacket(ID_PLAYER_DEATH)->setPlayer(this);
    Main::get().getNetworking()->getPlayerPacket(ID_PLAYER_DEATH)->Send();
}

void LocalPlayer::resurrect()
{
    creatureStats.mDead = false;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    if (resurrectType == mwmp::RESURRECT_TYPE::IMPERIAL_SHRINE)
        MWBase::Environment::get().getWorld()->teleportToClosestMarker(ptrPlayer, refId("divinemarker"));
    else if (resurrectType == mwmp::RESURRECT_TYPE::TRIBUNAL_TEMPLE)
        MWBase::Environment::get().getWorld()->teleportToClosestMarker(ptrPlayer, refId("templemarker"));

    MWBase::Environment::get().getMechanicsManager()->resurrect(ptrPlayer);

    // The player could have died from a hand-to-hand attack, so reset their fatigue
    // as well
    if (creatureStats.mDynamic[2].mMod < 1)
        creatureStats.mDynamic[2].mMod = 1;

    creatureStats.mDynamic[2].mCurrent = creatureStats.mDynamic[2].mMod;
    MWMechanics::DynamicStat<float> fatigue;
    fatigue.readState(creatureStats.mDynamic[2]);
    ptrPlayer.getClass().getCreatureStats(ptrPlayer).setFatigue(fatigue);

    // If this player had a weapon or spell readied when dying, they will still have it
    // readied but be unable to use it unless we clear it here
    ptrPlayer.getClass().getNpcStats(ptrPlayer).setDrawState(MWMechanics::DrawState::Nothing);

    // Record that the player has died since the last attempt was made to arrest them,
    // used to make guards lenient enough to attempt an arrest again
    diedSinceArrestAttempt = true;
    // Witnesses of crimes from before the death stop being hostile, as when a bounty is paid
    MWBase::Environment::get().getWorld()->getPlayer().recordCrimeId();

    deathTime = time(0);

    LOG_APPEND(TimedLog::LOG_INFO, "- diedSinceArrestAttempt is now true");

    // Record that we are no longer a known werewolf, to avoid being attacked infinitely
    MWBase::Environment::get().getWorld()->setGlobalInt(MWWorld::Globals::sPCKnownWerewolf, 0);

    // Ensure we unequip any items with constant effects that can put us into an infinite
    // death loop
    static const ESM::RefId damageEffects[5] = { ESM::MagicEffect::DrainHealth, ESM::MagicEffect::FireDamage,
        ESM::MagicEffect::FrostDamage, ESM::MagicEffect::ShockDamage, ESM::MagicEffect::SunDamage };

    for (const auto &damageEffect : damageEffects)
        MechanicsHelper::unequipItemsByEffect(ptrPlayer, ESM::Enchantment::ConstantEffect, damageEffect);

    Main::get().getNetworking()->getPlayerPacket(ID_PLAYER_RESURRECT)->setPlayer(this);
    Main::get().getNetworking()->getPlayerPacket(ID_PLAYER_RESURRECT)->Send();

    updateStatsDynamic(true);
}

void LocalPlayer::closeInventoryWindows()
{
    if (MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Container) ||
        MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Inventory))
        MWBase::Environment::get().getWindowManager()->popGuiMode();

    MWBase::Environment::get().getWindowManager()->finishDragDrop();
}

void LocalPlayer::updateInventoryWindow()
{
    MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updateItemView();
}

void LocalPlayer::setCharacter()
{
    receivedCharacter = true;

    MWBase::World *world = MWBase::Environment::get().getWorld();

    // Ignore invalid races
    if (world->getStore().get<ESM::Race>().search(npc.mRace) != 0)
    {
        MWBase::Environment::get().getWorld()->getPlayer().setBirthSign(refId(birthsign));

        if (resetStats)
        {
            MWBase::Environment::get().getMechanicsManager()->setPlayerRace(npc.mRace, npc.isMale(), npc.mHead, npc.mHair);
            setEquipment();
        }
        else
        {
            ESM::NPC player = *world->getPlayerPtr().get<ESM::NPC>()->mBase;

            player.mRace = npc.mRace;
            player.mHead = npc.mHead;
            player.mHair = npc.mHair;
            player.mModel = npc.mModel;
            player.setIsMale(npc.isMale());
            world->getStore().insert(player);

            MWBase::Environment::get().getMechanicsManager()->playerLoaded();

            // This is needed to update the player's model instantly if they're in 3rd person
            world->reattachPlayerCamera();
        }

        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->rebuildAvatar();
    }
    else
    {
        LOG_APPEND(TimedLog::LOG_INFO, "- Character update was ignored due to invalid race %s", idString(npc.mRace).c_str());
    }
}

void LocalPlayer::setDynamicStats()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
    MWMechanics::DynamicStat<float> dynamicStat;

    for (int i = 0; i < 3; ++i)
    {
        dynamicStat = ptrCreatureStats->getDynamic(i);
        dynamicStat.setBase(creatureStats.mDynamic[i].mBase);
        dynamicStat.setCurrent(creatureStats.mDynamic[i].mCurrent);
        ptrCreatureStats->setDynamic(i, dynamicStat);
    }
}

void LocalPlayer::setAttributes()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
    MWMechanics::AttributeValue attributeValue;

    for (int attributeIndex = 0; attributeIndex < ESM::Attribute::Length; ++attributeIndex)
    {
        const ESM::Attribute::AttributeID id = attributeId(attributeIndex);

        // If the server wants to clear our attribute's non-zero modifier, we need to remove
        // the spell effect causing it, to avoid an infinite loop where the effect keeps resetting
        // the modifier
        if (creatureStats.mAttributes[attributeIndex].mMod == 0 && ptrNpcStats->getAttribute(id).getModifier() > 0)
        {
            ptrNpcStats->getActiveSpells().purgeEffect(ptrPlayer, ESM::MagicEffect::FortifyAttribute, id);
            MWBase::Environment::get().getMechanicsManager()->updateMagicEffects(ptrPlayer);

            // Is the modifier for this attribute still higher than 0? If so, unequip items that
            // fortify the attribute
            if (ptrNpcStats->getAttribute(id).getModifier() > 0)
            {
                MechanicsHelper::unequipItemsByEffect(ptrPlayer, ESM::Enchantment::ConstantEffect, ESM::MagicEffect::FortifyAttribute, id);
                mwmp::Main::get().getGUIController()->refreshGuiMode(MWGui::GM_Inventory);
            }
        }

        attributeValue.readState(creatureStats.mAttributes[attributeIndex]);
        ptrNpcStats->setAttribute(id, attributeValue);

        ptrNpcStats->setSkillIncreasesForAttribute(id, npcStats.mSkillIncrease[attributeIndex]);
    }
}

void LocalPlayer::setSkills()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
    MWMechanics::SkillValue skillValue;

    for (int skillIndex = 0; skillIndex < ESM::Skill::Length; ++skillIndex)
    {
        const ESM::RefId id = skillId(skillIndex);

        // If the server wants to clear our skill's non-zero modifier, we need to remove
        // the spell effect causing it, to avoid an infinite loop where the effect keeps resetting
        // the modifier
        if (npcStats.mSkills[skillIndex].mMod == 0 && ptrNpcStats->getSkill(id).getModifier() > 0)
        {
            ptrNpcStats->getActiveSpells().purgeEffect(ptrPlayer, ESM::MagicEffect::FortifySkill, id);
            MWBase::Environment::get().getMechanicsManager()->updateMagicEffects(ptrPlayer);

            // Is the modifier for this skill still higher than 0? If so, unequip items that
            // fortify the skill
            if (ptrNpcStats->getSkill(id).getModifier() > 0)
            {
                MechanicsHelper::unequipItemsByEffect(ptrPlayer, ESM::Enchantment::ConstantEffect, ESM::MagicEffect::FortifySkill, {}, id);
                mwmp::Main::get().getGUIController()->refreshGuiMode(MWGui::GM_Inventory);
            }
        }

        skillValue.readState(npcStats.mSkills[skillIndex]);
        ptrNpcStats->setSkill(id, skillValue);
    }
}

void LocalPlayer::setLevel()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
    ptrNpcStats->setLevel(creatureStats.mLevel);
    ptrNpcStats->setLevelProgress(npcStats.mLevelProgress);
}

void LocalPlayer::setBounty()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
    ptrNpcStats->setBounty(npcStats.mBounty);
}

void LocalPlayer::setReputation()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
    ptrNpcStats->setReputation(npcStats.mReputation);
}

void LocalPlayer::setPosition()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    // If we're ignoring this position packet because of an invalid cell change,
    // don't make the next one get ignored as well
    if (ignorePosPacket)
        ignorePosPacket = false;
    else
    {
        world->getPlayer().setTeleported(true);

        world->moveObject(ptrPlayer, position.asVec3());
        world->rotateObject(ptrPlayer, position.asRotationVec3());
        world->setInertialForce(ptrPlayer, osg::Vec3f(0.f, 0.f, 0.f));
    }

    updatePosition(true);

    // Make sure we update our draw state, or we'll end up with the wrong one
    updateAnimFlags(true);
}

void LocalPlayer::setMomentum()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();
    world->setInertialForce(ptrPlayer, momentum.asVec3());
}

void LocalPlayer::setCell()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    ESM::Position pos;

    // To avoid crashes, close container windows this player may be in
    closeInventoryWindows();

    // Placed for the first time: out of the void, falling as everyone does
    if (waitingInVoid)
    {
        waitingInVoid = false;
        if (!world->toggleCollisionMode())
            world->toggleCollisionMode();
    }
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    ptrPlayer.getClass().getCreatureStats(ptrPlayer).land(true);

    world->getPlayer().setTeleported(true);

    if (cell.isExterior())
    {
        const ESM::ExteriorCellLocation location(cell.mData.mX, cell.mData.mY, ESM::Cell::sDefaultWorldspaceId);
        const osg::Vec2f corner = ESM::indexToPosition(location, true);
        pos.pos[0] = corner.x();
        pos.pos[1] = corner.y();
        pos.pos[2] = 0;

        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

        world->changeToCell(ESM::RefId::esm3ExteriorCell(location.mX, location.mY), pos, true);
        world->fixPosition();
    }
    else
    {
        ESM::RefId cellId = world->findExteriorPosition(cell.mName, pos);
        if (!cellId.empty())
        {
            world->changeToCell(cellId, pos, true);
            world->fixPosition();
        }
        else
        {
            try
            {
                world->findInteriorPosition(cell.mName, pos);
                world->changeToInteriorCell(cell.mName, pos, true);
            }
            // If we've been sent to an invalid interior, ignore the incoming
            // packet about our position in that cell
            catch (std::exception&)
            {
                LOG_APPEND(TimedLog::LOG_INFO, "%s", "- Cell doesn't exist on this client");
                ignorePosPacket = true;
            }
        }
    }

    updateCell(true);
}

void LocalPlayer::setClass()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_CLASS from server");

    if (charClass.mId.empty()) // custom class
    {
        charClass.mData.mIsPlayable = 0x1;
        MWBase::Environment::get().getMechanicsManager()->setPlayerClass(charClass);
    }
    else
    {
        const ESM::Class *existingCharClass = MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().search(charClass.mId);

        if (existingCharClass)
        {
            MWBase::Environment::get().getMechanicsManager()->setPlayerClass(charClass.mId);
        }
        else
            LOG_APPEND(TimedLog::LOG_INFO, "- Ignored invalid default class %s", idString(charClass.mId).c_str());
    }
}

void LocalPlayer::setEquipment()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWWorld::InventoryStore &ptrInventory = ptrPlayer.getClass().getInventoryStore(ptrPlayer);

    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
    {
        mwmp::Item &currentItem = equipmentItems[slot];

        if (!currentItem.refId.empty())
        {
            auto it = std::find_if(ptrInventory.begin(), ptrInventory.end(), [&currentItem](const MWWorld::Ptr &itemPtr) {
                return itemPtr.getCellRef().getRefId() == currentItem.refId;
            });

            // If the item is not in our inventory, add it as long as it's not a bound item
            if (it == ptrInventory.end())
            {
                try
                {
                    MWWorld::ManualRef itemRef(MWBase::Environment::get().getWorld()->getStore(), refId(currentItem.refId), currentItem.count);

                    if (!MWBase::Environment::get().getMechanicsManager()->isBoundItem(itemRef.getPtr()))
                    {
                        auto addIter = ptrInventory.add(itemRef.getPtr(), currentItem.count, false);

                        ptrInventory.equip(slot, addIter);
                    }
                }
                catch (std::exception&)
                {
                    LOG_APPEND(TimedLog::LOG_INFO, "- Ignored addition of invalid equipment item %s", currentItem.refId.c_str());
                }
            }
            else
            {
                // Don't try to equip an item that is already equipped
                if (ptrInventory.getSlot(slot) != it)
                    ptrInventory.equip(slot, it);
            }
        }
        else
            ptrInventory.unequipSlot(slot);
    }

    MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updatePlayer();
}

void LocalPlayer::setInventory()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::ContainerStore &ptrStore = ptrPlayer.getClass().getContainerStore(ptrPlayer);

    // Ensure no item is being drag and dropped
    MWBase::Environment::get().getWindowManager()->finishDragDrop();

    // Clear items in inventory
    ptrStore.clear();

    // Proceed by adding items
    addItems();

    // Don't automatically setEquipment() here, or the player could end
    // up getting a new set of their starting clothes, or other items
    // supposed to no longer exist
    //
    // Instead, expect server scripts to do that manually
}

void LocalPlayer::setSpellbook()
{
    // What the server says isn't sent back to it
    hooks::ServerChange serverChange;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    // Clear spells in spellbook, while ignoring abilities, powers, etc.
    std::vector<ESM::RefId> spellsToRemove;
    for (const ESM::Spell* spell : ptrSpells)
    {
        if (spell->mData.mType == ESM::Spell::ST_Spell)
            spellsToRemove.push_back(spell->mId);
    }
    for (const ESM::RefId& id : spellsToRemove)
        ptrSpells.remove(id);

    // Proceed by adding spells
    addSpells();
}

void LocalPlayer::setSpellsActive()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::ActiveSpells& activeSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getActiveSpells();
    activeSpells.clear(ptrPlayer);

    // Proceed by adding spells active: they were running already, so quietly
    for (const auto& activeSpell : spellsActiveChanges.activeSpells)
        MechanicsHelper::addActiveSpell(ptrPlayer, activeSpell, true);
}

void LocalPlayer::setCooldowns()
{
    MWBase::World* world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells& ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    for (const auto& cooldown : cooldownChanges)
    {
        if (const ESM::Spell* spell = world->getStore().get<ESM::Spell>().search(refId(cooldown.id)))
            ptrSpells.setPowerUseTimestamp(spell, cooldown.startTimestampDay, static_cast<float>(cooldown.startTimestampHour));
    }
}

void LocalPlayer::setQuickKeys()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_QUICKKEYS from server");

    // So the quick keys menu doesn't send these back
    isReceivingQuickKeys = true;

    for (const auto &quickKey : quickKeyChanges)
    {
        LOG_APPEND(TimedLog::LOG_INFO, "- slot: %i, type: %i, itemId: %s", quickKey.slot, quickKey.type, quickKey.itemId.c_str());

        if (quickKey.type == QuickKey::ITEM || quickKey.type == QuickKey::ITEM_MAGIC)
        {
            MWWorld::InventoryStore &ptrInventory = ptrPlayer.getClass().getInventoryStore(ptrPlayer);

            auto it = std::find_if(ptrInventory.begin(), ptrInventory.end(), [&quickKey](const MWWorld::Ptr &inventoryItem) {
                return inventoryItem.getCellRef().getRefId() == quickKey.itemId;
            });

            if (it != ptrInventory.end())
                MWBase::Environment::get().getWindowManager()->setQuickKey(quickKey.slot, quickKey.type, (*it), ESM::RefId());
        }
        else if (quickKey.type == QuickKey::MAGIC)
        {
            MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

            if (ptrSpells.hasSpell(refId(quickKey.itemId)))
                MWBase::Environment::get().getWindowManager()->setQuickKey(quickKey.slot, quickKey.type, MWWorld::Ptr(), refId(quickKey.itemId));
        }
        else
            MWBase::Environment::get().getWindowManager()->setQuickKey(quickKey.slot, quickKey.type, MWWorld::Ptr(), ESM::RefId());
    }

    isReceivingQuickKeys = false;
}

void LocalPlayer::setFactions()
{
    // What the server says isn't sent back to it
    hooks::ServerChange serverChange;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_FACTION from server - action: %i", factionChanges.action);

    for (const auto &faction : factionChanges.factions)
    {
        LOG_APPEND(TimedLog::LOG_VERBOSE, " - processing faction: %s", faction.factionId.c_str());
        const ESM::RefId factionId = refId(faction.factionId);
        const ESM::Faction *esmFaction = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().search(factionId);

        if (!esmFaction)
        {
            LOG_APPEND(TimedLog::LOG_INFO, "- Ignored invalid faction %s", faction.factionId.c_str());
            continue;
        }

        if (factionChanges.action == mwmp::FactionChanges::RANK)
        {
            if (!ptrNpcStats.isInFaction(factionId))
            {
                // If the player isn't in this faction, make them join it
                ptrNpcStats.joinFaction(factionId);
                LOG_APPEND(TimedLog::LOG_VERBOSE, "\t>JOINED FACTION: %s on rank change to: %d.",
                    faction.factionId.c_str(), faction.rank);
            }

            if (faction.rank != ptrNpcStats.getFactionRank(factionId))
                ptrNpcStats.setFactionRank(factionId, faction.rank);
        }
        else if (factionChanges.action == mwmp::FactionChanges::EXPULSION)
        {
            // If the expelled state is different in the packet than in the NpcStats,
            // adjust the NpcStats accordingly
            if (faction.isExpelled != ptrNpcStats.getExpelled(factionId))
            {
                if (faction.isExpelled)
                    ptrNpcStats.expell(factionId, false);
                else
                    ptrNpcStats.clearExpelled(factionId);
            }
        }

        else if (factionChanges.action == mwmp::FactionChanges::REPUTATION)
            ptrNpcStats.setFactionReputation(factionId, faction.reputation);
    }
}

void LocalPlayer::setBooks()
{
    // What the server says isn't sent back to it
    hooks::ServerChange serverChange;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    for (const auto &book : bookChanges)
        ptrNpcStats.flagAsUsed(refId(book.bookId));
}

void LocalPlayer::setShapeshift()
{
    // What the server says isn't sent back to it
    hooks::ServerChange serverChange;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWBase::Environment::get().getWorld()->scaleObject(ptrPlayer, scale);
    MWBase::Environment::get().getMechanicsManager()->setWerewolf(ptrPlayer, isWerewolf);
}

void LocalPlayer::setMarkLocation()
{
    MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(markCell);

    if (ptrCellStore)
        MWBase::Environment::get().getWorld()->getPlayer().markPosition(ptrCellStore, markPosition);
}

void LocalPlayer::setSelectedSpell()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWMechanics::CreatureStats& stats = ptrPlayer.getClass().getCreatureStats(ptrPlayer);
    MWMechanics::Spells& spells = stats.getSpells();

    // Nothing selected (or a spell this game doesn't have): hasSpell looks the spell up and would throw
    const ESM::RefId spellId = refId(selectedSpellId);
    if (spellId.empty() || !MWBase::Environment::get().getESMStore()->get<ESM::Spell>().search(spellId)
        || !spells.hasSpell(spellId))
        return;

    MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId,
        int(MWMechanics::getSpellSuccessChance(spellId, ptrPlayer)));
}

void LocalPlayer::sendDeath(char newDeathState)
{
    if (MechanicsHelper::isEmptyTarget(killer))
        killer = MechanicsHelper::getTarget(getPlayerPtr());

    deathState = newDeathState;

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_DEATH about myself to server\n- deathState: %d", deathState);
    getNetworking()->getPlayerPacket(ID_PLAYER_DEATH)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_DEATH)->Send();

    MechanicsHelper::clearTarget(killer);
}

void LocalPlayer::sendClass()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const ESM::NPC *npcBase = world->getPlayerPtr().get<ESM::NPC>()->mBase;
    const ESM::Class *esmClass = world->getStore().get<ESM::Class>().find(npcBase->mClass);

    if (isLocalOnlyId(npcBase->mClass)) // custom class
    {
        charClass.mId = ESM::RefId();
        charClass.mName = esmClass->mName;
        charClass.mDescription = esmClass->mDescription;
        charClass.mData = esmClass->mData;
    }
    else
        charClass.mId = esmClass->mId;

    getNetworking()->getPlayerPacket(ID_PLAYER_CHARCLASS)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_CHARCLASS)->Send();
}

void LocalPlayer::sendInventory()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending entire inventory to server");

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::InventoryStore &ptrInventory = ptrPlayer.getClass().getInventoryStore(ptrPlayer);
    mwmp::Item item;

    inventoryChanges.items.clear();

    for (const auto &iter : ptrInventory)
    {
        // Skip any items that somehow have clientside-only dynamic IDs
        if (isLocalOnlyId(iter.getCellRef().getRefId()))
            continue;

        // Skip bound items
        if (MWBase::Environment::get().getMechanicsManager()->isBoundItem(iter))
            continue;

        item.refId = idString(iter.getCellRef().getRefId());

        item.count = iter.getCellRef().getCount();
        item.charge = iter.getCellRef().getCharge();
        item.enchantmentCharge = iter.getCellRef().getEnchantmentCharge();
        item.soul = idString(iter.getCellRef().getSoul());

        inventoryChanges.items.push_back(item);
    }

    inventoryChanges.action = InventoryChanges::SET;
    getNetworking()->getPlayerPacket(ID_PLAYER_INVENTORY)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_INVENTORY)->Send();
}

void LocalPlayer::sendItemChange(const mwmp::Item& item, unsigned int action)
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending item change for %s with action %i, count %i",
        item.refId.c_str(), action, item.count);

    inventoryChanges.items.clear();
    inventoryChanges.items.push_back(item);
    inventoryChanges.action = action;

    getNetworking()->getPlayerPacket(ID_PLAYER_INVENTORY)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_INVENTORY)->Send();
}

void LocalPlayer::sendItemChange(const MWWorld::Ptr& itemPtr, int count, unsigned int action)
{
    mwmp::Item item = MechanicsHelper::getItem(itemPtr, count);
    sendItemChange(item, action);
}

void LocalPlayer::sendItemChange(const ESM::RefId& refId, int count, unsigned int action)
{
    if (isLocalOnlyId(refId))
        return;

    mwmp::Item item;
    item.refId = idString(refId);
    item.count = count;
    item.charge = -1;
    item.enchantmentCharge = -1;
    item.soul = "";

    sendItemChange(item, action);
}

void LocalPlayer::sendStoredItemRemovals()
{
    inventoryChanges.items.clear();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending stored item removals for LocalPlayer:");

    for (auto storedItemRemoval : storedItemRemovals)
    {
        mwmp::Item item;
        item.refId = storedItemRemoval.first;
        item.count = storedItemRemoval.second;
        item.charge = -1;
        item.enchantmentCharge = -1;
        item.soul = "";
        inventoryChanges.items.push_back(item);

        LOG_APPEND(TimedLog::LOG_INFO, "- %s with count %i", item.refId.c_str(), item.count);
    }

    inventoryChanges.action = mwmp::InventoryChanges::ACTION_TYPE::REMOVE;
    getNetworking()->getPlayerPacket(ID_PLAYER_INVENTORY)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_INVENTORY)->Send();

    storedItemRemovals.clear();
}

void LocalPlayer::sendSpellbook()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    spellbookChanges.spells.clear();

    // Send spells in spellbook, while ignoring abilities, powers, etc.
    for (const ESM::Spell* spell : ptrSpells)
    {
        if (spell->mData.mType == ESM::Spell::ST_Spell && !isLocalOnlyId(spell->mId))
            spellbookChanges.spells.push_back(*spell);
    }

    spellbookChanges.action = SpellbookChanges::SET;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->Send();
}

void LocalPlayer::sendSpellChange(const ESM::RefId& id, unsigned int action)
{
    // Skip any bugged spells that somehow have clientside-only dynamic IDs
    if (isLocalOnlyId(id))
        return;

    spellbookChanges.spells.clear();

    ESM::Spell spell;
    spell.mId = id;
    spellbookChanges.spells.push_back(spell);

    spellbookChanges.action = action;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->Send();
}

void LocalPlayer::sendSpellsActive()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::ActiveSpells& activeSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getActiveSpells();

    spellsActiveChanges.activeSpells.clear();

    // Send spells in spellbook, while ignoring abilities, powers, etc.
    for (const MWMechanics::ActiveSpells::ActiveSpellParams& params : activeSpells)
    {
        if (isLocalOnlyId(params.getSourceSpellId()))
            continue;

        mwmp::ActiveSpell packetSpell;
        packetSpell.id = idString(params.getSourceSpellId());
        packetSpell.isStackingSpell = params.hasFlag(ESM::ActiveSpells::Flag_Stackable);
        packetSpell.params.mDisplayName = params.getDisplayName();
        packetSpell.params.mEffects = params.getEffects();
        spellsActiveChanges.activeSpells.push_back(packetSpell);
    }

    spellsActiveChanges.action = mwmp::SpellsActiveChanges::SET;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLS_ACTIVE)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLS_ACTIVE)->Send();
}

void LocalPlayer::sendSpellsActiveAddition(const MWMechanics::ActiveSpells::ActiveSpellParams& params)
{
    // Skip any bugged spells that somehow have clientside-only dynamic IDs
    if (isLocalOnlyId(params.getSourceSpellId()))
        return;

    spellsActiveChanges.activeSpells.clear();

    MWWorld::Ptr caster = MWBase::Environment::get().getWorldModel()->getPtr(params.getCaster());
    MWWorld::TimeStamp timestamp = MWBase::Environment::get().getWorld()->getTimeStamp();

    mwmp::ActiveSpell spell;
    spell.id = idString(params.getSourceSpellId());
    spell.isStackingSpell = params.hasFlag(ESM::ActiveSpells::Flag_Stackable);
    spell.caster = MechanicsHelper::getTarget(caster);
    spell.timestampDay = timestamp.getDay();
    spell.timestampHour = timestamp.getHour();
    spell.params.mEffects = params.getEffects();
    spell.params.mDisplayName = params.getDisplayName();
    spellsActiveChanges.activeSpells.push_back(spell);

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending active spell addition with stacking %s, timestamp %i %f",
        spell.isStackingSpell ? "true" : "false", spell.timestampDay, spell.timestampHour);

    spellsActiveChanges.action = mwmp::SpellsActiveChanges::ADD;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLS_ACTIVE)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLS_ACTIVE)->Send();
}

void LocalPlayer::sendSpellsActiveRemoval(const ESM::RefId& id, bool isStackingSpell)
{
    // Skip any bugged spells that somehow have clientside-only dynamic IDs
    if (isLocalOnlyId(id))
        return;

    spellsActiveChanges.activeSpells.clear();

    MWWorld::TimeStamp timestamp = MWBase::Environment::get().getWorld()->getTimeStamp();

    mwmp::ActiveSpell spell;
    spell.id = idString(id);
    spell.isStackingSpell = isStackingSpell;
    spell.timestampDay = timestamp.getDay();
    spell.timestampHour = timestamp.getHour();
    spellsActiveChanges.activeSpells.push_back(spell);

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending active spell removal with stacking %s",
        spell.isStackingSpell ? "true" : "false");

    spellsActiveChanges.action = mwmp::SpellsActiveChanges::REMOVE;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLS_ACTIVE)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLS_ACTIVE)->Send();
}

void LocalPlayer::sendCooldownChange(const ESM::RefId& id, int startTimestampDay, float startTimestampHour)
{
    // Skip any bugged spells that somehow have clientside-only dynamic IDs
    if (isLocalOnlyId(id))
        return;

    cooldownChanges.clear();

    SpellCooldown spellCooldown;
    spellCooldown.id = idString(id);
    spellCooldown.startTimestampDay = startTimestampDay;
    spellCooldown.startTimestampHour = startTimestampHour;

    cooldownChanges.push_back(spellCooldown);

    getNetworking()->getPlayerPacket(ID_PLAYER_COOLDOWNS)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_COOLDOWNS)->Send();
}

void LocalPlayer::sendQuickKey(unsigned short slot, int type, const ESM::RefId& itemId)
{
    quickKeyChanges.clear();

    mwmp::QuickKey quickKey;
    quickKey.slot = slot;
    quickKey.type = type;
    quickKey.itemId = idString(itemId);

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_QUICKKEYS");
    LOG_APPEND(TimedLog::LOG_INFO, "- slot: %i, type: %i, itemId: %s", quickKey.slot, quickKey.type, quickKey.itemId.c_str());

    quickKeyChanges.push_back(quickKey);

    getNetworking()->getPlayerPacket(ID_PLAYER_QUICKKEYS)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_QUICKKEYS)->Send();
}

void LocalPlayer::sendJournalEntry(const ESM::RefId& quest, int index, const MWWorld::Ptr& actor)
{
    journalChanges.clear();

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::ENTRY;
    journalItem.quest = idString(quest);
    journalItem.index = index;
    journalItem.actorRefId = actor.isEmpty() ? std::string() : idString(actor.getCellRef().getRefId());
    journalItem.hasTimestamp = false;

    journalChanges.push_back(journalItem);

    getNetworking()->getPlayerPacket(ID_PLAYER_JOURNAL)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_JOURNAL)->Send();
}

void LocalPlayer::sendJournalIndex(const ESM::RefId& quest, int index)
{
    journalChanges.clear();

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::INDEX;
    journalItem.quest = idString(quest);
    journalItem.index = index;

    journalChanges.push_back(journalItem);

    getNetworking()->getPlayerPacket(ID_PLAYER_JOURNAL)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_JOURNAL)->Send();
}

void LocalPlayer::sendFactionRank(const ESM::RefId& factionId, int rank)
{
    factionChanges.factions.clear();
    factionChanges.action = FactionChanges::RANK;

    mwmp::Faction faction;
    faction.factionId = idString(factionId);
    faction.rank = rank;

    factionChanges.factions.push_back(faction);

    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->Send();
}

void LocalPlayer::sendFactionExpulsionState(const ESM::RefId& factionId, bool isExpelled)
{
    factionChanges.factions.clear();
    factionChanges.action = FactionChanges::EXPULSION;

    mwmp::Faction faction;
    faction.factionId = idString(factionId);
    faction.isExpelled = isExpelled;

    factionChanges.factions.push_back(faction);

    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->Send();
}

void LocalPlayer::sendFactionReputation(const ESM::RefId& factionId, int reputation)
{
    factionChanges.factions.clear();
    factionChanges.action = FactionChanges::REPUTATION;

    mwmp::Faction faction;
    faction.factionId = idString(factionId);
    faction.reputation = reputation;

    factionChanges.factions.push_back(faction);

    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->Send();
}

void LocalPlayer::sendTopic(const ESM::RefId& topicId)
{
    topicChanges.clear();

    mwmp::Topic topic;

    // TODO: TES3MP translated topics back into English for translated versions of the game (see addTopics)
    topic.topicId = idString(topicId);

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_TOPIC with topic %s", topic.topicId.c_str());

    topicChanges.push_back(topic);

    getNetworking()->getPlayerPacket(ID_PLAYER_TOPIC)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_TOPIC)->Send();
}

void LocalPlayer::sendBook(const ESM::RefId& bookId)
{
    bookChanges.clear();

    mwmp::Book book;
    book.bookId = idString(bookId);

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_BOOK with book %s", book.bookId.c_str());

    bookChanges.push_back(book);

    getNetworking()->getPlayerPacket(ID_PLAYER_BOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_BOOK)->Send();
}

void LocalPlayer::sendWerewolfState(bool werewolfState)
{
    isWerewolf = werewolfState;

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_SHAPESHIFT with isWerewolf of %s", isWerewolf ? "true" : "false");

    getNetworking()->getPlayerPacket(ID_PLAYER_SHAPESHIFT)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SHAPESHIFT)->Send();
}

void LocalPlayer::sendMarkLocation(const ESM::Cell& newMarkCell, const ESM::Position& newMarkPosition)
{
    miscellaneousChangeType = mwmp::MISCELLANEOUS_CHANGE_TYPE::MARK_LOCATION;
    markCell = newMarkCell;
    markPosition = newMarkPosition;

    getNetworking()->getPlayerPacket(ID_PLAYER_MISCELLANEOUS)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_MISCELLANEOUS)->Send();
}

void LocalPlayer::sendSelectedSpell(const ESM::RefId& newSelectedSpellId)
{
    miscellaneousChangeType = mwmp::MISCELLANEOUS_CHANGE_TYPE::SELECTED_SPELL;
    selectedSpellId = idString(newSelectedSpellId);

    getNetworking()->getPlayerPacket(ID_PLAYER_MISCELLANEOUS)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_MISCELLANEOUS)->Send();
}

void LocalPlayer::sendItemUse(const MWWorld::Ptr& itemPtr, bool itemMagicState, char currentDrawState)
{
    usedItem.refId = idString(itemPtr.getCellRef().getRefId());
    usedItem.count = itemPtr.getCellRef().getCount();
    usedItem.charge = itemPtr.getCellRef().getCharge();
    usedItem.enchantmentCharge = itemPtr.getCellRef().getEnchantmentCharge();
    usedItem.soul = idString(itemPtr.getCellRef().getSoul());

    usingItemMagic = itemMagicState;
    itemUseDrawState = currentDrawState;

    getNetworking()->getPlayerPacket(ID_PLAYER_ITEM_USE)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_ITEM_USE)->Send();
}

void LocalPlayer::sendCellStates()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_CELL_STATE to server");
    getNetworking()->getPlayerPacket(ID_PLAYER_CELL_STATE)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_CELL_STATE)->Send();
}

void LocalPlayer::clearCellStates()
{
    cellStateChanges.clear();
}

void LocalPlayer::clearCurrentContainer()
{
    currentContainer.refId = "";
    currentContainer.refNum = 0;
    currentContainer.mpNum = 0;
}

void LocalPlayer::storeCellState(const ESM::Cell& storedCell, int stateType)
{
    std::vector<CellState>::iterator iter;

    for (iter = cellStateChanges.begin(); iter != cellStateChanges.end(); )
    {
        // If there's already a cell state recorded for this particular cell,
        // remove it
        if (storedCell.getShortDescription() == (*iter).cell.getShortDescription())
            iter = cellStateChanges.erase(iter);
        else
            ++iter;
    }

    CellState cellState;
    cellState.cell = storedCell;
    cellState.type = stateType;

    cellStateChanges.push_back(cellState);
}

void LocalPlayer::storeCurrentContainer(const MWWorld::Ptr &container)
{
    currentContainer.refId = idString(container.getCellRef().getRefId());
    currentContainer.refNum = container.getCellRef().getMpRefNum();
    currentContainer.mpNum = container.getCellRef().getMpNum();
}

void LocalPlayer::storeItemRemoval(const ESM::RefId& refId, int count)
{
    if (isLocalOnlyId(refId))
        return;

    const std::string id = idString(refId);
    storedItemRemovals[id] = storedItemRemovals[id] + count;
}

void LocalPlayer::storeLastEnchantmentQuantity(unsigned int quantity)
{
    lastEnchantmentQuantity = quantity;
}

void LocalPlayer::playAnimation()
{
    MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(getPlayerPtr(),
        animation.groupname, animation.mode, animation.count, animation.persist);

    isPlayingAnimation = true;
}

void LocalPlayer::playSpeech()
{
    MWBase::Environment::get().getSoundManager()->say(getPlayerPtr(), VFS::Path::Normalized(sound));

    if (Settings::gui().mSubtitles)
        MWBase::Environment::get().getWindowManager()->messageBox(MechanicsHelper::getVoiceCaption(sound), MWGui::ShowInDialogueMode_Never);
}
