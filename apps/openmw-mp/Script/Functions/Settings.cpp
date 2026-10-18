#include "Settings.hpp"

#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/TimedLog.hpp>

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>

#include <iostream>

void SettingFunctions::SetDifficulty(unsigned short pid, int difficulty)
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->difficulty = difficulty;
}

void SettingFunctions::SetEnforcedLogLevel(unsigned short pid, int enforcedLogLevel)
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->enforcedLogLevel = enforcedLogLevel;
}

void SettingFunctions::SetPhysicsFramerate(unsigned short pid, double physicsFramerate)
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->physicsFramerate = physicsFramerate;
}

void SettingFunctions::SetConsoleAllowed(unsigned short pid, bool state)
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->consoleAllowed = state;
}

void SettingFunctions::SetBedRestAllowed(unsigned short pid, bool state)
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->bedRestAllowed = state;
}

void SettingFunctions::SetWildernessRestAllowed(unsigned short pid, bool state)
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->wildernessRestAllowed = state;
}

void SettingFunctions::SetWaitAllowed(unsigned short pid, bool state)
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->waitAllowed = state;
}

void SettingFunctions::SetGameSettingValue(unsigned short pid, const char* setting, const char* value) {
    Player* player;
    GET_PLAYER(pid, player, );

    player->gameSettings[setting] = value;
}

void SettingFunctions::ClearGameSettingValues(unsigned short pid) {
    Player* player;
    GET_PLAYER(pid, player, );

    player->gameSettings.clear();
}

void SettingFunctions::SetVRSettingValue(unsigned short pid, const char* setting, const char* value) {
    Player* player;
    GET_PLAYER(pid, player, );

    player->vrSettings[setting] = value;
}

void SettingFunctions::ClearVRSettingValues(unsigned short pid) {
    Player* player;
    GET_PLAYER(pid, player, );

    player->vrSettings.clear();
}

void SettingFunctions::SendSettings(unsigned short pid, bool sendToOtherPlayers, bool skipAttachedPlayer) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::PlayerPacket *packet = mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_SETTINGS);
    packet->setPlayer(player);

    if (!skipAttachedPlayer)
        packet->Send(false);
    if (sendToOtherPlayers)
        packet->Send(true);
}
