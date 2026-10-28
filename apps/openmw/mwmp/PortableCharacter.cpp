#include "PortableCharacter.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <vector>

#include <components/debug/debuglog.hpp>
#include <components/esm/defs.hpp>
#include <components/esm3/dialoguestate.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/formatversion.hpp>
#include <components/esm3/npcstate.hpp>
#include <components/esm3/journalentry.hpp>
#include <components/esm/attr.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadglob.hpp>
#include <components/esm3/quickkeys.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/esm3/player.hpp>
#include <components/esm3/queststate.hpp>
#include <components/esm3/savedgame.hpp>
#include <components/files/conversion.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/TimedLog.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldmodel.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/ptr.hpp"

#include "Ids.hpp"
#include "LocalPlayer.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "Worldstate.hpp"

namespace mwmp
{
namespace
{
    // Records a game made for the player (potions they brewed, items they enchanted, spells they made), by id: what
    // their inventory and spells can need besides the game's own
    struct MadeRecords
    {
        std::map<ESM::RefId, ESM::Potion> potions;
        std::map<ESM::RefId, ESM::Spell> spells;
        std::map<ESM::RefId, ESM::Enchantment> enchantments;
        std::map<ESM::RefId, ESM::Weapon> weapons;
        std::map<ESM::RefId, ESM::Armor> armors;
        std::map<ESM::RefId, ESM::Clothing> clothes;
        std::map<ESM::RefId, ESM::Book> books;

        template <class T>
        static void readInto(ESM::ESMReader &reader, std::map<ESM::RefId, T> &records)
        {
            T record;
            bool isDeleted = false;
            record.load(reader, isDeleted);
            if (!isDeleted)
                records[record.mId] = record;
        }

        // Reads the record the reader is at if it's one of these kinds; false if it isn't
        bool read(ESM::ESMReader &reader, uint32_t type)
        {
            switch (type)
            {
                case ESM::REC_ALCH: readInto(reader, potions); return true;
                case ESM::REC_SPEL: readInto(reader, spells); return true;
                case ESM::REC_ENCH: readInto(reader, enchantments); return true;
                case ESM::REC_WEAP: readInto(reader, weapons); return true;
                case ESM::REC_ARMO: readInto(reader, armors); return true;
                case ESM::REC_CLOT: readInto(reader, clothes); return true;
                case ESM::REC_BOOK: readInto(reader, books); return true;
                default: return false;
            }
        }
    };

    // What the server needs from a save: the player and their records
    struct SaveContents
    {
        ESM::SavedGame profile;
        ESM::Player player;
        bool hasPlayer = false;
        ESM::NPC npc;
        bool hasNpc = false;
        std::map<ESM::RefId, ESM::Class> classes;
        std::vector<ESM::JournalEntry> journal;
        std::vector<ESM::QuestState> quests;
        ESM::DialogueState dialogue;
        MadeRecords made;
        // Kills by actor, the global variables, and the quick keys
        std::map<ESM::RefId, int> kills;
        std::vector<ESM::Global> globals;
        ESM::QuickKeys quickKeys;
    };

    // (Filled in place: the player's state can't be copied)
    void readSave(const std::string &save, SaveContents &contents)
    {
        ESM::ESMReader reader;
        reader.open(Files::pathFromUnicodeString(save));
        while (reader.hasMoreRecs())
        {
            ESM::NAME name = reader.getRecName();
            reader.getRecHeader();
            bool isDeleted = false;
            switch (name.toInt())
            {
                case ESM::REC_SAVE:
                    contents.profile.load(reader);
                    break;
                case ESM::REC_PLAY:
                    contents.player.load(reader);
                    contents.hasPlayer = true;
                    break;
                case ESM::REC_NPC_:
                {
                    ESM::NPC npc;
                    npc.load(reader, isDeleted);
                    if (isSameId(npc.mId, "player"))
                    {
                        contents.npc = npc;
                        contents.hasNpc = true;
                    }
                    break;
                }
                case ESM::REC_CLAS:
                {
                    ESM::Class record;
                    record.load(reader, isDeleted);
                    contents.classes[record.mId] = record;
                    break;
                }
                case ESM::REC_JOUR:
                {
                    ESM::JournalEntry entry;
                    entry.load(reader);
                    if (entry.mType == ESM::JournalEntry::Type_Journal)
                        contents.journal.push_back(entry);
                    break;
                }
                case ESM::REC_QUES:
                {
                    ESM::QuestState quest;
                    quest.load(reader);
                    contents.quests.push_back(quest);
                    break;
                }
                case ESM::REC_DIAS:
                    contents.dialogue.load(reader);
                    break;
                case ESM::REC_DCOU:
                    // (as MWMechanics::Actors keeps them)
                    while (reader.isNextSub("ID__"))
                    {
                        ESM::RefId id = reader.getRefId();
                        int count = 0;
                        reader.getHNT(count, "COUN");
                        contents.kills[id] = count;
                    }
                    break;
                case ESM::REC_GLOB:
                {
                    ESM::Global global;
                    global.load(reader, isDeleted);
                    contents.globals.push_back(global);
                    break;
                }
                case ESM::REC_KEYS:
                    contents.quickKeys.load(reader);
                    break;
                default:
                    if (!contents.made.read(reader, name.toInt()))
                        reader.skipRecord();
                    break;
            }
        }
    }

    // Just enough JSON: objects and arrays written in order, their members separated by commas
    class JsonWriter
    {
    public:
        void beginObject(std::string_view key = {})
        {
            prefix(key);
            mOut += '{';
            mFirst = true;
        }

        void endObject()
        {
            mOut += '}';
            mFirst = false;
        }

        void beginArray(std::string_view key = {})
        {
            prefix(key);
            mOut += '[';
            mFirst = true;
        }

        void endArray()
        {
            mOut += ']';
            mFirst = false;
        }

        void string(std::string_view key, std::string_view value)
        {
            prefix(key);
            quote(value);
        }

        void number(std::string_view key, double value)
        {
            prefix(key);
            if (!std::isfinite(value))
                value = 0;
            std::array<char, 32> buffer;
            auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
            mOut.append(buffer.data(), result.ptr);
        }

        void boolean(std::string_view key, bool value)
        {
            prefix(key);
            mOut += value ? "true" : "false";
        }

        const std::string &str() const { return mOut; }

    private:
        // The comma before a member, and its key in an object (none in an array)
        void prefix(std::string_view key)
        {
            if (!mFirst)
                mOut += ',';
            mFirst = false;
            if (!key.empty())
            {
                quote(key);
                mOut += ':';
            }
        }

        void quote(std::string_view text)
        {
            mOut += '"';
            for (char c : text)
            {
                if (c == '"' || c == '\\')
                {
                    mOut += '\\';
                    mOut += c;
                }
                else if (static_cast<unsigned char>(c) < 0x20)
                {
                    char escaped[8];
                    std::snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned char>(c));
                    mOut += escaped;
                }
                else
                    mOut += c;
            }
            mOut += '"';
        }

