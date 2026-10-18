#ifndef OPENMW_SERVERAPI_HPP
#define OPENMW_SERVERAPI_HPP

#include "../Types.hpp"

#define SERVERAPI \
    {"LogMessage",                      ServerFunctions::LogMessage},\
    {"LogAppend",                       ServerFunctions::LogAppend},\
    \
    {"StopServer",                      ServerFunctions::StopServer},\
    \
    {"Kick",                            ServerFunctions::Kick},\
    {"BanAddress",                      ServerFunctions::BanAddress},\
    {"UnbanAddress",                    ServerFunctions::UnbanAddress},\
    \
    {"DoesFilePathExist",               ServerFunctions::DoesFilePathExist},\
    {"GetCaseInsensitiveFilename",      ServerFunctions::GetCaseInsensitiveFilename},\
    {"GetDataPath",                     ServerFunctions::GetDataPath},\
    {"GetMillisecondsSinceServerStart", ServerFunctions::GetMillisecondsSinceServerStart},\
    {"GetOperatingSystemType",          ServerFunctions::GetOperatingSystemType},\
    {"GetArchitectureType",             ServerFunctions::GetArchitectureType},\
    {"GetServerVersion",                ServerFunctions::GetServerVersion},\
    {"GetProtocolVersion",              ServerFunctions::GetProtocolVersion},\
    {"GetAvgPing",                      ServerFunctions::GetAvgPing},\
    {"GetIP",                           ServerFunctions::GetIP},\
    {"GetMaxPlayers",                   ServerFunctions::GetMaxPlayers},\
    {"GetPort",                         ServerFunctions::GetPort},\
    {"HasPassword",                     ServerFunctions::HasPassword},\
    {"GetDataFileEnforcementState",     ServerFunctions::GetDataFileEnforcementState},\
    {"GetScriptErrorIgnoringState",     ServerFunctions::GetScriptErrorIgnoringState},\
    \
    {"SetGameMode",                     ServerFunctions::SetGameMode},\
    {"SetHostname",                     ServerFunctions::SetHostname},\
    {"SetServerPassword",               ServerFunctions::SetServerPassword},\
    {"SetDataFileEnforcementState",     ServerFunctions::SetDataFileEnforcementState},\
    {"SetScriptErrorIgnoringState",     ServerFunctions::SetScriptErrorIgnoringState},\
    {"SetRuleString",                   ServerFunctions::SetRuleString},\
    {"SetRuleValue",                    ServerFunctions::SetRuleValue},\
    \
    {"AddDataFileRequirement",          ServerFunctions::AddDataFileRequirement},\
    \
    {"DoesFileExist",                   ServerFunctions::DoesFileExist},\
    {"GetModDir",                       ServerFunctions::GetModDir},\
    {"GetPluginEnforcementState",       ServerFunctions::GetPluginEnforcementState},\
    {"SetPluginEnforcementState",       ServerFunctions::SetPluginEnforcementState},\
    {"AddPluginHash",                   ServerFunctions::AddPluginHash}

class ServerFunctions
{
public:

    /**
    * \brief Write a log message with its own timestamp.
    *
    * It will have "[Script]:" prepended to it so as to mark it as a script-generated log message.
    *
    * \param level The logging level used (0 for LOG_VERBOSE, 1 for LOG_INFO, 2 for LOG_WARN,
    *              3 for LOG_ERROR, 4 for LOG_FATAL).
    * \param message The message logged.
    * \return void
    */
    static void LogMessage(unsigned short level, const char *message) noexcept;

    /**
    * \brief Write a log message without its own timestamp.
    *
    * It will have "[Script]:" prepended to it so as to mark it as a script-generated log message.
    *
    * \param level The logging level used (0 for LOG_VERBOSE, 1 for LOG_INFO, 2 for LOG_WARN,
    *              3 for LOG_ERROR, 4 for LOG_FATAL).
    * \param message The message logged.
    * \return void
    */
    static void LogAppend(unsigned short level, const char *message) noexcept;

    /**
    * \brief Shut down the server.
    *
    * \param code The shutdown code.
    * \return void
    */
    static void StopServer(int code) noexcept;

    /**
    * \brief Kick a certain player from the server.
    *
    * \param pid The player ID.
    * \return void
    */
    static void Kick(unsigned short pid) noexcept;

    /**
    * \brief Ban a certain IP address from the server.
    *
    * \param ipAddress The IP address.
    * \return void
    */
    static void BanAddress(const char *ipAddress) noexcept;

    /**
    * \brief Unban a certain IP address from the server.
    *
    * \param ipAddress The IP address.
    * \return void
    */
    static void UnbanAddress(const char *ipAddress) noexcept;

    /**
    * \brief Check whether a certain file path exists.
    *
    * This will be a case sensitive check on case sensitive filesystems.
    *
    * Whenever you want to enforce case insensitivity, use GetCaseInsensitiveFilename() instead.
    *
    * \return Whether the file exists or not.
    */
    static bool DoesFilePathExist(const char *filePath) noexcept;

