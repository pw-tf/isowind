#include <components/esm3/loadmgef.hpp>
#include <components/openmw-mp/EffectArgs.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseActor.hpp>

#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Utils.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>

#include <components/esm3/creaturestats.hpp>

#include "Actors.hpp"

using namespace mwmp;

BaseActorList *readActorList;
BaseActorList writeActorList;

BaseActor tempActor;
const BaseActor emptyActor = {};
std::vector<ESM::ActiveEffect> storedActorActiveEffects;

static std::string tempCellDescription;

void ActorFunctions::ReadReceivedActorList() noexcept
{
    readActorList = mwmp::Networking::getPtr()->getReceivedActorList();
}

void ActorFunctions::ReadCellActorList(const char* cellDescription) noexcept
{
    ESM::Cell esmCell = Utils::getCellFromDescription(cellDescription);
    Cell *serverCell = CellController::get()->getCell(&esmCell);

    if (serverCell != nullptr)
        readActorList = serverCell->getActorList();
    else
        readActorList = {};
}

void ActorFunctions::ClearActorList() noexcept
{
    writeActorList.cell.blank();
    writeActorList.baseActors.clear();
}

void ActorFunctions::SetActorListPid(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    writeActorList.guid = player->guid;
}

void ActorFunctions::CopyReceivedActorListToStore() noexcept
{
    writeActorList = *readActorList;
}

unsigned int ActorFunctions::GetActorListSize() noexcept
{
    if (readActorList == nullptr)
        return 0;

    return readActorList->count;
}

unsigned char ActorFunctions::GetActorListAction() noexcept
{
    return readActorList->action;
}

const char *ActorFunctions::GetActorCell(unsigned int index) noexcept
{
    tempCellDescription = readActorList->baseActors.at(index).cell.getShortDescription();
    return tempCellDescription.c_str();
}

const char *ActorFunctions::GetActorRefId(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).refId.c_str();
}

unsigned int ActorFunctions::GetActorRefNum(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).refNum;
}

unsigned int ActorFunctions::GetActorMpNum(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).mpNum;
}

double ActorFunctions::GetActorPosX(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).position.pos[0];
}

double ActorFunctions::GetActorPosY(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).position.pos[1];
}

double ActorFunctions::GetActorPosZ(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).position.pos[2];
}

double ActorFunctions::GetActorRotX(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).position.rot[0];
}

double ActorFunctions::GetActorRotY(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).position.rot[1];
}

double ActorFunctions::GetActorRotZ(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).position.rot[2];
}

double ActorFunctions::GetActorHealthBase(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[0].mBase;
}

double ActorFunctions::GetActorHealthCurrent(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[0].mCurrent;
}

double ActorFunctions::GetActorHealthModified(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[0].mMod;
}

double ActorFunctions::GetActorMagickaBase(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[1].mBase;
}

double ActorFunctions::GetActorMagickaCurrent(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[1].mCurrent;
}

double ActorFunctions::GetActorMagickaModified(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[1].mMod;
}

double ActorFunctions::GetActorFatigueBase(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[2].mBase;
}

double ActorFunctions::GetActorFatigueCurrent(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[2].mCurrent;
}

double ActorFunctions::GetActorFatigueModified(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).creatureStats.mDynamic[2].mMod;
}

const char *ActorFunctions::GetActorEquipmentItemRefId(unsigned int index, unsigned short slot) noexcept
{
    return readActorList->baseActors.at(index).equipmentItems[slot].refId.c_str();
}

int ActorFunctions::GetActorEquipmentItemCount(unsigned int index, unsigned short slot) noexcept
{
    return readActorList->baseActors.at(index).equipmentItems[slot].count;
}

int ActorFunctions::GetActorEquipmentItemCharge(unsigned int index, unsigned short slot) noexcept
{
    return readActorList->baseActors.at(index).equipmentItems[slot].charge;
}

double ActorFunctions::GetActorEquipmentItemEnchantmentCharge(unsigned int index, unsigned short slot) noexcept
{
    return readActorList->baseActors.at(index).equipmentItems[slot].enchantmentCharge;
}

