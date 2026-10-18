#include <components/esm3/cellid.hpp>
#include <components/openmw-mp/TimedLog.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/livecellref.hpp"
#include "../mwworld/worldimp.hpp"

#include "Cell.hpp"
#include "Ids.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"

using namespace mwmp;

mwmp::Cell::Cell(MWWorld::CellStore* cellStore)
{
    store = cellStore;
    shouldInitializeActors = false;

    updateTimer = 0;
}

Cell::~Cell()
{

}

void Cell::updateLocal(bool forceUpdate)
{
    if (localActors.empty())
        return;

    const float timeoutSec = 0.025;

    if (!forceUpdate && (updateTimer += MWBase::Environment::get().getFrameDuration()) < timeoutSec)
        return;
    else
        updateTimer = 0;

    CellController *cellController = Main::get().getCellController();
    ActorList *actorList = mwmp::Main::get().getNetworking()->getActorList();
    actorList->reset();

    actorList->cell = store->getCell()->getEsm3();

    for (auto it = localActors.begin(); it != localActors.end();)
    {
        LocalActor *actor = it->second;

        MWWorld::CellStore *newStore = actor->getPtr().getCell();

        if (newStore != store)
        {
            actor->updateCell();
            std::string mapIndex = it->first;

            // If the cell this actor has moved to is under our authority, move them to it
            if (cellController->hasLocalAuthority(actor->cell))
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "- Moving LocalActor %s to our authority in %s",
                    mapIndex.c_str(), actor->cell.getShortDescription().c_str());
                Cell *newCell = cellController->getCell(actor->cell);
                newCell->localActors[mapIndex] = actor;
                cellController->setLocalActorRecord(mapIndex, newCell->getShortDescription());
            }
            else
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "- Deleting LocalActor %s which is no longer under our authority",
                    mapIndex.c_str(), getShortDescription().c_str());
                cellController->removeLocalActorRecord(mapIndex);
                delete actor;
            }

            localActors.erase(it++);
        }
        else
        {
            if (actor->getPtr().getRefData().isEnabled())
            {
                if (actor->getPtr().mRef->isDeleted())
                {
                    std::string mapIndex = it->first;
                    LOG_APPEND(TimedLog::LOG_VERBOSE, "- Deleting LocalActor %s whose reference has been deleted",
                        mapIndex.c_str(), getShortDescription().c_str());
                    cellController->removeLocalActorRecord(mapIndex);
                    delete actor;
                    localActors.erase(it++);
                }
                else
                {
                    // Forcibly update this local actor if its data has never been sent before;
                    // otherwise, use the current forceUpdate value
                    actor->update(actor->hasSentData ? forceUpdate : true);
                }
            }

            ++it;
        }
    }

    actorList->sendPositionActors();
    actorList->sendAnimFlagsActors();
    actorList->sendAnimPlayActors();
    actorList->sendSpeechActors();
    actorList->sendDeathActors();
    actorList->sendStatsDynamicActors();
    actorList->sendEquipmentActors();
    actorList->sendAttackActors();
    actorList->sendCastActors();
    actorList->sendCellChangeActors();
}

void Cell::updateDedicated(float dt)
{
    if (dedicatedActors.empty()) return;
    
    for (auto &actor : dedicatedActors)
        actor.second->update(dt);

    // Are we the authority over this cell? If so, uninitialize DedicatedActors
    // after the above update
    if (hasLocalAuthority())
        uninitializeDedicatedActors();
}

void Cell::readPositions(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    if (dedicatedActors.empty()) return;
    
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->position = baseActor.position;
            actor->direction = baseActor.direction;

            if (!actor->hasPositionData)
            {
                actor->hasPositionData = true;

                // If this is our first packet about this actor's position, force an update
                // now instead of waiting for its frame
                //
                // That way, if this actor is about to become a LocalActor, initial data about it
                // received from the server still gets set
                actor->setPosition();
            }
        }
    }
}

void Cell::readAnimFlags(ActorList& actorList)
{
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->movementFlags = baseActor.movementFlags;
            actor->drawState = baseActor.drawState;
            actor->isFlying = baseActor.isFlying;
        }
    }
}

