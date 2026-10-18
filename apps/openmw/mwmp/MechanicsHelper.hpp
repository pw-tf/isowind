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