    /**
    * \brief Get the first filename in a folder that has a case insensitive match with the filename
    * argument.
    *
    * This is used to retain case insensitivity when opening data files on Linux.
    *
    * \return The filename that matches.
    */
    static const char *GetCaseInsensitiveFilename(const char *folderPath, const char *filename) noexcept;

    /**
    * \brief Get the path of the server's data folder.
    *
    * \return The data path.
    */
    static const char *GetDataPath() noexcept;

    /**
    * \brief Get the milliseconds elapsed since the server was started.
    *
    * \return The time since the server's startup in milliseconds.
    */
    static unsigned int GetMillisecondsSinceServerStart() noexcept;

    /**
    * \brief Get the type of the operating system used by the server.
    *
    * Note: Currently, the type can be "Windows", "Linux", "OS X" or "Unknown OS".
    *
    * \return The type of the operating system.
    */
    static const char *GetOperatingSystemType() noexcept;

    /**
    * \brief Get the architecture type used by the server.
    *
    * Note: Currently, the type can be "64-bit", "32-bit", "ARMv#" or "Unknown architecture".
    *
    * \return The architecture type.
    */
    static const char *GetArchitectureType() noexcept;

    /**
    * \brief Get the TES3MP version of the server.
    *
    * \return The server version.
    */
    static const char *GetServerVersion() noexcept;

    /**
    * \brief Get the protocol version of the server.
    *
    * \return The protocol version.
    */
    static const char *GetProtocolVersion() noexcept;

    /**
    * \brief Get the average ping of a certain player.
    *
    * \param pid The player ID.
    * \return The average ping.
    */
    static int GetAvgPing(unsigned short pid) noexcept;

    /**
    * \brief Get the IP address of a certain player.
    *
    * \param pid The player ID.
    * \return The IP address.
    */
    static const char* GetIP(unsigned short pid) noexcept;

    /**
     * \brief Get the port used by the server.
     *
     * \return The port.
     */
    static unsigned short GetPort() noexcept;

    /**
     * \brief Get the maximum number of players.
     *
     * \return Max players
     */
    static unsigned int GetMaxPlayers() noexcept;

    /**
     * \brief Checking if the server requires a password to connect.
     *
     * \return Whether the server requires a password
     */
    static bool HasPassword() noexcept;

    /**
    * \brief Get the data file enforcement state of the server.
    *
    * If true, clients are required to use the same data files as set for the server.
    *
    * \return The enforcement state.
    */
    static bool GetDataFileEnforcementState() noexcept;

    /**
    * \brief Get the script error ignoring state of the server.
    *
    * If true, script errors will not crash the server.
    *
    * \return The script error ignoring state.
    */
    static bool GetScriptErrorIgnoringState() noexcept;

    /**
    * \brief Set the game mode of the server, as displayed in the server browser.
    *
    * \param gameMode The new game mode.
    * \return void
    */
    static void SetGameMode(const char* gameMode) noexcept;

    /**
    * \brief Set the name of the server, as displayed in the server browser.
    *
    * \param name The new name.
    * \return void
    */
    static void SetHostname(const char* name) noexcept;

    /**
    * \brief Set the password required to join the server.
    *
    * \param password The password.
    * \return void
    */
    static void SetServerPassword(const char *password) noexcept;

    /**
    * \brief Set the data file enforcement state of the server.
    *
    * If true, clients are required to use the same data files as set for the server.
    *
    * \param state The new enforcement state.
    * \return void
    */
    static void SetDataFileEnforcementState(bool state) noexcept;

    /**
    * \brief Set whether script errors should be ignored or not.
    *
    * If true, script errors will not crash the server, but could have any number
    * of unforeseen consequences, which is why this is a highly experimental
    * setting.
    *
    * \param state The new script error ignoring state.
    * \return void
    */
    static void SetScriptErrorIgnoringState(bool state) noexcept;

    /**
    * \brief Set a rule string for the server details displayed in the server browser.
    *
    * \param key The name of the rule.
    * \param value The string value of the rule.
    * \return void
    */
    static void SetRuleString(const char *key, const char *value) noexcept;

    /**
    * \brief Set a rule value for the server details displayed in the server browser.
    *
    * \param key The name of the rule.
    * \param value The numerical value of the rule.
    * \return void
    */
    static void SetRuleValue(const char *key, double value) noexcept;

    /**
     * \brief Add a data file and a corresponding CRC32 checksum to the data file loadout
     *        that connecting clients need to match.
     *
     * It can be used multiple times to set multiple checksums for the same data file.
     *
     * Note: If an empty string is provided for the checksum, a checksum will not be
     *       required for that data file.
     *
     * @param dataFilename The filename of the data file.
     * @param checksumString A string with the CRC32 checksum required.
     */
    static void AddDataFileRequirement(const char *dataFilename, const char *checksumString) noexcept;

    // All methods below are deprecated versions of methods from above

    static bool DoesFileExist(const char *filePath) noexcept;
    static const char *GetModDir() noexcept;
    static bool GetPluginEnforcementState() noexcept;
    static void SetPluginEnforcementState(bool state) noexcept;
    static void AddPluginHash(const char *pluginName, const char *checksumString) noexcept;
};

#endif //OPENMW_SERVERAPI_HPP
