#ifndef MWGUI_INVENTORYTABSSOVERLAY_H
#define MWGUI_INVENTORYTABSSOVERLAY_H

#include "windowbase.hpp"

namespace MyGUI
{
    class Button;
}

namespace Gui
{
    class HBox;
}

namespace MWGui
{
    class InventoryTabsOverlay : public WindowBase
    {
    public:
        InventoryTabsOverlay();

        int getHeight();
        void setTab(size_t index);
        /// Isowind: a tab only for a page open (character creation opens them one by one)
        void setTabAvailable(size_t index, bool available);
        void onControllerMenusChanged() override;

    private:
        std::vector<MyGUI::Button*> mTabs;
        Gui::HBox* mTabBox = nullptr;

        void onTabClicked(MyGUI::Widget* sender);
    };
}

#endif
