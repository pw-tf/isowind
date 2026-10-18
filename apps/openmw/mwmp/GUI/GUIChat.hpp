#ifndef OPENMW_GUICHAT_HPP
#define OPENMW_GUICHAT_HPP

#include <list>
#include <string>
#include <vector>

#include "apps/openmw/mwgui/windowbase.hpp"

namespace mwmp
{
    class GUIController;
    class GUIChat : public MWGui::WindowBase
    {
        friend class GUIController;
    public:
        enum
        {
            CHAT_DISABLED = 0,
            CHAT_ENABLED,
            CHAT_HIDDENMODE
        } CHAT_WIN_STATE;

        MyGUI::EditBox* mCommandLine;
        MyGUI::EditBox* mHistory;

        typedef std::list<std::string> StringList;

        // History of previous entered commands
        StringList mCommandHistory;
        StringList::iterator mCurrent;
        std::string mEditString;

        GUIChat(int x, int y, int w, int h);

        void pressedChatMode(); //switch chat mode
        void pressedSay(); // switch chat focus (if chat mode != CHAT_DISABLED)
        void setDelay(float newDelay);

        void update(float dt);

        virtual void onOpen();
        virtual void onClose();

        virtual bool exit();

        bool getEditState();

        void setFont(const std::string &fntName);

        void onResChange(int width, int height);

        // Print a message to the console, in specified color.
        void print(const std::string &msg, const std::string& color = "#FFFFFF");

        // Clean chat
        void clean();

        // These are pre-colored versions that you should use.

        /// Output from successful console command
        void printOK(const std::string &msg);

        /// Error message
        void printError(const std::string &msg);

        void send(const std::string &str);

    protected:

    private:

        void keyPress(MyGUI::Widget* _sender,
                      MyGUI::KeyCode key,
                      MyGUI::Char _char);

        void acceptCommand(MyGUI::EditBox* _sender);

        void setEditState(bool state);

        int windowState;
        bool editState;
        float delay;
        float curTime;
    };
}
#endif //OPENMW_GUICHAT_HPP
