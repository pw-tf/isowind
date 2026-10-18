#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseWorldstate.hpp>

#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Script/Functions/Worldstate.hpp>
#include <fstream>

#include <components/esm/attr.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadskil.hpp>

#include "RecordsDynamic.hpp"

using namespace mwmp;

SpellRecord tempSpell;
PotionRecord tempPotion;
EnchantmentRecord tempEnchantment;
ArmorRecord tempArmor;
BookRecord tempBook;
ClothingRecord tempClothing;
MiscellaneousRecord tempMiscellaneous;
WeaponRecord tempWeapon;
ActivatorRecord tempActivator;
ApparatusRecord tempApparatus;
BodyPartRecord tempBodyPart;
CellRecord tempCell;
ContainerRecord tempContainer;
CreatureRecord tempCreature;
DoorRecord tempDoor;
IngredientRecord tempIngredient;
LightRecord tempLight;
LockpickRecord tempLockpick;
NpcRecord tempNpc;
ProbeRecord tempProbe;
RepairRecord tempRepair;
ScriptRecord tempScript;
StaticRecord tempStatic;
SoundRecord tempSound;
GameSettingRecord tempGameSetting;

BaseOverrides tempOverrides;

unsigned int effectCount = 0;
ESM::ENAMstruct tempEffect;
ESM::PartReference tempBodyPartReference;
mwmp::Item tempInventoryItem;

const ESM::EffectList emptyEffectList = {};

const ESM::EffectList& GetRecordEffects(unsigned int recordIndex)
{
    unsigned short recordsType = RecordsDynamicFunctions::GetRecordType();

    if (recordsType == mwmp::RECORD_TYPE::SPELL)
        return WorldstateFunctions::readWorldstate->spellRecords.at(recordIndex).data.mEffects;
    else if (recordsType == mwmp::RECORD_TYPE::POTION)
        return WorldstateFunctions::readWorldstate->potionRecords.at(recordIndex).data.mEffects;
    else if (recordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        return WorldstateFunctions::readWorldstate->enchantmentRecords.at(recordIndex).data.mEffects;

    return emptyEffectList;
}

void RecordsDynamicFunctions::ClearRecords() noexcept
{
    WorldstateFunctions::writeWorldstate.spellRecords.clear();
    WorldstateFunctions::writeWorldstate.potionRecords.clear();
    WorldstateFunctions::writeWorldstate.enchantmentRecords.clear();
    WorldstateFunctions::writeWorldstate.armorRecords.clear();
    WorldstateFunctions::writeWorldstate.bookRecords.clear();
    WorldstateFunctions::writeWorldstate.clothingRecords.clear();
    WorldstateFunctions::writeWorldstate.miscellaneousRecords.clear();
    WorldstateFunctions::writeWorldstate.weaponRecords.clear();
    WorldstateFunctions::writeWorldstate.activatorRecords.clear();
    WorldstateFunctions::writeWorldstate.apparatusRecords.clear();
    WorldstateFunctions::writeWorldstate.bodyPartRecords.clear();
    WorldstateFunctions::writeWorldstate.cellRecords.clear();
    WorldstateFunctions::writeWorldstate.containerRecords.clear();
    WorldstateFunctions::writeWorldstate.creatureRecords.clear();
    WorldstateFunctions::writeWorldstate.doorRecords.clear();
    WorldstateFunctions::writeWorldstate.ingredientRecords.clear();
    WorldstateFunctions::writeWorldstate.lightRecords.clear();
    WorldstateFunctions::writeWorldstate.lockpickRecords.clear();
    WorldstateFunctions::writeWorldstate.npcRecords.clear();
    WorldstateFunctions::writeWorldstate.probeRecords.clear();
    WorldstateFunctions::writeWorldstate.repairRecords.clear();
    WorldstateFunctions::writeWorldstate.scriptRecords.clear();
    WorldstateFunctions::writeWorldstate.staticRecords.clear();
    WorldstateFunctions::writeWorldstate.soundRecords.clear();
    WorldstateFunctions::writeWorldstate.gameSettingRecords.clear();
}

unsigned short RecordsDynamicFunctions::GetRecordType() noexcept
{
    return WorldstateFunctions::readWorldstate->recordsType;
}

unsigned int RecordsDynamicFunctions::GetRecordCount() noexcept
{
    return WorldstateFunctions::readWorldstate->recordsCount;
}

unsigned int RecordsDynamicFunctions::GetRecordEffectCount(unsigned int recordIndex) noexcept
{
    return GetRecordEffects(recordIndex).mList.size();
}

int RecordsDynamicFunctions::GetRecordSubtype(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::SPELL)
        return WorldstateFunctions::readWorldstate->spellRecords.at(index).data.mData.mType;
    else if (readRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        return WorldstateFunctions::readWorldstate->enchantmentRecords.at(index).data.mData.mType;

    return -1;
}

const char *RecordsDynamicFunctions::GetRecordId(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::SPELL)
        return Utils::refIdCStr(WorldstateFunctions::readWorldstate->spellRecords.at(index).data.mId);
    else if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return Utils::refIdCStr(WorldstateFunctions::readWorldstate->potionRecords.at(index).data.mId);
    else if (readRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        return Utils::refIdCStr(WorldstateFunctions::readWorldstate->enchantmentRecords.at(index).data.mId);

    return "invalid";
}

const char *RecordsDynamicFunctions::GetRecordBaseId(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::SPELL)
        return WorldstateFunctions::readWorldstate->spellRecords.at(index).baseId.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return WorldstateFunctions::readWorldstate->potionRecords.at(index).baseId.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        return WorldstateFunctions::readWorldstate->enchantmentRecords.at(index).baseId.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::ARMOR)
        return WorldstateFunctions::readWorldstate->armorRecords.at(index).baseId.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::BOOK)
        return WorldstateFunctions::readWorldstate->bookRecords.at(index).baseId.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        return WorldstateFunctions::readWorldstate->clothingRecords.at(index).baseId.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::WEAPON)
        return WorldstateFunctions::readWorldstate->weaponRecords.at(index).baseId.c_str();

    return "invalid";
}