        std::string mOut;
        bool mFirst = true;
    };

    // Whether the server can know a record: one of the game's own, not one only this player's game made (a potion
    // they brewed, a spell they made)
    bool isKnown(const ESM::RefId &id)
    {
        return !id.empty() && !isLocalOnlyId(id) && MWBase::Environment::get().getESMStore()->find(id) != 0;
    }

    // The game's own item an enchanted one was made from (the server makes it again from that): the same model and
    // icon, not enchanted
    template <class T>
    const T *findBase(const T &item)
    {
        for (const T &candidate : MWBase::Environment::get().getESMStore()->get<T>())
        {
            if (!isLocalOnlyId(candidate.mId) && candidate.mEnchant.empty() && candidate.mModel == item.mModel
                && candidate.mIcon == item.mIcon)
                return &candidate;
        }
        return nullptr;
    }

    void writeEffects(JsonWriter &json, const ESM::EffectList &effects)
    {
        json.beginArray("effects");
        for (const ESM::IndexedENAMstruct &indexed : effects.mList)
        {
            const ESM::ENAMstruct &effect = indexed.mData;
            json.beginObject();
            json.number("id", ESM::MagicEffect::refIdToIndex(effect.mEffectID));
            json.number("attribute", ESM::Attribute::refIdToIndex(effect.mAttribute));
            json.number("skill", ESM::Skill::refIdToIndex(effect.mSkill));
            json.number("rangeType", effect.mRange);
            json.number("area", effect.mArea);
            json.number("duration", effect.mDuration);
            json.number("magnitudeMin", effect.mMagnMin);
            json.number("magnitudeMax", effect.mMagnMax);
            json.endObject();
        }
        json.endArray();
    }

    // The records the player's own game made that their items and spells take along, written as the server's record
    // stores keep them, under the ids the save has (the server gives them ids of its own)
    class MadeForServer
    {
    public:
        explicit MadeForServer(const MadeRecords &made)
            : mMade(made)
        {
        }

        // Whether an item or spell only the player's game knows can go (its records going with it)
        bool take(const ESM::RefId &id)
        {
            if (mMade.potions.count(id))
                mPotions.insert(id);
            else if (mMade.spells.count(id))
                mSpells.insert(id);
            else
                return takeEnchanted(mMade.weapons, mWeapons, id) || takeEnchanted(mMade.armors, mArmors, id)
                    || takeEnchanted(mMade.clothes, mClothes, id) || takeEnchanted(mMade.books, mBooks, id);
            return true;
        }

        void write(JsonWriter &json) const
        {
            json.beginObject("records");
            json.beginObject("enchantment");
            for (const ESM::RefId &id : mEnchantments)
            {
                const ESM::Enchantment &record = mMade.enchantments.at(id);
                json.beginObject(idString(id));
                json.number("subtype", record.mData.mType);
                json.number("cost", record.mData.mCost);
                json.number("charge", record.mData.mCharge);
                json.number("flags", record.mData.mFlags);
                writeEffects(json, record.mEffects);
                json.endObject();
            }
            json.endObject();
            json.beginObject("potion");
            for (const ESM::RefId &id : mPotions)
            {
                const ESM::Potion &record = mMade.potions.at(id);
                json.beginObject(idString(id));
                json.string("name", record.mName);
                json.number("weight", record.mData.mWeight);
                json.number("value", record.mData.mValue);
                json.number("autoCalc", record.mData.mFlags & ESM::Potion::Autocalc);
                json.string("icon", record.mIcon);
                json.string("model", record.mModel);
                json.string("script", idString(record.mScript));
                writeEffects(json, record.mEffects);
                json.endObject();
            }
            json.endObject();
            json.beginObject("spell");
            for (const ESM::RefId &id : mSpells)
            {
                const ESM::Spell &record = mMade.spells.at(id);
                json.beginObject(idString(id));
                json.string("name", record.mName);
                json.number("subtype", record.mData.mType);
                json.number("cost", record.mData.mCost);
                json.number("flags", record.mData.mFlags);
                writeEffects(json, record.mEffects);
                json.endObject();
            }
            json.endObject();
            writeEnchanted(json, "weapon", mMade.weapons, mWeapons);
            writeEnchanted(json, "armor", mMade.armors, mArmors);
            writeEnchanted(json, "clothing", mMade.clothes, mClothes);
            writeEnchanted(json, "book", mMade.books, mBooks);
            json.endObject();
        }

    private:
        // An enchanted item goes as the game's item it was made from, its name, and its enchantment
        template <class T>
        bool takeEnchanted(const std::map<ESM::RefId, T> &records, std::set<ESM::RefId> &taken, const ESM::RefId &id)
        {
            auto found = records.find(id);
            if (found == records.end() || findBase(found->second) == nullptr)
                return false;
            const ESM::RefId &enchantment = found->second.mEnchant;
            if (isLocalOnlyId(enchantment))
            {
                if (!mMade.enchantments.count(enchantment))
                    return false;
                mEnchantments.insert(enchantment);
            }
            taken.insert(id);
            return true;
        }

        template <class T>
        static void writeEnchanted(JsonWriter &json, std::string_view storeType, const std::map<ESM::RefId, T> &records,
            const std::set<ESM::RefId> &taken)
        {
            json.beginObject(storeType);
            for (const ESM::RefId &id : taken)
            {
                const T &record = records.at(id);
                json.beginObject(idString(id));
                json.string("name", record.mName);
                json.string("baseId", idString(findBase(record)->mId));
                json.number("enchantmentCharge", record.mData.mEnchant);
                json.string("enchantmentId", idString(record.mEnchant));
                json.endObject();
            }
            json.endObject();
        }

        const MadeRecords &mMade;
        std::set<ESM::RefId> mPotions, mSpells, mEnchantments, mWeapons, mArmors, mClothes, mBooks;
    };

    std::string cellDescription(const ESM::RefId &cellId)
    {
        if (const auto *exterior = cellId.getIf<ESM::ESM3ExteriorCellRefId>())
            return std::to_string(exterior->getX()) + ", " + std::to_string(exterior->getY());
        return idString(cellId);
    }

