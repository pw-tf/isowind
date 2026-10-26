#include "windowbase.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_ComboBox.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include <components/lua_ui/widget.hpp>
#include <components/settings/values.hpp>
#include <components/widgets/imagebutton.hpp>
#include <components/widgets/numericeditbox.hpp>

#include "draganddrop.hpp"
#include "exposedwindow.hpp"
#include "textcolours.hpp"

using namespace MWGui;

void WindowBase::setActiveControllerWindow(bool active)
{
    mActiveControllerWindow = active;
    // Isowind: with several windows side by side, the one the controller works in is lit and the others dimmed
    if (Settings::gui().mControllerMenus && mMainWidget)
    {
        const float alpha
            = active || !MWBase::Environment::get().getWindowManager()->controllerCyclesWindows() ? 1.f : 0.55f;
        mMainWidget->setAlpha(alpha);
        // Windows that fade under a dragged item settle back to this rather than to fully opaque
        if (NoDrop* noDrop = dynamic_cast<NoDrop*>(this))
            noDrop->setRestAlpha(alpha);
    }
}

bool WindowBase::isGamepadCursorAllowed() const
{
    return !(mDisableGamepadCursor && Settings::gui().mControllerMenus);
}

void WindowBase::onControllerMenusChanged()
{
    // Back from the menus made for a controller: no window stays dimmed (setActiveControllerWindow)
    if (!Settings::gui().mControllerMenus && mMainWidget)
    {
        mMainWidget->setAlpha(1.f);
        if (NoDrop* noDrop = dynamic_cast<NoDrop*>(this))
            noDrop->setRestAlpha(1.f);
    }
}

size_t MWGui::wrap(size_t index, size_t max, int delta)
{
    if (max == 0)
        return 0;
    if (delta >= 0)
    {
        unsigned absDelta = static_cast<unsigned>(delta);
        if (absDelta >= max)
            return 0;
        else if (index >= max - absDelta)
            return 0;
        return index + absDelta;
    }
    size_t absDelta = static_cast<size_t>(-static_cast<ptrdiff_t>(delta));
    index = std::min(index, max);
    if (index >= absDelta)
        return index - absDelta;
    return max - 1;
}

void MWGui::setControllerFocus(const std::vector<MyGUI::Button*>& buttons, size_t index, bool focused)
{
    if (index < buttons.size())
        buttons[index]->setStateSelected(focused);
}

bool MWGui::isTextField(MyGUI::Widget* widget)
{
    if (!widget)
        return false;
    if (LuaUi::WidgetExtension* luaWidget = dynamic_cast<LuaUi::WidgetExtension*>(widget))
        return luaWidget->isTextInput();
    MyGUI::EditBox* edit = widget->castType<MyGUI::EditBox>(false);
    if (!edit || edit->getEditReadOnly() || edit->getEditStatic() || widget->castType<Gui::NumericEditBox>(false))
        return false;
    if (MyGUI::ComboBox* comboBox = widget->castType<MyGUI::ComboBox>(false))
        return !comboBox->getComboModeDrop();
    return true;
}

void MWGui::setTextFieldSelected(MyGUI::TextBox* field, bool selected)
{
    const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
    field->setTextColour(selected ? textColours.link : textColours.normal);
}

MyGUI::Widget* WindowBase::getControllerTextField()
{
    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    if (!isTextField(focus))
        return nullptr;
    for (MyGUI::Widget* widget = focus; widget; widget = widget->getParent())
    {
        if (widget == mMainWidget)
            return focus;
    }
    return nullptr;
}

WindowBase::WindowBase(std::string_view parLayout)
    : Layout(parLayout)
{
    mMainWidget->setVisible(false);

    Window* window = mMainWidget->castType<Window>(false);
    if (!window)
        return;

    MyGUI::Button* button = nullptr;
    MyGUI::VectorWidgetPtr widgets = window->getSkinWidgetsByName("Action");
    for (MyGUI::Widget* widget : widgets)
    {
        if (widget->isUserString("SupportDoubleClick"))
            button = widget->castType<MyGUI::Button>();
    }

    if (button)
        button->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &WindowBase::onDoubleClick);
}

void WindowBase::onTitleDoubleClicked()
{
    if (MyGUI::InputManager::getInstance().isShiftPressed())
        MWBase::Environment::get().getWindowManager()->toggleMaximized(this);
}

void WindowBase::onDoubleClick(MyGUI::Widget* /*sender*/)
{
    onTitleDoubleClicked();
}

