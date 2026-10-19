#ifndef OPENMW_GUICONTROLLER_HPP
#define OPENMW_GUICONTROLLER_HPP

#include <components/settings/settings.hpp>

#include "apps/openmw/mwgui/mode.hpp"

#include <components/openmw-mp/Base/BasePlayer.hpp>
#include "GUI/PlayerMarkerCollection.hpp"
#include "GUI/TextInputDialog.hpp"

namespace MWGui
{
    class LocalMapBase;
    class MapWindow;
}

namespace mwmp
{
    class GUIDialogList;
    class GUIChat;
    class GUIController
    {
    public:
        enum GM
        {
            GM_VR_MetaMenu = MWGui::GM_QuickKeysMenu + 1, // Put this dummy GuiMode here because it's used in VR
            GM_TES3MP_InputBox,
            GM_TES3MP_ListBox,
            // Isowind: the chat opened with a controller (openChatWithController)
            GM_TES3MP_Chat
        };
        GUIController();
        ~GUIController();
        void cleanUp();

        void refreshGuiMode(MWGui::GuiMode guiMode);

        void setupChat();

        void printChatMessage(std::string &msg);
        /// The last chat line, without colour codes (the server explains a kick that way)
        const std::string& getLastChatMessage() const { return mLastChatMessage; }
        void setChatVisible(bool chatVisible);

        void showMessageBox(const BasePlayer::GUIMessageBox &guiMessageBox);
        void showCustomMessageBox(const BasePlayer::GUIMessageBox &guiMessageBox);
        void showInputBox(const BasePlayer::GUIMessageBox &guiMessageBox);

        void showDialogList(const BasePlayer::GUIMessageBox &guiMessageBox);

        /// Returns 0 if there was no events
        bool pressedKey(int key);

        void changeChatMode();

        /// Isowind: a controller opens the chat line, in a GUI mode of its own so that the sticks and buttons work the
        /// chat (and the keyboard on the screen) and not the character. False if there is no chat to open.
        bool openChatWithController();
        /// Isowind: the chat line closed (the message sent, or nothing to send); leaves that mode
        void chatInputEnded();

        bool getChatEditState();

        void update(float dt);

        void processCustomMessageBoxInput(int pressedButton);

        void WM_UpdateVisible(MWGui::GuiMode mode);

        void updatePlayersMarkers(MWGui::LocalMapBase *localMapBase);
        void updateGlobalMapMarkerTooltips(MWGui::MapWindow *pWindow);

        ESM::CustomMarker createMarker(const RakNet::RakNetGUID &guid);
        PlayerMarkerCollection mPlayerMarkers;
    private:
        void setGlobalMapMarkerTooltip(MWGui::MapWindow *mapWindow ,MyGUI::Widget* markerWidget, int x, int y);

    private:
        std::string mLastChatMessage;
        GUIChat *mChat;
        int keySay;
        // Whether a menu was open in the last frame: the Enter that closes a menu (the password box at login, a
        // message box) mustn't also start typing in the chat
        bool mMenuWasOpen = true;
        // Isowind: the chat is open in its own GUI mode (openChatWithController), and the keyboard on the screen
        // has come up for it
        bool mChatModeOpen = false;
        bool mChatKeyboardShown = false;
        int keyChatMode;

        long id;
        TextInputDialog *mInputBox;
        GUIDialogList *mListBox;
        void onInputBoxDone(MWGui::WindowBase* parWindow);
        //MyGUI::Widget *oldFocusWidget, *currentFocusWidget;
    };
}

#endif //OPENMW_GUICONTROLLER_HPP