void Cell::readAnimPlay(ActorList& actorList)
{
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->animation.groupname = baseActor.animation.groupname;
            actor->animation.mode = baseActor.animation.mode;
            actor->animation.count = baseActor.animation.count;
            actor->animation.persist = baseActor.animation.persist;
            actor->playAnimation();
        }
    }
}

void Cell::readStatsDynamic(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    if (dedicatedActors.empty()) return;

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->creatureStats = baseActor.creatureStats;

            if (!actor->hasStatsDynamicData)
            {
                actor->hasStatsDynamicData = true;

                // If this is our first packet about this actor's dynamic stats, force an update
                // now instead of waiting for its frame
                //
                // That way, if this actor is about to become a LocalActor, initial data about it
                // received from the server still gets set
                actor->setStatsDynamic();
            }
        }
    }
}

void Cell::readDeath(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    if (dedicatedActors.empty()) return;

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->creatureStats.mDead = true;
            actor->creatureStats.mDynamic[0].mCurrent = 0;

            Main::get().getCellController()->setQueuedDeathState(actor->getPtr(), baseActor.deathState);

            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_ACTOR_DEATH about %s %i-%i in cell %s\n- deathState: %d\n-isInstantDeath: %s",
                actor->refId.c_str(), actor->refNum, actor->mpNum, getShortDescription().c_str(),
                baseActor.deathState, baseActor.isInstantDeath ? "true" : "false");

            if (baseActor.isInstantDeath)
            {
                actor->getPtr().getClass().getCreatureStats(actor->getPtr()).setDeathAnimationFinished(true);
                MWBase::Environment::get().getWorld()->enableActorCollision(actor->getPtr(), false);
            }
        }
    }
}

void Cell::readEquipment(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    if (dedicatedActors.empty()) return;

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];

            for (int slot = 0; slot < 19; ++slot)
                actor->equipmentItems[slot] = baseActor.equipmentItems[slot];

            actor->setEquipment();
        }
    }

    if (hasLocalAuthority())
        uninitializeDedicatedActors(actorList);
}

void Cell::readSpeech(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    if (dedicatedActors.empty()) return;

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->sound = baseActor.sound;
            actor->playSound();
        }
    }

    if (hasLocalAuthority())
        uninitializeDedicatedActors(actorList);
}

void Cell::readSpellsActive(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    if (dedicatedActors.empty()) return;

    for (const auto& baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor* actor = dedicatedActors[mapIndex];
            actor->spellsActiveChanges = baseActor.spellsActiveChanges;

            int spellsActiveAction = baseActor.spellsActiveChanges.action;

            if (spellsActiveAction == SpellsActiveChanges::ADD)
                actor->addSpellsActive();
            else if (spellsActiveAction == SpellsActiveChanges::REMOVE)
                actor->removeSpellsActive();
            else
                actor->setSpellsActive();
        }
    }

    if (hasLocalAuthority())
        uninitializeDedicatedActors(actorList);
}

void Cell::readAi(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    if (dedicatedActors.empty()) return;

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->aiAction = baseActor.aiAction;
            actor->aiDistance = baseActor.aiDistance;
            actor->aiDuration = baseActor.aiDuration;
            actor->aiShouldRepeat = baseActor.aiShouldRepeat;
            actor->aiCoordinates = baseActor.aiCoordinates;
            actor->hasAiTarget = baseActor.hasAiTarget;
            actor->aiTarget = baseActor.aiTarget;
            actor->setAi();
        }
    }

    if (hasLocalAuthority())
        uninitializeDedicatedActors(actorList);
}

void Cell::readAttack(ActorList& actorList)
{
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Reading ActorAttack about %s", mapIndex.c_str());

            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->attack = baseActor.attack;

            MechanicsHelper::processAttack(actor->attack, actor->getPtr());
        }
    }
}

