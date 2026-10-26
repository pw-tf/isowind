#include "inventorytabsoverlay.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Window.h>

#include <components/settings/values.hpp>
#include <components/widgets/box.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    InventoryTabsOverlay::InventoryTabsOverlay()
        : WindowBase("openmw_inventory_tabs.layout")
    {
        MyGUI::Button* tab;
        constexpr std::string_view kTabIds[] = { "TabMap", "TabInventory", "TabSpells", "TabStats" };

        for (const std::string_view id : kTabIds)
        {
            getWidget(tab, id);
            tab->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryTabsOverlay::onTabClicked);
            mTabs.push_back(tab);
        }

        getWidget(mTabBox, "TabBox");

        MyGUI::ImageBox* image;
        // Isowind: the bumpers switch between these windows
        getWidget(image, "BtnL2Image");
        image->setImageTexture(
            MWBase::Environment::get().getInputManager()->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_LEFTSHOULDER));

        getWidget(image, "BtnR2Image");
        image->setImageTexture(MWBase::Environment::get().getInputManager()->getControllerButtonIcon(
            SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));

        onControllerMenusChanged();
    }

    void InventoryTabsOverlay::onControllerMenusChanged()
    {
        // Isowind: the controller's bumpers, or the keyboard's Q and E without a controller (the bar is for the mouse
        // and keyboard too)
        const bool controller = Settings::gui().mControllerMenus;
        const auto show = [this](std::string_view id, bool shown) {
            MyGUI::Widget* widget;
            getWidget(widget, id);
            widget->setVisible(shown);
            widget->setUserString("Hidden", shown ? "false" : "true");
        };
        show("BtnL2Image", controller);
        show("BtnR2Image", controller);
        show("KeyPrevText", !controller);
        show("KeyNextText", !controller);
        mTabBox->notifyChildrenSizeChanged();
        WindowBase::onControllerMenusChanged();
    }

    int InventoryTabsOverlay::getHeight()
    {
        MyGUI::Window* window = mMainWidget->castType<MyGUI::Window>();
        return window->getHeight();
    }

    void InventoryTabsOverlay::onTabClicked(MyGUI::Widget* sender)
    {
        if (!MWBase::Environment::get().getWindowManager()->getJournalAllowed())
            return;

        for (size_t i = 0; i < mTabs.size(); ++i)
        {
            if (mTabs[i] == sender)
            {
                MWBase::Environment::get().getWindowManager()->setActiveControllerWindow(GM_Inventory, i);
                setTab(i);
                break;
            }
        }
    }

    void InventoryTabsOverlay::setTab(size_t index)
    {
        for (size_t i = 0; i < mTabs.size(); ++i)
            mTabs[i]->setStateSelected(i == index);
    }

    void InventoryTabsOverlay::setTabAvailable(size_t index, bool available)
    {
        if (index >= mTabs.size() || mTabs[index]->getVisible() == available)
            return;
        mTabs[index]->setVisible(available);
        mTabs[index]->setUserString("Hidden", available ? "false" : "true");
        mTabBox->notifyChildrenSizeChanged();
    }
}