const char *RecordsDynamicFunctions::GetRecordName(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::SPELL)
        return WorldstateFunctions::readWorldstate->spellRecords.at(index).data.mName.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return WorldstateFunctions::readWorldstate->potionRecords.at(index).data.mName.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::ARMOR)
        return WorldstateFunctions::readWorldstate->armorRecords.at(index).data.mName.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::BOOK)
        return WorldstateFunctions::readWorldstate->bookRecords.at(index).data.mName.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        return WorldstateFunctions::readWorldstate->clothingRecords.at(index).data.mName.c_str();
    else if (readRecordsType == mwmp::RECORD_TYPE::WEAPON)
        return WorldstateFunctions::readWorldstate->weaponRecords.at(index).data.mName.c_str();

    return "invalid";
}

const char *RecordsDynamicFunctions::GetRecordModel(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return WorldstateFunctions::readWorldstate->potionRecords.at(index).data.mModel.c_str();

    return "invalid";
}

const char *RecordsDynamicFunctions::GetRecordIcon(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return WorldstateFunctions::readWorldstate->potionRecords.at(index).data.mIcon.c_str();

    return "invalid";
}

const char *RecordsDynamicFunctions::GetRecordScript(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return Utils::refIdCStr(WorldstateFunctions::readWorldstate->potionRecords.at(index).data.mScript);

    return "invalid";
}

const char *RecordsDynamicFunctions::GetRecordEnchantmentId(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::ARMOR)
        return Utils::refIdCStr(WorldstateFunctions::readWorldstate->armorRecords.at(index).data.mEnchant);
    else if (readRecordsType == mwmp::RECORD_TYPE::BOOK)
        return Utils::refIdCStr(WorldstateFunctions::readWorldstate->bookRecords.at(index).data.mEnchant);
    else if (readRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        return Utils::refIdCStr(WorldstateFunctions::readWorldstate->clothingRecords.at(index).data.mEnchant);
    else if (readRecordsType == mwmp::RECORD_TYPE::WEAPON)
        return Utils::refIdCStr(WorldstateFunctions::readWorldstate->weaponRecords.at(index).data.mEnchant);

    return "invalid";
}

int RecordsDynamicFunctions::GetRecordEnchantmentCharge(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::ARMOR)
        return WorldstateFunctions::readWorldstate->armorRecords.at(index).data.mData.mEnchant;
    else if (readRecordsType == mwmp::RECORD_TYPE::BOOK)
        return WorldstateFunctions::readWorldstate->bookRecords.at(index).data.mData.mEnchant;
    else if (readRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        return WorldstateFunctions::readWorldstate->clothingRecords.at(index).data.mData.mEnchant;
    else if (readRecordsType == mwmp::RECORD_TYPE::WEAPON)
        return WorldstateFunctions::readWorldstate->weaponRecords.at(index).data.mData.mEnchant;

    return -1;
}

int RecordsDynamicFunctions::GetRecordAutoCalc(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return (WorldstateFunctions::readWorldstate->potionRecords.at(index).data.mData.mFlags & ESM::Potion::Autocalc) != 0;

    return -1;
}

int RecordsDynamicFunctions::GetRecordCharge(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        return WorldstateFunctions::readWorldstate->enchantmentRecords.at(index).data.mData.mCharge;

    return -1;
}

int RecordsDynamicFunctions::GetRecordCost(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::SPELL)
        return WorldstateFunctions::readWorldstate->spellRecords.at(index).data.mData.mCost;
    else if (readRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        return WorldstateFunctions::readWorldstate->enchantmentRecords.at(index).data.mData.mCost;

    return -1;
}

