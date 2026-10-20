#include <components/openmw-mp/TimedLog.hpp>
#include <components/openmw-mp/Base/BasePlayer.hpp>
#include <components/settings/values.hpp>

#include <SDL_system.h>

#include <algorithm>
#include <cctype>

#include <MyGUI_FactoryManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_RotatingSkin.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_TextIterator.h>

#include <extern/PicoSHA2/picosha2.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwgui/mapwindow.hpp"

#include "../mwworld/worldimp.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/cellstore.hpp"

#include "GUIController.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "GUI/PlayerMarkerCollection.hpp"
#include "GUI/GUIDialogList.hpp"
#include "GUI/GUIChat.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"
#include "PlayerList.hpp"


mwmp::GUIController::GUIController(): mInputBox(0), mListBox(0)
{
    mChat = nullptr;
    keySay = SDL_SCANCODE_Y;
    keyChatMode = SDL_SCANCODE_F2;
}

mwmp::GUIController::~GUIController()
{

}

void mwmp::GUIController::cleanUp()
{
    mPlayerMarkers.clear();
    if (mChat != nullptr)
        delete mChat;
    mChat = nullptr;
}

void mwmp::GUIController::refreshGuiMode(MWGui::GuiMode guiMode)
{
    if (MWBase::Environment::get().getWindowManager()->containsMode(guiMode))
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(guiMode);
        MWBase::Environment::get().getWindowManager()->pushGuiMode(guiMode);
    }
}

void mwmp::GUIController::setupChat()
{
    assert(mChat == nullptr);

    // TES3MP read these from a [Chat] settings section; OpenMW 0.51 only accepts the settings it knows, so they're
    // fixed here for now. Enter to talk, F2 to show or hide the chat window.
    const float chatDelay = 5.0f;
    const int chatX = 0;
    const int chatY = 0;
    const int chatW = 390;
    const int chatH = 250;

    keySay = SDL_SCANCODE_RETURN;
    keyChatMode = SDL_SCANCODE_F2;

    mChat = new GUIChat(chatX, chatY, chatW, chatH);
    mChat->setDelay(chatDelay);
}

void mwmp::GUIController::printChatMessage(std::string &msg)
{
    // Kept without #RRGGBB colour codes or line breaks at the end
    mLastChatMessage.clear();
    for (size_t i = 0; i < msg.size(); ++i)
    {
        if (msg[i] == '#' && i + 6 < msg.size()
            && std::all_of(msg.begin() + i + 1, msg.begin() + i + 7,
                [](char c) { return std::isxdigit(static_cast<unsigned char>(c)) != 0; }))
        {
            i += 6;
            continue;
        }
        mLastChatMessage += msg[i];
    }
    while (!mLastChatMessage.empty() && (mLastChatMessage.back() == '\n' || mLastChatMessage.back() == '\r'))
        mLastChatMessage.pop_back();

    if (mChat != nullptr)
        mChat->print(msg);
}


void mwmp::GUIController::setChatVisible(bool chatVisible)
{
    mChat->setVisible(chatVisible);
}

void mwmp::GUIController::showDialogList(const mwmp::BasePlayer::GUIMessageBox &guiMessageBox)
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    
    if (mListBox != NULL)
    {
        windowManager->removeCurrentModal(mListBox);
        windowManager->removeDialog(std::unique_ptr<MWGui::Layout>(mListBox));
        mListBox = NULL;
    }

    std::vector<std::string> list;

    std::string buf;

    for (const auto &data : guiMessageBox.data)
    {
        if (data == '\n')
        {
            list.push_back(buf);
            buf.erase();
            continue;
        }
        buf += data;
    }

    list.push_back(buf);

    mListBox = new GUIDialogList(guiMessageBox.label, list);
    windowManager->pushGuiMode((MWGui::GuiMode)GM_TES3MP_ListBox);
}

void mwmp::GUIController::showMessageBox(const BasePlayer::GUIMessageBox &guiMessageBox)
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    windowManager->messageBox(guiMessageBox.label);
}

std::vector<std::string> splitString(const std::string &str, char delim = ';')
{
    std::istringstream ss(str);
    std::vector<std::string> result;
    std::string token;
    while (std::getline(ss, token, delim))
        result.push_back(token);
    return result;
}

