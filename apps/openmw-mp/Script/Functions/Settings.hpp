#ifndef OPENMW_SETTINGSAPI_HPP
#define OPENMW_SETTINGSAPI_HPP

#include "../Types.hpp"

#define SETTINGSAPI \
    {"SetDifficulty",               SettingFunctions::SetDifficulty},\
    {"SetEnforcedLogLevel",         SettingFunctions::SetEnforcedLogLevel},\
    {"SetPhysicsFramerate",         SettingFunctions::SetPhysicsFramerate},\
    \
    {"SetConsoleAllowed",           SettingFunctions::SetConsoleAllowed},\
    {"SetBedRestAllowed",           SettingFunctions::SetBedRestAllowed},\
    {"SetWildernessRestAllowed",    SettingFunctions::SetWildernessRestAllowed},\
    {"SetWaitAllowed",              SettingFunctions::SetWaitAllowed},\
    \
    {"SetGameSettingValue",         SettingFunctions::SetGameSettingValue},\
    {"ClearGameSettingValues",      SettingFunctions::ClearGameSettingValues},\
    \
    {"SetVRSettingValue",           SettingFunctions::SetVRSettingValue},\
    {"ClearVRSettingValues",        SettingFunctions::ClearVRSettingValues},\
    \
    {"SendSettings",                SettingFunctions::SendSettings}

class SettingFunctions
{
public:

    /**
    * \brief Set the difficulty for a player.
    *
    * This changes the difficulty for that player in the server memory, but does not by itself
    * send a packet.
    *
    * \param pid The player ID.
    * \param difficulty The difficulty.
    * \return void
    */
    static void SetDifficulty(unsigned short pid, int difficulty);

    /**
    * \brief Set the client log level enforced for a player.
    *
    * This changes the enforced log level for that player in the server memory, but does not by itself
    * send a packet.
    *
    * Enforcing a certain log level is necessary to prevent players from learning information from
    * their console window that they are otherwise unable to obtain, such as the locations of
    * other players.
    *
    * If you do not wish to enforce a log level, simply set enforcedLogLevel to -1
    *
    * \param pid The player ID.
    * \param enforcedLogLevel The enforced log level.
    * \return void
    */
    static void SetEnforcedLogLevel(unsigned short pid, int enforcedLogLevel);

    /**
    * \brief Set the physics framerate for a player.
    *
    * This changes the physics framerate for that player in the server memory, but does not by itself
    * send a packet.
    *
    * \param pid The player ID.
    * \param physicsFramerate The physics framerate.
    * \return void
    */
    static void SetPhysicsFramerate(unsigned short pid, double physicsFramerate);

    /**
    * \brief Set whether the console is allowed for a player.
    *
    * This changes the console permission for that player in the server memory, but does not
    * by itself send a packet.
    *
    * \param pid The player ID.
    * \param state The console permission state.
    * \return void
    */
    static void SetConsoleAllowed(unsigned short pid, bool state);

    /**
    * \brief Set whether resting in beds is allowed for a player.
    *
    * This changes the resting permission for that player in the server memory, but does not
    * by itself send a packet.
    *
    * \param pid The player ID.
    * \param state The resting permission state.
    * \return void
    */
    static void SetBedRestAllowed(unsigned short pid, bool state);

    /**
    * \brief Set whether resting in the wilderness is allowed for a player.
    *
    * This changes the resting permission for that player in the server memory, but does not
    * by itself send a packet.
    *
    * \param pid The player ID.
    * \param state The resting permission state.
    * \return void
    */
    static void SetWildernessRestAllowed(unsigned short pid, bool state);

    /**
    * \brief Set whether waiting is allowed for a player.
    *
    * This changes the waiting permission for that player in the server memory, but does not
    * by itself send a packet.
    *
    * \param pid The player ID.
    * \param state The waiting permission state.
    * \return void
    */
    static void SetWaitAllowed(unsigned short pid, bool state);

    /**
    * \brief Set value for a game setting.
    *
    * This overrides the setting value set in OpenMW Launcher. Only applies to the Game category.
    *
    * \param pid The player ID.
    * \param setting Name of a setting in the Game category
    * \param value Value of the setting (as a string)
    * \return void
    */
    static void SetGameSettingValue(unsigned short pid, const char* setting, const char* value);

    /**
    * \brief Clear the Game setting values stored for a player.
    *
    * Clear any changes done by SetGameSettingValue()
    *
    * \param pid The player ID.
    * \return void
    */
    static void ClearGameSettingValues(unsigned short pid);

    /**
    * \brief Set value for a VR setting.
    *
    * This overrides the setting value set in OpenMW Launcher. Only applies to the VR category.
    *
    * \param pid The player ID.
    * \param setting Name of a setting in the VR category
    * \param value Value of the setting (as a string)
    * \return void
    */
    static void SetVRSettingValue(unsigned short pid, const char* setting, const char* value);

    /**
    * \brief Clear the VR setting values stored for a player.
    *
    * Clear any changes done by SetVRSettingValue()
    *
    * \param pid The player ID.
    * \return void
    */
    static void ClearVRSettingValues(unsigned short pid);

    /**
    * \brief Send a PlayerSettings packet to the player affected by it.
    *
    * \param pid The player ID to send it to.
    * \return void
    */
    static void SendSettings(unsigned short pid, bool sendToOtherPlayers, bool skipAttachedPlayer) noexcept;
};

#endif //OPENMW_SETTINGSAPI_HPP
