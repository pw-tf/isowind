#include <cstdio>
#include <locale>
#include <map>
#include <memory>
#include <sstream>

#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/Utils.hpp>

#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/strings/algorithm.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwgui/mode.hpp"
#include "Hooks.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/scene.hpp"

#include "../mwmechanics/activespells.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/aifollow.hpp"
#include "../mwmechanics/aisequence.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/damagesourcetype.hpp"
#include "../mwmechanics/levelledlist.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/worldmodel.hpp"

#include "MechanicsHelper.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"
#include "PlayerList.hpp"
#include "ObjectList.hpp"
#include "CellController.hpp"
#include "Ids.hpp"
#include "LocalActor.hpp"

using namespace mwmp;

namespace
{
    // The weapon attack type the attack animation stands for (ESM::Weapon::AT_*).
    int getAttackType(const std::string& attackAnimation)
    {
        if (attackAnimation.find("slash") != std::string::npos)
            return ESM::Weapon::AT_Slash;
        if (attackAnimation.find("thrust") != std::string::npos)
            return ESM::Weapon::AT_Thrust;
        return ESM::Weapon::AT_Chop;
    }
}

osg::Vec3f MechanicsHelper::getLinearInterpolation(osg::Vec3f start, osg::Vec3f end, float percent)
{
    osg::Vec3f position(percent, percent, percent);

    return (start + osg::componentMultiply(position, (end - start)));
}

ESM::Position MechanicsHelper::getPositionFromVector(osg::Vec3f vector)
{
    ESM::Position position;
    position.pos[0] = vector.x();
    position.pos[1] = vector.y();
    position.pos[2] = vector.z();

    return position;
}

// Inspired by similar code in mwclass\creaturelevlist.cpp
//
// TODO: Add handling of scaling based on leveled list's assigned scale
void MechanicsHelper::spawnLeveledCreatures(MWWorld::CellStore* cellStore)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    mwmp::ObjectList *objectList = mwmp::Main::get().getNetworking()->getObjectList();
    objectList->reset();
    objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;

    // Pick the creatures first: the cell can't change while it's being visited
    std::vector<std::pair<ESM::RefId, ESM::Position>> spawns;
    cellStore->forEachType<ESM::CreatureLevList>([&](const MWWorld::Ptr& ptr) {
        ESM::RefId id = MWMechanics::getLevelledItem(ptr.get<ESM::CreatureLevList>()->mBase, true, world->getPrng());
        if (!id.empty())
            spawns.emplace_back(id, ptr.getCellRef().getPosition());
        return true;
    });

    for (const auto& [id, position] : spawns)
    {
        MWWorld::ManualRef manualRef(world->getStore(), id);
        manualRef.getPtr().getCellRef().setPosition(position);
        MWWorld::Ptr placed = world->placeObject(manualRef.getPtr(), cellStore, position);
        objectList->addObjectSpawn(placed);
        world->deleteObject(placed);
    }

    if (!spawns.empty())
        objectList->sendObjectSpawn();
}

bool MechanicsHelper::respawnActor(const MWWorld::Ptr& ptr)
{
    if (ptr.isEmpty() || !ptr.getClass().isActor() || ptr.getCell()->getState() != MWWorld::CellStore::State_Loaded)
        return false;

    MWBase::World* world = MWBase::Environment::get().getWorld();
    MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();

    // Someone looting its body stops: the body is gone
    if (mwmp::hooks::isCurrentContainer(ptr) && windowManager->containsMode(MWGui::GM_Container))
        windowManager->removeGuiMode(MWGui::GM_Container);

    // In a cell in use it leaves the scene while it's reset and comes back into it fresh (as for Disable/Enable: its
    // animation, physics and AI start over); a disabled one (a quest's) stays disabled
    const bool inScene = MWBase::Environment::get().getWorldScene()->isCellActive(*ptr.getCell())
        && ptr.getRefData().isEnabled();
    if (inScene)
    {
        if (ptr.getCellRef().getCount() > 0)
        {
            world->removeContainerScripts(ptr);
            world->disable(ptr);
        }
        else
            ptr.getRefData().disable(); // a body cleared away: enabling below puts it back
    }

    // What Npc::respawn and Creature::respawn do, whatever the Respawn flag and time since death (its local
    // script comes back when the cell becomes active)
    if (ptr.getCellRef().getCount() == 0)
        ptr.getCellRef().setCount(1);
    windowManager->onDeleteCustomData(ptr);
    ptr.getRefData().setCustomData(nullptr);

    const ESM::Position& start = ptr.getCellRef().getPosition();
    MWWorld::Ptr moved = world->moveObject(ptr, ptr.getCell()->getOriginCell(ptr), start.asVec3());
    world->rotateObject(moved, start.asRotationVec3(), MWBase::RotationFlag_none);

    if (inScene)
        world->enable(moved);
    return true;
}

bool MechanicsHelper::runClientLocalCommand(const std::string& command, MWWorld::CellStore* cellStore,
    unsigned int refNum, unsigned int mpNum, const std::string& refIdString)
{
    if (runPrivateFollowerCommand(command, cellStore, refNum, mpNum, refIdString))
        return true;

    static const std::string deadPrefix = "isowind local dead ";
    const bool dead = command.rfind(deadPrefix, 0) == 0;
    if (command != "isowind local" && !dead)
        return false;

    CellController* cellController = Main::get().getCellController();
    const bool wasClientLocal = cellController->isClientLocalActor(refNum, mpNum);

    MWWorld::Ptr ptr;
    if (cellStore != nullptr && cellStore->getState() == MWWorld::CellStore::State_Loaded)
        ptr = cellStore->searchExact(refNum, mpNum, refId(refIdString));

    // A thing the event changes, not a person: nothing to run
    if (!ptr.isEmpty() && !ptr.getClass().isActor())
        return true;

    cellController->setClientLocalActor(refNum, mpNum);

    // In a cell this client hasn't loaded the actor is as the game's files have it when it loads (the server tells
    // this client again then)
    if (ptr.isEmpty())
        return true;

    if (!dead)
    {
        // What the server sent about it before, from whichever game ran it, is undone; after that it's this
        // client's own for the session
        if (!wasClientLocal)
            respawnActor(ptr);
        return true;
    }

    // This client's own body of it, as it lies
    if (wasClientLocal && ptr.getClass().getCreatureStats(ptr).isDead())
        return true;

    osg::Vec3f position;
    std::istringstream stream(command.substr(deadPrefix.size()));
    stream.imbue(std::locale::classic());
    if (!(stream >> position.x() >> position.y() >> position.z()))
        return true;

    // Its inventory as the game's files have it, under the player's own loot (which the server sends after this)
    if (!wasClientLocal)
    {
        respawnActor(ptr);
        ptr = cellStore->searchExact(refNum, mpNum, refId(refIdString));
        if (ptr.isEmpty())
            return true;
    }

    MWBase::World* world = MWBase::Environment::get().getWorld();
    ptr = world->moveObject(ptr, position);
    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
    if (!stats.isDead())
    {
        // Dead already, its death not played again (no dying words, no OnDeath for scripts)
        MWMechanics::DynamicStat<float> health = stats.getHealth();
        health.setCurrent(0.f);
        stats.setHealth(health);
        stats.setDeathAnimationFinished(true);
    }
    world->enableActorCollision(ptr, false);
    return true;
}