bool ActorFunctions::DoesActorHavePlayerKiller(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).killer.isPlayer;
}

int ActorFunctions::GetActorKillerPid(unsigned int index) noexcept
{
    Player *player = Players::getPlayer(readActorList->baseActors.at(index).killer.guid);

    if (player != nullptr)
        return player->getId();

    return -1;
}

int ActorFunctions::GetActorAIAction(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).aiAction;
}

int ActorFunctions::GetActorAITargetPid(unsigned int index) noexcept
{
    const BaseActor& actor = readActorList->baseActors.at(index);
    if (!actor.hasAiTarget || !actor.aiTarget.isPlayer)
        return -1;

    Player *player = Players::getPlayer(actor.aiTarget.guid);
    return player != nullptr ? player->getId() : -1;
}

const char *ActorFunctions::GetActorKillerRefId(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).killer.refId.c_str();
}

unsigned int ActorFunctions::GetActorKillerRefNum(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).killer.refNum;
}

unsigned int ActorFunctions::GetActorKillerMpNum(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).killer.mpNum;
}

const char *ActorFunctions::GetActorKillerName(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).killer.name.c_str();
}

unsigned int ActorFunctions::GetActorDeathState(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).deathState;
}

unsigned int ActorFunctions::GetActorSpellsActiveChangesSize(unsigned int actorIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.size();
}

unsigned int ActorFunctions::GetActorSpellsActiveChangesAction(unsigned int actorIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.action;
}

const char* ActorFunctions::GetActorSpellsActiveId(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).id.c_str();
}

const char* ActorFunctions::GetActorSpellsActiveDisplayName(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).params.mDisplayName.c_str();
}

bool ActorFunctions::GetActorSpellsActiveStackingState(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).isStackingSpell;
}

unsigned int ActorFunctions::GetActorSpellsActiveEffectCount(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).params.mEffects.size();
}

unsigned int ActorFunctions::GetActorSpellsActiveEffectId(unsigned int actorIndex, unsigned int spellIndex, unsigned int effectIndex) noexcept
{
    return ESM::MagicEffect::refIdToIndex(readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).params.mEffects.at(effectIndex).mEffectId);
}

int ActorFunctions::GetActorSpellsActiveEffectArg(unsigned int actorIndex, unsigned int spellIndex, unsigned int effectIndex) noexcept
{
    return mwmp::effectArgToIndex(readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).params.mEffects.at(effectIndex).mArg);
}

double ActorFunctions::GetActorSpellsActiveEffectMagnitude(unsigned int actorIndex, unsigned int spellIndex, unsigned int effectIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).params.mEffects.at(effectIndex).mMagnitude;
}

double ActorFunctions::GetActorSpellsActiveEffectDuration(unsigned int actorIndex, unsigned int spellIndex, unsigned int effectIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).params.mEffects.at(effectIndex).mDuration;
}

double ActorFunctions::GetActorSpellsActiveEffectTimeLeft(unsigned int actorIndex, unsigned int spellIndex, unsigned int effectIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).params.mEffects.at(effectIndex).mTimeLeft;
}

bool ActorFunctions::DoesActorSpellsActiveHavePlayerCaster(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).caster.isPlayer;
}

int ActorFunctions::GetActorSpellsActiveCasterPid(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    Player* caster = Players::getPlayer(readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).caster.guid);

    if (caster != nullptr)
        return caster->getId();

    return -1;
}

const char* ActorFunctions::GetActorSpellsActiveCasterRefId(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).caster.refId.c_str();
}

unsigned int ActorFunctions::GetActorSpellsActiveCasterRefNum(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).caster.refNum;
}

unsigned int ActorFunctions::GetActorSpellsActiveCasterMpNum(unsigned int actorIndex, unsigned int spellIndex) noexcept
{
    return readActorList->baseActors.at(actorIndex).spellsActiveChanges.activeSpells.at(spellIndex).caster.mpNum;
}

bool ActorFunctions::DoesActorHavePosition(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).hasPositionData;
}

