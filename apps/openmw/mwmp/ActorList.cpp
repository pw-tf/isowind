#include "Ids.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "ActorList.hpp"
#include "CellController.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "MechanicsHelper.hpp"

#include "../mwworld/class.hpp"

#include <components/openmw-mp/TimedLog.hpp>

using namespace mwmp;

ActorList::ActorList()
{

}

ActorList::~ActorList()
{

}

Networking *ActorList::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}

void ActorList::reset()
{
    cell.blank();
    baseActors.clear();
    positionActors.clear();
    animFlagsActors.clear();
    animPlayActors.clear();
    speechActors.clear();
    statsDynamicActors.clear();
    deathActors.clear();
    equipmentActors.clear();
    aiActors.clear();
    attackActors.clear();
    castActors.clear();
    cellChangeActors.clear();
    guid = mwmp::Main::get().getNetworking()->getLocalPlayer()->guid;
}

void ActorList::addActor(BaseActor baseActor)
{
    baseActors.push_back(baseActor);
}

void ActorList::addPositionActor(BaseActor baseActor)
{
    positionActors.push_back(baseActor);
}

void ActorList::addAnimFlagsActor(BaseActor baseActor)
{
    animFlagsActors.push_back(baseActor);
}

void ActorList::addAnimPlayActor(BaseActor baseActor)
{
    animPlayActors.push_back(baseActor);
}

void ActorList::addSpeechActor(BaseActor baseActor)
{
    speechActors.push_back(baseActor);
}

void ActorList::addStatsDynamicActor(BaseActor baseActor)
{
    statsDynamicActors.push_back(baseActor);
}

void ActorList::addDeathActor(BaseActor baseActor)
{
    deathActors.push_back(baseActor);
}

void ActorList::addEquipmentActor(BaseActor baseActor)
{
    equipmentActors.push_back(baseActor);
}

void ActorList::addAiActor(BaseActor baseActor)
{
    aiActors.push_back(baseActor);
}

void ActorList::addAiActor(const MWWorld::Ptr& actorPtr, const MWWorld::Ptr& targetPtr, unsigned int aiAction)
{
    mwmp::BaseActor baseActor;
    baseActor.refNum = actorPtr.getCellRef().getMpRefNum();
    baseActor.mpNum = actorPtr.getCellRef().getMpNum();
    baseActor.aiAction = aiAction;
    baseActor.aiTarget = MechanicsHelper::getTarget(targetPtr);

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Preparing to send ID_ACTOR_AI about %s %i-%i\n- action: %i",
        mwmp::idString(actorPtr.getCellRef().getRefId()).c_str(), baseActor.refNum, baseActor.mpNum, aiAction);

    if (baseActor.aiTarget.isPlayer)
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "- Has player target %s",
            std::string(targetPtr.getClass().getName(targetPtr)).c_str());
    }
    else
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "- Has actor target %s %i-%i",
            mwmp::idString(targetPtr.getCellRef().getRefId()).c_str(), baseActor.aiTarget.refNum, baseActor.aiTarget.mpNum);
    }

    addAiActor(baseActor);
}

void ActorList::addAttackActor(BaseActor baseActor)
{
    attackActors.push_back(baseActor);
}

void ActorList::addAttackActor(const MWWorld::Ptr& actorPtr, const mwmp::Attack &attack)
{
    mwmp::BaseActor baseActor;
    baseActor.refNum = actorPtr.getCellRef().getMpRefNum();
    baseActor.mpNum = actorPtr.getCellRef().getMpNum();
    baseActor.attack = attack;
    attackActors.push_back(baseActor);
}

void ActorList::addCastActor(BaseActor baseActor)
{
    castActors.push_back(baseActor);
}

void ActorList::addCellChangeActor(BaseActor baseActor)
{
    cellChangeActors.push_back(baseActor);
}

void ActorList::sendPositionActors()
{
    if (positionActors.size() > 0)
    {
        baseActors = positionActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_POSITION)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_POSITION)->Send();
    }
}

void ActorList::sendAnimFlagsActors()
{
    if (animFlagsActors.size() > 0)
    {
        baseActors = animFlagsActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ANIM_FLAGS)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ANIM_FLAGS)->Send();
    }
}

void ActorList::sendAnimPlayActors()
{
    if (animPlayActors.size() > 0)
    {
        baseActors = animPlayActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ANIM_PLAY)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ANIM_PLAY)->Send();
    }
}

void ActorList::sendSpeechActors()
{
    if (speechActors.size() > 0)
    {
        baseActors = speechActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_SPEECH)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_SPEECH)->Send();
    }
}

void ActorList::sendStatsDynamicActors()
{
    if (statsDynamicActors.size() > 0)
    {
        baseActors = statsDynamicActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_STATS_DYNAMIC)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_STATS_DYNAMIC)->Send();
    }
}

void ActorList::sendDeathActors()
{
    if (deathActors.size() > 0)
    {
        baseActors = deathActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_DEATH)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_DEATH)->Send();
    }
}

void ActorList::sendEquipmentActors()
{
    if (equipmentActors.size() > 0)
    {
        baseActors = equipmentActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_EQUIPMENT)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_EQUIPMENT)->Send();
    }
}

void ActorList::sendAiActors()
{
    if (aiActors.size() > 0)
    {
        baseActors = aiActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_AI)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_AI)->Send();
    }
}

void ActorList::sendAttackActors()
{
    if (attackActors.size() > 0)
    {
        baseActors = attackActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ATTACK)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ATTACK)->Send();
    }
}

void ActorList::sendCastActors()
{
    if (castActors.size() > 0)
    {
        baseActors = castActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_CAST)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_CAST)->Send();
    }
}

void ActorList::sendCellChangeActors()
{
    if (cellChangeActors.size() > 0)
    {
        baseActors = cellChangeActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_CELL_CHANGE)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_CELL_CHANGE)->Send();
    }
}

void ActorList::sendActorsInCell(MWWorld::CellStore* cellStore)
{
    reset();
    cell = cellStore->getCell()->getEsm3();
    action = BaseActorList::SET;

    cellStore->forEach([&](const MWWorld::Ptr& ptr) {
        if (!ptr.getClass().isActor())
            return true;

        // If this Ptr is lacking a unique index, ignore it
        if (ptr.getCellRef().getMpRefNum() == 0 && ptr.getCellRef().getMpNum() == 0)
            return true;

        // Isowind MMO: nor one this client runs for itself only (a one-off event's)
        if (Main::get().getCellController()->isClientLocalActor(ptr))
            return true;

        BaseActor actor;
        actor.refId = idString(ptr.getCellRef().getRefId());
        actor.refNum = ptr.getCellRef().getMpRefNum();
        actor.mpNum = ptr.getCellRef().getMpNum();

        addActor(actor);
        return true;
    });

    mwmp::Main::get().getNetworking()->getActorPacket(ID_ACTOR_LIST)->setActorList(this);
    mwmp::Main::get().getNetworking()->getActorPacket(ID_ACTOR_LIST)->Send();
}