void MechanicsHelper::sendClientLocalDeath(const MWWorld::Ptr& actor)
{
    if (!mwmp::hooks::isLoggedIn() || actor.isEmpty() || !actor.isInCell() || actor.getCell()->getCell()->isEsm4())
        return;
    // (A private follower's death is told with the rest about it: updatePrivateFollowers)
    if (isPrivateFollower(actor))
        return;

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending the death of %s %i-%i, which this client runs itself",
        idString(actor.getCellRef().getRefId()).c_str(), actor.getCellRef().getMpRefNum(),
        actor.getCellRef().getMpNum());

    const osg::Vec3f position = actor.getRefData().getPosition().asVec3();
    ObjectList* objectList = Main::get().getNetworking()->getObjectList();
    objectList->reset();
    objectList->packetOrigin = CLIENT_GAMEPLAY;
    objectList->cell = actor.getCell()->getCell()->getEsm3();
    BaseObject baseObject = objectList->getBaseObjectFromPtr(actor);
    for (int i = 0; i < 3; ++i)
    {
        ClientVariable place;
        place.internalIndex = i;
        place.variableType = VARIABLE_TYPE::FLOAT;
        place.intValue = 0;
        place.floatValue = position[i];
        baseObject.clientLocals.push_back(place);
    }
    objectList->addBaseObject(baseObject);
    objectList->sendClientScriptLocal();
}

namespace
{
    enum PrivateFollowerState
    {
        FOLLOWING = 1,
        STAYING = 2,
        DEAD = 3,
        // (asked of the server about someone's follower as this game shows it: requestFollowerCopy)
        WANTED = 4
    };

    // Where each private follower was last found, what the server was last told of it, and what's sent about it as
    // the others' copy of it while it follows
    struct PrivateFollower
    {
        unsigned int refNum = 0;
        unsigned int mpNum = 0;
        ESM::RefId refId;
        MWWorld::CellStore* cell = nullptr;
        int reportedState = 0;
        MWWorld::CellStore* reportedCell = nullptr;
        osg::Vec3f reportedPosition;
        unsigned int copyMpNum = 0;
        std::unique_ptr<LocalActor> mirror;
        // Gone to another member of the player's party ("isowind release"): out of this game, kept for if it comes
        // back
        bool released = false;
    };

    std::map<std::string, PrivateFollower> privateFollowers;
    float privateFollowerTimer = 0.f;
    float mirrorTimer = 0.f;

    // The others' followers as this game shows them, by their mpNum: who they look like, and where the server last
    // said they were (cell: where the look-alike is, once made)
    struct FollowerCopy
    {
        ESM::RefId originalId;
        ESM::Position position;
        MWWorld::CellStore* cell = nullptr;
    };

    std::map<unsigned int, FollowerCopy> followerCopies;
    // The look-alikes' records, one for each actor they look like: the same but with no script (so nothing a quest
    // does by the actor's id happens to them)
    std::map<ESM::RefId, ESM::RefId> copyRecords;

    std::string followerIndex(unsigned int refNum, unsigned int mpNum)
    {
        return Main::get().getCellController()->generateMapIndex(refNum, mpNum);
    }

    // Where it was last found, else in the cells around the player (it walked or went through a door with them)
    MWWorld::Ptr findPrivateFollower(PrivateFollower& follower)
    {
        MWWorld::Ptr ptr = follower.cell->searchExact(follower.refNum, follower.mpNum, follower.refId, true);
        if (ptr.isEmpty())
        {
            for (MWWorld::CellStore* cell : MWBase::Environment::get().getWorldScene()->getActiveCells())
            {
                ptr = cell->searchExact(follower.refNum, follower.mpNum, follower.refId, true);
                if (!ptr.isEmpty())
                    break;
            }
        }
        if (!ptr.isEmpty())
            follower.cell = ptr.getCell();
        return ptr;
    }

    // As the game counts a follower (MWMechanics::Actors::getActorsFollowing): following the player, with only
    // fighting or wandering before that
    bool isFollowingPlayer(const MWWorld::Ptr& ptr)
    {
        const MWWorld::Ptr player = MWMechanics::getPlayer();
        for (const auto& package : ptr.getClass().getCreatureStats(ptr).getAiSequence())
        {
            if (package->followTargetThroughDoors() && package->targetIs(player))
                return true;
            if (package->getTypeId() != MWMechanics::AiPackageTypeId::Combat
                && package->getTypeId() != MWMechanics::AiPackageTypeId::Wander)
                return false;
        }
        return false;
    }

    // What's sent about a private follower as its copy, while it follows (its attacks and casts go with it)
    LocalActor* getMirror(const MWWorld::Ptr& ptr)
    {
        if (privateFollowers.empty() || ptr.isEmpty() || ptr.mRef == nullptr)
            return nullptr;
        auto found
            = privateFollowers.find(followerIndex(ptr.getCellRef().getMpRefNum(), ptr.getCellRef().getMpNum()));
        return found != privateFollowers.end() ? found->second.mirror.get() : nullptr;
    }

    void followPlayer(const MWWorld::Ptr& ptr)
    {
        MWMechanics::AiFollow package(MWMechanics::getPlayer());
        package.allowAnyDistance(true);
        ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(package, ptr);
    }

    // What's sent about the follower goes as about its copy
    void setMirrorPtr(PrivateFollower& follower, const MWWorld::Ptr& ptr)
    {
        follower.mirror->setPtr(ptr);
        follower.mirror->refNum = 0;
        follower.mirror->mpNum = follower.copyMpNum;
        follower.mirror->cell = ptr.getCell()->getCell()->getEsm3();
    }

    // A cell as the server describes it: "<x>, <y>" outdoors, its name indoors
    MWWorld::CellStore* getCellStoreFromDescription(const std::string& description)
    {
        MWWorld::WorldModel* worldModel = MWBase::Environment::get().getWorldModel();
        int x = 0;
        int y = 0;
        int length = 0;
        if (std::sscanf(description.c_str(), "%d, %d%n", &x, &y, &length) == 2
            && length == static_cast<int>(description.size()))
            return &worldModel->getExterior(ESM::ExteriorCellLocation(x, y, ESM::Cell::sDefaultWorldspaceId));
        try
        {
            return worldModel->findInterior(description);
        }
        catch (std::exception&)
        {
            return nullptr;
        }
    }

    // A command's words after its name, or false if it isn't that command
    bool takeCommand(const std::string& command, const std::string& name, std::string& rest)
    {
        if (command == name)
        {
            rest.clear();
            return true;
        }
        if (command.rfind(name + " ", 0) != 0)
            return false;
        rest = command.substr(name.size() + 1);
        return true;
    }