void WindowBase::setVisible(bool visible)
{
    visible = visible && !mDisabledByLua;
    bool wasVisible = mMainWidget->getVisible();
    mMainWidget->setVisible(visible);

    if (visible)
        onOpen();
    else if (wasVisible)
        onClose();
}

bool WindowBase::isVisible() const
{
    return mMainWidget->getVisible();
}

void WindowBase::center()
{
    // Centre dialog

    MyGUI::IntSize layerSize = MyGUI::RenderManager::getInstance().getViewSize();
    if (mMainWidget->getLayer())
        layerSize = mMainWidget->getLayer()->getSize();

    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (layerSize.width - coord.width) / 2;
    coord.top = (layerSize.height - coord.height) / 2;
    mMainWidget->setCoord(coord);
}

void WindowBase::clampWindowCoordinates(MyGUI::Window* window)
{
    MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
    if (window->getLayer())
        viewSize = window->getLayer()->getSize();

    // Window's minimum size is larger than the screen size, can not clamp coordinates
    auto minSize = window->getMinSize();
    if (minSize.width > viewSize.width || minSize.height > viewSize.height)
        return;

    int left = std::max(0, window->getPosition().left);
    int top = std::max(0, window->getPosition().top);
    int width = std::clamp(window->getSize().width, 0, viewSize.width);
    int height = std::clamp(window->getSize().height, 0, viewSize.height);
    if (left + width > viewSize.width)
        left = viewSize.width - width;

    if (top + height > viewSize.height)
        top = viewSize.height - height;

    if (window->getSize().width != width || window->getSize().height != height)
        window->setSize(width, height);

    if (window->getPosition().left != left || window->getPosition().top != top)
        window->setPosition(left, top);
}

WindowModal::WindowModal(const std::string& parLayout)
    : WindowBase(parLayout)
{
}

void WindowModal::onOpen()
{
    MWBase::Environment::get().getWindowManager()->addCurrentModal(this); // Set so we can escape it if needed

    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    MyGUI::InputManager::getInstance().addWidgetModal(mMainWidget);
    MyGUI::InputManager::getInstance().setKeyFocusWidget(focus);
}

void WindowModal::onClose()
{
    MWBase::Environment::get().getWindowManager()->removeCurrentModal(this);
    MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();

    MyGUI::InputManager::getInstance().removeWidgetModal(mMainWidget);
}

NoDrop::NoDrop(DragAndDrop* drag, MyGUI::Widget* widget)
    : mWidget(widget)
    , mDrag(drag)
    , mTransparent(false)
{
}

void NoDrop::onFrame(float dt)
{
    if (!mWidget)
        return;

    MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();

    if (mDrag->mIsOnDragAndDrop)
    {
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        while (focus && focus != mWidget)
            focus = focus->getParent();

        if (focus == mWidget)
            mTransparent = true;
    }
    if (!mWidget->getAbsoluteCoord().inside(mousePos))
        mTransparent = false;

    if (mTransparent)
    {
        mWidget->setNeedMouseFocus(false); // Allow click-through
        setAlpha(std::max(0.13f, mWidget->getAlpha() - dt * 5));
    }
    else
    {
        mWidget->setNeedMouseFocus(true);
        const float alpha = mWidget->getAlpha();
        setAlpha(alpha < mRestAlpha ? std::min(mRestAlpha, alpha + dt * 5) : std::max(mRestAlpha, alpha - dt * 5));
    }
}

void NoDrop::setAlpha(float alpha)
{
    if (mWidget)
        mWidget->setAlpha(alpha);
}

BookWindowBase::BookWindowBase(std::string_view parLayout)
    : WindowBase(parLayout)
{
}

float BookWindowBase::adjustButton(std::string_view name)
{
    Gui::ImageButton* button;
    WindowBase::getWidget(button, name);
    MyGUI::IntSize requested = button->getRequestedSize();
    float scale = float(requested.height) / button->getSize().height;
    MyGUI::IntSize newSize(static_cast<int>(requested.width / scale), static_cast<int>(requested.height / scale));
    button->setSize(newSize);

    if (button->getAlign().isRight())
    {
        MyGUI::IntSize diff = (button->getSize() - requested);
        diff.width = static_cast<int>(diff.width / scale);
        diff.height = static_cast<int>(diff.height / scale);
        button->setPosition(button->getPosition() + MyGUI::IntPoint(diff.width, 0));
    }

    return scale;
}