int RecordsDynamicFunctions::GetRecordFlags(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::SPELL)
        return WorldstateFunctions::readWorldstate->spellRecords.at(index).data.mData.mFlags;
    else if (readRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        return WorldstateFunctions::readWorldstate->enchantmentRecords.at(index).data.mData.mFlags;

    return -1;
}

int RecordsDynamicFunctions::GetRecordValue(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return WorldstateFunctions::readWorldstate->potionRecords.at(index).data.mData.mValue;

    return -1;
}

double RecordsDynamicFunctions::GetRecordWeight(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return WorldstateFunctions::readWorldstate->potionRecords.at(index).data.mData.mWeight;

    return -1;
}

unsigned int RecordsDynamicFunctions::GetRecordQuantity(unsigned int index) noexcept
{
    unsigned short readRecordsType = RecordsDynamicFunctions::GetRecordType();

    if (readRecordsType == mwmp::RECORD_TYPE::POTION)
        return WorldstateFunctions::readWorldstate->potionRecords.at(index).quantity;
    else if (readRecordsType == mwmp::RECORD_TYPE::WEAPON)
        return WorldstateFunctions::readWorldstate->weaponRecords.at(index).quantity;

    return 1;
}

unsigned int RecordsDynamicFunctions::GetRecordEffectId(unsigned int recordIndex, unsigned int effectIndex) noexcept
{
    return ESM::MagicEffect::refIdToIndex(GetRecordEffects(recordIndex).mList.at(effectIndex).mData.mEffectID);
}

int RecordsDynamicFunctions::GetRecordEffectAttribute(unsigned int recordIndex, unsigned int effectIndex) noexcept
{
    return ESM::Attribute::refIdToIndex(GetRecordEffects(recordIndex).mList.at(effectIndex).mData.mAttribute);
}

int RecordsDynamicFunctions::GetRecordEffectSkill(unsigned int recordIndex, unsigned int effectIndex) noexcept
{
    return ESM::Skill::refIdToIndex(GetRecordEffects(recordIndex).mList.at(effectIndex).mData.mSkill);
}

unsigned int RecordsDynamicFunctions::GetRecordEffectRangeType(unsigned int recordIndex, unsigned int effectIndex) noexcept
{
    return GetRecordEffects(recordIndex).mList.at(effectIndex).mData.mRange;
}

int RecordsDynamicFunctions::GetRecordEffectArea(unsigned int recordIndex, unsigned int effectIndex) noexcept
{
    return GetRecordEffects(recordIndex).mList.at(effectIndex).mData.mArea;
}

int RecordsDynamicFunctions::GetRecordEffectDuration(unsigned int recordIndex, unsigned int effectIndex) noexcept
{
    return GetRecordEffects(recordIndex).mList.at(effectIndex).mData.mDuration;
}

int RecordsDynamicFunctions::GetRecordEffectMagnitudeMax(unsigned int recordIndex, unsigned int effectIndex) noexcept
{
    return GetRecordEffects(recordIndex).mList.at(effectIndex).mData.mMagnMax;
}

int RecordsDynamicFunctions::GetRecordEffectMagnitudeMin(unsigned int recordIndex, unsigned int effectIndex) noexcept
{
    return GetRecordEffects(recordIndex).mList.at(effectIndex).mData.mMagnMin;
}

void RecordsDynamicFunctions::SetRecordType(unsigned int type) noexcept
{
    WorldstateFunctions::writeWorldstate.recordsType = type;
}

void RecordsDynamicFunctions::SetRecordId(const char* id) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
        tempSpell.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        tempEnchantment.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
        tempMiscellaneous.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::ACTIVATOR)
        tempActivator.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
        tempBodyPart.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
        tempContainer.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::DOOR)
        tempDoor.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
        tempIngredient.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::SCRIPT)
        tempScript.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::STATIC)
        tempStatic.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::SOUND)
        tempSound.data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::GAMESETTING)
        tempGameSetting.data.mId = ESM::RefId::stringRefId(id);
    else
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set id for record type %i which lacks that property", writeRecordsType);
}

void RecordsDynamicFunctions::SetRecordBaseId(const char* baseId) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
        tempSpell.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        tempEnchantment.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
        tempMiscellaneous.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ACTIVATOR)
        tempActivator.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
        tempBodyPart.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CELL)
        tempCell.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
        tempContainer.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::DOOR)
        tempDoor.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
        tempIngredient.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::SCRIPT)
        tempScript.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::STATIC)
        tempStatic.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::SOUND)
        tempSound.baseId = baseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::GAMESETTING)
        tempGameSetting.baseId = baseId;
    else
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set baseId for record type %i which lacks that property", writeRecordsType);
}

void RecordsDynamicFunctions::SetRecordInventoryBaseId(const char* inventoryBaseId) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.inventoryBaseId = inventoryBaseId;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.inventoryBaseId = inventoryBaseId;
    else
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set inventoryBaseId for record type %i which lacks that property", writeRecordsType);
}