    // The character as the server keeps it, in its terms (attributes and skills by their numbers; it names them)
    std::string toJson(const SaveContents &save)
    {
        const MWWorld::ESMStore &store = *MWBase::Environment::get().getESMStore();
        const ESM::NpcState &state = save.player.mObject;
        const ESM::CreatureStats &creatureStats = state.mCreatureStats;
        const ESM::NpcStats &npcStats = state.mNpcStats;

        JsonWriter json;
        json.beginObject();
        json.number("version", 1);
        json.string("name", save.npc.mName);
        json.string("race", idString(save.npc.mRace));
        json.string("head", idString(save.npc.mHead));
        json.string("hair", idString(save.npc.mHair));
        json.boolean("male", save.npc.isMale());
        json.string("birthsign", idString(save.player.mBirthsign));

        // A class of the game's is named; one the player made comes whole
        auto customClass = save.classes.find(save.npc.mClass);
        if (store.get<ESM::Class>().search(save.npc.mClass) != nullptr || customClass == save.classes.end())
            json.string("class", idString(save.npc.mClass));
        else
        {
            const ESM::Class &record = customClass->second;
            json.beginObject("customClass");
            json.string("name", record.mName);
            json.string("description", record.mDescription);
            json.number("specialization", record.mData.mSpecialization);
            json.beginArray("majorAttributes");
            for (int attribute : record.mData.mAttribute)
                json.number({}, attribute);
            json.endArray();
            json.beginArray("majorSkills");
            for (int i = 0; i < 5; ++i)
                json.number({}, record.mData.getSkill(i, true));
            json.endArray();
            json.beginArray("minorSkills");
            for (int i = 0; i < 5; ++i)
                json.number({}, record.mData.getSkill(i, false));
            json.endArray();
            json.endObject();
        }
        // Character creation not finished in single-player: nothing to bring yet
        json.boolean("unfinished", save.npc.mClass.empty() || save.player.mBirthsign.empty());

        json.number("level", creatureStats.mLevel);
        json.number("levelProgress", npcStats.mLevelProgress);
        const char *dynamicNames[] = { "health", "magicka", "fatigue" };
        for (int i = 0; i < 3; ++i)
        {
            json.beginArray(dynamicNames[i]);
            json.number({}, creatureStats.mDynamic[i].mBase);
            json.number({}, creatureStats.mDynamic[i].mCurrent);
            json.endArray();
        }
        json.beginArray("attributes");
        for (size_t i = 0; i < creatureStats.mAttributes.size(); ++i)
        {
            json.beginArray();
            json.number({}, creatureStats.mAttributes[i].mBase);
            json.number({}, creatureStats.mAttributes[i].mDamage);
            json.number({}, npcStats.mSkillIncrease[i]);
            json.endArray();
        }
        json.endArray();
        json.beginArray("skills");
        for (const auto &skill : npcStats.mSkills)
        {
            json.beginArray();
            json.number({}, skill.mBase);
            json.number({}, skill.mDamage);
            json.number({}, skill.mProgress);
            json.endArray();
        }
        json.endArray();

        // Items and spells the player made go with the records they need; any the server can't be given (an item
        // whose own records are missing from the save, say) are left behind, and counted so the player can be told
        MadeForServer made(save.made);
        int skippedItems = 0;
        const std::vector<ESM::ObjectState> &items = state.mInventory.mItems;
        std::vector<bool> goes(items.size());
        for (size_t i = 0; i < items.size(); ++i)
        {
            const ESM::RefId &id = items[i].mRef.mRefID;
            goes[i] = isKnown(id) || (isLocalOnlyId(id) && made.take(id));
        }
        json.beginArray("inventory");
        for (size_t i = 0; i < items.size(); ++i)
        {
            const ESM::ObjectState &item = items[i];
            if (!goes[i])
            {
                ++skippedItems;
                continue;
            }
            json.beginArray();
            json.string({}, idString(item.mRef.mRefID));
            json.number({}, item.mRef.mCount);
            json.number({}, item.mRef.mChargeInt);
            json.number({}, item.mRef.mEnchantmentCharge);
            json.string({}, idString(item.mRef.mSoul));
            json.endArray();
        }
        json.endArray();
        // The equipment slots, by the inventory store's numbers (the server's too)
        json.beginObject("equipment");
        for (const auto &[index, slot] : state.mInventory.mEquipmentSlots)
        {
            if (index >= items.size() || !goes[index])
                continue;
            const ESM::CellRef &ref = items[index].mRef;
            json.beginArray(std::to_string(slot));
            json.string({}, idString(ref.mRefID));
            json.number({}, ref.mCount);
            json.number({}, ref.mChargeInt);
            json.number({}, ref.mEnchantmentCharge);
            json.endArray();
        }
        json.endObject();

        // The spells known: a save keeps those learned in the player's record and the rest in their state
        std::vector<ESM::RefId> knownSpells = save.npc.mSpells.mList;
        for (const ESM::RefId &spell : creatureStats.mSpells.mSpells)
        {
            if (std::find(knownSpells.begin(), knownSpells.end(), spell) == knownSpells.end())
                knownSpells.push_back(spell);
        }
        int skippedSpells = 0;
        json.beginArray("spells");
        for (const ESM::RefId &spell : knownSpells)
        {
            bool known = isLocalOnlyId(spell) ? made.take(spell) : store.get<ESM::Spell>().search(spell) != nullptr;
            if (known)
                json.string({}, idString(spell));
            else
                ++skippedSpells;
        }
        json.endArray();
        json.number("skippedItems", skippedItems);
        json.number("skippedSpells", skippedSpells);
        made.write(json);

        // Skill books read
        json.beginArray("books");
        for (const ESM::RefId &book : npcStats.mUsedIds)
            json.string({}, idString(book));
        json.endArray();

        json.number("bounty", npcStats.mBounty);
        json.number("reputation", npcStats.mReputation);
        // Rank (-1: not a member), expelled, reputation
        json.beginObject("factions");
        for (const auto &[faction, stats] : npcStats.mFactions)
        {
            json.beginArray(idString(faction));
            json.number({}, stats.mRank);
            json.boolean({}, stats.mExpelled);
            json.number({}, stats.mReputation);
            json.endArray();
        }
        json.endObject();

        // The journal's entries in order (quest, its index, and the day: days passed, month, day of the month), the
        // quests' stages, and the topics known
        json.beginArray("journal");
        for (const ESM::JournalEntry &entry : save.journal)
        {
            const ESM::Dialogue *quest = store.get<ESM::Dialogue>().search(entry.mTopic);
            if (quest == nullptr)
                continue;
            auto info = std::find_if(quest->mInfo.begin(), quest->mInfo.end(),
                [&](const ESM::DialInfo &each) { return each.mId == entry.mInfo; });
            if (info == quest->mInfo.end())
                continue;
            json.beginArray();
            json.string({}, idString(quest->mId));
            json.number({}, info->mData.mJournalIndex);
            json.number({}, entry.mDay);
            json.number({}, entry.mMonth);
            json.number({}, entry.mDayOfMonth);
            json.endArray();
        }
        json.endArray();
        json.beginArray("quests");
        for (const ESM::QuestState &quest : save.quests)
        {
            const ESM::Dialogue *dialogue = store.get<ESM::Dialogue>().search(quest.mTopic);
            if (dialogue == nullptr)
                continue;
            json.beginArray();
            json.string({}, idString(dialogue->mId));
            json.number({}, quest.mState);
            json.endArray();
        }
        json.endArray();
        json.beginArray("topics");
        for (const ESM::RefId &topic : save.dialogue.mKnownTopics)
        {
            if (const ESM::Dialogue *dialogue = store.get<ESM::Dialogue>().search(topic))
                json.string({}, idString(dialogue->mId));
        }
        json.endArray();

        // Kills by actor (quests count them), and the global variables the server keeps in step (quests' own among
        // them): type (0 short, 1 long, 2 float) and value
        json.beginObject("kills");
        for (const auto &[id, count] : save.kills)
        {
            if (store.find(id) != 0)
                json.number(idString(id), count);
        }
        json.endObject();
        json.beginObject("globals");
        for (const ESM::Global &global : save.globals)
        {
            const std::string id = Misc::StringUtils::lowerCase(idString(global.mId));
            if (!Main::isValidPacketGlobal(id))
                continue;
            json.beginArray(id);
            if (global.mValue.getType() == ESM::VT_Float)
            {
                json.number({}, 2);
                json.number({}, global.mValue.getFloat());
            }
            else
            {
                json.number({}, global.mValue.getType() == ESM::VT_Short ? 0 : 1);
                json.number({}, global.mValue.getInteger());
            }
            json.endArray();
        }
        json.endObject();

        // The quick keys: slot (from 1), type (0 item, 1 spell, 2 item's magic) and the item or spell
        json.beginArray("quickKeys");
        for (size_t i = 0; i < save.quickKeys.mKeys.size() && i < 9; ++i)
        {
            const ESM::QuickKeys::QuickKey &key = save.quickKeys.mKeys[i];
            if (key.mType != ESM::QuickKeys::Type::Item && key.mType != ESM::QuickKeys::Type::Magic
                && key.mType != ESM::QuickKeys::Type::MagicItem)
                continue;
            json.beginArray();
            json.number({}, static_cast<double>(i + 1));
            json.number({}, static_cast<double>(key.mType));
            json.string({}, idString(key.mId));
            json.endArray();
        }
        json.endArray();

        // Where they were
        json.string("cell", cellDescription(save.player.mCellId));
        json.beginArray("position");
        for (float coordinate : state.mPosition.pos)
            json.number({}, coordinate);
        json.endArray();
        json.beginArray("rotation");
        json.number({}, state.mPosition.rot[0]);
        json.number({}, state.mPosition.rot[2]);
        json.endArray();

        json.endObject();
        return json.str();
    }