    void sendPrivateFollower(const MWWorld::Ptr& ptr, int state)
    {
        const osg::Vec3f position = ptr.getRefData().getPosition().asVec3();
        ObjectList* objectList = Main::get().getNetworking()->getObjectList();
        objectList->reset();
        objectList->packetOrigin = CLIENT_GAMEPLAY;
        objectList->cell = ptr.getCell()->getCell()->getEsm3();
        BaseObject baseObject = objectList->getBaseObjectFromPtr(ptr);
        for (int i = 0; i < 3; ++i)
        {
            ClientVariable place;
            place.internalIndex = i;
            place.variableType = VARIABLE_TYPE::FLOAT;
            place.intValue = 0;
            place.floatValue = position[i];
            baseObject.clientLocals.push_back(place);
        }
        ClientVariable stateVariable;
        stateVariable.internalIndex = 0;
        stateVariable.variableType = VARIABLE_TYPE::SHORT;
        stateVariable.intValue = state;
        stateVariable.floatValue = 0.f;
        baseObject.clientLocals.push_back(stateVariable);
        objectList->addBaseObject(baseObject);
        objectList->sendClientScriptLocal();
    }

    ESM::RefId getCopyRecord(const ESM::RefId& originalId)
    {
        auto found = copyRecords.find(originalId);
        if (found != copyRecords.end())
            return found->second;
        MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        ESM::RefId recordId;
        if (const ESM::NPC* npc = store.get<ESM::NPC>().search(originalId))
        {
            ESM::NPC record = *npc;
            record.mScript = ESM::RefId();
            recordId = store.insert(record)->mId;
        }
        else if (const ESM::Creature* creature = store.get<ESM::Creature>().search(originalId))
        {
            ESM::Creature record = *creature;
            record.mScript = ESM::RefId();
            recordId = store.insert(record)->mId;
        }
        copyRecords[originalId] = recordId;
        return recordId;
    }

    MWWorld::Ptr findFollowerCopy(unsigned int mpNum, FollowerCopy& copy)
    {
        if (copy.cell == nullptr)
            return {};
        MWWorld::Ptr ptr = copy.cell->searchExact(0, mpNum, {}, true);
        if (ptr.isEmpty())
            copy.cell = nullptr;
        return ptr;
    }

    void removeFollowerCopy(unsigned int mpNum, FollowerCopy& copy)
    {
        MWWorld::Ptr ptr = findFollowerCopy(mpNum, copy);
        Main::get().getCellController()->forgetActor(0, mpNum);
        if (!ptr.isEmpty())
            MWBase::Environment::get().getWorld()->deleteObject(ptr);
        copy.cell = nullptr;
    }

    // The look-alike in a cell in use here: made, or brought over from the cell it was in
    MWWorld::Ptr placeFollowerCopy(
        unsigned int mpNum, FollowerCopy& copy, MWWorld::CellStore* store, const ESM::Position& position)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr ptr = findFollowerCopy(mpNum, copy);
        if (!ptr.isEmpty())
        {
            if (ptr.getCell() == store)
                return ptr;
            Main::get().getCellController()->forgetActor(0, mpNum);
            ptr = world->moveObject(ptr, store, position.asVec3());
            copy.cell = ptr.getCell();
            return ptr;
        }

        const ESM::RefId recordId = getCopyRecord(copy.originalId);
        if (recordId.empty())
            return {};
        MWWorld::ManualRef manualRef(world->getStore(), recordId);
        manualRef.getPtr().getCellRef().setMpNum(mpNum);
        ptr = world->placeObject(manualRef.getPtr(), store, position);
        // Moved only by what its owner's game sends, not by AI of its own here
        MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        stats.getAiSequence().clear();
        stats.setAiSetting(MWMechanics::AiSetting::Fight, 0);
        stats.setAiSetting(MWMechanics::AiSetting::Hello, 0);
        copy.cell = ptr.getCell();
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Showing someone's %s as 0-%u in %s", idString(copy.originalId).c_str(),
            mpNum, std::string(store->getCell()->getDescription()).c_str());
        return ptr;
    }

    // "isowind copy <x> <y> <z>" (someone's follower is there: the look-alike, if this game has the cell in use, else
    // later when what its owner's game sends about it comes) and "isowind uncopy" (it no longer follows them)
    bool runFollowerCopyCommand(const std::string& command, MWWorld::CellStore* cellStore, unsigned int mpNum,
        const std::string& refIdString)
    {
        std::string rest;
        if (takeCommand(command, "isowind uncopy", rest))
        {
            auto found = followerCopies.find(mpNum);
            if (found != followerCopies.end())
            {
                removeFollowerCopy(mpNum, found->second);
                followerCopies.erase(found);
            }
            return true;
        }
        if (!takeCommand(command, "isowind copy", rest))
            return false;
        if (mpNum < FOLLOWER_COPY_BASE)
            return true;

        FollowerCopy& copy = followerCopies[mpNum];
        copy.originalId = refId(refIdString);
        std::istringstream stream(rest);
        stream.imbue(std::locale::classic());
        copy.position = ESM::Position();
        stream >> copy.position.pos[0] >> copy.position.pos[1] >> copy.position.pos[2];

        if (cellStore != nullptr && cellStore->getState() == MWWorld::CellStore::State_Loaded
            && MWBase::Environment::get().getWorldScene()->isCellActive(*cellStore))
            placeFollowerCopy(mpNum, copy, cellStore, copy.position);
        else
        {
            // Gone where this game doesn't show
            MWWorld::Ptr ptr = findFollowerCopy(mpNum, copy);
            if (!ptr.isEmpty())
                removeFollowerCopy(mpNum, copy);
        }
        return true;
    }
}

