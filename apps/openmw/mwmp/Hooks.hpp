#ifndef OPENMW_MWMP_HOOKS_HPP
#define OPENMW_MWMP_HOOKS_HPP

#include <string>
#include <utility>
#include <vector>

#include <components/esm/refid.hpp>
#include <components/esm3/loadcell.hpp>

#include "../mwmechanics/activespells.hpp"
#include "../mwworld/doorstate.hpp"
#include "../mwworld/ptr.hpp"

namespace ESM
{
    struct Position;
    struct Potion;
    struct Enchantment;
    struct Armor;
    struct Book;
    struct Clothing;
    struct Weapon;
    struct Spell;
}

namespace mwmp
{
    class Attack;
    class Cast;
}

namespace Interpreter
{
    class Context;
}

namespace osg
{
    class Quat;
}

/// Isowind multiplayer: what the game engine tells the server, or asks it for, from the places where things happen.
/// Everything here does nothing in single player.
namespace mwmp::hooks
{
    bool isMultiplayer();
    bool isLoggedIn();

    bool isLocalPlayer(const MWWorld::Ptr& ptr);
    bool isDedicatedPlayer(const MWWorld::Ptr& ptr);
    /// An NPC or creature whose cell this client has authority over
    bool isLocalActor(const MWWorld::Ptr& ptr);
    /// An NPC or creature another client has authority over
    bool isDedicatedActor(const MWWorld::Ptr& ptr);
    /// The local player, or an NPC or creature this client has authority over: their actions are this client's to
    /// send
    bool isLocallyControlled(const MWWorld::Ptr& actor);
    /// Another player, or an NPC or creature another client has authority over: their actions come from packets
    bool isRemotelyControlled(const MWWorld::Ptr& actor);

    /// Activation of objects by the local player (or by local actors' AI) goes through the server: this sends the
    /// request and returns true if it did, and the activation then happens when the server sends it back
    /// (mwmp::ObjectList::activateObjects). Returns false when the activation should just happen here.
    bool requestActivation(const MWWorld::Ptr& object, const MWWorld::Ptr& actor);

    void sendObjectSound(const MWWorld::Ptr& ptr, const ESM::RefId& soundId, float volume = 1.f, float pitch = 1.f);
    void sendObjectDelete(const MWWorld::Ptr& ptr);
    void sendObjectLock(const MWWorld::Ptr& ptr, int lockLevel);
    void sendObjectTrap(const MWWorld::Ptr& ptr, const ESM::Position& pos, bool isDisarmed);
    void sendObjectState(const MWWorld::Ptr& ptr, bool enabled);
    void sendObjectScale(const MWWorld::Ptr& ptr, float scale);
    void sendObjectAnimPlay(const MWWorld::Ptr& ptr, const std::string& group, int mode);
    void sendDoorState(const MWWorld::Ptr& door, MWWorld::DoorState state);
    /// An object placed in the world (dropped, or by a script): the server gives it its own number and sends it to
    /// everyone, this client included
    void sendObjectPlace(const MWWorld::Ptr& ptr, bool droppedByPlayer);
    void sendObjectSpawn(const MWWorld::Ptr& ptr);

    /// Using an item from the local player's inventory goes through the server: this sends the request and
    /// returns true if it did; the item gets used when the server sends it back (ProcessorPlayerItemUse).
    /// drawState: what the player should hold ready afterwards (MWMechanics::DrawState)
    bool requestItemUse(const MWWorld::Ptr& item, bool usingItemMagic = false, int drawState = 0);
    /// The player selected a spell: the server keeps it between sessions
    void sendSelectedSpell(const ESM::RefId& spellId);

    /// Items going into or out of a container (or actor) in the world. action: mwmp::BaseObjectList::ADD/REMOVE,
    /// subAction: mwmp::BaseObjectList::NONE/DRAG/DROP/TAKE_ALL. items: the items with their counts.
    void sendContainerChange(const MWWorld::Ptr& container, int action, int subAction,
        const std::vector<std::pair<MWWorld::Ptr, int>>& items);
    /// Whether this is the container the local player has open
    bool isCurrentContainer(const MWWorld::Ptr& container);

    /// Another player, or an NPC another client has authority over, only changes cells when their packets say so:
    /// whether this move into newCell should be refused
    bool denyRemoteCellChange(const MWWorld::Ptr& ptr, const MWWorld::CellStore& newCell);
    /// An object moved to another cell has a new Ptr: the multiplayer classes that keep its Ptr get the new one
    void updatePtr(const MWWorld::Ptr& oldPtr, const MWWorld::Ptr& newPtr);