    // The character's id, beside its saves; made the first time it's needed
    std::string characterId(const std::filesystem::path &save)
    {
        std::filesystem::path file = save.parent_path() / "isowind-character.txt";
        std::string id;
        {
            std::ifstream in(file);
            std::getline(in, id);
        }
        id.erase(std::remove_if(id.begin(), id.end(), [](char c) { return std::isspace(static_cast<unsigned char>(c)); }),
            id.end());
        if (!id.empty())
            return id;

        std::random_device random;
        std::ostringstream text;
        for (int i = 0; i < 4; ++i)
            text << std::hex << std::setw(8) << std::setfill('0') << random();
        id = text.str();
        std::ofstream out(file);
        out << id << "\n";
        if (!out)
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_WARN, "Could not write %s", Files::pathToUnicodeString(file).c_str());
        return id;
    }

    // When the save was last written, in seconds since 1970
    double lastPlayed(const std::filesystem::path &save)
    {
        std::error_code error;
        auto fileTime = std::filesystem::last_write_time(save, error);
        if (error)
            return 0;
        auto systemTime = std::chrono::system_clock::now()
            + std::chrono::duration_cast<std::chrono::system_clock::duration>(
                fileTime - std::filesystem::file_time_type::clock::now());
        return std::chrono::duration<double>(systemTime.time_since_epoch()).count();
    }

    void sendMiscellaneous(LocalPlayer &player)
    {
        PlayerPacket *packet = Main::get().getNetworking()->getPlayerPacket(ID_PLAYER_MISCELLANEOUS);
        packet->setPlayer(&player);
        packet->Send();
    }

    // What the character gains on a server, beside its saves, in a save's terms: the player's inventory, NPC stats and
    // creature stats (an NpcState's own parts, in its order) in a record of their own; the records the server made
    // that the inventory and spells need (potions brewed, items enchanted, spells made there, and their
    // enchantments); and the journal and the topics known, as a save has them
    const char *const progressFileName = "isowind-server-progress.dat";
    const float progressInterval = 30;

    bool linked = false;
    float sinceWritten = 0;
    // Where the character's saves are (the one joined with, or one made on the server)
    std::filesystem::path progressFolder;

    // A record the server made (none of the game's own), kept to write with the progress
    template <class T>
    bool keepMade(const ESM::RefId &id, std::map<ESM::RefId, T> &records)
    {
        const MWWorld::Store<T> &store = MWBase::Environment::get().getESMStore()->get<T>();
        const T *record = store.search(id);
        if (record == nullptr || store.searchStatic(id) != nullptr)
            return false;
        records[id] = *record;
        return true;
    }

    template <class T>
    void keepMadeEnchanted(const ESM::RefId &id, std::map<ESM::RefId, T> &records, MadeRecords &made)
    {
        if (keepMade(id, records))
            keepMade(records[id].mEnchant, made.enchantments);
    }

    template <class T>
    void writeRecords(ESM::ESMWriter &writer, const std::map<ESM::RefId, T> &records)
    {
        for (const auto &[id, record] : records)
        {
            writer.startRecord(T::sRecordId);
            record.save(writer);
            writer.endRecord(T::sRecordId);
        }
    }

    void writeProgress(LocalPlayer &player)
    {
        if (!linked || progressFolder.empty() || !player.isLoggedIn())
            return;
        MWBase::World &world = *MWBase::Environment::get().getWorld();
        const MWWorld::ESMStore &store = *MWBase::Environment::get().getESMStore();
        MWWorld::Ptr ptr = MWMechanics::getPlayer();
        // Dead, they'd come back dead: the progress written last stays
        if (ptr.isEmpty() || !ptr.isInCell() || ptr.getClass().getCreatureStats(ptr).isDead())
            return;

        // The player as a save has them, and where they are
        ESM::Player record{};
        ptr.getBase()->save(record.mObject);
        if (!record.mObject.mHasCustomState)
            return;
        record.mCellId = ptr.getCell()->getCell()->getId();
        record.mBirthsign = world.getPlayer().getBirthSign();
        record.mHasMark = 0;
        record.mCurrentCrimeId = -1;
        record.mPaidCrimeId = -1;
        ESM::NpcState &state = record.mObject;
        // Every spell known (a state leaves out those its record has, and the ones learned are there)
        state.mCreatureStats.mSpells.mSpells.clear();
        for (const ESM::Spell *spell : ptr.getClass().getCreatureStats(ptr).getSpells())
            state.mCreatureStats.mSpells.mSpells.push_back(spell->mId);

        // Who they are: race, looks, name and class (a class the player made comes whole)
        const ESM::NPC &npc = *ptr.get<ESM::NPC>()->mBase;
        const ESM::Class *customClass
            = store.get<ESM::Class>().isDynamic(npc.mClass) ? store.get<ESM::Class>().search(npc.mClass) : nullptr;

        // What the launchers show of a character, as a save's header has it
        ESM::SavedGame profile;
        profile.mContentFiles = world.getContentFiles();
        profile.mPlayerName = npc.mName;
        profile.mPlayerLevel = ptr.getClass().getCreatureStats(ptr).getLevel();
        if (customClass != nullptr)
            profile.mPlayerClassName = customClass->mName;
        else
            profile.mPlayerClassId = npc.mClass;
        profile.mPlayerCellName = world.getCellName();
        profile.mInGameTime = world.getTimeManager()->getEpochTimeStamp();
        profile.mTimePlayed = 0;
        profile.mDescription = "From multiplayer";
        profile.mCurrentDay = world.getTimeManager()->getTimeStamp().getDay();
        profile.mCurrentHealth = ptr.getClass().getCreatureStats(ptr).getHealth().getCurrent();
        profile.mMaximumHealth = ptr.getClass().getCreatureStats(ptr).getHealth().getModified();

        MadeRecords made;
        for (const ESM::ObjectState &item : state.mInventory.mItems)
        {
            const ESM::RefId &id = item.mRef.mRefID;
            if (!keepMade(id, made.potions))
            {
                keepMadeEnchanted(id, made.weapons, made);
                keepMadeEnchanted(id, made.armors, made);
                keepMadeEnchanted(id, made.clothes, made);
                keepMadeEnchanted(id, made.books, made);
            }
        }
        for (const ESM::RefId &spell : state.mCreatureStats.mSpells.mSpells)
            keepMade(spell, made.spells);

        std::filesystem::path file = progressFolder / progressFileName;
        std::filesystem::path temporary = file;
        temporary += ".tmp";
        try
        {
            {
                std::ofstream out(temporary, std::ios::binary);
                ESM::ESMWriter writer;
                writer.setFormatVersion(ESM::CurrentSaveGameFormatVersion);
                writer.setVersion(0);
                writer.setType(0);
                writer.setAuthor("");
                writer.setDescription("");
                writer.save(out);
                writer.startRecord(ESM::REC_SAVE);
                profile.save(writer);
                writer.endRecord(ESM::REC_SAVE);
                writer.startRecord(ESM::REC_PLAY);
                record.save(writer);
                writer.endRecord(ESM::REC_PLAY);
                writer.startRecord(ESM::REC_NPC_);
                npc.save(writer);
                writer.endRecord(ESM::REC_NPC_);
                if (customClass != nullptr)
                {
                    writer.startRecord(ESM::REC_CLAS);
                    customClass->save(writer);
                    writer.endRecord(ESM::REC_CLAS);
                }
                writeRecords(writer, made.enchantments);
                writeRecords(writer, made.potions);
                writeRecords(writer, made.spells);
                writeRecords(writer, made.weapons);
                writeRecords(writer, made.armors);
                writeRecords(writer, made.clothes);
                writeRecords(writer, made.books);
                Loading::Listener listener;
                MWBase::Environment::get().getJournal()->write(writer, listener);
                MWBase::Environment::get().getDialogueManager()->write(writer, listener);
                // Kills (and things stolen, which stay behind), the map and the quick keys, as a save has them
                MWBase::Environment::get().getMechanicsManager()->write(writer, listener);
                MWBase::Environment::get().getWindowManager()->write(writer, listener);
                // The global variables the server keeps in step (quests' own among them)
                for (const std::string &id : Main::get().getNetworking()->getWorldstate()->synchronizedClientGlobalIds)
                {
                    char type = world.getGlobalVariableType(id);
                    if (type == ' ')
                        continue;
                    ESM::Global global;
                    global.mRecordFlags = 0;
                    global.mId = ESM::RefId::stringRefId(id);
                    if (type == 'f')
                    {
                        global.mValue.setType(ESM::VT_Float);
                        global.mValue.setFloat(world.getGlobalFloat(id));
                    }
                    else
                    {
                        global.mValue.setType(type == 's' ? ESM::VT_Short : ESM::VT_Long);
                        global.mValue.setInteger(world.getGlobalInt(id));
                    }
                    writer.startRecord(ESM::REC_GLOB);
                    global.save(writer);
                    writer.endRecord(ESM::REC_GLOB);
                }
                writer.close();
                if (!out)
                    throw std::runtime_error("writing failed");
            }
            // Whole or not at all
            std::filesystem::rename(temporary, file);
        }
        catch (const std::exception &e)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Could not write the character's progress to %s: %s",
                Files::pathToUnicodeString(file).c_str(), e.what());
        }
    }

    // What a progress file has, but its journal and topics (read in applyJournal, once these have gone on)
    struct Progress
    {
        ESM::Player player{};
        bool hasPlayer = false;
        ESM::NPC npc;
        bool hasNpc = false;
        std::map<ESM::RefId, ESM::Class> classes;
        MadeRecords made;
    };

    // (Filled in place: the player's state can't be copied)
    bool readProgress(const std::filesystem::path &file, Progress &progress)
    {
        ESM::ESMReader reader;
        reader.open(file);
        while (reader.hasMoreRecs())
        {
            ESM::NAME name = reader.getRecName();
            reader.getRecHeader();
            bool isDeleted = false;
            switch (name.toInt())
            {
                case ESM::REC_PLAY:
                    progress.player.load(reader);
                    progress.hasPlayer = true;
                    break;
                case ESM::REC_NPC_:
                    progress.npc.load(reader, isDeleted);
                    progress.hasNpc = true;
                    break;
                case ESM::REC_CLAS:
                {
                    ESM::Class record;
                    record.load(reader, isDeleted);
                    progress.classes[record.mId] = record;
                    break;
                }
                default:
                    if (!progress.made.read(reader, name.toInt()))
                        reader.skipRecord();
                    break;
            }
        }
        return progress.hasPlayer;
    }

    // A record as a save has it, but its id: two the same are the same thing
    template <class T>
    std::string recordBytes(T record)
    {
        record.mId = ESM::RefId();
        std::ostringstream out;
        ESM::ESMWriter writer;
        writer.setFormatVersion(ESM::CurrentSaveGameFormatVersion);
        writer.save(out);
        writer.startRecord(T::sRecordId);
        record.save(writer);
        writer.endRecord(T::sRecordId);
        writer.close();
        return out.str();
    }

    // The records the server made, made again here (under ids of this game's), or found here already (one made the
    // same before, coming back from a server once more): the ids they had there, to this one's
    std::map<ESM::RefId, ESM::RefId> remakeRecords(const MadeRecords &made)
    {
        MWWorld::ESMStore &store = *MWBase::Environment::get().getESMStore();
        std::map<ESM::RefId, ESM::RefId> ids;
        auto remake = [&](const auto &records) {
            for (auto [id, record] : records)
            {
                if constexpr (requires { record.mEnchant; })
                {
                    auto enchantment = ids.find(record.mEnchant);
                    if (enchantment != ids.end())
                        record.mEnchant = enchantment->second;
                }
                using Record = std::decay_t<decltype(record)>;
                const std::string bytes = recordBytes(record);
                ESM::RefId same;
                for (const Record &existing : store.get<Record>())
                {
                    if (store.get<Record>().isDynamic(existing.mId) && recordBytes(existing) == bytes)
                    {
                        same = existing.mId;
                        break;
                    }
                }
                ids[id] = same.empty() ? store.insert(record)->mId : same;
            }
        };
        remake(made.enchantments);
        remake(made.potions);
        remake(made.spells);
        remake(made.weapons);
        remake(made.armors);
        remake(made.clothes);
        remake(made.books);
        return ids;
    }

    // The character as they were on the server, on the single-player character: stats, skills, spells, the whole
    // inventory, factions, bounty and reputation. Everything else (where they are, their world) stays.
    void mergeProgress(ESM::NpcState &current, const ESM::NpcState &progress,
        const std::map<ESM::RefId, ESM::RefId> &remade)
    {
        const MWWorld::ESMStore &store = *MWBase::Environment::get().getESMStore();
        ESM::CreatureStats &creatureStats = current.mCreatureStats;
        ESM::NpcStats &npcStats = current.mNpcStats;
        const ESM::CreatureStats &serverCreatureStats = progress.mCreatureStats;
        const ESM::NpcStats &serverNpcStats = progress.mNpcStats;

        // An id the server's game had, as this one has it (a record it made, made again here); empty for one only that
        // game knew. (Whether this game has the record is up to the caller: the store finds items by id, not spells.)
        auto idHere = [&](const ESM::RefId &id) {
            auto found = remade.find(id);
            if (found != remade.end())
                return found->second;
            return isLocalOnlyId(id) ? ESM::RefId() : id;
        };

        for (size_t i = 0; i < creatureStats.mAttributes.size(); ++i)
        {
            creatureStats.mAttributes[i].mBase = serverCreatureStats.mAttributes[i].mBase;
            creatureStats.mAttributes[i].mDamage = serverCreatureStats.mAttributes[i].mDamage;
        }
        for (size_t i = 0; i < creatureStats.mDynamic.size(); ++i)
        {
            creatureStats.mDynamic[i].mBase = serverCreatureStats.mDynamic[i].mBase;
            creatureStats.mDynamic[i].mCurrent = serverCreatureStats.mDynamic[i].mCurrent;
        }
        creatureStats.mLevel = serverCreatureStats.mLevel;
        for (size_t i = 0; i < npcStats.mSkills.size(); ++i)
        {
            npcStats.mSkills[i].mBase = serverNpcStats.mSkills[i].mBase;
            npcStats.mSkills[i].mDamage = serverNpcStats.mSkills[i].mDamage;
            npcStats.mSkills[i].mProgress = serverNpcStats.mSkills[i].mProgress;
        }
        npcStats.mLevelProgress = serverNpcStats.mLevelProgress;
        npcStats.mSkillIncrease = serverNpcStats.mSkillIncrease;
        npcStats.mSpecIncreases = serverNpcStats.mSpecIncreases;
        npcStats.mFactions = serverNpcStats.mFactions;
        npcStats.mBounty = serverNpcStats.mBounty;
        npcStats.mReputation = serverNpcStats.mReputation;
        // Skill books read in either
        for (const ESM::RefId &book : serverNpcStats.mUsedIds)
        {
            if (std::find(npcStats.mUsedIds.begin(), npcStats.mUsedIds.end(), book) == npcStats.mUsedIds.end())
                npcStats.mUsedIds.push_back(book);
        }

        std::vector<ESM::RefId> spells;
        creatureStats.mSpells.mSpellParams.clear();
        for (const ESM::RefId &spell : serverCreatureStats.mSpells.mSpells)
        {
            ESM::RefId here = idHere(spell);
            if (here.empty() || store.get<ESM::Spell>().search(here) == nullptr)
                continue;
            spells.push_back(here);
            auto params = serverCreatureStats.mSpells.mSpellParams.find(spell);
            if (params != serverCreatureStats.mSpells.mSpellParams.end())
                creatureStats.mSpells.mSpellParams[here] = params->second;
        }
        creatureStats.mSpells.mSpells = spells;

        // The whole inventory as it was there, equipped as it was
        ESM::InventoryState inventory;
        for (size_t i = 0; i < progress.mInventory.mItems.size(); ++i)
        {
            const ESM::ObjectState &item = progress.mInventory.mItems[i];
            ESM::RefId here = idHere(item.mRef.mRefID);
            if (here.empty() || store.find(here) == 0)
                continue;
            auto slot = progress.mInventory.mEquipmentSlots.find(static_cast<uint32_t>(i));
            if (slot != progress.mInventory.mEquipmentSlots.end())
                inventory.mEquipmentSlots[static_cast<uint32_t>(inventory.mItems.size())] = slot->second;
            inventory.mItems.push_back(item);
            inventory.mItems.back().mRef.mRefID = here;
            // Numbered by the server's game, not this one's
            inventory.mItems.back().mRef.mRefNum = ESM::RefNum();
        }
        for (const auto &[id, magnitudes] : progress.mInventory.mPermanentMagicEffectMagnitudes)
        {
            ESM::RefId here = idHere(id);
            if (!here.empty() && store.find(here) != 0)
                inventory.mPermanentMagicEffectMagnitudes[here] = magnitudes;
        }
        current.mInventory = std::move(inventory);
    }

    // The rest of the progress, as a save has it, the server's in place of the single-player: the journal (quests,
    // its entries, the topics' entries) and the topics known, kills, the map, the quick keys (on the items and
    // spells as this game has them now) and the global variables the server kept in step
    void applyRecords(const std::filesystem::path &file, const std::map<ESM::RefId, ESM::RefId> &remade)
    {
        MWBase::World &world = *MWBase::Environment::get().getWorld();
        MWBase::WindowManager &windows = *MWBase::Environment::get().getWindowManager();
        ESM::ESMReader reader;
        reader.open(file);
        bool cleared = false;
        while (reader.hasMoreRecs())
        {
            ESM::NAME name = reader.getRecName();
            reader.getRecHeader();
            uint32_t type = name.toInt();
            if (type == ESM::REC_JOUR || type == ESM::REC_QUES)
            {
                if (!cleared)
                {
                    MWBase::Environment::get().getJournal()->clear();
                    cleared = true;
                }
                MWBase::Environment::get().getJournal()->readRecord(reader, type);
            }
            else if (type == ESM::REC_DIAS)
                MWBase::Environment::get().getDialogueManager()->readRecord(reader, type);
            else if (type == ESM::REC_DCOU)
                MWBase::Environment::get().getMechanicsManager()->readRecord(reader, type);
            else if (type == ESM::REC_GMAP)
                windows.readRecord(reader, type);
            else if (type == ESM::REC_KEYS)
            {
                ESM::QuickKeys keys;
                keys.load(reader);
                MWWorld::Ptr player = MWMechanics::getPlayer();
                MWWorld::InventoryStore &inventory = player.getClass().getInventoryStore(player);
                for (size_t i = 0; i < keys.mKeys.size() && i < 9; ++i)
                {
                    const int slot = static_cast<int>(i) + 1;
                    const ESM::QuickKeys::QuickKey &key = keys.mKeys[i];
                    auto found = remade.find(key.mId);
                    const ESM::RefId id = found != remade.end() ? found->second : key.mId;
                    if (key.mType == ESM::QuickKeys::Type::Item || key.mType == ESM::QuickKeys::Type::MagicItem)
                    {
                        auto item = std::find_if(inventory.begin(), inventory.end(),
                            [&](const MWWorld::Ptr &each) { return each.getCellRef().getRefId() == id; });
                        if (item != inventory.end())
                            windows.setQuickKey(slot, static_cast<int>(key.mType), *item, ESM::RefId());
                        else
                            windows.setQuickKey(slot, static_cast<int>(ESM::QuickKeys::Type::Unassigned),
                                MWWorld::Ptr(), ESM::RefId());
                    }
                    else if (key.mType == ESM::QuickKeys::Type::Magic
                        && player.getClass().getCreatureStats(player).getSpells().hasSpell(id))
                        windows.setQuickKey(slot, static_cast<int>(key.mType), MWWorld::Ptr(), id);
                    else
                        windows.setQuickKey(
                            slot, static_cast<int>(ESM::QuickKeys::Type::Unassigned), MWWorld::Ptr(), ESM::RefId());
                }
            }
            else if (type == ESM::REC_GLOB)
            {
                ESM::Global global;
                bool isDeleted = false;
                global.load(reader, isDeleted);
                const std::string id = idString(global.mId);
                const char kind = world.getGlobalVariableType(id);
                if (kind == 'f')
                    world.setGlobalFloat(id, global.mValue.getFloat());
                else if (kind == 's' || kind == 'l')
                    world.setGlobalInt(id, global.mValue.getInteger());
            }
            else
                reader.skipRecord();
        }
    }
}