void mwmp::GUIController::showCustomMessageBox(const BasePlayer::GUIMessageBox &guiMessageBox)
{
    MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
    std::vector<std::string> buttons = splitString(guiMessageBox.buttons);
    windowManager->serverInteractiveMessageBox(guiMessageBox.label, buttons);
}

void mwmp::GUIController::showInputBox(const BasePlayer::GUIMessageBox &guiMessageBox)
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();

    windowManager->removeDialog(std::unique_ptr<MWGui::Layout>(mInputBox));
    windowManager->pushGuiMode((MWGui::GuiMode)GM_TES3MP_InputBox);
    mInputBox = 0;
    mInputBox = new TextInputDialog();

    mInputBox->setEditPassword(guiMessageBox.type == BasePlayer::GUIMessageBox::PasswordDialog);

    mInputBox->setTextLabel(guiMessageBox.label);
    mInputBox->setTextNote(guiMessageBox.note);

    mInputBox->eventDone += MyGUI::newDelegate(this, &GUIController::onInputBoxDone);

    mInputBox->setVisible(true);
}

void mwmp::GUIController::onInputBoxDone(MWGui::WindowBase *parWindow)
{
    LocalPlayer *localPlayer = Main::get().getLocalPlayer();
    std::string textInput = mInputBox->getTextInput();

    // Send input for password dialogs after it's been hashed and rehashed, for some slight
    // extra security that doesn't require the client to keep storing a salt
    if (localPlayer->guiMessageBox.type == BasePlayer::GUIMessageBox::PasswordDialog)
    {
        textInput = picosha2::hash256_hex_string(textInput);
        textInput = picosha2::hash256_hex_string(textInput + picosha2::hash256_hex_string(picosha2::hash256_hex_string((textInput))));
    }

    localPlayer->guiMessageBox.data = textInput;

    PlayerPacket *playerPacket = Main::get().getNetworking()->getPlayerPacket(ID_GUI_MESSAGEBOX);
    playerPacket->setPlayer(Main::get().getLocalPlayer());
    playerPacket->Send();

    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    windowManager->removeDialog(std::unique_ptr<MWGui::Layout>(mInputBox));
    mInputBox = 0;
    windowManager->popGuiMode();
}

bool mwmp::GUIController::pressedKey(int key)
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    if (mChat == nullptr || windowManager->isConsoleMode() || windowManager->getMode() != MWGui::GM_None)
        return false;
    // Isowind: Escape while typing closes the chat's input line, not opens the menu
    if (key == SDL_SCANCODE_ESCAPE && mChat->editState)
    {
        mChat->closeInput();
        return true;
    }
    if (key == keyChatMode)
    {
        mChat->pressedChatMode();
        return true;
    }
    else if (key == keySay)
    {
        // The Enter that just sent a message (or closed the empty input) doesn't open it again
        if (mChat->takeClosedByEnter())
            return true;
        if (mMenuWasOpen)
            return false;
        mChat->pressedSay();
        return true;
    }
    return false;
}

void mwmp::GUIController::changeChatMode()
{
    mChat->pressedChatMode();
}

// No controller button opens the chat for now (D-pad left did); it is opened with the keyboard
bool mwmp::GUIController::openChatWithController()
{
    MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
    if (mChat == nullptr || !Settings::gui().mControllerMenus || windowManager->isGuiMode()
        || windowManager->isConsoleMode())
        return false;

    if (mChat->windowState == GUIChat::CHAT_DISABLED)
        mChat->pressedChatMode();

    // The mode first, so the keyboard on the screen comes up with the chat line (it does only in a menu); the mode
    // ends with the typing (chatInputEnded), with the keyboard put away, or with B
    mChatModeOpen = true;
    mChatKeyboardShown = false;
    windowManager->pushGuiMode(static_cast<MWGui::GuiMode>(GM_TES3MP_Chat));
    mChat->pressedSay();
    return true;
}

