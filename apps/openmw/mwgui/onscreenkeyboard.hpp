#ifndef MWGUI_ONSCREENKEYBOARD_H
#define MWGUI_ONSCREENKEYBOARD_H

#include <string>
#include <vector>

#include <MyGUI_IUnlinkWidget.h>

#include "windowbase.hpp"

namespace MyGUI
{
    class Button;
}

namespace MWGui
{
    /// Isowind: a keyboard on the screen, for typing with a controller (a character's name, a server's password, a
    /// save's name, a spell's). X brings it up over the text field the controller selection is on (ControllerManager,
    /// WindowBase::getControllerTextField); it never comes up by itself. It types into the field as a real keyboard
    /// would, so the field and its window notice nothing.
    ///   D-pad / left stick  pick a key          A         type it
    ///   B                   backspace           X         space
    ///   Y                   shift               L1 / R1   move the text cursor
    ///   Start               done (Enter)        Back      put the keyboard away
    /// Done only puts the keyboard away, without Enter, for a field with the user string KeyboardDone = Close (one
    /// reached by selection in a window whose own buttons brew, buy or confirm: alchemy, spellmaking, enchanting,
    /// class creation, map notes); elsewhere Enter confirms the dialog (a name, a password, a save).
    class OnScreenKeyboard : public WindowBase, public MyGUI::IUnlinkWidget
    {
    public:
        OnScreenKeyboard();
        ~OnScreenKeyboard() override;

        /// Comes up for the widget that has the keyboard focus (nothing if none has)
        void open();
        void close();

        /// The widget being typed into
        MyGUI::Widget* getTarget() const { return mTarget; }

        void onFrame(float dt) override;
        void onResChange(int width, int height) override;
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        bool onControllerThumbstickEvent(const SDL_ControllerAxisEvent& arg) override;

        void _unlinkWidget(MyGUI::Widget* widget) override;

    private:
        enum class Special
        {
            None,
            Shift,
            Space,
            Backspace,
            Done
        };

        struct Key
        {
            MyGUI::Button* mButton = nullptr;
            std::string mLower;
            std::string mUpper;
            Special mSpecial = Special::None;
        };

        void buildKeys();
        void refreshCaptions();
        void place();
        void setFocus(int row, int column);
        void moveFocus(int rowDelta, int columnDelta);
        void press(const Key& key);
        void typeText(const std::string& utf8);
        void sendKey(MyGUI::KeyCode key);
        void updateStick(float dt);
        void onKeyClicked(MyGUI::Widget* sender);

        MyGUI::Widget* mTarget = nullptr;
        // What was typed into last: a mouse click on a key takes the focus from it, and it gets it back
        MyGUI::Widget* mLastTarget = nullptr;
        MyGUI::Widget* mKeysWidget = nullptr;
        std::vector<std::vector<Key>> mRows;
        int mRow = 1;
        int mColumn = 0;
        bool mShift = false;

        // The left stick as a D-pad: the direction it is held in, repeating after a delay
        int mStickX = 0;
        int mStickY = 0;
        float mStickHeld = 0.f;
        bool mStickRepeating = false;
    };
}

#endif