std::string PortableCharacter::readName(const std::string &save)
{
    try
    {
        ESM::ESMReader reader;
        reader.open(Files::pathFromUnicodeString(save));
        if (reader.getRecName() != ESM::REC_SAVE)
            return {};
        reader.getRecHeader();
        ESM::SavedGame profile;
        profile.load(reader);
        return profile.mPlayerName;
    }
    catch (const std::exception &e)
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Could not read the character's save %s: %s", save.c_str(), e.what());
        return {};
    }
}

void PortableCharacter::sendInfo(LocalPlayer &player)
{
    std::filesystem::path save = Files::pathFromUnicodeString(Main::getCharacterSave());
    player.miscellaneousChangeType = MISCELLANEOUS_CHANGE_TYPE::PORTABLE_CHARACTER;
    player.portableId = characterId(save);
    player.portablePlayed = lastPlayed(save);
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Joining with the single-player character %s (%s)",
        player.portableId.c_str(), Main::getCharacterSave().c_str());
    sendMiscellaneous(player);
}

void PortableCharacter::send(LocalPlayer &player)
{
    std::string json;
    try
    {
        SaveContents save;
        readSave(Main::getCharacterSave(), save);
        if (!save.hasPlayer || !save.hasNpc)
            throw std::runtime_error("it has no player in it");
        json = toJson(save);
    }
    catch (const std::exception &e)
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Could not read the character's save %s: %s",
            Main::getCharacterSave().c_str(), e.what());
        // The server tells the player, and lets them go
        json = "{\"version\":1,\"error\":\"the save couldn't be read\"}";
    }

    // In pieces below the packets' longest string
    const size_t chunkSize = 60000;
    const unsigned int count = static_cast<unsigned int>(std::max<size_t>(1, (json.size() + chunkSize - 1) / chunkSize));
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending the single-player character (%zu bytes, %u packets)", json.size(),
        count);
    for (unsigned int i = 0; i < count; ++i)
    {
        player.miscellaneousChangeType = MISCELLANEOUS_CHANGE_TYPE::PORTABLE_DATA;
        player.portableChunk = i;
        player.portableChunkCount = count;
        player.portableData = json.substr(i * chunkSize, chunkSize);
        sendMiscellaneous(player);
    }
    player.portableData.clear();
}

