#include "onscreenkeyboard.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <utility>

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_UString.h>
#include <MyGUI_WidgetManager.h>

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    namespace
    {
        constexpr int sKeyWidth = 36;
        constexpr int sKeyHeight = 30;
        constexpr int sSpacing = 4;
        constexpr int sMargin = 8;
        // The button legend along the bottom of the screen (openmw_controllerbuttons.layout)
        constexpr int sLegendHeight = 48;
        // The left stick deflected this far picks the next key, and let go to this much releases it
        constexpr float sStickOn = 0.55f;
        constexpr float sStickOff = 0.3f;
        constexpr float sRepeatDelay = 0.4f;
        constexpr float sRepeatInterval = 0.1f;

        // Done (Start, or the key) sends Enter to the field, which confirms its dialog; for a field whose window has
        // buttons of its own to do that (brew, buy, enchant: the user string set on it) it only puts the keyboard away
        bool doneCloses(const MyGUI::Widget* target)
        {
            return target && target->getUserString("KeyboardDone") == "Close";
        }

        // MyGUI reads # in a caption as the start of a colour code
        std::string escapeCaption(const std::string& text)
        {
            std::string result;
            for (const char c : text)
            {
                if (c == '#')
                    result += '#';
                result += c;
            }
            return result;
        }
    }

    OnScreenKeyboard::OnScreenKeyboard()
        : WindowBase("openmw_onscreen_keyboard.layout")
    {
        getWidget(mKeysWidget, "Keys");
        buildKeys();
        refreshCaptions();

        mDisableGamepadCursor = true;
        mControllerButtons.mDpad = "#{Interface:Select}";
        mControllerButtons.mA = "#{OMWEngine:KeyboardType}";
        mControllerButtons.mB = "#{OMWEngine:KeyboardBackspace}";
        mControllerButtons.mX = "#{OMWEngine:KeyboardSpace}";
        mControllerButtons.mY = "#{OMWEngine:KeyboardShift}";
        mControllerButtons.mView = "#{Interface:Done}";
        mControllerButtons.mMenu = "#{Interface:Close}";

        MyGUI::WidgetManager::getInstance().registerUnlinker(this);
    }

    OnScreenKeyboard::~OnScreenKeyboard()
    {
        try
        {
            MyGUI::WidgetManager::getInstance().unregisterUnlinker(this);
        }
        catch (const MyGUI::Exception& e)
        {
            Log(Debug::Error) << "Error in the destructor: " << e.what();
        }
    }

    void OnScreenKeyboard::buildKeys()
    {
        // What each key types, plain and with shift, laid out like a US keyboard
        const std::vector<std::vector<std::pair<std::string, std::string>>> rows = {
            { { "`", "~" }, { "1", "!" }, { "2", "@" }, { "3", "#" }, { "4", "$" }, { "5", "%" }, { "6", "^" },
                { "7", "&" }, { "8", "*" }, { "9", "(" }, { "0", ")" }, { "-", "_" }, { "=", "+" } },
            { { "q", "Q" }, { "w", "W" }, { "e", "E" }, { "r", "R" }, { "t", "T" }, { "y", "Y" }, { "u", "U" },
                { "i", "I" }, { "o", "O" }, { "p", "P" }, { "[", "{" }, { "]", "}" }, { "\\", "|" } },
            { { "a", "A" }, { "s", "S" }, { "d", "D" }, { "f", "F" }, { "g", "G" }, { "h", "H" }, { "j", "J" },
                { "k", "K" }, { "l", "L" }, { ";", ":" }, { "'", "\"" } },
            { { "z", "Z" }, { "x", "X" }, { "c", "C" }, { "v", "V" }, { "b", "B" }, { "n", "N" }, { "m", "M" },
                { ",", "<" }, { ".", ">" }, { "/", "?" } },
        };
        // The bottom row, in key widths
        const std::vector<std::pair<Special, float>> specials = { { Special::Shift, 2.5f }, { Special::Space, 5.f },
            { Special::Backspace, 2.5f }, { Special::Done, 2.5f } };

        const auto widthOf = [](float keys) { return static_cast<int>(keys * sKeyWidth + (keys - 1) * sSpacing); };
        const int rowWidth = widthOf(static_cast<float>(rows.front().size()));

        const auto makeButton = [this](int left, int top, int width) {
            MyGUI::Button* button = mKeysWidget->createWidget<MyGUI::Button>(
                "MW_Button", MyGUI::IntCoord(left, top, width, sKeyHeight), MyGUI::Align::Default);
            // Clicking a key mustn't take the keyboard focus from the text box
            button->setNeedKeyFocus(false);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &OnScreenKeyboard::onKeyClicked);
            return button;
        };

        int top = 0;
        for (const auto& rowKeys : rows)
        {
            std::vector<Key> row;
            int left = (rowWidth - widthOf(static_cast<float>(rowKeys.size()))) / 2;
            for (const auto& [lower, upper] : rowKeys)
            {
                Key key;
                key.mLower = lower;
                key.mUpper = upper;
                key.mButton = makeButton(left, top, sKeyWidth);
                row.push_back(std::move(key));
                left += sKeyWidth + sSpacing;
            }
            mRows.push_back(std::move(row));
            top += sKeyHeight + sSpacing;
        }

        int specialsWidth = -sSpacing;
        for (const auto& [special, keys] : specials)
            specialsWidth += widthOf(keys) + sSpacing;
        std::vector<Key> row;
        int left = (rowWidth - specialsWidth) / 2;
        for (const auto& [special, keys] : specials)
        {
            Key key;
            key.mSpecial = special;
            key.mButton = makeButton(left, top, widthOf(keys));
            row.push_back(std::move(key));
            left += widthOf(keys) + sSpacing;
        }
        mRows.push_back(std::move(row));
        const int height = top + sKeyHeight;

        mKeysWidget->setCoord(sMargin, sMargin, rowWidth, height);
        // The dialog's frame around that
        const MyGUI::IntCoord client = mMainWidget->getClientCoord();
        mMainWidget->setSize(rowWidth + 2 * sMargin + mMainWidget->getWidth() - client.width,
            height + 2 * sMargin + mMainWidget->getHeight() - client.height);
    }

    void OnScreenKeyboard::refreshCaptions()
    {
        for (auto& row : mRows)
        {
            for (Key& key : row)
            {
                switch (key.mSpecial)
                {
                    case Special::None:
                        key.mButton->setCaption(escapeCaption(mShift ? key.mUpper : key.mLower));
                        break;
                    case Special::Shift:
                        key.mButton->setCaptionWithReplacing(
                            mShift ? "#{OMWEngine:KeyboardShiftHeld}" : "#{OMWEngine:KeyboardShift}");
                        break;
                    case Special::Space:
                        key.mButton->setCaptionWithReplacing("#{OMWEngine:KeyboardSpace}");
                        break;
                    case Special::Backspace:
                        key.mButton->setCaptionWithReplacing("#{OMWEngine:KeyboardBackspace}");
                        break;
                    case Special::Done:
                        key.mButton->setCaptionWithReplacing("#{Interface:Done}");
                        break;
                }
            }
        }
    }

    void OnScreenKeyboard::open()
    {
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        if (!focus)
            return;
        mTarget = focus;
        mLastTarget = focus;
        mShift = false;
        mStickX = 0;
        mStickY = 0;
        mStickHeld = 0.f;
        mStickRepeating = false;
        refreshCaptions();
        place();
        setVisible(true);
        setFocus(mRow, mColumn);
        // The left stick picks keys now, not a mouse pointer
        MWBase::Environment::get().getInputManager()->setGamepadGuiCursorEnabled(false);
        MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
        windowManager->setCursorActive(false);
        windowManager->updateControllerButtonsOverlay();
    }

    void OnScreenKeyboard::close()
    {
        mTarget = nullptr;
        if (!isVisible())
            return;
        setVisible(false);
        MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
    }

    void OnScreenKeyboard::place()
    {
        const MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        const MyGUI::IntSize size = mMainWidget->getSize();
        const int left = std::max(0, (viewSize.width - size.width) / 2);
        // Along the bottom, above the button legend; at the top instead if that would cover what is typed into
        int top = viewSize.height - sLegendHeight - sMargin - size.height;
        if (mTarget)
        {
            const MyGUI::IntCoord target = mTarget->getAbsoluteCoord();
            if (target.bottom() > top && target.top < top + size.height)
                top = sLegendHeight + sMargin;
        }
        mMainWidget->setPosition(left, std::max(0, top));
    }

    void OnScreenKeyboard::setFocus(int row, int column)
    {
        if (mRows.empty())
            return;
        if (mRow >= 0 && mRow < static_cast<int>(mRows.size()) && mColumn >= 0
            && mColumn < static_cast<int>(mRows[mRow].size()))
            mRows[mRow][mColumn].mButton->setStateSelected(false);
        mRow = std::clamp(row, 0, static_cast<int>(mRows.size()) - 1);
        mColumn = std::clamp(column, 0, static_cast<int>(mRows[mRow].size()) - 1);
        mRows[mRow][mColumn].mButton->setStateSelected(true);
    }

    void OnScreenKeyboard::moveFocus(int rowDelta, int columnDelta)
    {
        if (mRows.empty())
            return;
        const int rowCount = static_cast<int>(mRows.size());
        int row = mRow;
        int column = mColumn;
        if (rowDelta != 0)
        {
            row = ((row + rowDelta) % rowCount + rowCount) % rowCount;
            // The key in the same place along the row: the rows differ in their number of keys
            const MyGUI::Button* current = mRows[mRow][mColumn].mButton;
            const int centre = current->getLeft() + current->getWidth() / 2;
            int best = INT_MAX;
            for (size_t i = 0; i < mRows[row].size(); ++i)
            {
                const MyGUI::Button* button = mRows[row][i].mButton;
                const int distance = std::abs(button->getLeft() + button->getWidth() / 2 - centre);
                if (distance < best)
                {
                    best = distance;
                    column = static_cast<int>(i);
                }
            }
        }
        if (columnDelta != 0)
        {
            const int count = static_cast<int>(mRows[row].size());
            column = ((column + columnDelta) % count + count) % count;
        }
        setFocus(row, column);
    }

    void OnScreenKeyboard::press(const Key& key)
    {
        switch (key.mSpecial)
        {
            case Special::None:
                typeText(mShift ? key.mUpper : key.mLower);
                if (mShift)
                {
                    mShift = false;
                    refreshCaptions();
                }
                break;
            case Special::Shift:
                mShift = !mShift;
                refreshCaptions();
                break;
            case Special::Space:
                typeText(" ");
                break;
            case Special::Backspace:
                sendKey(MyGUI::KeyCode::Backspace);
                break;
            case Special::Done:
                if (doneCloses(mTarget))
                    close();
                else
                    sendKey(MyGUI::KeyCode::Return);
                break;
        }
        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
    }

    void OnScreenKeyboard::typeText(const std::string& utf8)
    {
        if (!mTarget)
            return;
        MyGUI::InputManager& input = MyGUI::InputManager::getInstance();
        if (input.getKeyFocusWidget() != mTarget)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTarget);
        const MyGUI::UString::utf32string text = MyGUI::UString(utf8).asUTF32();
        for (const MyGUI::Char character : text)
            input.injectKeyPress(MyGUI::KeyCode::None, character);
    }

    void OnScreenKeyboard::sendKey(MyGUI::KeyCode key)
    {
        if (!mTarget)
            return;
        MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
        if (MyGUI::InputManager::getInstance().getKeyFocusWidget() != mTarget)
            windowManager->setKeyFocusWidget(mTarget);
        windowManager->injectKeyPress(key, 0, false);
        // Or MyGUI would keep repeating it
        MyGUI::InputManager::getInstance().injectKeyRelease(key);
    }

    void OnScreenKeyboard::updateStick(float dt)
    {
        const MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        const auto direction = [](float value, int current) {
            if (value > sStickOn)
                return 1;
            if (value < -sStickOn)
                return -1;
            if (std::abs(value) < sStickOff)
                return 0;
            return current;
        };
        const int x = direction(input->getControllerAxisValue(SDL_CONTROLLER_AXIS_LEFTX), mStickX);
        const int y = direction(input->getControllerAxisValue(SDL_CONTROLLER_AXIS_LEFTY), mStickY);
        if (x != mStickX || y != mStickY)
        {
            mStickX = x;
            mStickY = y;
            mStickHeld = 0.f;
            mStickRepeating = false;
            if (x != 0 || y != 0)
                moveFocus(y, x);
            return;
        }
        if (x == 0 && y == 0)
            return;
        mStickHeld += dt;
        const float wait = mStickRepeating ? sRepeatInterval : sRepeatDelay;
        if (mStickHeld >= wait)
        {
            mStickHeld -= wait;
            mStickRepeating = true;
            moveFocus(y, x);
        }
    }

    void OnScreenKeyboard::onFrame(float dt)
    {
        if (!isVisible())
            return;
        // Put away once what it typed into is gone, or no longer being typed into
        if (!mTarget || !mTarget->getInheritedVisible()
            || MyGUI::InputManager::getInstance().getKeyFocusWidget() != mTarget)
        {
            close();
            return;
        }
        updateStick(dt);
    }

    void OnScreenKeyboard::onResChange(int /*width*/, int /*height*/)
    {
        if (isVisible())
            place();
    }

    bool OnScreenKeyboard::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        switch (arg.button)
        {
            case SDL_CONTROLLER_BUTTON_A:
                if (!mRows.empty())
                    press(mRows[mRow][mColumn]);
                break;
            case SDL_CONTROLLER_BUTTON_B:
            {
                // Backspace; with nothing left to erase, the keyboard goes away
                MyGUI::EditBox* edit = mTarget ? mTarget->castType<MyGUI::EditBox>(false) : nullptr;
                if (edit && edit->getOnlyText().empty())
                    close();
                else
                    sendKey(MyGUI::KeyCode::Backspace);
                break;
            }
            case SDL_CONTROLLER_BUTTON_X:
                typeText(" ");
                break;
            case SDL_CONTROLLER_BUTTON_Y:
                mShift = !mShift;
                refreshCaptions();
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                moveFocus(-1, 0);
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                moveFocus(1, 0);
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                moveFocus(0, -1);
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                moveFocus(0, 1);
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                sendKey(MyGUI::KeyCode::ArrowLeft);
                break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                sendKey(MyGUI::KeyCode::ArrowRight);
                break;
            case SDL_CONTROLLER_BUTTON_START:
                if (doneCloses(mTarget))
                    close();
                else
                    sendKey(MyGUI::KeyCode::Return);
                break;
            case SDL_CONTROLLER_BUTTON_BACK:
                close();
                break;
            default:
                break;
        }
        // Nothing gets past the keyboard while it is up
        return true;
    }

    bool OnScreenKeyboard::onControllerThumbstickEvent(const SDL_ControllerAxisEvent& /*arg*/)
    {
        // The sticks are read each frame instead (updateStick): small moves back to rest don't arrive here
        return true;
    }

    void OnScreenKeyboard::onKeyClicked(MyGUI::Widget* sender)
    {
        // The click took the keyboard focus from the text box, and so put the keyboard away: back to it
        if (!mTarget && mLastTarget)
        {
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mLastTarget);
            open();
        }
        for (size_t row = 0; row < mRows.size(); ++row)
        {
            for (size_t column = 0; column < mRows[row].size(); ++column)
            {
                if (mRows[row][column].mButton == sender)
                {
                    setFocus(static_cast<int>(row), static_cast<int>(column));
                    press(mRows[row][column]);
                    return;
                }
            }
        }
    }

    void OnScreenKeyboard::_unlinkWidget(MyGUI::Widget* widget)
    {
        if (widget == mTarget)
            mTarget = nullptr;
        if (widget == mLastTarget)
            mLastTarget = nullptr;
    }
}
