#include <cstdlib>

#include <components/files/configurationmanager.hpp>
#include <components/files/conversion.hpp>
#include <components/openmw-mp/Utils.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/Version.hpp>

#include <cassert>

#include "../mwbase/environment.hpp"

#include "../mwclass/creature.hpp"
#include "../mwclass/npc.hpp"

#include "../mwdialogue/dialoguemanagerimp.hpp"

#include "../mwgui/windowmanagerimp.hpp"

#include "../mwinput/inputmanagerimp.hpp"

#include "../mwmechanics/aitravel.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "../mwscript/scriptmanagerimp.hpp"

#include "../mwstate/statemanagerimp.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/worldimp.hpp"

#include "Main.hpp"
#include "Networking.hpp"
#include "LocalSystem.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"
#include "PlayerList.hpp"
#include "GUIController.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"
#include "RecordHelper.hpp"
#include "PortableCharacter.hpp"

using namespace mwmp;

Main *Main::pMain = 0;
std::string Main::address = "";
std::string Main::serverPassword = TES3MP_DEFAULT_PASSW;
std::string Main::characterSave = "";
std::string Main::serverCharacter = "";
std::filesystem::path Main::savesPath;

Main::Main()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Multiplayer started");
    mNetworking = new Networking();
    mLocalSystem = new LocalSystem();
    mLocalPlayer = new LocalPlayer();
    mGUIController = new GUIController();
    mCellController = new CellController();

    port = 25565;
}

Main::~Main()
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Multiplayer stopped");
    delete mNetworking;
    delete mLocalSystem;
    delete mLocalPlayer;
    delete mCellController;
    delete mGUIController;
    PlayerList::cleanUp();
}

void Main::optionsDesc(boost::program_options::options_description *desc)
{
    namespace bpo = boost::program_options;
    desc->add_options()
            ("connect", bpo::value<std::string>()->default_value(""),
                        "play multiplayer on this server (e.g. --connect=127.0.0.1:25565)")
            ("password", bpo::value<std::string>()->default_value(TES3MP_DEFAULT_PASSW),
                        "the server's password, if it has one")
            ("character", bpo::value<Files::MaybeQuotedPath>()->default_value(Files::MaybeQuotedPath(), ""),
                        "play multiplayer with this single-player character, from its save (if the server takes them)")
            ("server-character", bpo::value<Files::MaybeQuotedPath>()->default_value(Files::MaybeQuotedPath(), ""),
                        "play a character made on a multiplayer server offline for the first time (its saves folder)");
}

void Main::configure(const boost::program_options::variables_map &variables)
{
    address = variables["connect"].as<std::string>();
    serverPassword = variables["password"].as<std::string>();
    if (serverPassword.empty())
        serverPassword = TES3MP_DEFAULT_PASSW;
    characterSave = Files::pathToUnicodeString(variables["character"].as<Files::MaybeQuotedPath>());
    serverCharacter = Files::pathToUnicodeString(variables["server-character"].as<Files::MaybeQuotedPath>());
    if (isRequested())
        LOG_INIT(TimedLog::LOG_INFO);
}

bool Main::isRequested()
{
    return !address.empty();
}

bool Main::init(const std::vector<std::string> &content, const Files::Collections &collections)
{
    assert(!pMain);
    pMain = new Main();

    size_t delimPos = address.find(':');
    pMain->server = address.substr(0, delimPos);
    if (delimPos != std::string::npos)
        pMain->port = (unsigned short) atoi(address.substr(delimPos + 1).c_str());

    get().mLocalSystem->serverPassword = serverPassword;

    pMain->mNetworking->connect(pMain->server, pMain->port, content, collections);
    return pMain->mNetworking->isConnected();
}

void Main::postInit()
{
    pMain->mGUIController->setupChat();

    const MWBase::Environment &environment = MWBase::Environment::get();
    environment.getStateManager()->newGame(true);
    // AI only runs for the actors this client has authority over (LocalActors), see Actors::update
    if (MWBase::Environment::get().getMechanicsManager()->isAIActive())
        MWBase::Environment::get().getMechanicsManager()->toggleAI();
    // Only the client the server makes a region's authority creates its weather; the others follow it
    MWBase::Environment::get().getWorld()->setWeatherCreationState(false);
    RecordHelper::createPlaceholderInteriorCell();
    // Out of sight until the server places the player (not where a new game starts, which isn't where they are)
    pMain->mLocalPlayer->waitInVoid();
}

bool Main::isInitialized()
{
    return pMain != nullptr;
}

void Main::destroy()
{
    assert(pMain);

    delete pMain;
    pMain = 0;
}

void Main::frame(float dt)
{
    get().getNetworking()->update();

    PlayerList::update(dt);
    get().getCellController()->updateDedicated(dt);
    get().updateWorld(dt);

    get().getGUIController()->update(dt);
}

void Main::updateWorld(float dt) const
{

    if (!mLocalPlayer->processCharGen())
        return;

    static bool init = true;
    if (init)
    {
        init = false;
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Sending ID_PLAYER_BASEINFO to server");

        mNetworking->getPlayerPacket(ID_PLAYER_BASEINFO)->setPlayer(getLocalPlayer());
        mNetworking->getPlayerPacket(ID_LOADED)->setPlayer(getLocalPlayer());
        mNetworking->getPlayerPacket(ID_PLAYER_BASEINFO)->Send();
        mNetworking->getPlayerPacket(ID_LOADED)->Send();
        // A character from single-player: the server asks for it if it takes them
        if (!characterSave.empty())
            PortableCharacter::sendInfo(*mLocalPlayer);
        mLocalPlayer->updateStatsDynamic(true);
        get().getGUIController()->setChatVisible(true);
    }
    else
    {
        mLocalPlayer->update();
        mCellController->updateLocal(false);
        MechanicsHelper::updatePrivateFollowers(dt);
        PortableCharacter::update(*mLocalPlayer, dt);
    }
}

const Main &Main::get()
{
    return *pMain;
}

Networking *Main::getNetworking() const
{
    return mNetworking;
}

LocalSystem *Main::getLocalSystem() const
{
    return mLocalSystem;
}

LocalPlayer *Main::getLocalPlayer() const
{
    return mLocalPlayer;
}

GUIController *Main::getGUIController() const
{
    return mGUIController;
}

CellController *Main::getCellController() const
{
    return mCellController;
}

const std::string &Main::getCharacterSave()
{
    return characterSave;
}

const std::string &Main::getServerCharacter()
{
    return serverCharacter;
}

void Main::setSavesPath(const std::filesystem::path &path)
{
    savesPath = path;
}

const std::filesystem::path &Main::getSavesPath()
{
    return savesPath;
}

bool Main::isValidPacketScript(const std::string &scriptId)
{
    mwmp::BaseWorldstate *worldstate = get().getNetworking()->getWorldstate();

    if (Utils::vectorContains(worldstate->synchronizedClientScriptIds, scriptId))
        return true;

    return false;
}

bool Main::isValidPacketGlobal(const std::string &globalId)
{
    mwmp::BaseWorldstate *worldstate = get().getNetworking()->getWorldstate();

    if (Utils::vectorContains(worldstate->synchronizedClientGlobalIds, globalId))
        return true;

    return false;
}