namespace
{
    // A character's saves folder by the id kept in it, among the single-player characters; empty if there's none
    std::filesystem::path findCharacterFolder(const std::string &id)
    {
        std::error_code error;
        for (const auto &entry : std::filesystem::directory_iterator(Main::getSavesPath(), error))
        {
            if (!entry.is_directory())
                continue;
            std::ifstream in(entry.path() / "isowind-character.txt");
            std::string kept;
            std::getline(in, kept);
            kept.erase(std::remove_if(kept.begin(), kept.end(),
                           [](char c) { return std::isspace(static_cast<unsigned char>(c)); }),
                kept.end());
            if (kept == id)
                return entry.path();
        }
        return {};
    }

    // A saves folder for a character made on a server, named as the game names its characters' (letters and digits
    // of the name, a number after it if that's taken), with the character's id in it
    std::filesystem::path makeCharacterFolder(const std::string &name, const std::string &id)
    {
        std::string base;
        for (char c : name)
            base += (static_cast<unsigned char>(c) <= 0x7F && std::isalnum(static_cast<unsigned char>(c))) ? c : '_';
        std::filesystem::path folder = Main::getSavesPath() / base;
        for (int i = 1; std::filesystem::exists(folder); ++i)
            folder = Main::getSavesPath() / (base + " - " + std::to_string(i));
        std::filesystem::create_directories(folder);
        std::ofstream out(folder / "isowind-character.txt");
        out << id << "\n";
        return folder;
    }

