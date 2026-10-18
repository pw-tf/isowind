#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>

#include <components/misc/strings/algorithm.hpp>
#include <components/version/version.hpp>

#include <components/openmw-mp/ErrorMessages.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Utils.hpp>
#include <components/openmw-mp/Version.hpp>

#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <RakPeer.h>
#include <RakPeerInterface.h>

#include "Player.hpp"
#include "Networking.hpp"
#include "MasterClient.hpp"
#include "Utils.hpp"
#include "ServerConfig.hpp"

#include <apps/openmw-mp/Script/Script.hpp>

#ifdef ENABLE_BREAKPAD
#include <handler/exception_handler.h>
#endif

using namespace mwmp;

#ifdef ENABLE_BREAKPAD
google_breakpad::ExceptionHandler *pHandler = 0;
#if defined(_WIN32)
bool DumpCallback(const wchar_t* _dump_dir,const wchar_t* _minidump_id,void* context,EXCEPTION_POINTERS* exinfo,MDRawAssertionInfo* assertion,bool success)
#elif defined(__linux)
bool DumpCallback(const google_breakpad::MinidumpDescriptor &md, void *context, bool success)
#endif
{
    // NO STACK USE, NO HEAP USE THERE !!!
    return success;
}

void breakpad(std::string pathToDump)
{
#ifdef _WIN32
    pHandler = new google_breakpad::ExceptionHandler(
            L"crashdumps\\",
            /*FilterCallback*/ 0,
            DumpCallback,
            0,
            google_breakpad::ExceptionHandler::HANDLER_ALL);
#else
    google_breakpad::MinidumpDescriptor md(pathToDump);
    pHandler = new google_breakpad::ExceptionHandler(
            md,
            /*FilterCallback*/ 0,
            DumpCallback,
            /*context*/ 0,
            true,
            -1
    );
#endif
}

void breakpad_close()
{
    delete pHandler;
}
#else
void breakpad(std::string pathToDump){}
void breakpad_close(){}
#endif

// The folder with tes3mp-server-default.cfg: the working directory, else the server's own folder. Its logs and
// tes3mp-server.cfg (your overrides, optional) live there too, so a server folder is self-contained.
std::filesystem::path findServerHome(const char *argv0)
{
    const std::filesystem::path candidates[] = { std::filesystem::current_path(),
        std::filesystem::absolute(std::filesystem::path(argv0)).parent_path() };
    for (const auto &dir : candidates)
        if (std::filesystem::exists(dir / "tes3mp-server-default.cfg"))
            return dir;
    throw std::runtime_error("No default settings file found! Make sure the file \"tes3mp-server-default.cfg\" "
                             "is in the server's folder.");
}

void loadSettings(mwmp::ServerConfig &settings, const std::filesystem::path &home)
{
    settings.load(home / "tes3mp-server-default.cfg");
    if (std::filesystem::exists(home / "tes3mp-server.cfg"))
        settings.load(home / "tes3mp-server.cfg");
}

// Writes everything to two streams: the console and the log file.
class Tee : public std::streambuf
{
public:
    Tee(std::streambuf *first, std::streambuf *second)
        : mFirst(first), mSecond(second)
    {
    }

protected:
    int overflow(int c) override
    {
        if (c == traits_type::eof())
            return traits_type::not_eof(c);
        mFirst->sputc(static_cast<char>(c));
        mSecond->sputc(static_cast<char>(c));
        return c;
    }

    std::streamsize xsputn(const char *str, std::streamsize size) override
    {
        mFirst->sputn(str, size);
        mSecond->sputn(str, size);
        return size;
    }

    int sync() override
    {
        return (mFirst->pubsync() == 0 && mSecond->pubsync() == 0) ? 0 : -1;
    }

private:
    std::streambuf *mFirst;
    std::streambuf *mSecond;
};

int main(int argc, char *argv[])
{
    bool noLogs = false; // --no-logs: useful for daemonizing
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--no-logs") == 0)
            noLogs = true;

    const std::filesystem::path home = findServerHome(argv[0]);
    std::filesystem::current_path(home); // the plugin and data paths in the settings are relative to it

    breakpad((home / "crashdumps").string());

    mwmp::ServerConfig mgr;
    loadSettings(mgr, home);

    std::string commitHash(Version::getCommitHash());

    int logLevel = mgr.getInt("logLevel", "General");
    if (logLevel < TimedLog::LOG_VERBOSE || logLevel > TimedLog::LOG_FATAL)
        logLevel = TimedLog::LOG_VERBOSE;

    // Some objects used to redirect cout and cerr
    // Scope must be here, so this still works inside the catch block for logging exceptions
    std::streambuf* cout_rdbuf = std::cout.rdbuf ();
    std::streambuf* cerr_rdbuf = std::cerr.rdbuf ();

    std::ofstream logfile;
    Tee coutsb(logfile.rdbuf(), cout_rdbuf);
    Tee cerrsb(logfile.rdbuf(), cerr_rdbuf);

    if (!noLogs)
    {
        // Redirect cout and cerr to tes3mp server log

        std::filesystem::create_directories(home / "logs");
        logfile.open(home / "logs" / ("tes3mp-server-" + TimedLog::getFilenameTimestamp() + ".log"));

        std::cout.rdbuf(&coutsb);
        std::cerr.rdbuf(&cerrsb);
    }

    LOG_INIT(logLevel);

    int players = mgr.getInt("maximumPlayers", "General");
    std::string address = mgr.getString("localAddress", "General");
    int port = mgr.getInt("port", "General");

    std::string password = mgr.getString("password", "General");

    std::string pluginHome = mgr.getString("home", "Plugins");
    std::string dataDirectory = Utils::convertPath(pluginHome + "/data");

    std::vector<std::string> plugins(Utils::split(mgr.getString("plugins", "Plugins"), ','));

    std::string versionInfo = Utils::getVersionInfo("TES3MP dedicated server", TES3MP_VERSION, commitHash, TES3MP_PROTO_VERSION);
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "%s", versionInfo.c_str());
    
    Script::SetModDir(dataDirectory);