bool ActorFunctions::DoesActorHaveStatsDynamic(unsigned int index) noexcept
{
    return readActorList->baseActors.at(index).hasStatsDynamicData;
}

void ActorFunctions::SetActorListCell(const char* cellDescription) noexcept
{
    writeActorList.cell = Utils::getCellFromDescription(cellDescription);
}

void ActorFunctions::SetActorListAction(unsigned char action) noexcept
{
    writeActorList.action = action;
}

void ActorFunctions::SetActorCell(const char* cellDescription) noexcept
{
    tempActor.cell = Utils::getCellFromDescription(cellDescription);
}

void ActorFunctions::SetActorRefId(const char* refId) noexcept
{
    tempActor.refId = refId;
}

void ActorFunctions::SetActorRefNum(int refNum) noexcept
{
    tempActor.refNum = refNum;
}

void ActorFunctions::SetActorMpNum(int mpNum) noexcept
{
    tempActor.mpNum = mpNum;
}

void ActorFunctions::SetActorPosition(double x, double y, double z) noexcept
{
    tempActor.position.pos[0] = x;
    tempActor.position.pos[1] = y;
    tempActor.position.pos[2] = z;
}

void ActorFunctions::SetActorRotation(double x, double y, double z) noexcept
{
    tempActor.position.rot[0] = x;
    tempActor.position.rot[1] = y;
    tempActor.position.rot[2] = z;
}

void ActorFunctions::SetActorHealthBase(double value) noexcept
{
    tempActor.creatureStats.mDynamic[0].mBase = value;
}

void ActorFunctions::SetActorHealthCurrent(double value) noexcept
{
    tempActor.creatureStats.mDynamic[0].mCurrent = value;
}

void ActorFunctions::SetActorHealthModified(double value) noexcept
{
    tempActor.creatureStats.mDynamic[0].mMod = value;
}

void ActorFunctions::SetActorMagickaBase(double value) noexcept
{
    tempActor.creatureStats.mDynamic[1].mBase = value;
}

void ActorFunctions::SetActorMagickaCurrent(double value) noexcept
{
    tempActor.creatureStats.mDynamic[1].mCurrent = value;
}

void ActorFunctions::SetActorMagickaModified(double value) noexcept
{
    tempActor.creatureStats.mDynamic[1].mMod = value;
}

void ActorFunctions::SetActorFatigueBase(double value) noexcept
{
    tempActor.creatureStats.mDynamic[2].mBase = value;
}

void ActorFunctions::SetActorFatigueCurrent(double value) noexcept
{
    tempActor.creatureStats.mDynamic[2].mCurrent = value;
}

void ActorFunctions::SetActorFatigueModified(double value) noexcept
{
    tempActor.creatureStats.mDynamic[2].mMod = value;
}

void ActorFunctions::SetActorSound(const char* sound) noexcept
{
    tempActor.sound = sound;
}

void ActorFunctions::SetActorDeathState(unsigned int deathState) noexcept
{
    tempActor.deathState = deathState;
}

void ActorFunctions::SetActorDeathInstant(bool isInstant) noexcept
{
    tempActor.isInstantDeath = isInstant;
}

void ActorFunctions::SetActorSpellsActiveAction(unsigned char action) noexcept
{
    tempActor.spellsActiveChanges.action = action;
}

void ActorFunctions::SetActorAIAction(unsigned int action) noexcept
{
    tempActor.aiAction = action;
}

void ActorFunctions::SetActorAITargetToPlayer(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    tempActor.hasAiTarget = true;
    tempActor.aiTarget.isPlayer = true;

    tempActor.aiTarget.guid = player->guid;
}

void ActorFunctions::SetActorAITargetToObject(int refNum, int mpNum) noexcept
{
    tempActor.hasAiTarget = true;
    tempActor.aiTarget.isPlayer = false;

    tempActor.aiTarget.refNum = refNum;
    tempActor.aiTarget.mpNum = mpNum;
}