    /// Set while carrying out a change the server sent, so it isn't reported back to it
    struct ServerChange
    {
        ServerChange();
        ~ServerChange();
        static bool isActive();
    };

    // --- Combat. Attacks by the local player and local actors are sent to the other clients, which play them on
    // their DedicatedPlayers and DedicatedActors (MechanicsHelper::processAttack); hits are worked out by the
    // attacker's client only.

    /// The attack being prepared for sending for a local player or actor, or nullptr
    mwmp::Attack* getLocalAttack(const MWWorld::Ptr& actor);
    /// The latest attack received about another player or remote actor, or nullptr
    mwmp::Attack* getDedicatedAttack(const MWWorld::Ptr& actor);
    mwmp::Cast* getLocalCast(const MWWorld::Ptr& actor);
    mwmp::Cast* getDedicatedCast(const MWWorld::Ptr& actor);

    /// Every frame for each actor: a local one starting or releasing an attack with a weapon (or fists) is sent
    void updateLocalAttackState(const MWWorld::Ptr& actor);
    /// The attack animation (chop, slash, thrust, shoot) a local actor starts
    void recordAttackAnimation(const MWWorld::Ptr& actor, const std::string& attackType);
    /// A local melee attack reaching its target: what is known before the damage is worked out
    void prepareLocalHit(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, const osg::Vec3f& hitPosition,
        bool success, bool appliedEnchantment);
    /// A local melee swing that hit no one
    void finishLocalMiss(const MWWorld::Ptr& attacker);
    /// The damage of a local attacker's hit is known (Class::onHit): the attack is sent, with whether it knocked
    /// the victim down. ranged: from a projectile (else melee); other sources (magic, falling) aren't attacks.
    void finishLocalHit(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, float damage, bool success);
    /// Whether a hit knocks the victim down: as the attacker's client decided, for remote attackers
    bool decideKnockdown(const MWWorld::Ptr& attacker, bool localDecision);
    /// Whether a melee attack is blocked: as the attacker's client decided, for remote attackers; recorded for
    /// local ones
    bool decideBlock(const MWWorld::Ptr& attacker, bool localDecision);
    /// The killer of the local player or a local actor, for the server
    void recordKiller(const MWWorld::Ptr& victim, const MWWorld::Ptr& killer);

    /// A ranged weapon (or thrown one) released by an actor: for local ones the details are sent, for remote ones
    /// those received are used instead (strength, where the projectile starts and which way it goes)
    void onProjectileReleased(const MWWorld::Ptr& actor, const MWWorld::Ptr& weapon, const MWWorld::Ptr& ammo,
        float& attackStrength, osg::Vec3f& launchPos, osg::Quat& orient);
    /// A projectile of a local attacker landing; returns false when it is a remote attacker's, which is ignored here
    bool onProjectileHit(const MWWorld::Ptr& attacker, bool hasVictim, const osg::Vec3f& hitPosition,
        bool success, bool appliedEnchantment);

    /// An actor's death animation starts: the local player's or a local actor's death is sent (the server then
    /// decides, e.g. respawning the player). Actors whose death animation has already finished elsewhere start it at
    /// the end (startpoint).
    void onDeathAnimation(const MWWorld::Ptr& actor, int deathState, float& startpoint);
    void onDeathAnimationFinished(const MWWorld::Ptr& actor);

    /// A scripted animation (PlayGroup, LoopGroup) on a local actor: other clients play it too
    void recordAnimationPlay(const MWWorld::Ptr& actor, std::string_view group, int mode, int count, bool persist);

    void sendFactionExpulsion(const ESM::RefId& factionId, bool expelled);
    /// A guard reached the player to arrest them
    void onArrestAttempt();
    /// Whether the player has died since the last attempt to arrest them (guards then let them be)
    bool hasDiedSinceArrestAttempt();

    /// Whether other is a player allied to the player actor (both the local player or other players), for siding
    /// with each other (companions forgive their hits, they don't give away each other's sneaking)
    bool isAlliedPlayer(const MWWorld::Ptr& actor, const MWWorld::Ptr& other);
    /// A summoned creature going away: the server removes it for everyone, when this client is the one to say so
    void sendSummonRemoval(const MWWorld::Ptr& creature);

    // --- Spells. Casts by local actors are sent like attacks; spells landing on other players and remote actors are
    // applied by their own clients, which send the resulting active spells (SpellsActive packets).