bool MechanicsHelper::runPrivateFollowerCommand(const std::string& command, MWWorld::CellStore* cellStore,
    unsigned int refNum, unsigned int mpNum, const std::string& refIdString)
{
    if (runFollowerCopyCommand(command, cellStore, mpNum, refIdString))
        return true;

    std::string rest;
    const bool own = takeCommand(command, "isowind own", rest);
    const bool follow = !own && takeCommand(command, "isowind follow", rest);
    const bool stay = !own && !follow && takeCommand(command, "isowind stay", rest);
    const bool take = !own && !follow && !stay && takeCommand(command, "isowind take", rest);
    const bool release = !own && !follow && !stay && !take && takeCommand(command, "isowind release", rest);
    const bool dismiss
        = !own && !follow && !stay && !take && !release && takeCommand(command, "isowind dismiss", rest);
    if (!own && !follow && !stay && !take && !release && !dismiss)
        return false;

    const std::string index = followerIndex(refNum, mpNum);
    auto known = privateFollowers.find(index);
    const bool wasPrivate = known != privateFollowers.end();

    // The player sent it away (/dismiss): back at its post as the game's files have it, the world's again
    if (dismiss)
    {
        if (!wasPrivate)
            return true;
        MWWorld::Ptr ptr = findPrivateFollower(known->second);
        privateFollowers.erase(known);
        CellController* cellController = Main::get().getCellController();
        cellController->unsetClientLocalActor(refNum, mpNum);
        if (!ptr.isEmpty())
        {
            MWWorld::CellStore* post = ptr.getCell()->getOriginCell(ptr);
            if (!ptr.getRefData().isEnabled())
                MWBase::Environment::get().getWorld()->enable(ptr);
            ptr.getClass().getCreatureStats(ptr).getAiSequence().clear();
            respawnActor(ptr);
            // Run by whoever has authority over its cell again
            if (post != nullptr && !post->getCell()->isEsm4()
                && cellController->isInitializedCell(post->getCell()->getEsm3()))
                cellController->getCell(post->getCell()->getEsm3())->shouldInitializeActors = true;
        }
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "%s %s was sent back to its post", refIdString.c_str(), index.c_str());
        return true;
    }

    // It went to another member of the player's party: out of this game (they have it; this player sees its copy)
    if (release)
    {
        if (!wasPrivate)
            return true;
        PrivateFollower& follower = known->second;
        MWWorld::Ptr ptr = findPrivateFollower(follower);
        follower.mirror.reset();
        follower.released = true;
        follower.reportedState = 0;
        if (!ptr.isEmpty() && ptr.getRefData().isEnabled())
            MWBase::Environment::get().getWorld()->disable(ptr);
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "%s %s went to another member of the party", refIdString.c_str(),
            index.c_str());
        return true;
    }
    MWWorld::Ptr ptr;
    if (wasPrivate)
        ptr = findPrivateFollower(known->second);
    else if (cellStore != nullptr)
    {
        // Where the game's files have it, which the player's game may not have loaded (they logged in elsewhere)
        if (cellStore->getState() != MWWorld::CellStore::State_Loaded)
            cellStore->load();
        ptr = cellStore->searchExact(refNum, mpNum, refId(refIdString), true);
    }
    if (ptr.isEmpty())
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_WARN, "No actor %s %s for \"%s\"", refIdString.c_str(), index.c_str(),
            command.c_str());
        return true;
    }

    Main::get().getCellController()->setClientLocalActor(refNum, mpNum);
    PrivateFollower& follower = privateFollowers[index];
    follower.refNum = refNum;
    follower.mpNum = mpNum;
    follower.refId = ptr.getCellRef().getRefId();
    follower.cell = ptr.getCell();
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "%s %s is this player's own follower (\"%s\")", refIdString.c_str(),
        index.c_str(), command.c_str());

    // "own", "follow" and "take" come with the number of its copy for the others; "take" then with what to do with it
    // (1: follow the player, 2: talk to it, as the player did to its copy) and where it is
    int takeFlags = 0;
    if (own || follow || take)
    {
        std::istringstream stream(rest);
        unsigned int copyMpNum = 0;
        if (stream >> copyMpNum && copyMpNum >= FOLLOWER_COPY_BASE)
            follower.copyMpNum = copyMpNum;
        if (take)
        {
            stream >> takeFlags;
            std::getline(stream >> std::ws, rest);
        }
    }

    // Back from another member of the party
    if (follower.released)
    {
        follower.released = false;
        MWBase::Environment::get().getWorld()->enable(ptr);
    }

    // Just told to follow here, as it is (the follow order normally already stacked: stacked again in case what the
    // game running its cell sent undid it)
    if (own)
    {
        if (!isFollowingPlayer(ptr))
            followPlayer(ptr);
        return true;
    }

    // At login: as the game's files have it, undoing what the world's (others') sent about it
    if (!wasPrivate && respawnActor(ptr))
    {
        ptr = cellStore->searchExact(refNum, mpNum, refId(refIdString), true);
        if (ptr.isEmpty())
            return true;
    }

    MWBase::World* world = MWBase::Environment::get().getWorld();
    if (follow)
    {
        // Where the player is, as followers come through a door with them (MWWorld::ActionTeleport)
        const MWWorld::Ptr player = world->getPlayerPtr();
        ptr = world->moveObject(ptr, player.getCell(), player.getRefData().getPosition().asVec3());
        follower.cell = ptr.getCell();
        if (!isFollowingPlayer(ptr))
            followPlayer(ptr);
        return true;
    }

    static const std::string deadPrefix = "dead ";
    std::string place = rest;
    const bool dead = place.rfind(deadPrefix, 0) == 0;
    if (dead)
        place = place.substr(deadPrefix.size());
    osg::Vec3f position;
    std::istringstream stream(place);
    stream.imbue(std::locale::classic());
    if (!(stream >> position.x() >> position.y() >> position.z()))
        return true;
    std::string cellDescription;
    std::getline(stream >> std::ws, cellDescription);
    MWWorld::CellStore* destination = getCellStoreFromDescription(cellDescription);
    if (destination == nullptr)
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_WARN, "No cell %s for \"%s\"", cellDescription.c_str(), command.c_str());
        return true;
    }
    if (destination->getState() != MWWorld::CellStore::State_Loaded)
        destination->load();
    ptr = world->moveObject(ptr, destination, position);
    follower.cell = ptr.getCell();

    if (dead)
    {
        // Dead already, its death not played again (as for an event's person: runClientLocalCommand)
        MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        if (!stats.isDead())
        {
            MWMechanics::DynamicStat<float> health = stats.getHealth();
            health.setCurrent(0.f);
            stats.setHealth(health);
            stats.setDeathAnimationFinished(true);
        }
        world->enableActorCollision(ptr, false);
    }
    if (takeFlags & 1)
        followPlayer(ptr);
    if (takeFlags & 2)
        MWBase::Environment::get().getLuaManager()->objectActivated(ptr, world->getPlayerPtr());
    return true;
}

void MechanicsHelper::requestFollowerCopy(const MWWorld::Ptr& copy)
{
    if (!mwmp::hooks::isLoggedIn() || !isFollowerCopy(copy) || !copy.isInCell() || copy.getCell()->getCell()->isEsm4())
        return;
    sendPrivateFollower(copy, WANTED);
}

bool MechanicsHelper::isPrivateFollower(const MWWorld::Ptr& ptr)
{
    if (privateFollowers.empty() || ptr.isEmpty() || ptr.mRef == nullptr)
        return false;
    return privateFollowers.count(followerIndex(ptr.getCellRef().getMpRefNum(), ptr.getCellRef().getMpNum())) > 0;
}

void MechanicsHelper::onPrivateFollowerMoved(const MWWorld::Ptr& ptr)
{
    auto found
        = privateFollowers.find(followerIndex(ptr.getCellRef().getMpRefNum(), ptr.getCellRef().getMpNum()));
    if (found == privateFollowers.end() || !ptr.isInCell() || ptr.getCell()->getCell()->isEsm4())
        return;
    found->second.cell = ptr.getCell();
    if (found->second.mirror != nullptr)
        setMirrorPtr(found->second, ptr);
}

bool MechanicsHelper::isFollowerCopy(const MWWorld::Ptr& ptr)
{
    return !ptr.isEmpty() && ptr.mRef != nullptr && ptr.getCellRef().getMpNum() >= FOLLOWER_COPY_BASE;
}