void ActorFunctions::SetActorAICoordinates(double x, double y, double z) noexcept
{
    tempActor.aiCoordinates.pos[0] = x;
    tempActor.aiCoordinates.pos[1] = y;
    tempActor.aiCoordinates.pos[2] = z;
}

void ActorFunctions::SetActorAIDistance(unsigned int distance) noexcept
{
    tempActor.aiDistance = distance;
}

void ActorFunctions::SetActorAIDuration(unsigned int duration) noexcept
{
    tempActor.aiDuration = duration;
}

void ActorFunctions::SetActorAIRepetition(bool shouldRepeat) noexcept
{
    tempActor.aiShouldRepeat = shouldRepeat;
}

void ActorFunctions::EquipActorItem(unsigned short slot, const char *refId, unsigned int count, int charge, double enchantmentCharge) noexcept
{
    tempActor.equipmentItems[slot].refId = refId;
    tempActor.equipmentItems[slot].count = count;
    tempActor.equipmentItems[slot].charge = charge;
    tempActor.equipmentItems[slot].enchantmentCharge = enchantmentCharge;
}

void ActorFunctions::UnequipActorItem(unsigned short slot) noexcept
{
    ActorFunctions::EquipActorItem(slot, "", 0, -1, -1);
}

void ActorFunctions::AddActorSpellActive(const char* spellId, const char* displayName, bool stackingState) noexcept
{
    mwmp::ActiveSpell spell;
    spell.id = spellId;
    spell.isStackingSpell = stackingState;
    spell.params.mDisplayName = displayName;
    spell.params.mEffects = storedActorActiveEffects;

    tempActor.spellsActiveChanges.activeSpells.push_back(spell);

    storedActorActiveEffects.clear();
}

void ActorFunctions::AddActorSpellActiveEffect(int effectId, double magnitude, double duration, double timeLeft, int arg) noexcept
{
    ESM::ActiveEffect effect;
    effect.mEffectId = ESM::MagicEffect::indexToRefId(effectId);
    effect.mMagnitude = magnitude;
    effect.mMinMagnitude = magnitude;
    effect.mMaxMagnitude = magnitude;
    effect.mDuration = duration;
    effect.mTimeLeft = timeLeft;
    effect.mArg = mwmp::effectArgFromIndex(effect.mEffectId, arg);
    effect.mEffectIndex = static_cast<int32_t>(storedActorActiveEffects.size());
    effect.mFlags = ESM::ActiveEffect::Flag_None;

    storedActorActiveEffects.push_back(effect);
}

void ActorFunctions::AddActor() noexcept
{
    writeActorList.baseActors.push_back(tempActor);

    tempActor = emptyActor;
}

void ActorFunctions::SendActorList() noexcept
{
    mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_LIST);
    actorPacket->setActorList(&writeActorList);
    actorPacket->Send(writeActorList.guid);
}

void ActorFunctions::SendActorAuthority() noexcept
{
    Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

    if (serverCell != nullptr)
    {
        serverCell->setAuthority(writeActorList.guid);

        mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_AUTHORITY);
        actorPacket->setActorList(&writeActorList);

        // Always send the packet to everyone on the server, to reduce bugs caused by late-arriving packets
        actorPacket->Send(false);
        actorPacket->Send(true);
    }
}

void ActorFunctions::SendActorPosition(bool sendToOtherVisitors, bool skipAttachedPlayer) noexcept
{
    mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_POSITION);
    actorPacket->setActorList(&writeActorList);

    if (!skipAttachedPlayer)
        actorPacket->Send(writeActorList.guid);

    if (sendToOtherVisitors)
    {
        Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

        if (serverCell != nullptr)
        {
            serverCell->sendToLoaded(actorPacket, &writeActorList);
        }
    }
}

void ActorFunctions::SendActorStatsDynamic(bool sendToOtherVisitors, bool skipAttachedPlayer) noexcept
{
    mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_STATS_DYNAMIC);
    actorPacket->setActorList(&writeActorList);

    if (!skipAttachedPlayer)
        actorPacket->Send(writeActorList.guid);

    if (sendToOtherVisitors)
    {
        Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

        if (serverCell != nullptr)
        {
            serverCell->sendToLoaded(actorPacket, &writeActorList);
        }
    }
}