#ifdef ENABLE_LUA
    LangLua::AddPackagePath(Utils::convertPath(pluginHome + "/scripts/?.lua" + ";"
        + pluginHome + "/lib/lua/?.lua" + ";"));
#ifdef _WIN32
    LangLua::AddPackageCPath(Utils::convertPath(pluginHome + "/lib/?.dll"));
#else
    LangLua::AddPackageCPath(Utils::convertPath(pluginHome + "/lib/?.so"));
#endif

#endif

    int code;

    RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();

    std::stringstream sstr;
    sstr << TES3MP_VERSION;
    sstr << TES3MP_PROTO_VERSION;
    // Remove carriage returns added to version file on Windows
    commitHash.erase(std::remove(commitHash.begin(), commitHash.end(), '\r'), commitHash.end());
    sstr << commitHash;

    peer->SetIncomingPassword(sstr.str().c_str(), (int) sstr.str().size());

    if (RakNet::NonNumericHostString(address.c_str()))
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "You cannot use non-numeric addresses for the server.");
        return 1;
    }

    RakNet::SocketDescriptor sd((unsigned short) port, address.c_str());

    try
    {
        for (auto plugin : plugins)
            Script::LoadScript(plugin.c_str(), pluginHome.c_str());

        switch (peer->Startup((unsigned) players, &sd, 1))
        {
            case RakNet::CRABNET_STARTED:
                break;
            case RakNet::CRABNET_ALREADY_STARTED:
                throw std::runtime_error("Already started");
            case RakNet::INVALID_SOCKET_DESCRIPTORS:
                throw std::runtime_error("Incorrect port or address");
            case RakNet::INVALID_MAX_CONNECTIONS:
                throw std::runtime_error("Max players cannot be negative or 0");
            case RakNet::SOCKET_FAILED_TO_BIND:
            case RakNet::SOCKET_PORT_ALREADY_IN_USE:
            case RakNet::PORT_CANNOT_BE_ZERO:
                throw std::runtime_error("Failed to bind port. Make sure a server isn't already running on that port.");
            case RakNet::SOCKET_FAILED_TEST_SEND:
            case RakNet::SOCKET_FAMILY_NOT_SUPPORTED:
            case RakNet::FAILED_TO_CREATE_NETWORK_THREAD:
            case RakNet::COULD_NOT_GENERATE_GUID:
            case RakNet::STARTUP_OTHER_FAILURE:
                throw std::runtime_error("Cannot start server");
        }

        peer->SetMaximumIncomingConnections((unsigned short) (players));

        Networking networking(peer);
        networking.setServerPassword(password);

        if (mgr.getBool("enabled", "MasterServer"))
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sharing server query info to master enabled.");
            std::string masterAddr = mgr.getString("address", "MasterServer");
            int masterPort = mgr.getInt("port", "MasterServer");
            int updateRate = mgr.getInt("rate", "MasterServer");

            // Is this an attempt to connect to the official master server at the old port? If so,
            // redirect it to the correct port for the currently used fork of RakNet
            if (Misc::StringUtils::ciEqual(masterAddr, "master.tes3mp.com") && masterPort == 25560)
            {
                masterPort = 25561;
                LOG_APPEND(TimedLog::LOG_INFO, "- switching to port %i because the correct official master server for this version is on that port",
                    masterPort);
            }

            if (updateRate < 8000)
            {
                updateRate = 8000;
                LOG_APPEND(TimedLog::LOG_INFO, "- switching to updateRate %i because the one in the server config was too low", updateRate);
            }

            networking.InitQuery(masterAddr, (unsigned short) masterPort);
            networking.getMasterClient()->SetMaxPlayers((unsigned) players);
            networking.getMasterClient()->SetUpdateRate((unsigned) updateRate);
            std::string hostname = mgr.getString("hostname", "General");
            networking.getMasterClient()->SetHostname(hostname);
            networking.getMasterClient()->SetRuleString("CommitHash", commitHash.substr(0, 10));

            networking.getMasterClient()->Start();
        }

        networking.postInit();

        code = networking.mainLoop();

        networking.getMasterClient()->Stop();
    }
    catch (std::exception &e)
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, e.what());
        Script::Call<Script::CallbackIdentity("OnServerScriptCrash")>(e.what());
        throw; //fall through
    }

    RakNet::RakPeerInterface::DestroyInstance(peer);

    if (code == 0)
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Quitting peacefully.");

    LOG_QUIT();

    if (!noLogs)
    {
        // Restore cout and cerr
        std::cout.rdbuf(cout_rdbuf);
        std::cerr.rdbuf(cerr_rdbuf);
    }


    breakpad_close();
    return code;
}