MWWorld::Ptr MechanicsHelper::getPrivateFollowerByCopy(unsigned int mpNum)
{
    for (auto& [index, follower] : privateFollowers)
        if (follower.copyMpNum == mpNum)
            return findPrivateFollower(follower);
    return {};
}

MWWorld::Ptr MechanicsHelper::getFollowerCopy(MWWorld::CellStore* store, unsigned int mpNum)
{
    auto found = followerCopies.find(mpNum);
    if (found == followerCopies.end() || store == nullptr || store->getState() != MWWorld::CellStore::State_Loaded
        || !MWBase::Environment::get().getWorldScene()->isCellActive(*store))
        return {};
    MWWorld::Ptr ptr = findFollowerCopy(mpNum, found->second);
    return placeFollowerCopy(
        mpNum, found->second, store, ptr.isEmpty() ? found->second.position : ptr.getRefData().getPosition());
}

void MechanicsHelper::updatePrivateFollowers(float dt)
{
    if (privateFollowers.empty() || !mwmp::hooks::isLoggedIn())
        return;

    // What the others' copies do, as often as what's sent about the actors this game runs (Cell::updateLocal)
    mirrorTimer += dt;
    if (mirrorTimer >= 0.025f)
    {
        mirrorTimer = 0.f;
        ActorList* actorList = Main::get().getNetworking()->getActorList();
        for (auto& [index, follower] : privateFollowers)
        {
            if (follower.mirror == nullptr)
                continue;
            actorList->reset();
            actorList->cell = follower.mirror->cell;
            follower.mirror->update(!follower.mirror->hasSentData);
            actorList->sendPositionActors();
            actorList->sendAnimFlagsActors();
            actorList->sendAnimPlayActors();
            actorList->sendSpeechActors();
            actorList->sendAttackActors();
            actorList->sendCastActors();
        }
    }

    privateFollowerTimer += dt;
    if (privateFollowerTimer < 1.f)
        return;
    privateFollowerTimer = 0.f;

    for (auto& [index, follower] : privateFollowers)
    {
        if (follower.released)
            continue;
        MWWorld::Ptr ptr = findPrivateFollower(follower);
        if (ptr.isEmpty() || !ptr.isInCell() || ptr.getCell()->getCell()->isEsm4())
            continue;
        int state = STAYING;
        if (ptr.getClass().getCreatureStats(ptr).isDead())
            state = DEAD;
        else if (isFollowingPlayer(ptr))
            state = FOLLOWING;

        // The others see it only while it follows
        if (state == FOLLOWING && follower.copyMpNum != 0)
        {
            if (follower.mirror == nullptr)
            {
                follower.mirror = std::make_unique<LocalActor>();
                follower.mirror->isFollowerCopy = true;
                setMirrorPtr(follower, ptr);
            }
            else if (follower.mirror->getPtr().getCell() != ptr.getCell())
                setMirrorPtr(follower, ptr);
        }
        else
            follower.mirror.reset();

        const osg::Vec3f position = ptr.getRefData().getPosition().asVec3();
        // Where it stays or lies matters to the server; where it follows, roughly (where the others' games first
        // show it: their copy then goes by what's sent about it as it moves)
        const float far = state == FOLLOWING ? 1024.f : 128.f;
        const bool moved = ptr.getCell() != follower.reportedCell
            || (position - follower.reportedPosition).length2() > far * far;
        if (state == follower.reportedState && !moved)
            continue;

        sendPrivateFollower(ptr, state);
        follower.reportedState = state;
        follower.reportedCell = ptr.getCell();
        follower.reportedPosition = position;
    }
}

bool MechanicsHelper::isUsingRangedWeapon(const MWWorld::Ptr& ptr)
{
    if (ptr.getClass().hasInventoryStore(ptr))
    {
        MWWorld::InventoryStore &inventoryStore = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator weaponSlot = inventoryStore.getSlot(
            MWWorld::InventoryStore::Slot_CarriedRight);

        if (weaponSlot != inventoryStore.end() && weaponSlot->getType() == ESM::Weapon::sRecordId)
        {
            const ESM::Weapon* weaponRecord = weaponSlot->get<ESM::Weapon>()->mBase;

            if (weaponRecord->mData.mType >= ESM::Weapon::MarksmanBow)
                return true;
        }
    }

    return false;
}

Attack *MechanicsHelper::getLocalAttack(const MWWorld::Ptr& ptr)
{
    if (ptr == MWMechanics::getPlayer())
        return &mwmp::Main::get().getLocalPlayer()->attack;
    else if (mwmp::Main::get().getCellController()->isLocalActor(ptr))
        return &mwmp::Main::get().getCellController()->getLocalActor(ptr)->attack;
    else if (LocalActor* mirror = getMirror(ptr))
        return &mirror->attack;

    return nullptr;
}

Attack *MechanicsHelper::getDedicatedAttack(const MWWorld::Ptr& ptr)
{
    if (mwmp::PlayerList::isDedicatedPlayer(ptr))
        return &mwmp::PlayerList::getPlayer(ptr)->attack;
    else if (mwmp::Main::get().getCellController()->isDedicatedActor(ptr))
        return &mwmp::Main::get().getCellController()->getDedicatedActor(ptr)->attack;

    return nullptr;
}

Cast *MechanicsHelper::getLocalCast(const MWWorld::Ptr& ptr)
{
    if (ptr == MWMechanics::getPlayer())
        return &mwmp::Main::get().getLocalPlayer()->cast;
    else if (mwmp::Main::get().getCellController()->isLocalActor(ptr))
        return &mwmp::Main::get().getCellController()->getLocalActor(ptr)->cast;
    else if (LocalActor* mirror = getMirror(ptr))
        return &mirror->cast;

    return nullptr;
}

Cast *MechanicsHelper::getDedicatedCast(const MWWorld::Ptr& ptr)
{
    if (mwmp::PlayerList::isDedicatedPlayer(ptr))
        return &mwmp::PlayerList::getPlayer(ptr)->cast;
    else if (mwmp::Main::get().getCellController()->isDedicatedActor(ptr))
        return &mwmp::Main::get().getCellController()->getDedicatedActor(ptr)->cast;

    return nullptr;
}

MWWorld::Ptr MechanicsHelper::getPlayerPtr(const Target& target)
{
    if (target.guid == mwmp::Main::get().getLocalPlayer()->guid)
    {
        return MWMechanics::getPlayer();
    }
    else
    {
        mwmp::DedicatedPlayer* dedicatedPlayer = mwmp::PlayerList::getPlayer(target.guid);

        if (dedicatedPlayer != nullptr)
        {
            return dedicatedPlayer->getPtr();
        }
    }

    return MWWorld::Ptr();
}

MWWorld::Ptr MechanicsHelper::getTargetPtr(const mwmp::Target& target)
{
    if (target.isPlayer)
        return getPlayerPtr(target);

    // Isowind MMO: this player's own follower, as the others' games know it (its copy)
    if (target.refNum == 0 && target.mpNum >= FOLLOWER_COPY_BASE)
        return getPrivateFollowerByCopy(target.mpNum);

    auto controller = mwmp::Main::get().getCellController();
    if (controller->isLocalActor(target.refNum, target.mpNum))
        return controller->getLocalActor(target.refNum, target.mpNum)->getPtr();
    else if (controller->isDedicatedActor(target.refNum, target.mpNum))
        return controller->getDedicatedActor(target.refNum, target.mpNum)->getPtr();

    return MWWorld::Ptr();
}