void RecordsDynamicFunctions::SetRecordSubtype(unsigned int subtype) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
        tempSpell.data.mData.mType = subtype;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        tempEnchantment.data.mData.mType = subtype;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mData.mType = subtype;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mData.mType = subtype;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mData.mType = subtype;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mData.mType = subtype;
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mData.mType = subtype;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
        tempBodyPart.data.mData.mType = subtype;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set subtype for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasSubtype = true;
}

void RecordsDynamicFunctions::SetRecordName(const char* name) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
        tempSpell.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
        tempMiscellaneous.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ACTIVATOR)
        tempActivator.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CELL)
        tempCell.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
        tempContainer.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::DOOR)
        tempDoor.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
        tempIngredient.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mName = name;
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mName = name;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set name for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasName = true;
}

void RecordsDynamicFunctions::SetRecordModel(const char* model) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
        tempMiscellaneous.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ACTIVATOR)
        tempActivator.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
        tempBodyPart.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
        tempContainer.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::DOOR)
        tempDoor.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
        tempIngredient.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mModel = model;
    else if (writeRecordsType == mwmp::RECORD_TYPE::STATIC)
        tempStatic.data.mModel = model;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set model for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasModel = true;
}

void RecordsDynamicFunctions::SetRecordIcon(const char* icon) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
        tempMiscellaneous.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
        tempIngredient.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mIcon = icon;
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mIcon = icon;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set icon for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasIcon = true;
}

void RecordsDynamicFunctions::SetRecordScript(const char* script) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
        tempMiscellaneous.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::ACTIVATOR)
        tempActivator.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
        tempContainer.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::DOOR)
        tempDoor.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
        tempIngredient.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mScript = ESM::RefId::stringRefId(script);
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mScript = ESM::RefId::stringRefId(script);
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set script for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasScript = true;
}

void RecordsDynamicFunctions::SetRecordEnchantmentId(const char* enchantmentId) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mEnchant = ESM::RefId::stringRefId(enchantmentId);
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mEnchant = ESM::RefId::stringRefId(enchantmentId);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mEnchant = ESM::RefId::stringRefId(enchantmentId);
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mEnchant = ESM::RefId::stringRefId(enchantmentId);
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set enchantment id for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasEnchantmentId = true;
}

void RecordsDynamicFunctions::SetRecordEnchantmentCharge(int enchantmentCharge) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mData.mEnchant = enchantmentCharge;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mData.mEnchant = enchantmentCharge;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mData.mEnchant = enchantmentCharge;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mData.mEnchant = enchantmentCharge;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set enchantment charge for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasEnchantmentCharge = true;
}

void RecordsDynamicFunctions::SetRecordAutoCalc(int autoCalc) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
    {
        if (autoCalc)
            tempPotion.data.mData.mFlags |= ESM::Potion::Autocalc;
        else
            tempPotion.data.mData.mFlags &= ~ESM::Potion::Autocalc;
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
    {
        if (autoCalc)
        {
            tempNpc.data.mFlags |= ESM::NPC::Autocalc;
            tempNpc.data.mNpdtType = ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS;
        }
        else
        {
            tempNpc.data.mFlags &= ~ESM::NPC::Autocalc;
            tempNpc.data.mNpdtType = ESM::NPC::NPC_DEFAULT;
        }
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set autoCalc for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasAutoCalc = true;
}

void RecordsDynamicFunctions::SetRecordCharge(int charge) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        tempEnchantment.data.mData.mCharge = charge;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set charge for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasCharge = true;
}

void RecordsDynamicFunctions::SetRecordCost(int cost) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
        tempSpell.data.mData.mCost = cost;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        tempEnchantment.data.mData.mCost = cost;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set cost for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasCost = true;
}

void RecordsDynamicFunctions::SetRecordFlags(int flags) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
        tempSpell.data.mData.mFlags = flags;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        tempEnchantment.data.mData.mFlags = flags;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mData.mFlags = flags;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
        tempBodyPart.data.mData.mFlags = flags;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
        tempContainer.data.mFlags = flags;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mFlags = flags;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mData.mFlags = flags;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mFlags = flags;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set flags for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasFlags = true;
}

void RecordsDynamicFunctions::SetRecordValue(int value) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
        tempMiscellaneous.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
        tempIngredient.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mData.mValue = value;
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mData.mValue = value;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set value for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasValue = true;
}

void RecordsDynamicFunctions::SetRecordWeight(double weight) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
        tempMiscellaneous.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
        tempContainer.data.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
        tempIngredient.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mData.mWeight = weight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mData.mWeight = weight;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set weight for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasWeight = true;
}

void RecordsDynamicFunctions::SetRecordQuality(double quality) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
        tempApparatus.data.mData.mQuality = quality;
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mData.mQuality = quality;
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mData.mQuality = quality;
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mData.mQuality = quality;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set quality for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasQuality = true;
}