void Cell::readCast(ActorList& actorList)
{
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Reading ActorCast about %s", mapIndex.c_str());

            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->cast = baseActor.cast;

            // Set the correct drawState here if we've somehow we've missed a previous
            // AnimFlags packet
            if (actor->drawState != static_cast<char>(MWMechanics::DrawState::Spell))
            {
                actor->drawState = static_cast<char>(MWMechanics::DrawState::Spell);
                actor->setAnimFlags();
            }

            MechanicsHelper::processCast(actor->cast, actor->getPtr());
        }
    }
}

void Cell::readCellChange(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    if (dedicatedActors.empty()) return;

    CellController *cellController = Main::get().getCellController();

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        // Is a packet mistakenly moving the actor to the cell it's already in? If so, ignore it
        if (Misc::StringUtils::ciEqual(getShortDescription(), baseActor.cell.getShortDescription()))
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_WARN, "Server says DedicatedActor %s moved to %s, but it was already there",
                mapIndex.c_str(), getShortDescription().c_str());
            continue;
        }

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *dedicatedActor = dedicatedActors[mapIndex];
            dedicatedActor->cell = baseActor.cell;
            dedicatedActor->position = baseActor.position;
            dedicatedActor->direction = baseActor.direction;

            LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Server says DedicatedActor %s moved to %s",
                mapIndex.c_str(), dedicatedActor->cell.getShortDescription().c_str());

            MWWorld::CellStore *newStore = cellController->getCellStore(dedicatedActor->cell);
            dedicatedActor->setCell(newStore);

            // If the cell this actor has moved to is active and not under our authority, move them to it
            if (cellController->isActiveWorldCell(dedicatedActor->cell) && !cellController->hasLocalAuthority(dedicatedActor->cell))
            {
                LOG_APPEND(TimedLog::LOG_VERBOSE, "- Moving DedicatedActor %s to our active cell %s",
                    mapIndex.c_str(), dedicatedActor->cell.getShortDescription().c_str());
                cellController->initializeCell(dedicatedActor->cell);
                Cell *newCell = cellController->getCell(dedicatedActor->cell);
                newCell->dedicatedActors[mapIndex] = dedicatedActor;
                cellController->setDedicatedActorRecord(mapIndex, newCell->getShortDescription());
            }
            else
            {
                if (cellController->hasLocalAuthority(dedicatedActor->cell))
                {
                    LOG_APPEND(TimedLog::LOG_VERBOSE, "- Creating new LocalActor based on %s in %s",
                        mapIndex.c_str(), dedicatedActor->cell.getShortDescription().c_str());
                    Cell *newCell = cellController->getCell(dedicatedActor->cell);
                    LocalActor *localActor = new LocalActor();
                    localActor->cell = dedicatedActor->cell;
                    localActor->setPtr(dedicatedActor->getPtr());
                    localActor->position = dedicatedActor->position;
                    localActor->direction = dedicatedActor->direction;
                    localActor->movementFlags = dedicatedActor->movementFlags;
                    localActor->drawState = dedicatedActor->drawState;
                    localActor->isFlying = dedicatedActor->isFlying;
                    localActor->creatureStats = dedicatedActor->creatureStats;

                    newCell->localActors[mapIndex] = localActor;
                    cellController->setLocalActorRecord(mapIndex, newCell->getShortDescription());
                }

                LOG_APPEND(TimedLog::LOG_VERBOSE, "- Deleting DedicatedActor %s which is no longer needed",
                    mapIndex.c_str(), getShortDescription().c_str());
                cellController->removeDedicatedActorRecord(mapIndex);
                delete dedicatedActor;
            }

            dedicatedActors.erase(mapIndex);
        }
    }
}

void Cell::initializeLocalActor(const MWWorld::Ptr& ptr)
{
    std::string mapIndex = Main::get().getCellController()->generateMapIndex(ptr);
    LOG_APPEND(TimedLog::LOG_VERBOSE, "- Initializing LocalActor %s in %s", mapIndex.c_str(), getShortDescription().c_str());

    LocalActor *actor = new LocalActor();
    actor->cell = store->getCell()->getEsm3();
    actor->setPtr(ptr);

    localActors[mapIndex] = actor;

    Main::get().getCellController()->setLocalActorRecord(mapIndex, getShortDescription());

    LOG_APPEND(TimedLog::LOG_VERBOSE, "- Successfully initialized LocalActor %s in %s", mapIndex.c_str(), getShortDescription().c_str());
}

