#ifndef OPENMW_BASEPLAYER_HPP
#define OPENMW_BASEPLAYER_HPP

#include <cstdint>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/npcstats.hpp>
#include <components/esm3/creaturestats.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadspel.hpp>

#include <components/openmw-mp/Base/BaseStructs.hpp>

#include <components/openmw-mp/RakNetIncludes.hpp>

namespace mwmp
{
    struct CurrentContainer
    {
        std::string refId;
        unsigned int refNum;
        unsigned int mpNum;
        bool loot;
    };

    struct JournalItem
    {
        std::string quest;
        int index;
        enum JOURNAL_ITEM_TYPE
        {
            ENTRY = 0,
            INDEX = 1
        };

        std::string actorRefId;

        bool hasTimestamp;
        mwmp::Time timestamp;

        int type; // 0 - An entire entry, 1 - An index
    };

    struct Faction
    {
        std::string factionId;
        int rank;
        int reputation;
        bool isExpelled;
    };

    struct Topic
    {
        std::string topicId;
    };

    struct Book
    {
        std::string bookId;
    };

    struct QuickKey
    {
        std::string itemId;

        enum QUICKKEY_TYPE
        {   
            ITEM = 0,
            MAGIC = 1,
            ITEM_MAGIC = 2,
            UNASSIGNED = 3
        };

        unsigned short slot;
        int type;
    };

    struct CellState
    {
        ESM::Cell cell;

        enum CELL_STATE_ACTION
        {
            LOAD = 0,
            UNLOAD = 1
        };

        int type; // 0 - Cell load, 1 - Cell unload
    };

    struct FactionChanges
    {
        std::vector<Faction> factions;

        enum FACTION_ACTION
        {
            RANK = 0,
            EXPULSION = 1,
            REPUTATION = 2
        };

        int action; // 0 - Rank, 1 - Expulsion state, 2 - Faction reputation
    };

    struct InventoryChanges
    {
        std::vector<Item> items;
        enum ACTION_TYPE
        {
            SET = 0,
            ADD,
            REMOVE
        };
        int action; // 0 - Clear and set in entirety, 1 - Add item, 2 - Remove item
    };

    struct SpellbookChanges
    {
        std::vector<ESM::Spell> spells;
        enum ACTION_TYPE
        {
            SET = 0,
            ADD,
            REMOVE
        };
        int action; // 0 - Clear and set in entirety, 1 - Add spell, 2 - Remove spell
    };

    enum RESURRECT_TYPE
    {
        REGULAR = 0,
        IMPERIAL_SHRINE,
        TRIBUNAL_TEMPLE
    };

    enum MISCELLANEOUS_CHANGE_TYPE
    {
        MARK_LOCATION = 0,
        SELECTED_SPELL,
        // Isowind: a character brought from single-player. The client says which one it is (its id and when it was
        // last played), the server asks for it, and the client sends it (JSON, in chunks). Logged in as it (or as a
        // character made on the server), the server says so with the character's id, and what the character gains
        // there is written back to single-player
        PORTABLE_CHARACTER,
        PORTABLE_REQUEST,
        PORTABLE_DATA,
        PORTABLE_LINKED
    };

    class BasePlayer
    {
    public:

        struct CharGenState
        {
            int currentStage, endStage;
            bool isFinished;
        };

        struct GUIMessageBox
        {
            int id;
            int type;
            enum GUI_TYPE
            {
                MessageBox = 0,
                CustomMessageBox,
                InputDialog,
                PasswordDialog,
                ListBox
            };
            std::string label;
            std::string note;
            std::string buttons;

            std::string data;
        };

        BasePlayer(RakNet::RakNetGUID guid) : guid(guid)
        {
            inventoryChanges.action = 0;
            spellbookChanges.action = 0;

            exchangeFullInfo = false;
            displayCreatureName = false;
            resetStats = false;
            enforcedLogLevel = -1;
        }

        BasePlayer()
        {

        }

        RakNet::RakNetGUID guid;

        GUIMessageBox guiMessageBox;

        // Track only the indexes of the attributes that have been changed,
        // with the attribute values themselves being stored in creatureStats.mAttributes
        std::vector<uint8_t> attributeIndexChanges;

        // Track only the indexes of the skills that have been changed,
        // with the skill values themselves being stored in npcStats.mSkills
        std::vector<uint8_t> skillIndexChanges;

        // Track only the indexes of the dynamic states that have been changed,
        // with the dynamicStats themselves being stored in creatureStats.mDynamic
        std::vector<uint8_t> statsDynamicIndexChanges;

        // Track only the indexes of the equipment items that have been changed,
        // with the items themselves being stored in equipmentItems
        std::vector<int> equipmentIndexChanges;

        bool exchangeFullInfo;

        InventoryChanges inventoryChanges;
        SpellbookChanges spellbookChanges;
        std::vector<SpellCooldown> cooldownChanges;
        SpellsActiveChanges spellsActiveChanges;
        std::vector<QuickKey> quickKeyChanges;
        std::vector<JournalItem> journalChanges;
        FactionChanges factionChanges;
        std::vector<Topic> topicChanges;
        std::vector<Book> bookChanges;
        std::vector<CellState> cellStateChanges;

        std::vector<RakNet::RakNetGUID> alliedPlayers;
        CurrentContainer currentContainer;

        int difficulty = 0;
        int enforcedLogLevel;
        float physicsFramerate = 60.0;
        bool consoleAllowed = false;
        bool bedRestAllowed = true;
        bool wildernessRestAllowed = true;
        bool waitAllowed = true;

        bool ignorePosPacket;

        unsigned int movementFlags = 0;
        char drawState;
        bool isJumping = false;
        bool isFlying = false;
        bool hasTcl = false;

        ESM::Position position;
        ESM::Position direction;
        ESM::Position previousCellPosition;
        ESM::Position momentum;
        ESM::Cell cell;
        ESM::NPC npc;
        ESM::NpcStats npcStats;
        ESM::Creature creature;
        ESM::CreatureStats creatureStats;
        ESM::Class charClass;
        Item equipmentItems[19];
        Attack attack;
        Cast cast;
        std::string birthsign;
        std::string chatMessage;
        CharGenState charGenState;
        std::map<std::string, std::string> gameSettings;
        std::map<std::string, std::string> vrSettings;

        std::string sound;
        Animation animation;
        char deathState;

        bool resetStats = false;
        float scale = 1;
        bool isWerewolf = false;

        bool displayCreatureName;
        std::string creatureRefId;

        bool isChangingRegion;

        Target killer;

        int jailDays;
        bool ignoreJailTeleportation;
        bool ignoreJailSkillIncreases;
        std::string jailProgressText;
        std::string jailEndText;

        unsigned int resurrectType;
        unsigned int miscellaneousChangeType;

        ESM::Cell markCell;
        ESM::Position markPosition;
        std::string selectedSpellId;

        std::string portableId;
        double portablePlayed = 0; // seconds since 1970
        std::string portableData;
        unsigned int portableChunk = 0;
        unsigned int portableChunkCount = 0;

        mwmp::Item usedItem;
        bool usingItemMagic;
        char itemUseDrawState;
    };

    template <>
    struct IsRawWireType<BasePlayer::CharGenState> : std::true_type
    {
    };
}

#endif //OPENMW_BASEPLAYER_HPP