void RecordsDynamicFunctions::SetRecordUses(int uses) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
        tempLockpick.data.mData.mUses = uses;
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
        tempProbe.data.mData.mUses = uses;
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
        tempRepair.data.mData.mUses = uses;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set number of uses for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasUses = true;
}

void RecordsDynamicFunctions::SetRecordTime(int time) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mData.mTime = time;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set time for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasTime = true;
}

void RecordsDynamicFunctions::SetRecordRadius(int radius) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mData.mRadius = radius;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set radius for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasRadius = true;
}

void RecordsDynamicFunctions::SetRecordColor(unsigned int red, unsigned int green, unsigned int blue) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mData.mColor = red + (green << 8) + (blue << 16);
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set color for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasColor = true;
}

void RecordsDynamicFunctions::SetRecordArmorRating(int armorRating) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mData.mArmor = armorRating;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set armor rating for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasArmorRating = true;
}

void RecordsDynamicFunctions::SetRecordHealth(int health) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mData.mHealth = health;
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mData.mHealth = health;
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mData.mHealth = health;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mNpdt.mHealth = health;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set health for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasHealth = true;
}

void RecordsDynamicFunctions::SetRecordDamageChop(unsigned int minDamage, unsigned int maxDamage) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
    {
        tempWeapon.data.mData.mChop[0] = minDamage;
        tempWeapon.data.mData.mChop[1] = maxDamage;
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
    {
        tempCreature.data.mData.mAttack[0] = minDamage;
        tempCreature.data.mData.mAttack[1] = maxDamage;
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set chop damage for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasDamageChop = true;
}

void RecordsDynamicFunctions::SetRecordDamageSlash(unsigned int minDamage, unsigned int maxDamage) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
    {
        tempWeapon.data.mData.mSlash[0] = minDamage;
        tempWeapon.data.mData.mSlash[1] = maxDamage;
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
    {
        tempCreature.data.mData.mAttack[2] = minDamage;
        tempCreature.data.mData.mAttack[3] = maxDamage;
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set slash damage for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasDamageSlash = true;
}

void RecordsDynamicFunctions::SetRecordDamageThrust(unsigned int minDamage, unsigned int maxDamage) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
    {
        tempWeapon.data.mData.mThrust[0] = minDamage;
        tempWeapon.data.mData.mThrust[1] = maxDamage;
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
    {
        tempCreature.data.mData.mAttack[4] = minDamage;
        tempCreature.data.mData.mAttack[5] = maxDamage;
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set thrust damage for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasDamageThrust = true;
}

void RecordsDynamicFunctions::SetRecordReach(double reach) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mData.mReach = reach;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set reach for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasReach = true;
}

void RecordsDynamicFunctions::SetRecordSpeed(double speed) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        tempWeapon.data.mData.mSpeed = speed;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set speed for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasSpeed = true;
}

void RecordsDynamicFunctions::SetRecordKeyState(bool keyState) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
    {
        if (keyState)
            tempMiscellaneous.data.mData.mFlags |= ESM::Miscellaneous::Key;
        else
            tempMiscellaneous.data.mData.mFlags &= ~ESM::Miscellaneous::Key;
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set key state for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasKeyState = true;
}

void RecordsDynamicFunctions::SetRecordScrollState(bool scrollState) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mData.mIsScroll = scrollState;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set scroll state for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasScrollState = true;
}

void RecordsDynamicFunctions::SetRecordSkillId(int skillId) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mData.mSkillId = skillId;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set skill id for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasSkillId = true;
}

void RecordsDynamicFunctions::SetRecordText(const char* text) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        tempBook.data.mText = text;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set text for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasText = true;
}

void RecordsDynamicFunctions::SetRecordHair(const char* hair) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mHair = ESM::RefId::stringRefId(hair);
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set hair for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasHair = true;
}

void RecordsDynamicFunctions::SetRecordHead(const char* head) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mHead = ESM::RefId::stringRefId(head);
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set head for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasHead = true;
}

void RecordsDynamicFunctions::SetRecordGender(unsigned int gender) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
    {
        tempNpc.data.setIsMale(gender);
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set gender for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasGender = true;
}

void RecordsDynamicFunctions::SetRecordRace(const char* race) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
        tempBodyPart.data.mRace = ESM::RefId::stringRefId(race);
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mRace = ESM::RefId::stringRefId(race);
    else
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set race for record type %i which lacks that property", writeRecordsType);

    tempOverrides.hasRace = true;
}

void RecordsDynamicFunctions::SetRecordClass(const char* charClass) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mClass = ESM::RefId::stringRefId(charClass);
    else
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set character class for record type %i which lacks that property", writeRecordsType);
}

void RecordsDynamicFunctions::SetRecordFaction(const char* faction) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mFaction = ESM::RefId::stringRefId(faction);
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set faction for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasFaction = true;
}

void RecordsDynamicFunctions::SetRecordScale(double scale) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mScale = scale;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set level for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasScale = true;
}

