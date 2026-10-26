#ifndef OPENMW_MECHANICSHELPER_HPP
#define OPENMW_MECHANICSHELPER_HPP

#include <components/openmw-mp/Base/BaseStructs.hpp>

#include "../mwworld/containerstore.hpp"
#include "../mwworld/ptr.hpp"

#include <osg/Vec3f>


namespace MechanicsHelper
{
    osg::Vec3f getLinearInterpolation(osg::Vec3f start, osg::Vec3f end, float percent);
    ESM::Position getPositionFromVector(osg::Vec3f vector);

    void spawnLeveledCreatures(MWWorld::CellStore* cellStore);
    /// Isowind MMO: an actor the server brings back (respawns), alive and fresh at its starting place, as when
    /// Morrowind respawns one, in front of the players there too. Returns whether it was done (not for cells this
    /// game hasn't loaded, where it's as it starts anyway).
    bool respawnActor(const MWWorld::Ptr& ptr);
    /// Isowind MMO: the server's commands about a one-off event's people, which each player's game runs itself
    /// (CellController::setClientLocalActor): "isowind local" (the first time this session, the actor starts over as
    /// the game's files have it, undoing what others' games sent about it) and "isowind local dead <x> <y> <z>" (its
    /// body, where it died for this player). Things among the objects are left alone. Returns false for other commands.
    bool runClientLocalCommand(const std::string& command, MWWorld::CellStore* cellStore, unsigned int refNum,
        unsigned int mpNum, const std::string& refId);
    /// Isowind MMO: the server hears of the death of one of a one-off event's people that this client runs itself,
    /// with where the body lies, as that player's (a ClientScriptLocal packet as gameplay, floats 0 to 2 the place)
    void sendClientLocalDeath(const MWWorld::Ptr& actor);

    /// Isowind MMO: someone from the world told to follow this player, who has their own of it (the others keep the
    /// world's at its post): the server's commands "isowind own <n>" (just told to follow here: this client's from now
    /// on, as it is), "isowind follow <n>" (at the player's side, following, after logging in) and "isowind stay <x> <y> <z>
    /// <cell>" / "isowind stay dead <x> <y> <z> <cell>" (where it stopped following, or its body), the objects' cell
    /// being where the game's files have it. Returns false for other commands.
    bool runPrivateFollowerCommand(const std::string& command, MWWorld::CellStore* cellStore, unsigned int refNum,
        unsigned int mpNum, const std::string& refId);
    bool isPrivateFollower(const MWWorld::Ptr& ptr);
    /// One of them went to another cell (mwmp::hooks::updatePtr)
    void onPrivateFollowerMoved(const MWWorld::Ptr& ptr);
    /// Someone else's follower as this game shows it: a look-alike (a record like the actor's, with no script) that
    /// can't be talked to or hurt here, moved by what its owner's game sends about it (as about a copy numbered 0-<n>,
    /// n from mwmp::FOLLOWER_COPY_BASE); the server's commands "isowind copy <x> <y> <z>" (it's there; the objects'
    /// refId the actor it looks like) and "isowind uncopy" (it no longer follows them)
    bool isFollowerCopy(const MWWorld::Ptr& ptr);
    /// The player used someone's follower as this game shows it: the server hands it over if they're in the player's
    /// party ("isowind take <n> <flags> <x> <y> <z> <cell>", and "isowind release" to the game that had it)
    void requestFollowerCopy(const MWWorld::Ptr& copy);
    /// This player's own follower whose copy the others' games know by that number (what they send about hitting it)
    MWWorld::Ptr getPrivateFollowerByCopy(unsigned int mpNum);
    /// The look-alike in a cell this game has in use, made or brought there, for what's sent about it there
    MWWorld::Ptr getFollowerCopy(MWWorld::CellStore* store, unsigned int mpNum);
    /// Tells the server, every few seconds, of the private followers that changed: following, staying (and where) or
    /// dead (and where the body lies), as for sendClientLocalDeath with short 0 the state
    void updatePrivateFollowers(float dt);

    bool isUsingRangedWeapon(const MWWorld::Ptr& ptr);

    mwmp::Attack *getLocalAttack(const MWWorld::Ptr& ptr);
    mwmp::Attack *getDedicatedAttack(const MWWorld::Ptr& ptr);

    mwmp::Cast *getLocalCast(const MWWorld::Ptr& ptr);
    mwmp::Cast *getDedicatedCast(const MWWorld::Ptr& ptr);

    MWWorld::Ptr getPlayerPtr(const mwmp::Target& target);
    // The actor a target refers to, or an empty Ptr.
    MWWorld::Ptr getTargetPtr(const mwmp::Target& target);

    mwmp::Item getItem(const MWWorld::Ptr& itemPtr, int count);
    mwmp::Target getTarget(const MWWorld::Ptr& ptr);
    void clearTarget(mwmp::Target& target);
    bool isEmptyTarget(const mwmp::Target& target);

    void assignAttackTarget(mwmp::Attack* attack, const MWWorld::Ptr& target);
    void resetAttack(mwmp::Attack* attack);
    void resetCast(mwmp::Cast* cast);

    // See whether playerChecked belongs to playerWithTeam's team
    // Note: This is not supposed to also check if playerWithTeam is on playerChecked's
    //       team, because it should technically be possible to be allied to someone
    //       who isn't mutually allied to you
    bool isTeamMember(const MWWorld::Ptr& playerChecked, const MWWorld::Ptr& playerWithTeam);

    bool getSpellSuccess(const std::string& spellId, const MWWorld::Ptr& caster);

    void processAttack(mwmp::Attack attack, const MWWorld::Ptr& attacker);
    void processCast(mwmp::Cast cast, const MWWorld::Ptr& caster);

    void createSpellGfx(const MWWorld::Ptr& targetPtr, const std::vector<ESM::ActiveEffect>& mEffects);

    // Active spells as other clients describe them: added to, or removed from, an actor here.
    /// restored: an effect that was already running (logging in, a cell loading), which plays no hit sound again
    void addActiveSpell(const MWWorld::Ptr& actor, const mwmp::ActiveSpell& activeSpell, bool restored = false);
    void removeActiveSpell(const MWWorld::Ptr& actor, const mwmp::ActiveSpell& activeSpell);

    bool isStackingSpell(const std::string& id);
    bool doesEffectListContainEffect(const ESM::EffectList& effectList, const ESM::RefId& effectId,
        const ESM::RefId& attributeId = {}, const ESM::RefId& skillId = {});
    void unequipItemsByEffect(const MWWorld::Ptr& ptr, int enchantmentType, const ESM::RefId& effectId,
        const ESM::RefId& attributeId = {}, const ESM::RefId& skillId = {});

    // A merchant's gold pool changes: in multiplayer the server decides (this sends it the new value), otherwise
    // it is set here.
    void setGoldPool(const MWWorld::Ptr& merchant, int goldPool);
    // A merchant's gold restocked now (once a day): the same way, through the server in multiplayer
    void restockGoldPool(const MWWorld::Ptr& merchant, int goldPool);

    MWWorld::Ptr getItemPtrFromStore(const mwmp::Item& item, MWWorld::ContainerStore& store);

    // The words of a voiced line, for subtitles: the response of the voice dialogue using this sound file.
    std::string getVoiceCaption(const std::string& sound);
}


#endif //OPENMW_MECHANICSHELPER_HPP