    // The progress on the player: stats, skills, the whole inventory (the records the server made, made again here),
    // the spells known, factions, the journal and topics
    void putProgress(const std::filesystem::path &file, const Progress &progress)
    {
        MWWorld::Ptr ptr = MWMechanics::getPlayer();
        ESM::NpcState current;
        current.blank();
        ptr.getClass().writeAdditionalState(ptr, current);
        if (!current.mHasCustomState)
            throw std::runtime_error("the player has no state to put it on");
        const std::map<ESM::RefId, ESM::RefId> remade = remakeRecords(progress.made);
        mergeProgress(current, progress.player.mObject, remade);
        // The state read below adds the effects in force to those the player has (it's made for a player just
        // loaded), so those go first, or each ability's would be there twice, and come off its stat twice if the
        // ability ever went (a birthsign's +25 Speed is in the base stat). Taking them off undoes them on the stats,
        // which the state then sets as they should be.
        ptr.getClass().getCreatureStats(ptr).getActiveSpells().clear(ptr);
        ptr.getClass().readAdditionalState(ptr, current);
        // The spells known are the server's: those the player's record had here and the server's didn't go
        const std::vector<ESM::RefId> &spells = current.mCreatureStats.mSpells.mSpells;
        MWMechanics::Spells &known = ptr.getClass().getCreatureStats(ptr).getSpells();
        std::vector<ESM::RefId> forgotten;
        for (const ESM::Spell *spell : known)
        {
            if (std::find(spells.begin(), spells.end(), spell->mId) == spells.end())
                forgotten.push_back(spell->mId);
        }
        for (const ESM::RefId &spell : forgotten)
            known.remove(spell);
        for (const ESM::RefId &spell : spells)
        {
            if (!known.hasSpell(spell))
                known.add(spell);
        }
        applyRecords(file, remade);
    }
}