void RecordsDynamicFunctions::SetRecordBloodType(int bloodType) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mBloodType = bloodType;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set blood type for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasBloodType = true;
}

void RecordsDynamicFunctions::SetRecordVampireState(bool vampireState) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
        tempBodyPart.data.mData.mVampire = vampireState;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set vampire state for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasVampireState = true;
}

void RecordsDynamicFunctions::SetRecordLevel(int level) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mData.mLevel = level;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mNpdt.mLevel = level;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set level for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasLevel = true;
}

void RecordsDynamicFunctions::SetRecordMagicka(int magicka) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mData.mMana = magicka;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mNpdt.mMana = magicka;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set magicka for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasMagicka = true;
}

void RecordsDynamicFunctions::SetRecordFatigue(int fatigue) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mData.mFatigue = fatigue;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mNpdt.mFatigue = fatigue;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set fatigue for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasFatigue = true;
}

void RecordsDynamicFunctions::SetRecordSoulValue(int soulValue) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mData.mSoul = soulValue;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set soul value for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasSoulValue = true;
}

void RecordsDynamicFunctions::SetRecordAIFight(int aiFight) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mAiData.mFight = aiFight;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mAiData.mFight = aiFight;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set AI fight for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasAiFight = true;
}

void RecordsDynamicFunctions::SetRecordAIFlee(int aiFlee) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mAiData.mFlee = aiFlee;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mAiData.mFlee = aiFlee;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set AI fight for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasAiFlee = true;
}

void RecordsDynamicFunctions::SetRecordAIAlarm(int aiAlarm) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mAiData.mAlarm = aiAlarm;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mAiData.mAlarm = aiAlarm;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set AI fight for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasAiAlarm = true;
}

void RecordsDynamicFunctions::SetRecordAIServices(int aiServices) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.data.mAiData.mServices = aiServices;
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.data.mAiData.mServices = aiServices;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set AI services for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasAiServices = true;
}

void RecordsDynamicFunctions::SetRecordSound(const char* sound) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
        tempLight.data.mSound = ESM::RefId::stringRefId(sound);
    else if (writeRecordsType == mwmp::RECORD_TYPE::SOUND)
        tempSound.data.mSound = sound;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set sound for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasSound = true;
}

void RecordsDynamicFunctions::SetRecordVolume(double volume) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SOUND)
        tempSound.data.mData.mVolume = volume;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set sound for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasVolume = true;
}

void RecordsDynamicFunctions::SetRecordMinRange(double minRange) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SOUND)
        tempSound.data.mData.mMinRange = minRange;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set sound for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasMinRange = true;
}

void RecordsDynamicFunctions::SetRecordMaxRange(double maxRange) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SOUND)
        tempSound.data.mData.mMinRange = maxRange;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set sound for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasMaxRange = true;
}

void RecordsDynamicFunctions::SetRecordOpenSound(const char* sound) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::DOOR)
        tempDoor.data.mOpenSound = ESM::RefId::stringRefId(sound);
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set open sound for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasOpenSound = true;
}

void RecordsDynamicFunctions::SetRecordCloseSound(const char* sound) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::DOOR)
        tempDoor.data.mCloseSound = ESM::RefId::stringRefId(sound);
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set close sound for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasCloseSound = true;
}

void RecordsDynamicFunctions::SetRecordScriptText(const char* scriptText) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SCRIPT)
        tempScript.data.mScriptText = scriptText;
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set script text for record type %i which lacks that property", writeRecordsType);
        return;
    }

    tempOverrides.hasScriptText = true;
}

void RecordsDynamicFunctions::SetRecordIntegerVariable(int intVar) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::GAMESETTING)
    {
        tempGameSetting.variable.variableType = mwmp::INT;
        tempGameSetting.variable.intValue = intVar;
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set integer variable for record type %i which lacks that property", writeRecordsType);
        return;
    }
}

void RecordsDynamicFunctions::SetRecordFloatVariable(double floatVar) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::GAMESETTING)
    {
        tempGameSetting.variable.variableType = mwmp::FLOAT;
        tempGameSetting.variable.floatValue = floatVar;
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set float variable for record type %i which lacks that property", writeRecordsType);
        return;
    }
}

void RecordsDynamicFunctions::SetRecordStringVariable(const char* stringVar) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::GAMESETTING)
    {
        tempGameSetting.variable.variableType = mwmp::STRING;
        tempGameSetting.variable.stringValue = stringVar;
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set string variable for record type %i which lacks that property", writeRecordsType);
        return;
    }
}

