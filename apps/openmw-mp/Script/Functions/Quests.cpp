#include "Quests.hpp"

#include <components/misc/strings/algorithm.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>

using namespace mwmp;

void QuestFunctions::ClearJournalChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->journalChanges.clear();
}

unsigned int QuestFunctions::GetJournalChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.size();
}

void QuestFunctions::AddJournalEntry(unsigned short pid, const char* quest, unsigned int index, const char* actorRefId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::ENTRY;
    journalItem.quest = quest;
    journalItem.index = index;
    journalItem.actorRefId = actorRefId;
    journalItem.hasTimestamp = false;

    player->journalChanges.push_back(journalItem);
}

void QuestFunctions::AddJournalEntryWithTimestamp(unsigned short pid, const char* quest, unsigned int index, const char* actorRefId,
    unsigned int daysPassed, unsigned int month, unsigned int day) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::ENTRY;
    journalItem.quest = quest;
    journalItem.index = index;
    journalItem.actorRefId = actorRefId;
    journalItem.hasTimestamp = true;

    journalItem.timestamp.daysPassed = daysPassed;
    journalItem.timestamp.month = month;
    journalItem.timestamp.day = day;

    player->journalChanges.push_back(journalItem);
}

void QuestFunctions::AddJournalIndex(unsigned short pid, const char* quest, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::INDEX;
    journalItem.quest = quest;
    journalItem.index = index;

    player->journalChanges.push_back(journalItem);
}

void QuestFunctions::SetReputation(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->npcStats.mReputation = value;
}

const char *QuestFunctions::GetJournalItemQuest(unsigned short pid, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (index >= player->journalChanges.size())
        return "invalid";

    return player->journalChanges.at(index).quest.c_str();
}

int QuestFunctions::GetJournalItemIndex(unsigned short pid, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.at(index).index;
}

int QuestFunctions::GetJournalItemType(unsigned short pid, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.at(index).type;
}

const char *QuestFunctions::GetJournalItemActorRefId(unsigned short pid, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->journalChanges.at(index).actorRefId.c_str();
}

int QuestFunctions::GetReputation(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->npcStats.mReputation;
}

void QuestFunctions::SendJournalChanges(unsigned short pid, bool sendToOtherPlayers, bool skipAttachedPlayer) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::PlayerPacket *packet = mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_JOURNAL);
    packet->setPlayer(player);

    if (!skipAttachedPlayer)
        packet->Send(false);
    if (sendToOtherPlayers)
        packet->Send(true);
}

void QuestFunctions::SendReputation(unsigned short pid, bool sendToOtherPlayers, bool skipAttachedPlayer) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::PlayerPacket *packet = mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_REPUTATION);
    packet->setPlayer(player);

    if (!skipAttachedPlayer)
        packet->Send(false);
    if (sendToOtherPlayers)
        packet->Send(true);
}

// All methods below are deprecated versions of methods from above

void QuestFunctions::InitializeJournalChanges(unsigned short pid) noexcept
{
    ClearJournalChanges(pid);
}