void mwmp::GUIController::chatInputEnded()
{
    if (!mChatModeOpen)
        return;
    mChatModeOpen = false;
    mChatKeyboardShown = false;
    MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
    const MWGui::GuiMode chatMode = static_cast<MWGui::GuiMode>(GM_TES3MP_Chat);
    if (windowManager->containsMode(chatMode))
        windowManager->removeGuiMode(chatMode);
}

bool mwmp::GUIController::getChatEditState()
{
    return mChat->editState;
}

void mwmp::GUIController::update(float dt)
{
    if (mChat != nullptr)
        mChat->update(dt);

    MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();

    // Isowind: the chat opened with a controller is over once its mode was left (B), or the keyboard on the
    // screen was put away
    if (mChatModeOpen && mChat != nullptr)
    {
        if (windowManager->isOnScreenKeyboardVisible())
            mChatKeyboardShown = true;
        else if (mChatKeyboardShown || !windowManager->containsMode(static_cast<MWGui::GuiMode>(GM_TES3MP_Chat)))
            mChat->setEditState(false);
    }

    mMenuWasOpen = windowManager->isGuiMode() || windowManager->isConsoleMode();
}

void mwmp::GUIController::processCustomMessageBoxInput(int pressedButton)
{
    LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Pressed: %d", pressedButton);

    LocalPlayer* localPlayer = Main::get().getLocalPlayer();
    localPlayer->guiMessageBox.data = MyGUI::utility::toString(pressedButton);

    PlayerPacket* playerPacket = Main::get().getNetworking()->getPlayerPacket(ID_GUI_MESSAGEBOX);
    playerPacket->setPlayer(Main::get().getLocalPlayer());
    playerPacket->Send();
}

void mwmp::GUIController::WM_UpdateVisible(MWGui::GuiMode mode)
{
    switch((int)mode)
    {
        case GM_TES3MP_InputBox:
        {
            if (mInputBox != 0)
                mInputBox->setVisible(true);
            break;
        }
        case GM_TES3MP_ListBox:
        {
            if (mListBox != 0)
                mListBox->setVisible(true);
            break;
        }
        default:
            break;
    }
}

class MarkerWidget: public MyGUI::Widget
{
MYGUI_RTTI_DERIVED(MarkerWidget)

public:
    void setNormalColour(const MyGUI::Colour& colour)
    {
        mNormalColour = colour;
        setColour(colour);
    }

    void setHoverColour(const MyGUI::Colour& colour)
    {
        mHoverColour = colour;
    }

private:
    MyGUI::Colour mNormalColour;
    MyGUI::Colour mHoverColour;

    void onMouseLostFocus(MyGUI::Widget* _new)
    {
        setColour(mNormalColour);
    }

    void onMouseSetFocus(MyGUI::Widget* _old)
    {
        setColour(mHoverColour);
    }
};

ESM::CustomMarker mwmp::GUIController::createMarker(const RakNet::RakNetGUID &guid)
{
    DedicatedPlayer *player = PlayerList::getPlayer(guid);
    ESM::CustomMarker mEditingMarker;
    if (!player)
    {
        LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Unknown player guid: %s", guid.ToString());
        return mEditingMarker;
    }

    mEditingMarker.mNote = player->npc.mName;

    const ESM::Cell &playerCell = player->cell;
    mEditingMarker.mCell = playerCell.isExterior()
        ? ESM::RefId::esm3ExteriorCell(playerCell.getGridX(), playerCell.getGridY())
        : ESM::Cell::generateIdForCell(false, playerCell.mName, 0, 0);

    mEditingMarker.mWorldX = player->position.pos[0];
    mEditingMarker.mWorldY = player->position.pos[1];

    return mEditingMarker;
}


void mwmp::GUIController::updatePlayersMarkers(MWGui::LocalMapBase *localMapBase)
{
    // TODO: draw other players' markers on the local map (TES3MP used the map's internals, rewritten in 0.51)
}

void mwmp::GUIController::setGlobalMapMarkerTooltip(MWGui::MapWindow *mapWindow, MyGUI::Widget *markerWidget, int x, int y)
{
    // TODO: list other players in the world map's tooltips (see updatePlayersMarkers)
}

void mwmp::GUIController::updateGlobalMapMarkerTooltips(MWGui::MapWindow *mapWindow)
{
    // TODO: see setGlobalMapMarkerTooltip
}
