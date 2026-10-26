#include "controllermanager.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>

#include <SDL.h>

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>
#include <components/files/conversion.hpp>
#include <components/sdlutil/sdlmappings.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwgui/windowbase.hpp"

#include "actions.hpp"
#include "bindingsmanager.hpp"
#include "mousemanager.hpp"

namespace MWInput
{
    namespace
    {
        // Isowind: a D-pad direction held in a menu steps again after this long, then this often (as the left stick
        // does on the keyboard on the screen). Real time: the game may be paused under a menu.
        constexpr std::chrono::milliseconds sGuiRepeatDelay(400);
        constexpr std::chrono::milliseconds sGuiRepeatInterval(100);

        // Isowind: how far a stick or trigger has to move (of 32767) to make its controller the one read, so a resting
        // stick's drift doesn't
        constexpr int sControllerUsedAxisValue = 8000;

        bool isDpadButton(Uint8 button)
        {
            return button == SDL_CONTROLLER_BUTTON_DPAD_UP || button == SDL_CONTROLLER_BUTTON_DPAD_DOWN
                || button == SDL_CONTROLLER_BUTTON_DPAD_LEFT || button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        }
    }

    ControllerManager::ControllerManager(BindingsManager* bindingsManager, MouseManager* mouseManager,
        const std::filesystem::path& userControllerBindingsFile, const std::filesystem::path& controllerBindingsFile)
        : mBindingsManager(bindingsManager)
        , mMouseManager(mouseManager)
        , mGyroAvailable(false)
        , mGamepadGuiCursorEnabled(true)
        , mGuiCursorEnabled(true)
        , mJoystickLastUsed(false)
        , mGamepadMousePressed(false)
        , mLeftTriggerGuiPressed(false)
        , mRightTriggerGuiPressed(false)
    {
        if (!controllerBindingsFile.empty())
        {
            const int result
                = SDL_GameControllerAddMappingsFromFile(Files::pathToUnicodeString(controllerBindingsFile).c_str());
            if (result < 0)
                Log(Debug::Error) << "Failed to add game controller mappings from file \"" << controllerBindingsFile
                                  << "\": " << SDL_GetError();
        }

        if (!userControllerBindingsFile.empty())
        {
            const int result
                = SDL_GameControllerAddMappingsFromFile(Files::pathToUnicodeString(userControllerBindingsFile).c_str());
            if (result < 0)
                Log(Debug::Error) << "Failed to add game controller mappings from user file \""
                                  << userControllerBindingsFile << "\": " << SDL_GetError();
        }

        // Open all presently connected sticks
        const int numSticks = SDL_NumJoysticks();
        if (numSticks < 0)
            Log(Debug::Error) << "Failed to get number of joysticks: " << SDL_GetError();

        for (int i = 0; i < numSticks; i++)
        {
            if (SDL_IsGameController(i))
            {
                SDL_ControllerDeviceEvent evt;
                evt.which = i;
                static const int fakeDeviceID = 1;
                ControllerManager::controllerAdded(fakeDeviceID, evt);
                if (const char* name = SDL_GameControllerNameForIndex(i))
                    Log(Debug::Info) << "Detected game controller: " << name;
                else
                    Log(Debug::Warning) << "Detected game controller without a name: " << SDL_GetError();
            }
            else
            {
                if (const char* name = SDL_JoystickNameForIndex(i))
                    Log(Debug::Info) << "Detected unusable controller: " << name;
                else
                    Log(Debug::Warning) << "Detected unusable controller without a name: " << SDL_GetError();
            }
        }

        mBindingsManager->setJoystickDeadZone(Settings::input().mJoystickDeadZone);
    }

    void ControllerManager::update(float dt)
    {
        if (mGuiCursorEnabled && !(mJoystickLastUsed && !mGamepadGuiCursorEnabled))
        {
            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight) * 2.0f - 1.0f;
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward) * 2.0f - 1.0f;
            float zAxis = mBindingsManager->getActionValue(A_LookUpDown) * 2.0f - 1.0f;