void Cell::initializeLocalActors()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Initializing LocalActors in %s", getShortDescription().c_str());

    // Collect first: the cell can't change while it's being visited
    std::vector<MWWorld::Ptr> actors;
    store->forEach([&](const MWWorld::Ptr& ptr) {
        if (ptr.getClass().isActor())
            actors.push_back(ptr);
        return true;
    });

    for (const MWWorld::Ptr& ptr : actors)
    {
        // If this Ptr is lacking a unique index, ignore it
        if (ptr.getCellRef().getMpRefNum() == 0 && ptr.getCellRef().getMpNum() == 0) continue;

        // If this Ptr is disabled or deleted, ignore it
        if (!ptr.getRefData().isEnabled() || ptr.mRef->isDeleted()) continue;

        std::string mapIndex = Main::get().getCellController()->generateMapIndex(ptr);

        // Only initialize this actor if it isn't already initialized
        if (localActors.count(mapIndex) == 0)
            initializeLocalActor(ptr);
    }

    LOG_APPEND(TimedLog::LOG_VERBOSE, "- Successfully initialized LocalActors in %s", getShortDescription().c_str());
}

void Cell::initializeDedicatedActor(const MWWorld::Ptr& ptr)
{
    std::string mapIndex = Main::get().getCellController()->generateMapIndex(ptr);
    LOG_APPEND(TimedLog::LOG_VERBOSE, "- Initializing DedicatedActor %s in %s", mapIndex.c_str(), getShortDescription().c_str());

    DedicatedActor *actor = new DedicatedActor();
    actor->cell = store->getCell()->getEsm3();
    actor->setPtr(ptr);

    dedicatedActors[mapIndex] = actor;

    Main::get().getCellController()->setDedicatedActorRecord(mapIndex, getShortDescription());

    LOG_APPEND(TimedLog::LOG_VERBOSE, "- Successfully initialized DedicatedActor %s in %s", mapIndex.c_str(), getShortDescription().c_str());
}

void Cell::initializeDedicatedActors(ActorList& actorList)
{
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        // If this key doesn't exist, create it
        if (dedicatedActors.count(mapIndex) == 0)
        {
            MWWorld::Ptr ptrFound = store->searchExact(baseActor.refNum, baseActor.mpNum, refId(baseActor.refId), true);

            if (ptrFound.isEmpty()) continue;

            initializeDedicatedActor(ptrFound);
        }
    }
}

void Cell::uninitializeLocalActors()
{
    for (const auto &actor : localActors)
    {
        Main::get().getCellController()->removeLocalActorRecord(actor.first);
        delete actor.second;
    }

    localActors.clear();
}

void Cell::uninitializeDedicatedActors(ActorList& actorList)
{
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);
        Main::get().getCellController()->removeDedicatedActorRecord(mapIndex);
        delete dedicatedActors.at(mapIndex);
        dedicatedActors.erase(mapIndex);
    }
}

void Cell::uninitializeDedicatedActors()
{
    for (const auto &actor : dedicatedActors)
    {
        Main::get().getCellController()->removeDedicatedActorRecord(actor.first);
        delete actor.second;
    }

    dedicatedActors.clear();
}

LocalActor *Cell::getLocalActor(std::string actorIndex)
{
    return localActors.at(actorIndex);
}

DedicatedActor *Cell::getDedicatedActor(std::string actorIndex)
{
    return dedicatedActors.at(actorIndex);
}

bool Cell::hasLocalAuthority()
{
    return authorityGuid == Main::get().getLocalPlayer()->guid;
}

void Cell::setAuthority(const RakNet::RakNetGUID& guid)
{
    authorityGuid = guid;
}

MWWorld::CellStore *Cell::getCellStore()
{
    return store;
}

std::string Cell::getShortDescription()
{
    return store->getCell()->getEsm3().getShortDescription();
}