void ActorFunctions::SendActorEquipment(bool sendToOtherVisitors, bool skipAttachedPlayer) noexcept
{
    mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_EQUIPMENT);
    actorPacket->setActorList(&writeActorList);

    if (!skipAttachedPlayer)
        actorPacket->Send(writeActorList.guid);

    if (sendToOtherVisitors)
    {
        Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

        if (serverCell != nullptr)
        {
            serverCell->sendToLoaded(actorPacket, &writeActorList);
        }
    }
}

void ActorFunctions::SendActorSpellsActiveChanges(bool sendToOtherVisitors, bool skipAttachedPlayer) noexcept
{
    mwmp::ActorPacket* actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_SPELLS_ACTIVE);
    actorPacket->setActorList(&writeActorList);

    if (!skipAttachedPlayer)
        actorPacket->Send(writeActorList.guid);

    if (sendToOtherVisitors)
    {
        Cell* serverCell = CellController::get()->getCell(&writeActorList.cell);

        if (serverCell != nullptr)
        {
            serverCell->sendToLoaded(actorPacket, &writeActorList);
        }
    }
}

void ActorFunctions::SendActorSpeech(bool sendToOtherVisitors, bool skipAttachedPlayer) noexcept
{
    mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_SPEECH);
    actorPacket->setActorList(&writeActorList);

    if (!skipAttachedPlayer)
        actorPacket->Send(writeActorList.guid);

    if (sendToOtherVisitors)
    {
        Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

        if (serverCell != nullptr)
        {
            serverCell->sendToLoaded(actorPacket, &writeActorList);
        }
    }
}

void ActorFunctions::SendActorDeath(bool sendToOtherVisitors, bool skipAttachedPlayer) noexcept
{
    mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_DEATH);
    actorPacket->setActorList(&writeActorList);

    if (!skipAttachedPlayer)
        actorPacket->Send(writeActorList.guid);

    if (sendToOtherVisitors)
    {
        Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

        if (serverCell != nullptr)
        {
            serverCell->sendToLoaded(actorPacket, &writeActorList);
        }
    }
}

void ActorFunctions::SendActorAI(bool sendToOtherVisitors, bool skipAttachedPlayer) noexcept
{
    mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_AI);
    actorPacket->setActorList(&writeActorList);

    if (!skipAttachedPlayer)
        actorPacket->Send(writeActorList.guid);

    if (sendToOtherVisitors)
    {
        Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

        if (serverCell != nullptr)
        {
            serverCell->sendToLoaded(actorPacket, &writeActorList);
        }
    }
}

void ActorFunctions::SendActorCellChange(bool sendToOtherVisitors, bool skipAttachedPlayer) noexcept
{
    mwmp::ActorPacket *actorPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_CELL_CHANGE);
    actorPacket->setActorList(&writeActorList);

    if (!skipAttachedPlayer)
        actorPacket->Send(writeActorList.guid);

    if (sendToOtherVisitors)
    {
        Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

        if (serverCell != nullptr)
        {
            serverCell->sendToLoaded(actorPacket, &writeActorList);
        }
    }
}


// All methods below are deprecated versions of methods from above

void ActorFunctions::ReadLastActorList() noexcept
{
    ReadReceivedActorList();
}

void ActorFunctions::InitializeActorList(unsigned short pid) noexcept
{
    ClearActorList();
    SetActorListPid(pid);
}

void ActorFunctions::CopyLastActorListToStore() noexcept
{
    CopyReceivedActorListToStore();
}

unsigned int ActorFunctions::GetActorRefNumIndex(unsigned int index) noexcept
{
    return GetActorRefNum(index);
}

unsigned int ActorFunctions::GetActorKillerRefNumIndex(unsigned int index) noexcept
{
    return GetActorKillerRefNum(index);
}

void ActorFunctions::SetActorRefNumIndex(int refNum) noexcept
{
    tempActor.refNum = refNum;
}