            xAxis *= (1.5f - mBindingsManager->getActionValue(A_Use));
            yAxis *= (1.5f - mBindingsManager->getActionValue(A_Use));

            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            float uiScale = MWBase::Environment::get().getWindowManager()->getScalingFactor();
            const float gamepadCursorSpeed = Settings::input().mGamepadCursorSpeed
                * MWBase::Environment::get().getWindowManager()->getControllerCursorSpeedScale();
            const float xMove = xAxis * dt * 1500.0f / uiScale * gamepadCursorSpeed;
            const float yMove = yAxis * dt * 1500.0f / uiScale * gamepadCursorSpeed;

            float mouseWheelMove = -zAxis * dt * 1500.0f;
            if (xMove != 0 || yMove != 0 || mouseWheelMove != 0)
            {
                mMouseManager->injectMouseMove(xMove, yMove, mouseWheelMove);
                mMouseManager->warpMouse();
                MWBase::Environment::get().getWindowManager()->setCursorActive(true);
            }
        }

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode()
            && MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_Running
            && MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight);
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward);
            if (xAxis != 0.5 || yAxis != 0.5)
            {
                mJoystickLastUsed = true;
                MWBase::Environment::get().getInputManager()->resetIdleTime();
            }
        }

        updateGuiRepeat();
    }

    void ControllerManager::buttonPressed(int deviceID, const SDL_ControllerButtonEvent& arg)
    {
        // Isowind: another button pressed stops a D-pad direction repeating
        mGuiRepeatButton = SDL_CONTROLLER_BUTTON_INVALID;
        useController(arg.which);
        // Isowind: the menus become the ones made for a controller (when they can: see WindowManager)
        if (!Settings::gui().mControllerMenus)
            Log(Debug::Verbose) << "Controller used: button " << static_cast<int>(arg.button);
        MWBase::Environment::get().getWindowManager()->onInputDeviceUsed(true);

        if (!Settings::input().mEnableController || mBindingsManager->isDetectingBindingState())
            return;

        MWBase::Environment::get().getLuaManager()->inputEvent(
            { MWBase::LuaManager::InputEvent::ControllerPressed, arg.button });

        mJoystickLastUsed = true;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (gamepadToGuiControl(arg))
            {
                startGuiRepeat(arg);
                return;
            }

            if (mGamepadGuiCursorEnabled)
            {
                // Temporary mouse binding until keyboard controls are available:
                if (arg.button == SDL_CONTROLLER_BUTTON_A) // We'll pretend that A is left click.
                {
                    bool mousePressSuccess = mMouseManager->injectMouseButtonPress(SDL_BUTTON_LEFT);
                    mGamepadMousePressed = true;
                    if (MyGUI::InputManager::getInstance().getMouseFocusWidget())
                    {
                        MyGUI::Button* b
                            = MyGUI::InputManager::getInstance().getMouseFocusWidget()->castType<MyGUI::Button>(false);
                        if (b && b->getEnabled())
                            MWBase::Environment::get().getWindowManager()->playSound(
                                ESM::RefId::stringRefId("Menu Click"));
                    }

                    mBindingsManager->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            mBindingsManager->setPlayerControlsEnabled(true);

        // esc, to leave initial movie screen
        auto kc = SDLUtil::sdlKeyToMyGUI(SDLK_ESCAPE);
        mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyPress(kc, 0));

        if (!MWBase::Environment::get().getInputManager()->controlsDisabled())
            mBindingsManager->controllerButtonPressed(deviceID, arg);
    }

    void ControllerManager::buttonReleased(int deviceID, const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == mGuiRepeatButton)
            mGuiRepeatButton = SDL_CONTROLLER_BUTTON_INVALID;

        if (mBindingsManager->isDetectingBindingState())
        {
            mBindingsManager->controllerButtonReleased(deviceID, arg);
            return;
        }

        if (Settings::input().mEnableController)
        {
            MWBase::Environment::get().getLuaManager()->inputEvent(
                { MWBase::LuaManager::InputEvent::ControllerReleased, arg.button });
        }

        if (!Settings::input().mEnableController || MWBase::Environment::get().getInputManager()->controlsDisabled())
            return;

        mJoystickLastUsed = true;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (mGamepadGuiCursorEnabled && (!Settings::gui().mControllerMenus || mGamepadMousePressed))
            {
                // Temporary mouse binding until keyboard controls are available:
                if (arg.button == SDL_CONTROLLER_BUTTON_A) // We'll pretend that A is left click.
                {
                    bool mousePressSuccess = mMouseManager->injectMouseButtonRelease(SDL_BUTTON_LEFT);
                    mGamepadMousePressed = false;
                    if (mBindingsManager->isDetectingBindingState()) // If the player just triggered binding, don't let
                                                                     // button release bind.
                        return;

                    mBindingsManager->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            mBindingsManager->setPlayerControlsEnabled(true);

        // esc, to leave initial movie screen
        auto kc = SDLUtil::sdlKeyToMyGUI(SDLK_ESCAPE);
        mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(kc));

        mBindingsManager->controllerButtonReleased(deviceID, arg);
    }

    void ControllerManager::axisMoved(int deviceID, const SDL_ControllerAxisEvent& arg)
    {
        if (std::abs(arg.value) > sControllerUsedAxisValue)
        {
            useController(arg.which);
            if (!Settings::gui().mControllerMenus)
                Log(Debug::Verbose) << "Controller used: axis " << static_cast<int>(arg.axis) << " at " << arg.value;
            MWBase::Environment::get().getWindowManager()->onInputDeviceUsed(true);
        }

        if (mBindingsManager->isDetectingBindingState())
        {
            // Isowind: a trigger pulled to be bound is bound partway down; the rest of the pull isn't then a press in
            // the menu (it would switch Options' tab under the bindings)
            if (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
                mLeftTriggerGuiPressed = arg.value > 0;
            else if (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                mRightTriggerGuiPressed = arg.value > 0;
            mBindingsManager->controllerAxisMoved(deviceID, arg);
            return;
        }

        // Isowind: another controller's sticks say nothing (a stream's virtual pad left behind, resting off centre)
        // until one is pushed far enough to make it the one read
        const SDL_JoystickID used = mBindingsManager->getControllerUsed();
        if (used != -1 && arg.which != used)
            return;

        if (!Settings::input().mEnableController || MWBase::Environment::get().getInputManager()->controlsDisabled())
            return;

        mJoystickLastUsed = true;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (gamepadToGuiControl(arg))
                return;
        }
        else if (mBindingsManager->actionIsActive(A_TogglePOV)
            && (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT || arg.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT))
        {
            // Preview Mode Gamepad Zooming; do not propagate to mBindingsManager
            return;
        }
        mBindingsManager->controllerAxisMoved(deviceID, arg);
    }

    void ControllerManager::controllerAdded(int deviceID, const SDL_ControllerDeviceEvent& arg)
    {
        mBindingsManager->controllerAdded(deviceID, arg);
        enableGyroSensor();
    }

    void ControllerManager::controllerRemoved(const SDL_ControllerDeviceEvent& arg)
    {
        mBindingsManager->controllerRemoved(arg);
    }

    bool ControllerManager::gamepadToGuiControl(const SDL_ControllerButtonEvent& arg, bool repeat)
    {
        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();

        if (Settings::gui().mControllerMenus)
        {
            // Update cursor state.
            bool treatAsMouse = winMgr->getCursorVisible();
            winMgr->setCursorActive(false);

            MWGui::WindowBase* topWin = winMgr->getActiveControllerWindow();
            if (topWin && topWin->isVisible())
            {
                // When the inventory tooltip is visible, we don't actually want the A button to
                // act like a mouse button; it should act normally.
                if (treatAsMouse && arg.button == SDL_CONTROLLER_BUTTON_A && winMgr->getControllerTooltipVisible())
                    treatAsMouse = false;

                mGamepadGuiCursorEnabled = topWin->isGamepadCursorAllowed();

                // Fall through to mouse click
                if (mGamepadGuiCursorEnabled && treatAsMouse && arg.button == SDL_CONTROLLER_BUTTON_A
                    && !topWin->handlesPointerSelect())
                    return false;

                // Isowind: in menus with several windows side by side the bumpers switch between them
                if (winMgr->controllerCyclesWindows()
                    && (arg.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER
                        || arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
                {
                    winMgr->cycleActiveControllerWindow(arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
                    return true;
                }

                // Isowind: X brings the keyboard on the screen up for the text field the selection is on (instead of
                // what X does in that window); it never comes up by itself
                if (arg.button == SDL_CONTROLLER_BUTTON_X && !winMgr->isOnScreenKeyboardVisible())
                {
                    if (MyGUI::Widget* textField = topWin->getControllerTextField())
                    {
                        if (MyGUI::InputManager::getInstance().getKeyFocusWidget() != textField)
                            winMgr->setKeyFocusWidget(textField);
                        winMgr->openOnScreenKeyboard();
                        if (winMgr->isOnScreenKeyboardVisible())
                        {
                            winMgr->playSound(ESM::RefId::stringRefId("Menu Click"));
                            return true;
                        }
                    }
                }

                if (topWin->onControllerButtonEvent(arg))
                    return true;
            }
        }

        // Presumption of GUI mode will be removed in the future.
        // MyGUI KeyCodes *may* change.
        MyGUI::KeyCode key = MyGUI::KeyCode::None;
        switch (arg.button)
        {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                key = MyGUI::KeyCode::ArrowUp;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                key = MyGUI::KeyCode::ArrowRight;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                key = MyGUI::KeyCode::ArrowDown;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                key = MyGUI::KeyCode::ArrowLeft;
                break;
            case SDL_CONTROLLER_BUTTON_A:
                // If we are using the joystick as a GUI mouse, A must be handled via mouse.
                if (mGamepadGuiCursorEnabled)
                    return false;
                key = MyGUI::KeyCode::Space;
                break;
            case SDL_CONTROLLER_BUTTON_B:
                if (MyGUI::InputManager::getInstance().isModalAny())
                    winMgr->exitCurrentModal();
                else
                    winMgr->exitCurrentGuiMode();
                return true;
            case SDL_CONTROLLER_BUTTON_X:
                key = MyGUI::KeyCode::Semicolon;
                break;
            case SDL_CONTROLLER_BUTTON_Y:
                key = MyGUI::KeyCode::Apostrophe;
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::LeftShift);
                winMgr->injectKeyPress(MyGUI::KeyCode::Tab, 0, false);
                MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::LeftShift);
                return true;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Tab, 0, false);
                return true;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                mGamepadGuiCursorEnabled = !mGamepadGuiCursorEnabled;
                winMgr->setCursorActive(mGamepadGuiCursorEnabled);
                return true;
            default:
                return false;
        }

        // Some keys will work even when Text Input windows/modals are in focus.
        if (SDL_IsTextInputActive())
            return false;

        winMgr->injectKeyPress(key, 0, repeat);
        return true;
    }

    void ControllerManager::startGuiRepeat(const SDL_ControllerButtonEvent& arg)
    {
        if (!isDpadButton(arg.button))
            return;

        // MyGUI repeats a key it is given until a key is let go (the arrow keys the D-pad stands for, which also set
        // the count of items to take or buy): the D-pad repeats here instead, at one pace in every menu
        MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::None);

        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
        if (!winMgr->isGuiMode())
            return;
        mGuiRepeatButton = arg.button;
        mGuiRepeatController = arg.which;
        mGuiRepeatWindow = winMgr->getActiveControllerWindow();
        mGuiRepeatNext = std::chrono::steady_clock::now() + sGuiRepeatDelay;
    }

    void ControllerManager::updateGuiRepeat()
    {
        if (mGuiRepeatButton == SDL_CONTROLLER_BUTTON_INVALID)
            return;

        // Over once the direction is let go (its release can be missed), the menu is closed or another window has
        // the controller (the keyboard on the screen has come up, a message box)
        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
        SDL_GameController* controller = SDL_GameControllerFromInstanceID(mGuiRepeatController);
        if (!controller
            || !SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(mGuiRepeatButton))
            || !Settings::input().mEnableController || mBindingsManager->isDetectingBindingState()
            || !winMgr->isGuiMode() || winMgr->getActiveControllerWindow() != mGuiRepeatWindow)
        {
            mGuiRepeatButton = SDL_CONTROLLER_BUTTON_INVALID;
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now < mGuiRepeatNext)
            return;
        // From now, not from when it was due: a slow frame doesn't bring a burst of steps
        mGuiRepeatNext = now + sGuiRepeatInterval;

        // To the menu only, not to scripts or the key bindings (D-pad up and down zoom the camera)
        SDL_ControllerButtonEvent event{};
        event.type = SDL_CONTROLLERBUTTONDOWN;
        event.which = mGuiRepeatController;
        event.button = static_cast<Uint8>(mGuiRepeatButton);
        event.state = SDL_PRESSED;
        if (gamepadToGuiControl(event, true))
            MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::None);
        else
            mGuiRepeatButton = SDL_CONTROLLER_BUTTON_INVALID;
    }

    bool ControllerManager::gamepadToGuiControl(const SDL_ControllerAxisEvent& arg)
    {
        const int triggerPressThreshold = Settings::gui().mControllerTriggerPressThreshold;
        const int rawTriggerReleaseThreshold = Settings::gui().mControllerTriggerReleaseThreshold;
        const int triggerReleaseThreshold = std::clamp(rawTriggerReleaseThreshold, 0, triggerPressThreshold - 1);

        auto handleTriggerPress = [&](Sint16 value, bool& triggerGuiPressed, const auto& onPress) {
            if (value >= triggerPressThreshold && !triggerGuiPressed)
            {
                onPress();
                triggerGuiPressed = true;
            }
            else if (value <= triggerReleaseThreshold)
            {
                triggerGuiPressed = false;
            }
        };

        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();

        if (Settings::gui().mControllerMenus)
        {
            // Left and right triggers toggle through open GUI windows. Isowind: where the bumpers do that (several
            // windows side by side), the triggers instead do what the bumpers do in the active window; and not while
            // the keyboard on the screen is up, they would switch pages under it.
            const auto triggerPressed = [&](bool right) {
                if (winMgr->isOnScreenKeyboardVisible())
                    return;
                MWGui::WindowBase* activeWin = winMgr->getActiveControllerWindow();
                // A window may have its own use for them (Options: the tabs inside the page shown)
                if (activeWin && activeWin->isVisible() && activeWin->onControllerTrigger(right ? 1 : -1))
                    return;
                if (winMgr->controllerCyclesWindows() && activeWin && activeWin->isVisible())
                {
                    SDL_ControllerButtonEvent bumper{};
                    bumper.type = SDL_CONTROLLERBUTTONDOWN;
                    bumper.state = SDL_PRESSED;
                    bumper.button = right ? SDL_CONTROLLER_BUTTON_RIGHTSHOULDER : SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
                    activeWin->onControllerButtonEvent(bumper);
                }
                else
                    winMgr->cycleActiveControllerWindow(right);
            };
            if (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
            {
                handleTriggerPress(arg.value, mRightTriggerGuiPressed, [&] { triggerPressed(true); });
                return true;
            }
            else if (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
            {
                handleTriggerPress(arg.value, mLeftTriggerGuiPressed, [&] { triggerPressed(false); });
                return true;
            }

            MWGui::WindowBase* topWin = winMgr->getActiveControllerWindow();
            if (topWin && topWin->isVisible())
            {
                // Update cursor state
                mGamepadGuiCursorEnabled = topWin->isGamepadCursorAllowed();
                if (!mGamepadGuiCursorEnabled)
                    winMgr->setCursorActive(false);

                // Deadzone check
                if (std::abs(arg.value) < 2000)
                    return !mGamepadGuiCursorEnabled;

                if (mGamepadGuiCursorEnabled
                    && (arg.axis == SDL_CONTROLLER_AXIS_LEFTX || arg.axis == SDL_CONTROLLER_AXIS_LEFTY))
                {
                    // Treat the left stick like a cursor, which is the default behavior.
                    winMgr->setControllerTooltipVisible(false);
                    winMgr->setCursorVisible(true);
                    return false;
                }

                // Some windows have a specific widget to scroll with the right stick. Move the mouse there.
                if (arg.axis == SDL_CONTROLLER_AXIS_RIGHTY && topWin->getControllerScrollWidget() != nullptr)
                {
                    mMouseManager->warpMouseToWidget(topWin->getControllerScrollWidget());
                    winMgr->setCursorVisible(false);
                }

                if (topWin->onControllerThumbstickEvent(arg))
                {
                    // Window handled the event.
                    return true;
                }
                else if (arg.axis == SDL_CONTROLLER_AXIS_RIGHTX || arg.axis == SDL_CONTROLLER_AXIS_RIGHTY)
                {
                    // Only right-stick scroll if mouse is visible or there's a widget to scroll.
                    return !winMgr->getCursorVisible() && topWin->getControllerScrollWidget() == nullptr;
                }
            }
        }

        switch (arg.axis)
        {
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                handleTriggerPress(arg.value, mRightTriggerGuiPressed,
                    [&] { winMgr->injectKeyPress(MyGUI::KeyCode::Minus, 0, false); });
                break;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                handleTriggerPress(arg.value, mLeftTriggerGuiPressed,
                    [&] { winMgr->injectKeyPress(MyGUI::KeyCode::Equals, 0, false); });
                break;
            case SDL_CONTROLLER_AXIS_LEFTX:
            case SDL_CONTROLLER_AXIS_LEFTY:
            case SDL_CONTROLLER_AXIS_RIGHTX:
            case SDL_CONTROLLER_AXIS_RIGHTY:
                // If we are using the joystick as a GUI mouse, process mouse movement elsewhere.
                if (mGamepadGuiCursorEnabled)
                    return false;
                break;
            default:
                return false;
        }

        return true;
    }

    float ControllerManager::getAxisValue(SDL_GameControllerAxis axis) const
    {
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        constexpr float axisMaxAbsoluteValue = 32768;
        if (cntrl != nullptr)
            return SDL_GameControllerGetAxis(cntrl, axis) / axisMaxAbsoluteValue;
        return 0;
    }

    bool ControllerManager::isButtonPressed(SDL_GameControllerButton button) const
    {
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        if (cntrl)
            return SDL_GameControllerGetButton(cntrl, button) > 0;
        else
            return false;
    }

    void ControllerManager::enableGyroSensor()
    {
        mGyroAvailable = false;
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        if (!cntrl)
            return;
        if (!SDL_GameControllerHasSensor(cntrl, SDL_SENSOR_GYRO))
            return;
        if (const int result = SDL_GameControllerSetSensorEnabled(cntrl, SDL_SENSOR_GYRO, SDL_TRUE); result < 0)
        {
            Log(Debug::Error) << "Failed to enable game controller sensor: " << SDL_GetError();
            return;
        }
        mGyroAvailable = true;
    }

    void ControllerManager::useController(SDL_JoystickID instanceID)
    {
        if (mBindingsManager->getControllerUsed() == instanceID)
            return;
        // Whatever the one read before last said is over (a stream's pad that went quiet with a stick pushed would
        // otherwise push for good, the pointer drifting to the screen's edge)
        mBindingsManager->resetControllerControls();
        mBindingsManager->setControllerUsed(instanceID);
        enableGyroSensor();
    }

    bool ControllerManager::isGyroAvailable() const
    {
        return mGyroAvailable;
    }

    std::array<float, 3> ControllerManager::getGyroValues() const
    {
        float gyro[3] = { 0.f };
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        if (cntrl && mGyroAvailable)
        {
            const int result = SDL_GameControllerGetSensorData(cntrl, SDL_SENSOR_GYRO, gyro, 3);
            if (result < 0)
                Log(Debug::Error) << "Failed to get game controller sensor data: " << SDL_GetError();
        }
        return std::array<float, 3>({ gyro[0], gyro[1], gyro[2] });
    }

    int ControllerManager::getControllerType()
    {
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        if (cntrl)
            return SDL_GameControllerGetType(cntrl);
        return 0;
    }

    std::string ControllerManager::getControllerButtonIcon(int button)
    {
        int controllerType = ControllerManager::getControllerType();

        bool isXbox = controllerType == SDL_CONTROLLER_TYPE_XBOX360 || controllerType == SDL_CONTROLLER_TYPE_XBOXONE;
        bool isPsx = controllerType == SDL_CONTROLLER_TYPE_PS3 || controllerType == SDL_CONTROLLER_TYPE_PS4
            || controllerType == SDL_CONTROLLER_TYPE_PS5;
        bool isSwitch = controllerType == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;

        switch (button)
        {
            case SDL_CONTROLLER_BUTTON_A:
                if (isPsx)
                    return "textures/omw_psx_button_x.dds";
                return "textures/omw_steam_button_a.dds";
            case SDL_CONTROLLER_BUTTON_B:
                if (isPsx)
                    return "textures/omw_psx_button_circle.dds";
                return "textures/omw_steam_button_b.dds";
            case SDL_CONTROLLER_BUTTON_BACK:
                return "textures/omw_steam_button_view.dds";
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                if (isPsx)
                    return "textures/omw_psx_button_dpad.dds";
                return "textures/omw_steam_button_dpad.dds";
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                if (isXbox)
                    return "textures/omw_xbox_button_lb.dds";
                else if (isSwitch)
                    return "textures/omw_switch_button_l.dds";
                return "textures/omw_steam_button_l1.dds";
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                return "textures/omw_steam_button_l3.dds";
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                if (isXbox)
                    return "textures/omw_xbox_button_rb.dds";
                else if (isSwitch)
                    return "textures/omw_switch_button_r.dds";
                return "textures/omw_steam_button_r1.dds";
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                return "textures/omw_steam_button_r3.dds";
            case SDL_CONTROLLER_BUTTON_START:
                return "textures/omw_steam_button_menu.dds";
            case SDL_CONTROLLER_BUTTON_X:
                if (isPsx)
                    return "textures/omw_psx_button_square.dds";
                return "textures/omw_steam_button_x.dds";
            case SDL_CONTROLLER_BUTTON_Y:
                if (isPsx)
                    return "textures/omw_psx_button_triangle.dds";
                return "textures/omw_steam_button_y.dds";
            case SDL_CONTROLLER_BUTTON_GUIDE:
            case SDL_CONTROLLER_BUTTON_MISC1:
            case SDL_CONTROLLER_BUTTON_PADDLE1:
            case SDL_CONTROLLER_BUTTON_PADDLE2:
            case SDL_CONTROLLER_BUTTON_PADDLE3:
            case SDL_CONTROLLER_BUTTON_PADDLE4:
            case SDL_CONTROLLER_BUTTON_TOUCHPAD:
            default:
                return {};
        }
    }

    std::string ControllerManager::getControllerAxisIcon(int axis)
    {
        int controllerType = ControllerManager::getControllerType();

        bool isXbox = controllerType == SDL_CONTROLLER_TYPE_XBOX360 || controllerType == SDL_CONTROLLER_TYPE_XBOXONE;
        bool isSwitch = controllerType == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;

        switch (axis)
        {
            case SDL_CONTROLLER_AXIS_LEFTX:
            case SDL_CONTROLLER_AXIS_LEFTY:
                return "textures/omw_steam_button_lstick.dds";
            case SDL_CONTROLLER_AXIS_RIGHTX:
            case SDL_CONTROLLER_AXIS_RIGHTY:
                return "textures/omw_steam_button_rstick.dds";
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                if (isXbox)
                    return "textures/omw_xbox_button_lt.dds";
                else if (isSwitch)
                    return "textures/omw_switch_button_zl.dds";
                return "textures/omw_steam_button_l2.dds";
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                if (isXbox)
                    return "textures/omw_xbox_button_rt.dds";
                else if (isSwitch)
                    return "textures/omw_switch_button_zr.dds";
                return "textures/omw_steam_button_r2.dds";
            default:
                return {};
        }
    }

    void ControllerManager::touchpadMoved(int deviceId, const SDLUtil::TouchEvent& arg)
    {
        MWBase::Environment::get().getLuaManager()->inputEvent({ MWBase::LuaManager::InputEvent::TouchMoved, arg });
    }

    void ControllerManager::touchpadPressed(int deviceId, const SDLUtil::TouchEvent& arg)
    {
        MWBase::Environment::get().getLuaManager()->inputEvent({ MWBase::LuaManager::InputEvent::TouchPressed, arg });
    }

    void ControllerManager::touchpadReleased(int deviceId, const SDLUtil::TouchEvent& arg)
    {
        MWBase::Environment::get().getLuaManager()->inputEvent({ MWBase::LuaManager::InputEvent::TouchReleased, arg });
    }
}