void RecordsDynamicFunctions::SetRecordIdByIndex(unsigned int index, const char* id) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
        WorldstateFunctions::writeWorldstate.spellRecords.at(index).data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        WorldstateFunctions::writeWorldstate.potionRecords.at(index).data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        WorldstateFunctions::writeWorldstate.enchantmentRecords.at(index).data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        WorldstateFunctions::writeWorldstate.armorRecords.at(index).data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        WorldstateFunctions::writeWorldstate.bookRecords.at(index).data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        WorldstateFunctions::writeWorldstate.clothingRecords.at(index).data.mId = ESM::RefId::stringRefId(id);
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        WorldstateFunctions::writeWorldstate.weaponRecords.at(index).data.mId = ESM::RefId::stringRefId(id);
    else
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set id for record type %i which lacks that property", writeRecordsType);
}

void RecordsDynamicFunctions::SetRecordEnchantmentIdByIndex(unsigned int index, const char* enchantmentId) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        WorldstateFunctions::writeWorldstate.armorRecords.at(index).data.mEnchant = ESM::RefId::stringRefId(enchantmentId);
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
        WorldstateFunctions::writeWorldstate.bookRecords.at(index).data.mEnchant = ESM::RefId::stringRefId(enchantmentId);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        WorldstateFunctions::writeWorldstate.clothingRecords.at(index).data.mEnchant = ESM::RefId::stringRefId(enchantmentId);
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
        WorldstateFunctions::writeWorldstate.weaponRecords.at(index).data.mEnchant = ESM::RefId::stringRefId(enchantmentId);
    else
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Tried to set enchantmentId for record type %i which lacks that property", writeRecordsType);
}

void RecordsDynamicFunctions::SetRecordEffectId(unsigned int effectId) noexcept
{
    tempEffect.mEffectID = ESM::MagicEffect::indexToRefId(effectId);
}

void RecordsDynamicFunctions::SetRecordEffectAttribute(int attributeId) noexcept
{
    tempEffect.mAttribute = ESM::Attribute::indexToRefId(attributeId);
}

void RecordsDynamicFunctions::SetRecordEffectSkill(int skillId) noexcept
{
    tempEffect.mSkill = ESM::Skill::indexToRefId(skillId);
}

void RecordsDynamicFunctions::SetRecordEffectRangeType(unsigned int rangeType) noexcept
{
    tempEffect.mRange = rangeType;
}

void RecordsDynamicFunctions::SetRecordEffectArea(int area) noexcept
{
    tempEffect.mArea = area;
}

void RecordsDynamicFunctions::SetRecordEffectDuration(int duration) noexcept
{
    tempEffect.mDuration = duration;
}

void RecordsDynamicFunctions::SetRecordEffectMagnitudeMax(int magnitudeMax) noexcept
{
    tempEffect.mMagnMax = magnitudeMax;
}

void RecordsDynamicFunctions::SetRecordEffectMagnitudeMin(int magnitudeMin) noexcept
{
    tempEffect.mMagnMin = magnitudeMin;
}

void RecordsDynamicFunctions::SetRecordBodyPartType(unsigned int partType) noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
    {
        tempBodyPart.data.mData.mPart = partType;
        tempOverrides.hasBodyPartType = true;
    }
    else
        tempBodyPartReference.mPart = partType;
}

void RecordsDynamicFunctions::SetRecordBodyPartIdForMale(const char* partId) noexcept
{
    tempBodyPartReference.mMale = ESM::RefId::stringRefId(partId);
}

void RecordsDynamicFunctions::SetRecordBodyPartIdForFemale(const char* partId) noexcept
{
    tempBodyPartReference.mFemale = ESM::RefId::stringRefId(partId);
}

void RecordsDynamicFunctions::SetRecordInventoryItemId(const char* itemId) noexcept
{
    tempInventoryItem.refId = itemId;
}

void RecordsDynamicFunctions::SetRecordInventoryItemCount(unsigned int count) noexcept
{
    tempInventoryItem.count = count;
}