mwmp::Item MechanicsHelper::getItem(const MWWorld::Ptr& itemPtr, int count)
{
    mwmp::Item item;

    if (itemPtr.getClass().isGold(itemPtr))
        item.refId = idString(MWWorld::ContainerStore::sGoldId);
    else
        item.refId = idString(itemPtr.getCellRef().getRefId());

    item.count = count;
    item.charge = itemPtr.getCellRef().getCharge();
    item.enchantmentCharge = itemPtr.getCellRef().getEnchantmentCharge();
    item.soul = idString(itemPtr.getCellRef().getSoul());

    return item;
}

mwmp::Target MechanicsHelper::getTarget(const MWWorld::Ptr& ptr)
{
    mwmp::Target target;
    clearTarget(target);

    if (!ptr.isEmpty())
    {
        if (ptr == MWMechanics::getPlayer())
        {
            target.isPlayer = true;
            target.guid = mwmp::Main::get().getLocalPlayer()->guid;
        }
        else if (mwmp::PlayerList::isDedicatedPlayer(ptr))
        {
            target.isPlayer = true;
            target.guid = mwmp::PlayerList::getPlayer(ptr)->guid;
        }
        else
        {
            const MWWorld::CellRef &ptrRef = ptr.getCellRef();

            target.isPlayer = false;
            target.refId = idString(ptrRef.getRefId());
            target.refNum = ptrRef.getMpRefNum();
            target.mpNum = ptrRef.getMpNum();
            target.name = ptr.getClass().getName(ptr);
        }
    }

    return target;
}

void MechanicsHelper::clearTarget(mwmp::Target& target)
{
    target.isPlayer = false;
    target.refId.clear();
    target.refNum = -1;
    target.mpNum = -1;

    target.name.clear();
}

bool MechanicsHelper::isEmptyTarget(const mwmp::Target& target)
{
    if (target.isPlayer == false && target.refId.empty())
        return true;

    return false;
}

void MechanicsHelper::assignAttackTarget(Attack* attack, const MWWorld::Ptr& target)
{
    if (target == MWMechanics::getPlayer())
    {
        attack->target.isPlayer = true;
        attack->target.guid = mwmp::Main::get().getLocalPlayer()->guid;
    }
    else if (mwmp::PlayerList::isDedicatedPlayer(target))
    {
        attack->target.isPlayer = true;
        attack->target.guid = mwmp::PlayerList::getPlayer(target)->guid;
    }
    else
    {
        const MWWorld::CellRef &targetRef = target.getCellRef();

        attack->target.isPlayer = false;
        attack->target.refId = idString(targetRef.getRefId());
        attack->target.refNum = targetRef.getMpRefNum();
        attack->target.mpNum = targetRef.getMpNum();
    }
}

void MechanicsHelper::resetAttack(Attack* attack)
{
    attack->isHit = false;
    attack->success = false;
    attack->knockdown = false;
    attack->block = false;
    attack->applyWeaponEnchantment = false;
    attack->applyAmmoEnchantment = false;
    attack->hitPosition.pos[0] = attack->hitPosition.pos[1] = attack->hitPosition.pos[2] = 0;
    attack->target.guid = RakNet::RakNetGUID();
    attack->target.refId.clear();
    attack->target.refNum = 0;
    attack->target.mpNum = 0;
}

void MechanicsHelper::resetCast(Cast* cast)
{
    cast->isHit = false;
    cast->success = false;
    cast->target.guid = RakNet::RakNetGUID();
    cast->target.refId.clear();
    cast->target.refNum = 0;
    cast->target.mpNum = 0;
}

bool MechanicsHelper::getSpellSuccess(const std::string& spellId, const MWWorld::Ptr& caster)
{
    return Misc::Rng::roll0to99(MWBase::Environment::get().getWorld()->getPrng())
        < MWMechanics::getSpellSuccessChance(refId(spellId), caster, nullptr, true, false);
}

bool MechanicsHelper::isTeamMember(const MWWorld::Ptr& playerChecked, const MWWorld::Ptr& playerWithTeam)
{
    bool isTeamMember = false;
    bool playerCheckedIsLocal = playerChecked == MWMechanics::getPlayer();
    bool playerCheckedIsDedicated = !playerCheckedIsLocal ? mwmp::PlayerList::isDedicatedPlayer(playerChecked) : false;
    bool playerWithTeamIsLocal = !playerCheckedIsLocal ? playerWithTeam == MWMechanics::getPlayer() : false;
    bool playerWithTeamIsDedicated = !playerWithTeamIsLocal ? mwmp::PlayerList::isDedicatedPlayer(playerWithTeam) : false;

    if (playerCheckedIsLocal || playerCheckedIsDedicated)
    {
        if (playerWithTeamIsLocal || playerWithTeamIsDedicated)
        {
            RakNet::RakNetGUID playerCheckedGuid;

            if (playerCheckedIsLocal)
                playerCheckedGuid = mwmp::Main::get().getLocalPlayer()->guid;
            else
                playerCheckedGuid = PlayerList::getPlayer(playerChecked)->guid;

            if (playerWithTeamIsLocal)
                isTeamMember = Utils::vectorContains(mwmp::Main::get().getLocalPlayer()->alliedPlayers, playerCheckedGuid);
            else
                isTeamMember = Utils::vectorContains(PlayerList::getPlayer(playerWithTeam)->alliedPlayers, playerCheckedGuid);
        }
    }

    return isTeamMember;
}

