#include "Factions.hpp"

#include <components/misc/strings/algorithm.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>

using namespace mwmp;

Faction tempFaction;
const Faction emptyFaction = {};

void FactionFunctions::ClearFactionChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->factionChanges.factions.clear();
}

unsigned int FactionFunctions::GetFactionChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.factions.size();
}

unsigned char FactionFunctions::GetFactionChangesAction(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.action;
}

const char *FactionFunctions::GetFactionId(unsigned short pid, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (index >= player->factionChanges.factions.size())
        return "invalid";

    return player->factionChanges.factions.at(index).factionId.c_str();
}

int FactionFunctions::GetFactionRank(unsigned short pid, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.factions.at(index).rank;
}

bool FactionFunctions::GetFactionExpulsionState(unsigned short pid, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, false);

    return player->factionChanges.factions.at(index).isExpelled;
}

int FactionFunctions::GetFactionReputation(unsigned short pid, unsigned int index) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.factions.at(index).reputation;
}

void FactionFunctions::SetFactionChangesAction(unsigned short pid, unsigned char action) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->factionChanges.action = action;
}

void FactionFunctions::SetFactionId(const char* factionId) noexcept
{
    tempFaction.factionId = factionId;
}

void FactionFunctions::SetFactionRank(unsigned int rank) noexcept
{
    tempFaction.rank = rank;
}

void FactionFunctions::SetFactionExpulsionState(bool expulsionState) noexcept
{
    tempFaction.isExpelled = expulsionState;
}

void FactionFunctions::SetFactionReputation(int reputation) noexcept
{
    tempFaction.reputation = reputation;
}

void FactionFunctions::AddFaction(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->factionChanges.factions.push_back(tempFaction);

    tempFaction = emptyFaction;
}

void FactionFunctions::SendFactionChanges(unsigned short pid, bool sendToOtherPlayers, bool skipAttachedPlayer) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::PlayerPacket *packet = mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_FACTION);
    packet->setPlayer(player);

    if (!skipAttachedPlayer)
        packet->Send(false);
    if (sendToOtherPlayers)
        packet->Send(true);
}

// All methods below are deprecated versions of methods from above

void FactionFunctions::InitializeFactionChanges(unsigned short pid) noexcept
{
    ClearFactionChanges(pid);
}