void RecordsDynamicFunctions::AddRecord() noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
    {
        tempSpell.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.spellRecords.push_back(tempSpell);
        tempSpell = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
    {
        tempPotion.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.potionRecords.push_back(tempPotion);
        tempPotion = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
    {
        tempEnchantment.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.enchantmentRecords.push_back(tempEnchantment);
        tempEnchantment = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
    {
        tempArmor.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.armorRecords.push_back(tempArmor);
        tempArmor = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::BOOK)
    {
        tempBook.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.bookRecords.push_back(tempBook);
        tempBook = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
    {
        tempClothing.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.clothingRecords.push_back(tempClothing);
        tempClothing = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
    {
        tempMiscellaneous.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.miscellaneousRecords.push_back(tempMiscellaneous);
        tempMiscellaneous = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::WEAPON)
    {
        tempWeapon.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.weaponRecords.push_back(tempWeapon);
        tempWeapon = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::ACTIVATOR)
    {
        tempActivator.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.activatorRecords.push_back(tempActivator);
        tempActivator = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::APPARATUS)
    {
        tempApparatus.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.apparatusRecords.push_back(tempApparatus);
        tempApparatus = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::BODYPART)
    {
        tempBodyPart.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.bodyPartRecords.push_back(tempBodyPart);
        tempBodyPart = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::CELL)
    {
        tempCell.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.cellRecords.push_back(tempCell);
        tempCell = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
    {
        tempContainer.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.containerRecords.push_back(tempContainer);
        tempContainer = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
    {
        tempCreature.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.creatureRecords.push_back(tempCreature);
        tempCreature = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::DOOR)
    {
        tempDoor.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.doorRecords.push_back(tempDoor);
        tempDoor = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
    {
        tempIngredient.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.ingredientRecords.push_back(tempIngredient);
        tempIngredient = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::LIGHT)
    {
        tempLight.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.lightRecords.push_back(tempLight);
        tempLight = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::LOCKPICK)
    {
        tempLockpick.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.lockpickRecords.push_back(tempLockpick);
        tempLockpick = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
    {
        tempNpc.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.npcRecords.push_back(tempNpc);
        tempNpc = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::PROBE)
    {
        tempProbe.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.probeRecords.push_back(tempProbe);
        tempProbe = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::REPAIR)
    {
        tempRepair.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.repairRecords.push_back(tempRepair);
        tempRepair = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::SCRIPT)
    {
        tempScript.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.scriptRecords.push_back(tempScript);
        tempScript = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::STATIC)
    {
        tempStatic.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.staticRecords.push_back(tempStatic);
        tempStatic = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::SOUND)
    {
        tempSound.baseOverrides = tempOverrides;
        WorldstateFunctions::writeWorldstate.soundRecords.push_back(tempSound);
        tempSound = {};
    }
    else if (writeRecordsType == mwmp::RECORD_TYPE::GAMESETTING)
    {
        WorldstateFunctions::writeWorldstate.gameSettingRecords.push_back(tempGameSetting);
        tempGameSetting = {};
    }

    effectCount = 0;
    tempOverrides = {};
}

void RecordsDynamicFunctions::AddRecordEffect() noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::SPELL)
        tempSpell.data.mEffects.mList.push_back({ tempEffect, static_cast<uint32_t>(tempSpell.data.mEffects.mList.size()) });
    else if (writeRecordsType == mwmp::RECORD_TYPE::POTION)
        tempPotion.data.mEffects.mList.push_back({ tempEffect, static_cast<uint32_t>(tempPotion.data.mEffects.mList.size()) });
    else if (writeRecordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
        tempEnchantment.data.mEffects.mList.push_back({ tempEffect, static_cast<uint32_t>(tempEnchantment.data.mEffects.mList.size()) });
    else if (writeRecordsType == mwmp::RECORD_TYPE::INGREDIENT)
    {
        const static unsigned int effectCap = sizeof(tempIngredient.data.mData.mEffectID) / sizeof(tempIngredient.data.mData.mEffectID[0]);

        if (effectCount < effectCap)
        {
            tempIngredient.data.mData.mEffectID[effectCount] = tempEffect.mEffectID;
            tempIngredient.data.mData.mAttributes[effectCount] = tempEffect.mAttribute;
            tempIngredient.data.mData.mSkills[effectCount] = tempEffect.mSkill;
        }
        else
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Could not add record effect to temporary ingredient record because the cap of %i effects has been reached",
                effectCap);
        }
    }

    tempOverrides.hasEffects = true;
    effectCount++;
    tempEffect = {};
}

void RecordsDynamicFunctions::AddRecordBodyPart() noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::ARMOR)
        tempArmor.data.mParts.mParts.push_back(tempBodyPartReference);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CLOTHING)
        tempClothing.data.mParts.mParts.push_back(tempBodyPartReference);

    tempOverrides.hasBodyParts = true;
    tempBodyPart = {};
}

void RecordsDynamicFunctions::AddRecordInventoryItem() noexcept
{
    unsigned short writeRecordsType = WorldstateFunctions::writeWorldstate.recordsType;

    if (writeRecordsType == mwmp::RECORD_TYPE::CONTAINER)
        tempContainer.inventory.push_back(tempInventoryItem);
    else if (writeRecordsType == mwmp::RECORD_TYPE::CREATURE)
        tempCreature.inventory.push_back(tempInventoryItem);
    else if (writeRecordsType == mwmp::RECORD_TYPE::NPC)
        tempNpc.inventory.push_back(tempInventoryItem);

    tempOverrides.hasInventory = true;
    tempInventoryItem = {};
}

void RecordsDynamicFunctions::SendRecordDynamic(unsigned short pid, bool sendToOtherPlayers, bool skipAttachedPlayer) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    WorldstateFunctions::writeWorldstate.guid = player->guid;

    mwmp::WorldstatePacket *packet = mwmp::Networking::get().getWorldstatePacketController()->GetPacket(ID_RECORD_DYNAMIC);
    packet->setWorldstate(&WorldstateFunctions::writeWorldstate);

    if (!skipAttachedPlayer)
        packet->Send(false);
    if (sendToOtherPlayers)
        packet->Send(true);
}
