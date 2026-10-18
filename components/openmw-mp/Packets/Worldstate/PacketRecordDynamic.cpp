#include "PacketRecordDynamic.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>

#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Utils.hpp>

using namespace mwmp;

PacketRecordDynamic::PacketRecordDynamic(RakNet::RakPeerInterface *peer) : WorldstatePacket(peer)
{
    packetID = ID_RECORD_DYNAMIC;
    orderChannel = CHANNEL_WORLDSTATE;
}

void PacketRecordDynamic::Packet(RakNet::BitStream *newBitstream, bool send)
{
    WorldstatePacket::Packet(newBitstream, send);

    RW(worldstate->recordsType, send);

    if (send)
    {
        // These can be created by players through gameplay and should be checked first
        if (worldstate->recordsType == mwmp::RECORD_TYPE::SPELL)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->spellRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::POTION)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->potionRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->enchantmentRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::ARMOR)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->armorRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::BOOK)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->bookRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::CLOTHING)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->clothingRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->miscellaneousRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::WEAPON)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->weaponRecords);
        // These can only be created by the server
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::ACTIVATOR)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->activatorRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::APPARATUS)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->apparatusRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::BODYPART)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->bodyPartRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::CELL)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->cellRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::CONTAINER)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->containerRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::CREATURE)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->creatureRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::DOOR)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->doorRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::GAMESETTING)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->gameSettingRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::INGREDIENT)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->ingredientRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::LIGHT)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->lightRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::LOCKPICK)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->lockpickRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::NPC)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->npcRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::PROBE)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->probeRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::REPAIR)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->repairRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::SCRIPT)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->scriptRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::STATIC)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->staticRecords);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::SOUND)
            worldstate->recordsCount = Utils::getVectorSize(worldstate->soundRecords);
        else
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Processed invalid ID_RECORD_DYNAMIC packet about unimplemented recordsType %i",
                worldstate->recordsType);
            return;
        }
    }

    RW(worldstate->recordsCount, send);

    if (worldstate->recordsCount > maxRecords)
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Processed invalid ID_RECORD_DYNAMIC packet with %i records, above the maximum of %i",
            worldstate->recordsCount, maxRecords);
        LOG_APPEND(TimedLog::LOG_ERROR, "- The packet was ignored after that point");
        return;
    }

    if (!send)
    {
        if (worldstate->recordsType == mwmp::RECORD_TYPE::SPELL)
            Utils::resetVector(worldstate->spellRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::POTION)
            Utils::resetVector(worldstate->potionRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
            Utils::resetVector(worldstate->enchantmentRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::ARMOR)
            Utils::resetVector(worldstate->armorRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::BOOK)
            Utils::resetVector(worldstate->bookRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::CLOTHING)
            Utils::resetVector(worldstate->clothingRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
            Utils::resetVector(worldstate->miscellaneousRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::WEAPON)
            Utils::resetVector(worldstate->weaponRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::ACTIVATOR)
            Utils::resetVector(worldstate->activatorRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::APPARATUS)
            Utils::resetVector(worldstate->apparatusRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::BODYPART)
            Utils::resetVector(worldstate->bodyPartRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::CELL)
            Utils::resetVector(worldstate->cellRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::CONTAINER)
            Utils::resetVector(worldstate->containerRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::CREATURE)
            Utils::resetVector(worldstate->creatureRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::DOOR)
            Utils::resetVector(worldstate->doorRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::GAMESETTING)
            Utils::resetVector(worldstate->gameSettingRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::INGREDIENT)
            Utils::resetVector(worldstate->ingredientRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::LOCKPICK)
            Utils::resetVector(worldstate->lockpickRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::LIGHT)
            Utils::resetVector(worldstate->lightRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::NPC)
            Utils::resetVector(worldstate->npcRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::PROBE)
            Utils::resetVector(worldstate->probeRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::REPAIR)
            Utils::resetVector(worldstate->repairRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::SCRIPT)
            Utils::resetVector(worldstate->scriptRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::SOUND)
            Utils::resetVector(worldstate->soundRecords, worldstate->recordsCount);
        else if (worldstate->recordsType == mwmp::RECORD_TYPE::STATIC)
            Utils::resetVector(worldstate->staticRecords, worldstate->recordsCount);
    }

    if (worldstate->recordsType == mwmp::RECORD_TYPE::SPELL)
    {
        for (auto &&record : worldstate->spellRecords)
        {
            auto &&recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mData.mType, send);
            RW(recordData.mData.mCost, send);
            RW(recordData.mData.mFlags, send);
            ProcessEffects(recordData.mEffects, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasSubtype, send);
                RW(overrides.hasCost, send);
                RW(overrides.hasFlags, send);
                RW(overrides.hasEffects, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::POTION)
    {
        for (auto &&record : worldstate->potionRecords)
        {
            auto &recordData = record.data;

            RW(record.quantity, send);
            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mWeight, send);
            RW(recordData.mData.mValue, send);
            RW(recordData.mData.mFlags, send); // TES3MP's mAutoCalc: OpenMW 0.51 keeps it as flag 0x1
            RW(recordData.mScript, send, true);
            ProcessEffects(recordData.mEffects, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasAutoCalc, send);
                RW(overrides.hasScript, send);
                RW(overrides.hasEffects, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
    {
        for (auto &&record : worldstate->enchantmentRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mData.mType, send);
            RW(recordData.mData.mCost, send);
            RW(recordData.mData.mCharge, send);
            RW(recordData.mData.mFlags, send);
            ProcessEffects(recordData.mEffects, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasSubtype, send);
                RW(overrides.hasCost, send);
                RW(overrides.hasCharge, send);
                RW(overrides.hasFlags, send);
                RW(overrides.hasEffects, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::ARMOR)
    {
        for (auto &&record : worldstate->armorRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mType, send);
            RW(recordData.mData.mWeight, send);
            RW(recordData.mData.mValue, send);
            RW(recordData.mData.mHealth, send);
            RW(recordData.mData.mArmor, send);
            RW(recordData.mData.mEnchant, send);
            RW(recordData.mEnchant, send, true);
            RW(recordData.mScript, send, true);
            ProcessBodyParts(recordData.mParts, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasSubtype, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasHealth, send);
                RW(overrides.hasArmorRating, send);
                RW(overrides.hasEnchantmentCharge, send);
                RW(overrides.hasEnchantmentId, send);
                RW(overrides.hasScript, send);
                RW(overrides.hasBodyParts, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::BOOK)
    {
        for (auto &&record : worldstate->bookRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mText, send, true);
            RW(recordData.mData.mWeight, send);
            RW(recordData.mData.mValue, send);
            RW(recordData.mData.mIsScroll, send);
            RW(recordData.mData.mSkillId, send);
            RW(recordData.mData.mEnchant, send);
            RW(recordData.mEnchant, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasText, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasScrollState, send);
                RW(overrides.hasSkillId, send);
                RW(overrides.hasEnchantmentCharge, send);
                RW(overrides.hasEnchantmentId, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::CLOTHING)
    {
        for (auto &&record : worldstate->clothingRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mType, send);
            RW(recordData.mData.mWeight, send);
            RW(recordData.mData.mValue, send);
            RW(recordData.mData.mEnchant, send);
            RW(recordData.mEnchant, send, true);
            RW(recordData.mScript, send, true);
            ProcessBodyParts(recordData.mParts, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasSubtype, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasEnchantmentCharge, send);
                RW(overrides.hasEnchantmentId, send);
                RW(overrides.hasScript, send);
                RW(overrides.hasBodyParts, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
    {
        for (auto &&record : worldstate->miscellaneousRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mWeight, send);
            RW(recordData.mData.mValue, send);
            RW(recordData.mData.mFlags, send); // TES3MP's mIsKey: OpenMW 0.51 keeps it as flag 0x1 (Key)
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasKeyState, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::WEAPON)
    {
        for (auto &&record : worldstate->weaponRecords)
        {
            auto &recordData = record.data;

            RW(record.quantity, send);
            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mType, send);
            RW(recordData.mData.mWeight, send);
            RW(recordData.mData.mValue, send);
            RW(recordData.mData.mHealth, send);
            RW(recordData.mData.mSpeed, send);
            RW(recordData.mData.mReach, send);
            RW(recordData.mData.mChop[0], send);
            RW(recordData.mData.mChop[1], send);
            RW(recordData.mData.mSlash[0], send);
            RW(recordData.mData.mSlash[1], send);
            RW(recordData.mData.mThrust[0], send);
            RW(recordData.mData.mThrust[1], send);
            RW(recordData.mData.mFlags, send);
            RW(recordData.mData.mEnchant, send);
            RW(recordData.mEnchant, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasSubtype, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasHealth, send);
                RW(overrides.hasSpeed, send);
                RW(overrides.hasReach, send);
                RW(overrides.hasDamageChop, send);
                RW(overrides.hasDamageSlash, send);
                RW(overrides.hasDamageThrust, send);
                RW(overrides.hasFlags, send);
                RW(overrides.hasEnchantmentCharge, send);
                RW(overrides.hasEnchantmentId, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::ACTIVATOR)
    {
        for (auto &&record : worldstate->activatorRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::APPARATUS)
    {
        for (auto &&record : worldstate->apparatusRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mType, send, true);
            RW(recordData.mData.mWeight, send, true);
            RW(recordData.mData.mValue, send, true);
            RW(recordData.mData.mQuality, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasSubtype, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasQuality, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::BODYPART)
    {
        for (auto &&record : worldstate->bodyPartRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mRace, send, true);
            RW(recordData.mData.mType, send);
            RW(recordData.mData.mPart, send);
            RW(recordData.mData.mVampire, send);
            RW(recordData.mData.mFlags, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasModel, send);
                RW(overrides.hasRace, send);
                RW(overrides.hasSubtype, send);
                RW(overrides.hasBodyPartType, send);
                RW(overrides.hasVampireState, send);
                RW(overrides.hasFlags, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::CELL)
    {
        for (auto &&record : worldstate->cellRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mName, send, true);
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::CONTAINER)
    {
        for (auto &&record : worldstate->containerRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mWeight, send);
            RW(recordData.mFlags, send);
            RW(recordData.mScript, send, true);
            ProcessInventoryList(record.inventory, recordData.mInventory, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasFlags, send);
                RW(overrides.hasScript, send);
                RW(overrides.hasInventory, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::CREATURE)
    {
        for (auto &&record : worldstate->creatureRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(record.inventoryBaseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mScale, send);
            RW(recordData.mBloodType, send);
            RW(recordData.mData.mType, send);
            RW(recordData.mData.mLevel, send);
            RW(recordData.mData.mHealth, send);
            RW(recordData.mData.mMana, send);
            RW(recordData.mData.mFatigue, send);
            RW(recordData.mData.mSoul, send);
            RW(recordData.mData.mAttack[0], send);
            RW(recordData.mData.mAttack[1], send);
            RW(recordData.mData.mAttack[2], send);
            RW(recordData.mData.mAttack[3], send);
            RW(recordData.mData.mAttack[4], send);
            RW(recordData.mData.mAttack[5], send);
            RW(recordData.mAiData.mFight, send);
            RW(recordData.mAiData.mFlee, send);
            RW(recordData.mAiData.mAlarm, send);
            RW(recordData.mAiData.mServices, send);
            RW(recordData.mFlags, send);
            RW(recordData.mScript, send, true);
            ProcessInventoryList(record.inventory, recordData.mInventory, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasScale, send);
                RW(overrides.hasBloodType, send);
                RW(overrides.hasSubtype, send);
                RW(overrides.hasLevel, send);
                RW(overrides.hasHealth, send);
                RW(overrides.hasMagicka, send);
                RW(overrides.hasFatigue, send);
                RW(overrides.hasSoulValue, send);
                RW(overrides.hasDamageChop, send);
                RW(overrides.hasDamageSlash, send);
                RW(overrides.hasDamageThrust, send);
                RW(overrides.hasAiFight, send);
                RW(overrides.hasAiFlee, send);
                RW(overrides.hasAiAlarm, send);
                RW(overrides.hasAiServices, send);
                RW(overrides.hasFlags, send);
                RW(overrides.hasScript, send);
                RW(overrides.hasInventory, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::DOOR)
    {
        for (auto &&record : worldstate->doorRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mOpenSound, send, true);
            RW(recordData.mCloseSound, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasOpenSound, send);
                RW(overrides.hasCloseSound, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::GAMESETTING)
    {
        for (auto&& record : worldstate->gameSettingRecords)
        {
            auto& recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(record.variable.variableType, send, true);

            short variableType = record.variable.variableType;

            if (variableType == mwmp::VARIABLE_TYPE::INT)
            {
                RW(record.variable.intValue, send);
                recordData.mValue.setType(ESM::VarType::VT_Int);
                recordData.mValue.setInteger(record.variable.intValue);
            }
            else if (variableType == mwmp::VARIABLE_TYPE::FLOAT)
            {
                RW(record.variable.floatValue, send);

                if (variableType == mwmp::VARIABLE_TYPE::FLOAT)
                    recordData.mValue.setType(ESM::VarType::VT_Float);

                recordData.mValue.setFloat(record.variable.floatValue);
            }
            else if (variableType == mwmp::VARIABLE_TYPE::STRING)
            {
                RW(record.variable.stringValue, send, true);
                recordData.mValue.setType(ESM::VarType::VT_String);
                recordData.mValue.setString(record.variable.stringValue);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::INGREDIENT)
    {
        for (auto &&record : worldstate->ingredientRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mWeight, send);
            RW(recordData.mData.mValue, send);
            for (auto &effectId : recordData.mData.mEffectID)
                RWEffectId(effectId, send);
            for (auto &attribute : recordData.mData.mAttributes)
                RWAttributeId(attribute, send);
            for (auto &skill : recordData.mData.mSkills)
                RWSkillId(skill, send);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasEffects, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::LIGHT)
    {
        for (auto &&record : worldstate->lightRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mSound, send, true);
            RW(recordData.mData.mWeight, send, true);
            RW(recordData.mData.mValue, send, true);
            RW(recordData.mData.mTime, send, true);
            RW(recordData.mData.mRadius, send, true);
            RW(recordData.mData.mColor, send, true);
            RW(recordData.mData.mFlags, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasSound, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasTime, send);
                RW(overrides.hasRadius, send);
                RW(overrides.hasColor, send);
                RW(overrides.hasFlags, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::LOCKPICK)
    {
        for (auto &&record : worldstate->lockpickRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mWeight, send, true);
            RW(recordData.mData.mValue, send, true);
            RW(recordData.mData.mQuality, send, true);
            RW(recordData.mData.mUses, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasQuality, send);
                RW(overrides.hasUses, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::NPC)
    {
        for (auto &&record : worldstate->npcRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(record.inventoryBaseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mFlags, send);
            RW(recordData.mRace, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mHair, send, true);
            RW(recordData.mHead, send, true);
            RW(recordData.mClass, send, true);
            RW(recordData.mFaction, send, true);
            RW(recordData.mScript, send, true);
            RW(recordData.mNpdt.mLevel, send);
            RW(recordData.mNpdt.mHealth, send);
            RW(recordData.mNpdt.mMana, send);
            RW(recordData.mNpdt.mFatigue, send);
            RW(recordData.mAiData.mFight, send);
            RW(recordData.mAiData.mFlee, send);
            RW(recordData.mAiData.mAlarm, send);
            RW(recordData.mAiData.mServices, send);

            RW(recordData.mNpdtType, send);
            ProcessInventoryList(record.inventory, recordData.mInventory, send);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasGender, send);
                RW(overrides.hasFlags, send);
                RW(overrides.hasRace, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasHair, send);
                RW(overrides.hasHead, send);
                RW(overrides.hasFaction, send);
                RW(overrides.hasScript, send);
                RW(overrides.hasLevel, send);
                RW(overrides.hasHealth, send);
                RW(overrides.hasMagicka, send);
                RW(overrides.hasFatigue, send);
                RW(overrides.hasAiFight, send);
                RW(overrides.hasAiFlee, send);
                RW(overrides.hasAiAlarm, send);
                RW(overrides.hasAiServices, send);
                RW(overrides.hasAutoCalc, send);
                RW(overrides.hasInventory, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::PROBE)
    {
        for (auto &&record : worldstate->probeRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mWeight, send, true);
            RW(recordData.mData.mValue, send, true);
            RW(recordData.mData.mQuality, send, true);
            RW(recordData.mData.mUses, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasQuality, send);
                RW(overrides.hasUses, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::REPAIR)
    {
        for (auto &&record : worldstate->repairRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mName, send, true);
            RW(recordData.mModel, send, true);
            RW(recordData.mIcon, send, true);
            RW(recordData.mData.mWeight, send, true);
            RW(recordData.mData.mValue, send, true);
            RW(recordData.mData.mQuality, send, true);
            RW(recordData.mData.mUses, send, true);
            RW(recordData.mScript, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasName, send);
                RW(overrides.hasModel, send);
                RW(overrides.hasIcon, send);
                RW(overrides.hasWeight, send);
                RW(overrides.hasValue, send);
                RW(overrides.hasQuality, send);
                RW(overrides.hasUses, send);
                RW(overrides.hasScript, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::SCRIPT)
    {
        for (auto &&record : worldstate->scriptRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mScriptText, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasScriptText, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::STATIC)
    {
        for (auto &&record : worldstate->staticRecords)
        {
            auto &recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mModel, send, true);

            if (!record.baseId.empty())
            {
                auto &&overrides = record.baseOverrides;
                RW(overrides.hasModel, send);
            }
        }
    }
    else if (worldstate->recordsType == mwmp::RECORD_TYPE::SOUND)
    {
        for (auto&& record : worldstate->soundRecords)
        {
            auto& recordData = record.data;

            RW(record.baseId, send, true);
            RW(recordData.mId, send, true);
            RW(recordData.mSound, send, true);
            RW(recordData.mData.mVolume, send);
            RW(recordData.mData.mMinRange, send);
            RW(recordData.mData.mMaxRange, send);

            if (!record.baseId.empty())
            {
                auto&& overrides = record.baseOverrides;
                RW(overrides.hasSound, send);
                RW(overrides.hasVolume, send);
                RW(overrides.hasMinRange, send);
                RW(overrides.hasMaxRange, send);
            }
        }
    }
}

void PacketRecordDynamic::ProcessEffects(ESM::EffectList &effectList, bool send)
{
    uint32_t effectCount;

    if (send)
        effectCount = static_cast<uint32_t>(effectList.mList.size());

    RW(effectCount, send);

    if (effectCount > maxEffects)
    {
        return;
    }

    if (!send)
    {
        effectList.mList.clear();
        effectList.mList.resize(effectCount);
    }

    // On the wire effects, skills and attributes stay the numbers TES3MP sends; OpenMW 0.51 identifies them by RefId.
    uint32_t index = 0;
    for (auto &&indexedEffect : effectList.mList)
    {
        ESM::ENAMstruct &effect = indexedEffect.mData;
        int16_t effectId = static_cast<int16_t>(ESM::MagicEffect::refIdToIndex(effect.mEffectID));
        int8_t attribute = static_cast<int8_t>(ESM::Attribute::refIdToIndex(effect.mAttribute));
        int8_t skill = static_cast<int8_t>(ESM::Skill::refIdToIndex(effect.mSkill));
        RW(effectId, send);
        RW(attribute, send);
        RW(skill, send);
        RW(effect.mRange, send);
        RW(effect.mArea, send);
        RW(effect.mDuration, send);
        RW(effect.mMagnMax, send);
        RW(effect.mMagnMin, send);
        if (!send)
        {
            effect.mEffectID = ESM::MagicEffect::indexToRefId(effectId);
            effect.mAttribute = ESM::Attribute::indexToRefId(attribute);
            effect.mSkill = ESM::Skill::indexToRefId(skill);
            indexedEffect.mIndex = index;
        }
        ++index;
    }
}

void PacketRecordDynamic::ProcessBodyParts(ESM::PartReferenceList &partList, bool send)
{
    uint32_t partCount;

    if (send)
        partCount = static_cast<uint32_t>(partList.mParts.size());

    RW(partCount, send);

    if (partCount > maxParts)
    {
        return;
    }

    if (!send)
    {
        partList.mParts.clear();
        partList.mParts.resize(partCount);
    }

    for (auto &&part : partList.mParts)
    {
        RW(part.mPart, send);
        RW(part.mMale, send, true);
        RW(part.mFemale, send, true);
    }
}

// ESM::InventoryList has a strange structure that makes it hard to read in packets directly, so we just deal with it
// here with the help of a separate mwmp::Item vector
void PacketRecordDynamic::ProcessInventoryList(std::vector<mwmp::Item> &inventory, ESM::InventoryList &inventoryList, bool send)
{
    uint32_t itemCount;

    if (send)
        itemCount = static_cast<uint32_t>(inventory.size());

    RW(itemCount, send);

    if (itemCount > maxItems)
    {
        return;
    }

    if (!send)
    {
        inventory.clear();
        inventory.resize(itemCount);
        inventoryList.mList.clear();
    }

    for (auto &&item : inventory)
    {
        RW(item.refId, send, true);
        RW(item.count, send, true);

        if (!send)
        {
            ESM::ContItem contItem;
            contItem.mItem = ESM::RefId::stringRefId(item.refId);
            contItem.mCount = item.count;
            inventoryList.mList.push_back(contItem);
        }
    }
}