    /// A local actor starts casting (the cast animation): sent so other clients play it
    void onLocalCastStart(const MWWorld::Ptr& caster, const ESM::RefId& spellId, bool isMagicItem);
    /// A local actor casts an enchanted item's spell
    void onLocalItemCast(const MWWorld::Ptr& caster, const MWWorld::Ptr& item);
    /// Whether a spell cast succeeds: rolled here for local casters (and sent), as their client said for remote
    bool decideSpellSuccess(const MWWorld::Ptr& caster, bool localRoll);
    /// Whether magicka and the like are used up here for a cast: not for remote casters, whose clients do it
    bool isCastPaidHere(const MWWorld::Ptr& caster);
    /// A magic bolt launched: where it starts and which way, sent for local casters, as received for remote ones
    void onMagicBoltLaunched(const MWWorld::Ptr& caster, osg::Vec3f& pos, osg::Quat& orient);
    /// A Lock or Open spell on an object: in multiplayer the server does it (asked by the caster's client); returns
    /// true if it is handled that way
    bool requestSpellLock(const MWWorld::Ptr& caster, const MWWorld::Ptr& target, int lockLevel);
    void sendMarkLocation(const MWWorld::Ptr& player);
    /// A summon effect: the creature is spawned by the server (asked by the summoner's client), and linked to the
    /// effect when it arrives (linkSummon). Returns the creature, or an unset RefNum until then.
    ESM::RefNum requestSummon(const ESM::RefId& effectId, const MWWorld::Ptr& summoner,
        const ESM::RefId& spellId, float duration);
    /// The server's summoned creature arrived: it becomes the one of the summoner's effect
    void linkSummon(const MWWorld::Ptr& summoner, const ESM::RefId& effectId, ESM::RefNum creature);
    /// An active spell starts on, or wears off, an actor: sent for the local player and local actors
    void onActiveSpellAdded(const MWWorld::Ptr& actor, const MWMechanics::ActiveSpells::ActiveSpellParams& params);
    void onActiveSpellExpired(const MWWorld::Ptr& actor, const MWMechanics::ActiveSpells::ActiveSpellParams& params);
    /// The local player's spellbook changes (diseases, cures), and powers used (once a day)
    void sendSpellbookChange(const MWWorld::Ptr& actor, const ESM::RefId& spellId, bool added);
    void sendPowerUse(const MWWorld::Ptr& actor, const ESM::RefId& spellId);

    /// Items going into or out of the local player's inventory: the server keeps it (unless it is the server's own
    /// inventory update being applied). count: how many (gold: its value)
    void onPlayerItemAdded(const MWWorld::Ptr& container, const MWWorld::Ptr& item, int count);
    void onPlayerItemRemoved(const MWWorld::Ptr& container, const MWWorld::Ptr& item, int count);

    // --- The local player's progress, kept by the server: these are sent when they change here, except while the
    // server's own data is being applied (ServerChange)
    void onJournalEntry(const ESM::RefId& quest, int index, const MWWorld::Ptr& actor);
    void onJournalIndex(const ESM::RefId& quest, int index);
    void onTopicLearned(const ESM::RefId& topic);
    /// stats: whose they are (only the local player's are sent)
    void onFactionRank(const void* stats, const ESM::RefId& faction, int rank);
    void onFactionExpulsion(const void* stats, const ESM::RefId& faction, bool expelled);
    void onFactionReputation(const void* stats, const ESM::RefId& faction, int reputation);
    /// spells: whose spell list it is (only the local player's are sent)
    void onSpellbookChange(const void* spells, const ESM::RefId& spellId, bool added);

    // --- Crafting: potions, enchanted items and spells made here become records of the server's (with ids everyone
    // shares), which it then gives to the player. In multiplayer these send the new record and return true, and the
    // item or spell isn't made here.
    bool sendPotionRecord(const ESM::Potion& potion, int count);
    bool sendEnchantmentRecord(const ESM::Enchantment& enchantment, int itemCount);
    /// The enchanted item's record, based on the item that was enchanted
    bool sendEnchantedItemRecord(const ESM::Armor* armor, const ESM::Book* book, const ESM::Clothing* clothing,
        const ESM::Weapon* weapon, const ESM::RefId& baseId);
    bool sendSpellRecord(const ESM::Spell& spell);
    /// While crafting removes many ingredients, the inventory changes are sent once at the end
    /// The player's items changed in a way the item packets don't tell (repaired, recharged): the whole inventory
    void sendPlayerInventory();
    void beginBatchedItemRemovals();
    void endBatchedItemRemovals();