void PortableCharacter::setLinked(LocalPlayer &player)
{
    // (A server from before characters had ids: nothing to keep it under)
    if (player.portableId.empty() && Main::getCharacterSave().empty())
        return;
    linked = true;
    sinceWritten = 0;
    if (!Main::getCharacterSave().empty())
        progressFolder = Files::pathFromUnicodeString(Main::getCharacterSave()).parent_path();
    else
    {
        // A character made on the server: among the single-player characters too, in its own folder
        progressFolder = findCharacterFolder(player.portableId);
        if (progressFolder.empty())
        {
            try
            {
                MWWorld::Ptr ptr = MWMechanics::getPlayer();
                progressFolder = makeCharacterFolder(ptr.get<ESM::NPC>()->mBase->mName, player.portableId);
            }
            catch (const std::exception &e)
            {
                LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Could not make a saves folder for the character: %s", e.what());
                return;
            }
        }
    }
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "The character goes to single-player too: its progress goes to %s",
        Files::pathToUnicodeString(progressFolder).c_str());

    // The map as they know it (from their save, or what a server wrote back last): what they explore here goes on
    // it, and back with them
    std::filesystem::path mapSource = !Main::getCharacterSave().empty()
        ? Files::pathFromUnicodeString(Main::getCharacterSave())
        : progressFolder / progressFileName;
    std::error_code error;
    if (std::filesystem::exists(mapSource, error))
    {
        try
        {
            ESM::ESMReader reader;
            reader.open(mapSource);
            while (reader.hasMoreRecs())
            {
                ESM::NAME name = reader.getRecName();
                reader.getRecHeader();
                if (name.toInt() != ESM::REC_GMAP)
                {
                    reader.skipRecord();
                    continue;
                }
                MWBase::Environment::get().getWindowManager()->readRecord(reader, ESM::REC_GMAP);
                break;
            }
        }
        catch (const std::exception &e)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Could not read the character's map: %s", e.what());
        }
    }
}

void PortableCharacter::update(LocalPlayer &player, float dt)
{
    if (!linked)
        return;
    sinceWritten += dt;
    if (sinceWritten >= progressInterval)
    {
        sinceWritten = 0;
        writeProgress(player);
    }
}

void PortableCharacter::finish(LocalPlayer &player)
{
    writeProgress(player);
}

bool PortableCharacter::applyServerProgress(const std::filesystem::path &characterFolder,
    const std::filesystem::path &save)
{
    std::filesystem::path file = characterFolder / progressFileName;
    std::error_code error;
    if (!std::filesystem::exists(file, error))
        return false;
    try
    {
        // Played in single-player since (on another computer, say): that's the newer
        if (std::filesystem::last_write_time(file) <= std::filesystem::last_write_time(save))
        {
            Log(Debug::Info) << "The progress from multiplayer is older than " << save.filename() << ": dropped";
            std::filesystem::remove(file, error);
            return false;
        }

        Progress progress;
        if (!readProgress(file, progress))
            throw std::runtime_error("it has no character in it");
        putProgress(file, progress);
        Log(Debug::Info) << "Put the progress from multiplayer on the character";
        return true;
    }
    catch (const std::exception &e)
    {
        Log(Debug::Error) << "Could not put the progress from multiplayer (" << file << ") on the character: "
                          << e.what();
        return false;
    }
}

bool PortableCharacter::setUpServerCharacter(const std::filesystem::path &characterFolder)
{
    std::filesystem::path file = characterFolder / progressFileName;
    try
    {
        Progress progress;
        if (!readProgress(file, progress) || !progress.hasNpc)
            throw std::runtime_error("it has no character in it");
        const ESM::NPC &npc = progress.npc;
        MWBase::MechanicsManager &mechanics = *MWBase::Environment::get().getMechanicsManager();
        mechanics.setPlayerName(npc.mName);
        mechanics.setPlayerRace(npc.mRace, npc.isMale(), npc.mHead, npc.mHair);
        auto customClass = progress.classes.find(npc.mClass);
        if (customClass != progress.classes.end())
            mechanics.setPlayerClass(customClass->second);
        else
            mechanics.setPlayerClass(npc.mClass);
        mechanics.setPlayerBirthsign(progress.player.mBirthsign);
        Log(Debug::Info) << "Starting " << npc.mName << ", a character from multiplayer";
        return true;
    }
    catch (const std::exception &e)
    {
        Log(Debug::Error) << "Could not start the character from multiplayer (" << file << "): " << e.what();
        return false;
    }
}

bool PortableCharacter::finishServerCharacter(const std::filesystem::path &characterFolder)
{
    std::filesystem::path file = characterFolder / progressFileName;
    try
    {
        Progress progress;
        if (!readProgress(file, progress))
            throw std::runtime_error("it has no character in it");
        putProgress(file, progress);

        // Where they were on the server, if this game has the place (not one only the server made); as a teleport,
        // or the game puts them back where the new game started
        if (MWBase::Environment::get().getWorldModel()->findCell(progress.player.mCellId) != nullptr)
        {
            MWBase::World &world = *MWBase::Environment::get().getWorld();
            MWWorld::Ptr ptr = MWMechanics::getPlayer();
            ptr.getClass().getCreatureStats(ptr).land(true);
            ptr.getClass().getCreatureStats(ptr).setTeleported(true);
            world.getPlayer().setTeleported(true);
            const ESM::Position &position = progress.player.mObject.mPosition;
            world.changeToCell(progress.player.mCellId, position, false);
            // (and the physics told, as Lua's teleport does, or they're put back where they were)
            MWWorld::Ptr moved = world.getPlayerPtr();
            moved = world.moveObject(moved, position.asVec3());
            world.rotateObject(moved, position.asRotationVec3());
        }
        else
            Log(Debug::Warning) << "The character's cell from multiplayer isn't in this game: they start here";
        MWBase::Environment::get().getWindowManager()->updatePlayer();
        return true;
    }
    catch (const std::exception &e)
    {
        Log(Debug::Error) << "Could not start the character from multiplayer (" << file << "): " << e.what();
        return false;
    }
}

void PortableCharacter::clearServerProgress(const std::filesystem::path &characterFolder)
{
    std::error_code error;
    std::filesystem::remove(characterFolder / progressFileName, error);
}
}