void MechanicsHelper::processAttack(Attack attack, const MWWorld::Ptr& attacker)
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Processing attack from %s of type %i",
        std::string(attacker.getClass().getName(attacker)).c_str(), attack.type);

    LOG_APPEND(TimedLog::LOG_VERBOSE, "- pressed: %s", attack.pressed ? "true" : "false");

    if (!attack.pressed)
    {
        LOG_APPEND(TimedLog::LOG_VERBOSE, "- success: %s", attack.success ? "true" : "false");

        if (attack.success)
            LOG_APPEND(TimedLog::LOG_VERBOSE, "- damage: %f", attack.damage);
    }
    else
    {
        if (attack.type == attack.MELEE)
            LOG_APPEND(TimedLog::LOG_VERBOSE, "- animation: %s", attack.attackAnimation.c_str());
    }

    // The attack animation (chop, slash, thrust, shoot) is played as on the attacker's client: with AI off for remote
    // actors, the character controller takes it from their stats
    if (attack.pressed && !attack.attackAnimation.empty())
        attacker.getClass().getCreatureStats(attacker).setAttackType(attack.attackAnimation);
    attacker.getClass().getCreatureStats(attacker).setAttackingOrSpell(attack.pressed);

    MWWorld::Ptr victim = getTargetPtr(attack.target);

    if (attack.isHit)
    {
        bool isRanged = attack.type == attack.RANGED;

        MWWorld::Ptr weaponPtr;
        MWWorld::Ptr ammoPtr;

        bool usedTempRangedWeapon = false;
        bool usedTempRangedAmmo = false;

        // Get the attacker's current weapon
        //
        // Note: if using hand-to-hand, the weapon is equal to inv.end()
        if (attacker.getClass().hasInventoryStore(attacker))
        {
            MWWorld::InventoryStore &inventoryStore = attacker.getClass().getInventoryStore(attacker);
            MWWorld::ContainerStoreIterator weaponSlot = inventoryStore.getSlot(
                MWWorld::InventoryStore::Slot_CarriedRight);

            weaponPtr = weaponSlot != inventoryStore.end() ? *weaponSlot : MWWorld::Ptr();

            // Is the currently selected weapon different from the one recorded for this ranged attack?
            // If so, try to find the correct one in the attacker's inventory and use it here. If it
            // no longer exists, add it back temporarily.
            if (isRanged)
            {
                if (weaponPtr.isEmpty() || !(weaponPtr.getCellRef().getRefId() == attack.rangedWeaponId))
                {
                    weaponPtr = inventoryStore.search(refId(attack.rangedWeaponId));

                    if (weaponPtr.isEmpty())
                    {
                        weaponPtr = *inventoryStore.MWWorld::ContainerStore::add(refId(attack.rangedWeaponId), 1, false);
                        usedTempRangedWeapon = true;
                    }
                }

                if (!attack.rangedAmmoId.empty())
                {
                    MWWorld::ContainerStoreIterator ammoSlot = inventoryStore.getSlot(
                        MWWorld::InventoryStore::Slot_Ammunition);
                    ammoPtr = ammoSlot != inventoryStore.end() ? *ammoSlot : MWWorld::Ptr();

                    if (ammoPtr.isEmpty() || !(ammoPtr.getCellRef().getRefId() == attack.rangedAmmoId))
                    {
                        ammoPtr = inventoryStore.search(refId(attack.rangedAmmoId));

                        if (ammoPtr.isEmpty())
                        {
                            ammoPtr = *inventoryStore.MWWorld::ContainerStore::add(refId(attack.rangedAmmoId), 1, false);
                            usedTempRangedAmmo = true;
                        }
                    }
                }
            }

            if (!weaponPtr.isEmpty() && weaponPtr.getType() != ESM::Weapon::sRecordId)
                weaponPtr = MWWorld::Ptr();
        }

        if (!weaponPtr.isEmpty())
        {
            LOG_APPEND(TimedLog::LOG_VERBOSE, "- weapon: %s\n- isRanged: %s\n- applyWeaponEnchantment: %s\n- applyAmmoEnchantment: %s",
                idString(weaponPtr.getCellRef().getRefId()).c_str(), isRanged ? "true" : "false",
                attack.applyWeaponEnchantment ? "true" : "false", attack.applyAmmoEnchantment ? "true" : "false");

            if (attack.applyWeaponEnchantment)
            {
                MWMechanics::CastSpell cast(attacker, victim, isRanged);
                cast.mHitPosition = attack.hitPosition.asVec3();
                cast.cast(weaponPtr, false);
            }

            if (isRanged && !ammoPtr.isEmpty() && attack.applyAmmoEnchantment)
            {
                MWMechanics::CastSpell cast(attacker, victim, isRanged);
                cast.mHitPosition = attack.hitPosition.asVec3();
                cast.cast(ammoPtr, false);
            }
        }

        if (!victim.isEmpty())
        {
            bool isHealthDamage = true;
            float damage = attack.damage;

            if (weaponPtr.isEmpty())
            {
                if (attacker.getClass().isBipedal(attacker))
                {
                    MWMechanics::CreatureStats &victimStats = victim.getClass().getCreatureStats(victim);
                    isHealthDamage = victimStats.isParalyzed() || victimStats.getKnockedDown();
                }
            }

            if (!isRanged && MWMechanics::blockMeleeAttack(attacker, victim, weaponPtr, damage, 1))
                damage = 0;

            // The same path a hit made here takes (OpenMW's Lua combat handler applies it)
            MWBase::Environment::get().getLuaManager()->onHit(attacker, victim, weaponPtr, ammoPtr,
                getAttackType(attack.attackAnimation), attack.attackStrength, damage, isHealthDamage,
                attack.hitPosition.asVec3(), attack.success,
                isRanged ? MWMechanics::DamageSourceType::Ranged : MWMechanics::DamageSourceType::Melee);
        }

        // Remove temporary items that may have been added above for ranged attacks
        if (isRanged && attacker.getClass().hasInventoryStore(attacker))
        {
            MWWorld::InventoryStore &inventoryStore = attacker.getClass().getInventoryStore(attacker);

            if (usedTempRangedWeapon)
                inventoryStore.remove(weaponPtr, 1);

            if (usedTempRangedAmmo)
                inventoryStore.remove(ammoPtr, 1);
        }
    }
}

void MechanicsHelper::processCast(Cast cast, const MWWorld::Ptr& caster)
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Processing cast from %s of type %i",
        std::string(caster.getClass().getName(caster)).c_str(), cast.type);

    LOG_APPEND(TimedLog::LOG_VERBOSE, "- pressed: %s", cast.pressed ? "true" : "false");

    if (!cast.pressed)
    {
        LOG_APPEND(TimedLog::LOG_VERBOSE, "- success: %s", cast.success ? "true" : "false");
    }

    MWMechanics::CreatureStats &casterStats = caster.getClass().getCreatureStats(caster);
    casterStats.setAttackingOrSpell(cast.pressed);

    if (cast.type == cast.REGULAR)
    {
        casterStats.getSpells().setSelectedSpell(refId(cast.spellId));

        if (cast.success)
            MWBase::Environment::get().getWorld()->castSpell(caster);

        LOG_APPEND(TimedLog::LOG_VERBOSE, "- spellId: %s", cast.spellId.c_str());
    }
    else if (cast.type == cast.ITEM)
    {
        casterStats.getSpells().setSelectedSpell(ESM::RefId());

        MWWorld::InventoryStore& inventoryStore = caster.getClass().getInventoryStore(caster);

        MWWorld::ContainerStoreIterator it = inventoryStore.begin();
        for (; it != inventoryStore.end(); ++it)
        {
            if (it->getCellRef().getRefId() == cast.itemId)
                break;
        }

        // Add the item if it's missing
        if (it == inventoryStore.end())
            it = inventoryStore.MWWorld::ContainerStore::add(refId(cast.itemId), 1, false);

        inventoryStore.setSelectedEnchantItem(it);
        LOG_APPEND(TimedLog::LOG_VERBOSE, "- itemId: %s", cast.itemId.c_str());
        MWBase::Environment::get().getWorld()->castSpell(caster);
        inventoryStore.setSelectedEnchantItem(inventoryStore.end());
    }
}