    // --- Scripts (mwscript). What they change in the shared world goes through the server, which knows which
    // script asked (local, global, dialogue, console) and decides; objects without a network identity (made on
    // this client only) are changed here as usual. Each returns true if it sent the request, so the change isn't
    // made here.
    bool requestScriptObjectState(const Interpreter::Context& context, const MWWorld::Ptr& ptr, bool enabled);
    bool requestScriptLock(const Interpreter::Context& context, const MWWorld::Ptr& ptr, int lockLevel);
    bool requestScriptDelete(const Interpreter::Context& context, const MWWorld::Ptr& ptr);
    void sendScriptVideo(const Interpreter::Context& context, const std::string& name, bool allowSkipping);
    /// Script variables: those of scripts (and globals) the server asked to hear about are sent when they change
    void onScriptLocalChanged(const Interpreter::Context& context, bool sendPackets, const MWWorld::Ptr& ptr,
        int index, int value, bool isLong);
    void onScriptLocalFloatChanged(
        const Interpreter::Context& context, bool sendPackets, const MWWorld::Ptr& ptr, int index, float value);
    void onScriptGlobalChanged(bool sendPackets, std::string_view name, int value, bool isLong);
    void onScriptGlobalFloatChanged(bool sendPackets, std::string_view name, float value);
    void onScriptMemberShortChanged(
        const Interpreter::Context& context, bool sendPackets, const ESM::RefId& scriptId, int index, int value);
    /// SetScale: through the server (players' own scales are the server's alone); true if handled
    bool requestScriptScale(const Interpreter::Context& context, const MWWorld::Ptr& ptr, float scale);
    /// An object placed by a script (PlaceAt, PlaceItem, ...): the server places it, with its own number, for everyone
    /// including us, so the one placed here goes. True if handled that way.
    bool requestScriptPlacement(const Interpreter::Context& context, const MWWorld::Ptr& placed);
    /// An actor a script (or a follower travelling with the player) moved to another cell: other clients are told
    void sendActorCellChange(const MWWorld::Ptr& actor, const ESM::Cell& originalCell, bool isFollower);
    /// A remote actor this client moves anyway (a follower coming through a door with the player)
    void allowRemoteCellChange(const MWWorld::Ptr& actor, const MWWorld::CellStore& newCell);
    /// AddItem / RemoveItem on a container or NPC (not the player): the server hears of it. Returns true if the change
    /// is the server's alone to make (console commands), so it isn't made here.
    bool sendScriptContainerChange(
        const Interpreter::Context& context, const MWWorld::Ptr& ptr, const ESM::RefId& itemId, int count, bool added);

    /// AI given by scripts (combat with the player, following, StartCombat): the client with authority over the actor
    /// is told, through the server. aiAction: mwmp::BaseActorList::COMBAT or FOLLOW
    void sendScriptAi(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, int aiAction);
    /// PlayGroup on an object, by a script the server hears from: other clients play it too
    void sendScriptAnimation(const Interpreter::Context& context, const MWWorld::Ptr& ptr, const std::string& group,
        int mode);
    void sendScriptMusic(const Interpreter::Context& context, const std::string& music);
    /// The player's collision toggled (TCL), and werewolf state changes: the server keeps them
    void setNoClip(bool noClip);
    void sendWerewolfState(const MWWorld::Ptr& actor, bool werewolf);
    /// Whether the server lets the player rest in beds (asked when a bed's rest menu would open)
    bool mayRestInBed();

    /// Whether the server wants to hear about the variables of this script
    bool isScriptSynchronized(const ESM::RefId& scriptId);
    /// A console command run here (not one the server runs on this client): reported to the server
    void sendConsoleCommand(const std::string& command, const MWWorld::Ptr& target);

    /// A cell's objects changed (an actor spawned, placed or moved in): in a cell this client has authority over,
    /// its actors are looked at again, so new ones become LocalActors that other clients see
    void onCellRefsChanged(const MWWorld::CellStore& cell);

    /// Whether this actor has a collision body: the server can turn it off for other players, or for NPCs
    bool hasActorCollision(const MWWorld::Ptr& actor);

    /// Isowind: another player, as the maps show them
    struct OtherPlayer
    {
        std::string name;
        float x = 0.f;
        float y = 0.f;
        float yaw = 0.f;
        bool exterior = true;
        std::string cellName; // of an interior
    };
    /// The other players on the server (none in single player)
    std::vector<OtherPlayer> getOtherPlayers();

    /// Where a door leads: the server can make doors to an interior lead to another one instead
    ESM::RefId getDoorDestination(const ESM::RefId& destination);
    /// Whether the server wants objects without collision of their own (items) to collide, and how
    bool hasEnforcedCollision(const MWWorld::Ptr& ptr, bool& asActor);
}

#endif
