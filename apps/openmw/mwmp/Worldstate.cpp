#include <components/esm/util.hpp>
#include <components/esm3/loadcell.hpp>
#include "../mwworld/cellstore.hpp"
#include "../mwworld/worldmodel.hpp"
#include "Ids.hpp"
#include <components/openmw-mp/TimedLog.hpp>

#include "../mwbase/environment.hpp"

#include "../mwgui/windowmanagerimp.hpp"

#include "../mwmechanics/mechanicsmanagerimp.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/worldimp.hpp"

#include "Worldstate.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "PlayerList.hpp"
#include "DedicatedPlayer.hpp"
#include "RecordHelper.hpp"
#include "CellController.hpp"

using namespace mwmp;

Worldstate::Worldstate()
{
    hasPlayerCollision = true;
    hasActorCollision = true;
    hasPlacedObjectCollision = false;
    useActorCollisionForPlacedObjects = false;
}

Worldstate::~Worldstate()
{

}

Networking *Worldstate::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}

void Worldstate::addRecords()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_RECORD_DYNAMIC with %i records of type %i",
        recordsCount, recordsType);

    if (recordsType == mwmp::RECORD_TYPE::SPELL)
    {
        for (auto &&record : spellRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- spell record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::POTION)
    {
        for (auto &&record : potionRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- potion record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::ENCHANTMENT)
    {
        for (auto &&record : enchantmentRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- enchantment record %s, %i\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mData.mType,
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::CREATURE)
    {
        for (auto &&record : creatureRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- creature record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::NPC)
    {
        for (auto &&record : npcRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- NPC record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::ARMOR)
    {
        for (auto &&record : armorRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- armor record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::BOOK)
    {
        for (auto &&record : bookRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- book record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::CLOTHING)
    {
        for (auto &&record : clothingRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- clothing record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::MISCELLANEOUS)
    {
        for (auto &&record : miscellaneousRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- miscellaneous record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::WEAPON)
    {
        for (auto &&record : weaponRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- weapon record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::CONTAINER)
    {
        for (auto &&record : containerRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- container record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::DOOR)
    {
        for (auto &&record : doorRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- door record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::ACTIVATOR)
    {
        for (auto &&record : activatorRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- activator record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::STATIC)
    {
        for (auto &&record : staticRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- static record %s\n-- baseId is %s", idString(record.data.mId).c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::INGREDIENT)
    {
        for (auto &&record : ingredientRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- ingredient record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::APPARATUS)
    {
        for (auto &&record : apparatusRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- apparatus record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::LOCKPICK)
    {
        for (auto &&record : lockpickRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- lockpick record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::PROBE)
    {
        for (auto &&record : probeRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- probe record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::REPAIR)
    {
        for (auto &&record : repairRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- repair record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::LIGHT)
    {
        for (auto &&record : lightRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- light record %s, %s\n-- baseId is %s", idString(record.data.mId).c_str(), record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::CELL)
    {
        for (auto &&record : cellRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- cell record %s\n-- baseId is %s", record.data.mName.c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::SCRIPT)
    {
        for (auto &&record : scriptRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- script record %s\n-- baseId is %s", idString(record.data.mId).c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::BODYPART)
    {
        for (auto &&record : bodyPartRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- bodypart record %s\n-- baseId is %s", idString(record.data.mId).c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::SOUND)
    {
        for (auto&& record : soundRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- sound record %s\n-- baseId is %s", idString(record.data.mId).c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
    else if (recordsType == mwmp::RECORD_TYPE::GAMESETTING)
    {
        for (auto&& record : gameSettingRecords)
        {
            bool hasBaseId = !record.baseId.empty();

            LOG_APPEND(TimedLog::LOG_INFO, "- gameSetting record %s\n-- baseId is %s", idString(record.data.mId).c_str(),
                hasBaseId ? record.baseId.c_str() : "empty");

            RecordHelper::overrideRecord(record);
        }
    }
}

bool Worldstate::containsExploredMapTile(int cellX, int cellY)
{
    for (const auto &mapTile : exploredMapTiles)
    {
        if (mapTile.x == cellX && mapTile.y == cellY)
            return true;
    }

    return false;
}

void Worldstate::markExploredMapTile(int cellX, int cellY)
{
    mwmp::MapTile exploredTile;
    exploredTile.x = cellX;
    exploredTile.y = cellY;
    exploredMapTiles.push_back(exploredTile);
}

void Worldstate::setClientGlobals()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_CLIENT_SCRIPT_GLOBAL with the following global values:");
    std::string debugMessage = "";

    for (const auto &clientGlobal : clientGlobals)
    {
        if (TimedLog::GetLevel() <= TimedLog::LOG_INFO)
        {
            if (!debugMessage.empty())
                debugMessage += ", ";

            std::string variableTypeAsString;
            std::string valueAsString;

            if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::SHORT || clientGlobal.variableType == mwmp::VARIABLE_TYPE::LONG)
            {
                variableTypeAsString = clientGlobal.variableType == mwmp::VARIABLE_TYPE::SHORT ? "short" : "long";
                valueAsString = std::to_string(clientGlobal.intValue);
            }
            else if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::FLOAT)
            {
                variableTypeAsString = "float";
                valueAsString = std::to_string(clientGlobal.floatValue);
            }

            debugMessage += clientGlobal.id + ": " + variableTypeAsString + " " + valueAsString;
        }

        MWBase::World* world = MWBase::Environment::get().getWorld();

        // If this global doesn't exist, create it
        if (world->getGlobalVariableType(clientGlobal.id) == ' ')
        {
            ESM::VarType varType;

            if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::SHORT)
                varType = ESM::VarType::VT_Short;
            else if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::LONG)
                varType = ESM::VarType::VT_Long;
            if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::FLOAT)
                varType = ESM::VarType::VT_Float;

            world->createGlobal(clientGlobal.id, varType);
        }

        if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::SHORT || clientGlobal.variableType == mwmp::VARIABLE_TYPE::LONG)
            world->setGlobalInt(clientGlobal.id, clientGlobal.intValue);
        else if (clientGlobal.variableType == mwmp::VARIABLE_TYPE::FLOAT)
            world->setGlobalFloat(clientGlobal.id, clientGlobal.floatValue);
    }

    LOG_APPEND(TimedLog::LOG_INFO, "- %s", debugMessage.c_str());
}

void Worldstate::setKills()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_WORLD_KILL_COUNT with the following kill counts:");
    std::string debugMessage = "";

    for (const auto &killChange : killChanges)
    {
        if (TimedLog::GetLevel() <= TimedLog::LOG_INFO)
        {
            if (!debugMessage.empty())
                debugMessage += ", ";

            debugMessage += killChange.refId + ": " + std::to_string(killChange.number);
        }

        MWBase::Environment::get().getMechanicsManager()->setDeaths(refId(killChange.refId), killChange.number);
    }

    LOG_APPEND(TimedLog::LOG_INFO, "- %s", debugMessage.c_str());
}

void Worldstate::setMapExplored()
{
    for (const auto &mapTile : mapTiles)
    {
        const MWWorld::CellStore &cellStore = MWBase::Environment::get().getWorldModel()->getExterior(
            ESM::ExteriorCellLocation(mapTile.x, mapTile.y, ESM::Cell::sDefaultWorldspaceId));

        if (!cellStore.getCell()->getNameId().empty())
            MWBase::Environment::get().getWindowManager()->addVisitedLocation(
                std::string(cellStore.getCell()->getNameId()), mapTile.x, mapTile.y);

        // Keep this tile marked as explored so we don't send any more packets for it (before drawing it)
        markExploredMapTile(mapTile.x, mapTile.y);
        MWBase::Environment::get().getWindowManager()->setGlobalMapTile(mapTile.x, mapTile.y, mapTile.imageData);
    }
}

void Worldstate::setWeather()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    // There's a chance we've been sent the weather for a region right after a teleportation
    // that hasn't been registered in the WeatherManager yet, meaning the WeatherManager
    // doesn't have the correct new region set for us, so make sure we update it
    world->updateWeather(0);

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Setting weather for region: %s, currentWeather: %i, "
        "nextWeather: %i, queuedWeather: %i, transitionFactor: %f, forceWeather is %s",
        weather.region.c_str(), weather.currentWeather, weather.nextWeather,
        weather.queuedWeather, weather.transitionFactor, forceWeather ? "true" : "false");

    world->setRegionWeather(refId(weather.region), weather.currentWeather, weather.nextWeather,
        weather.queuedWeather, weather.transitionFactor, forceWeather);
}

void Worldstate::resetCells(std::vector<ESM::Cell>* cells)
{
    // TODO: TES3MP reset cells to their content file state here (the server does it to respawn dungeons), unloading
    // and clearing them. OpenMW 0.51 keeps loaded cells in its WorldModel, which can't drop one, so for now a reset
    // only takes effect for cells this client hasn't loaded yet.
    for (const ESM::Cell& cell : *cells)
        LOG_APPEND(TimedLog::LOG_INFO, "- Cell %s was reset by the server", cell.getShortDescription().c_str());
}

void Worldstate::sendClientGlobal(std::string varName, int value, mwmp::VARIABLE_TYPE variableType)
{
    clientGlobals.clear();

    mwmp::ClientVariable clientVariable;
    clientVariable.id = varName;
    clientVariable.variableType = variableType;
    clientVariable.intValue = value;

    std::string variableTypeAsString;

    if (variableType == mwmp::VARIABLE_TYPE::SHORT)
        variableTypeAsString = "short";
    else if (variableType == mwmp::VARIABLE_TYPE::LONG)
        variableTypeAsString = "long";

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_CLIENT_SCRIPT_GLOBAL with name %s, type %s, value %i",
        varName.c_str(), variableTypeAsString.c_str(), value);

    clientGlobals.push_back(clientVariable);

    getNetworking()->getWorldstatePacket(ID_CLIENT_SCRIPT_GLOBAL)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_CLIENT_SCRIPT_GLOBAL)->Send();
}

void Worldstate::sendClientGlobal(std::string varName, float value)
{
    clientGlobals.clear();

    mwmp::ClientVariable clientVariable;
    clientVariable.id = varName;
    clientVariable.variableType = mwmp::VARIABLE_TYPE::FLOAT;
    clientVariable.floatValue = value;

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_CLIENT_SCRIPT_GLOBAL with name %s, type float, value %f", varName.c_str(), value);

    clientGlobals.push_back(clientVariable);

    getNetworking()->getWorldstatePacket(ID_CLIENT_SCRIPT_GLOBAL)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_CLIENT_SCRIPT_GLOBAL)->Send();
}

void Worldstate::sendMapExplored(int cellX, int cellY, const std::vector<char>& imageData)
{
    mapTiles.clear();

    mwmp::MapTile mapTile;
    mapTile.x = cellX;
    mapTile.y = cellY;
    mapTile.imageData = imageData;

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_MAP with x: %i, y: %i", cellX, cellY);

    mapTiles.push_back(mapTile);

    getNetworking()->getWorldstatePacket(ID_WORLD_MAP)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_WORLD_MAP)->Send();
}

void Worldstate::sendWeather(std::string region, int currentWeather, int nextWeather, int queuedWeather, float transitionFactor)
{
    forceWeather = false;
    weather.region = region;
    weather.currentWeather = currentWeather;
    weather.nextWeather = nextWeather;
    weather.queuedWeather = queuedWeather;
    weather.transitionFactor = transitionFactor;

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_WEATHER with region: %s, currentWeather: %i, "
        "nextWeather: %i, queuedWeather, %i, transitionFactor: %f",
        region.c_str(), currentWeather, nextWeather, queuedWeather, transitionFactor);

    getNetworking()->getWorldstatePacket(ID_WORLD_WEATHER)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_WORLD_WEATHER)->Send();
}

void Worldstate::sendEnchantmentRecord(const ESM::Enchantment* enchantment)
{
    enchantmentRecords.clear();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_RECORD_DYNAMIC with enchantment");

    recordsType = mwmp::RECORD_TYPE::ENCHANTMENT;

    mwmp::EnchantmentRecord record;
    record.data = *enchantment;
    enchantmentRecords.push_back(record);

    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->Send();
}

void Worldstate::sendPotionRecord(const ESM::Potion* potion, unsigned int quantity)
{
    potionRecords.clear();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_RECORD_DYNAMIC with potion %s", potion->mName.c_str());

    recordsType = mwmp::RECORD_TYPE::POTION;

    mwmp::PotionRecord record;
    record.data = *potion;
    record.quantity = quantity;
    potionRecords.push_back(record);

    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->Send();
}

void Worldstate::sendSpellRecord(const ESM::Spell* spell)
{
    spellRecords.clear();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_RECORD_DYNAMIC with spell %s", spell->mName.c_str());

    recordsType = mwmp::RECORD_TYPE::SPELL;

    mwmp::SpellRecord record;
    record.data = *spell;
    spellRecords.push_back(record);

    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->Send();
}

void Worldstate::sendArmorRecord(const ESM::Armor* armor, std::string baseId)
{
    armorRecords.clear();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_RECORD_DYNAMIC with armor %s", armor->mName.c_str());

    recordsType = mwmp::RECORD_TYPE::ARMOR;

    mwmp::ArmorRecord record;
    record.data = *armor;
    record.baseId = baseId;
    record.baseOverrides.hasName = true;
    record.baseOverrides.hasEnchantmentId = true;
    record.baseOverrides.hasEnchantmentCharge = true;
    armorRecords.push_back(record);

    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->Send();
}

void Worldstate::sendBookRecord(const ESM::Book* book, std::string baseId)
{
    bookRecords.clear();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_RECORD_DYNAMIC with book %s", book->mName.c_str());

    recordsType = mwmp::RECORD_TYPE::BOOK;

    mwmp::BookRecord record;
    record.data = *book;
    record.baseId = baseId;
    record.baseOverrides.hasName = true;
    record.baseOverrides.hasEnchantmentId = true;
    record.baseOverrides.hasEnchantmentCharge = true;
    bookRecords.push_back(record);

    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->Send();
}

void Worldstate::sendClothingRecord(const ESM::Clothing* clothing, std::string baseId)
{
    clothingRecords.clear();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_RECORD_DYNAMIC with clothing %s", clothing->mName.c_str());

    recordsType = mwmp::RECORD_TYPE::CLOTHING;

    mwmp::ClothingRecord record;
    record.data = *clothing;
    record.baseId = baseId;
    record.baseOverrides.hasName = true;
    record.baseOverrides.hasEnchantmentId = true;
    record.baseOverrides.hasEnchantmentCharge = true;
    clothingRecords.push_back(record);

    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->Send();
}

void Worldstate::sendWeaponRecord(const ESM::Weapon* weapon, std::string baseId, unsigned int quantity)
{
    weaponRecords.clear();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_RECORD_DYNAMIC with weapon %s", weapon->mName.c_str());

    recordsType = mwmp::RECORD_TYPE::WEAPON;

    mwmp::WeaponRecord record;
    record.data = *weapon;
    record.quantity = quantity;
    record.baseId = baseId;
    record.baseOverrides.hasName = true;
    record.baseOverrides.hasEnchantmentId = true;
    record.baseOverrides.hasEnchantmentCharge = true;
    weaponRecords.push_back(record);

    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->setWorldstate(this);
    getNetworking()->getWorldstatePacket(ID_RECORD_DYNAMIC)->Send();
}
