#ifndef OPENMW_BASESTRUCTS_HPP
#define OPENMW_BASESTRUCTS_HPP

#include <string>

#include <components/esm3/activespells.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/statstate.hpp>
#include <components/esm/position.hpp>
#include <components/esm3/loadclas.hpp>

#include <components/openmw-mp/Packets/RawWireType.hpp>

#include <components/openmw-mp/RakNetIncludes.hpp>

namespace mwmp
{
    namespace DialogueChoiceType
    {
        enum DIALOGUE_CHOICE
        {
            TOPIC,
            PERSUASION,
            COMPANION_SHARE,
            BARTER,
            SPELLS,
            TRAVEL,
            SPELLMAKING,
            ENCHANTING,
            TRAINING,
            REPAIR
        };
    }

    enum PACKET_ORIGIN
    {
        CLIENT_GAMEPLAY = 0,
        CLIENT_CONSOLE = 1,
        CLIENT_DIALOGUE = 2,
        CLIENT_SCRIPT_LOCAL = 3,
        CLIENT_SCRIPT_GLOBAL = 4,
        SERVER_SCRIPT = 5
    };

    enum VARIABLE_TYPE
    {
        SHORT,
        LONG,
        FLOAT,
        INT,
        STRING
    };

    struct ClientVariable
    {
        std::string id;
        int internalIndex;

        char variableType;

        int intValue;
        float floatValue;
        std::string stringValue;
    };

    struct Time
    {
        float hour;
        int day;
        int month;
        int year;

        int daysPassed;
        float timeScale;
    };

    struct Item
    {
        std::string refId;
        int count;
        int charge;
        float enchantmentCharge;
        std::string soul;

        inline bool operator==(const Item& rhs)
        {
            return refId == rhs.refId && count == rhs.count && charge == rhs.charge &&
                enchantmentCharge == rhs.enchantmentCharge && soul == rhs.soul;
        }
    };

    struct ProjectileOrigin
    {
        float origin[3];
        float orientation[4];
    };
    
    struct Target
    {
        bool isPlayer;

        std::string refId;
        unsigned int refNum;
        unsigned int mpNum;

        std::string name; // Remove this once the server can get names corresponding to different refIds

        RakNet::RakNetGUID guid;
    };

    class Attack
    {
    public:

        Target target;

        enum TYPE
        {
            MELEE = 0,
            RANGED
        };

        char type;
        std::string attackAnimation;

        std::string rangedWeaponId;
        std::string rangedAmmoId;

        ESM::Position hitPosition;
        ProjectileOrigin projectileOrigin;

        float damage = 0;
        float attackStrength = 0;

        bool isHit = false;
        bool success = false;
        bool block = false;
        
        bool pressed = false;
        bool instant = false;
        bool knockdown = false;
        bool applyWeaponEnchantment = false;
        bool applyAmmoEnchantment = false;

        bool shouldSend = false;
    };

    class Cast
    {
    public:

        Target target;

        char type; // 0 - regular magic, 1 - item magic
        enum TYPE
        {
            REGULAR = 0,
            ITEM
        };

        std::string spellId; // id of spell (e.g. "fireball")
        std::string itemId;

        bool hasProjectile = false;
        ProjectileOrigin projectileOrigin;

        bool isHit;
        bool success;
        bool pressed;
        bool instant;

        bool shouldSend;
    };

    struct SpellCooldown
    {
        std::string id;
        int startTimestampDay;
        double startTimestampHour;
    };

    struct ActiveSpell
    {
        std::string id;
        bool isStackingSpell;
        int timestampDay;
        double timestampHour;
        Target caster;
        ESM::ActiveSpells::ActiveSpellParams params;
    };

    struct SpellsActiveChanges
    {
        std::vector<ActiveSpell> activeSpells;
        enum ACTION_TYPE
        {
            SET = 0,
            ADD,
            REMOVE
        };
        int action; // 0 - Clear and set in entirety, 1 - Add spell, 2 - Remove spell
    };

    struct Animation
    {
        std::string groupname;
        int mode;
        int count;
        bool persist;
    };

    struct SimpleCreatureStats
    {
        ESM::StatState<float> mDynamic[3];
        bool mDead;
        bool mDeathAnimationFinished;
    };

    // Plain records the packets send byte for byte (checked against OpenMW 0.51: numbers only).
    template <>
    struct IsRawWireType<ESM::Position> : std::true_type
    {
    };
    template <>
    struct IsRawWireType<ESM::Cell::DATAstruct> : std::true_type
    {
    };
    template <>
    struct IsRawWireType<ESM::Class::CLDTstruct> : std::true_type
    {
    };
    template <>
    struct IsRawWireType<ESM::StatState<float>> : std::true_type
    {
    };
}

#endif //OPENMW_BASESTRUCTS_HPP