void MechanicsHelper::createSpellGfx(const MWWorld::Ptr& targetPtr, const std::vector<ESM::ActiveEffect>& mEffects)
{
    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
    for (const ESM::ActiveEffect& effect : mEffects)
    {
        const ESM::MagicEffect* magicEffect = store.get<ESM::MagicEffect>().search(effect.mEffectId);
        if (magicEffect)
            MWMechanics::playEffects(targetPtr, *magicEffect, true);
    }
}

void MechanicsHelper::addActiveSpell(const MWWorld::Ptr& actor, const mwmp::ActiveSpell& activeSpell, bool restored)
{
    ESM::ActiveSpells::ActiveSpellParams params = activeSpell.params;
    params.mSourceSpellId = refId(activeSpell.id);
    params.mFlags = activeSpell.isStackingSpell
        ? static_cast<ESM::ActiveSpells::Flags>(ESM::ActiveSpells::Flag_Temporary | ESM::ActiveSpells::Flag_Stackable)
        : ESM::ActiveSpells::Flag_Temporary;
    if (restored)
        params.mFlags = static_cast<ESM::ActiveSpells::Flags>(params.mFlags | ESM::ActiveSpells::Flag_Restored);
    params.mWorsenings = -1;

    MWWorld::Ptr caster = getTargetPtr(activeSpell.caster);
    if (!caster.isEmpty())
        params.mCaster = caster.getCellRef().getRefNum();

    // Don't check whether the spell exists: effects from potions come here too
    actor.getClass().getCreatureStats(actor).getActiveSpells().addSpell(params);
}

void MechanicsHelper::removeActiveSpell(const MWWorld::Ptr& actor, const mwmp::ActiveSpell& activeSpell)
{
    MWMechanics::ActiveSpells& activeSpells = actor.getClass().getCreatureStats(actor).getActiveSpells();
    const ESM::RefId id = refId(activeSpell.id);

    if (!activeSpell.isStackingSpell)
    {
        activeSpells.removeEffectsBySourceSpellId(actor, id);
        return;
    }

    // TES3MP told stacked copies apart by when they were cast, which OpenMW 0.51 doesn't keep; they arrive in the
    // order they were cast on every client, so remove the oldest copy
    for (const auto& params : activeSpells)
    {
        if (params.getSourceSpellId() == id)
        {
            activeSpells.removeEffectsByActiveSpellId(actor, params.getActiveSpellId());
            return;
        }
    }

    LOG_APPEND(TimedLog::LOG_INFO, "-- spell %s is not active", activeSpell.id.c_str());
}

bool MechanicsHelper::isStackingSpell(const std::string& id)
{
    return !MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(refId(id));
}

bool MechanicsHelper::doesEffectListContainEffect(const ESM::EffectList& effectList, const ESM::RefId& effectId,
    const ESM::RefId& attributeId, const ESM::RefId& skillId)
{
    for (const auto &effect : effectList.mList)
    {
        if (effect.mData.mEffectID == effectId)
        {
            if (attributeId.empty() || effect.mData.mAttribute == attributeId)
            {
                if (skillId.empty() || effect.mData.mSkill == skillId)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void MechanicsHelper::unequipItemsByEffect(const MWWorld::Ptr& ptr, int enchantmentType, const ESM::RefId& effectId,
    const ESM::RefId& attributeId, const ESM::RefId& skillId)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::InventoryStore &ptrInventory = ptr.getClass().getInventoryStore(ptr);

    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
    {
        if (ptrInventory.getSlot(slot) != ptrInventory.end())
        {
            MWWorld::ConstContainerStoreIterator itemIterator = ptrInventory.getSlot(slot);
            ESM::RefId enchantmentId = itemIterator->getClass().getEnchantment(*itemIterator);

            if (!enchantmentId.empty())
            {
                const ESM::Enchantment* enchantment = world->getStore().get<ESM::Enchantment>().search(enchantmentId);

                if (enchantment && enchantment->mData.mType == enchantmentType
                    && doesEffectListContainEffect(enchantment->mEffects, effectId, attributeId, skillId))
                    ptrInventory.unequipSlot(slot);
            }
        }
    }
}

void MechanicsHelper::setGoldPool(const MWWorld::Ptr& merchant, int goldPool)
{
    MWMechanics::CreatureStats& stats = merchant.getClass().getCreatureStats(merchant);
    if (!mwmp::Main::isInitialized())
    {
        stats.setGoldPool(goldPool);
        return;
    }

    mwmp::ObjectList* objectList = mwmp::Main::get().getNetworking()->getObjectList();
    objectList->reset();
    objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;
    objectList->addObjectMiscellaneous(merchant, std::max(goldPool, 0), stats.getLastRestockTime().getHour(),
        stats.getLastRestockTime().getDay());
    objectList->sendObjectMiscellaneous();
}

void MechanicsHelper::restockGoldPool(const MWWorld::Ptr& merchant, int goldPool)
{
    MWMechanics::CreatureStats& stats = merchant.getClass().getCreatureStats(merchant);
    const MWWorld::TimeStamp now = MWBase::Environment::get().getWorld()->getTimeStamp();
    if (!mwmp::Main::isInitialized())
    {
        stats.setGoldPool(goldPool);
        stats.setLastRestockTime(now);
        return;
    }

    mwmp::ObjectList* objectList = mwmp::Main::get().getNetworking()->getObjectList();
    objectList->reset();
    objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;
    objectList->addObjectMiscellaneous(merchant, std::max(goldPool, 0), now.getHour(), now.getDay());
    objectList->sendObjectMiscellaneous();
}

MWWorld::Ptr MechanicsHelper::getItemPtrFromStore(const mwmp::Item& item, MWWorld::ContainerStore& store)
{
    MWWorld::Ptr closestPtr;

    for (MWWorld::ContainerStoreIterator storeIterator = store.begin(); storeIterator != store.end(); ++storeIterator)
    {
        // Enchantment charges are often in the process of refilling themselves, so don't check for them here
        if (storeIterator->getCellRef().getRefId() == item.refId &&
            item.count == storeIterator->getCellRef().getCount() &&
            item.charge == storeIterator->getCellRef().getCharge() &&
            isSameId(storeIterator->getCellRef().getSoul(), item.soul))
        {
            // If we have no closestPtr, set it to the Ptr corresponding to this storeIterator; otherwise, make
            // sure the storeIterator's enchantmentCharge is closer to our goal than that of the previous closestPtr
            if (closestPtr.isEmpty() || abs(storeIterator->getCellRef().getEnchantmentCharge() - item.enchantmentCharge) <
                abs(closestPtr.getCellRef().getEnchantmentCharge() - item.enchantmentCharge))
            {
                closestPtr = *storeIterator;
            }
        }
    }

    return closestPtr;
}

std::string MechanicsHelper::getVoiceCaption(const std::string& sound)
{
    for (const ESM::Dialogue& dialogue : MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>())
    {
        if (dialogue.mType != ESM::Dialogue::Voice)
            continue;

        for (const ESM::DialInfo& info : dialogue.mInfo)
        {
            if (!info.mSound.empty() && Misc::StringUtils::ciEqual(sound, info.mSound))
                return info.mResponse;
        }
    }

    return "???";
}
