#ifndef OPENMW_MWGUI_ACTIONBAR_H
#define OPENMW_MWGUI_ACTIONBAR_H

#include <array>
#include <string>

#include <components/esm/refid.hpp>
#include <components/esm3/quickkeys.hpp>

#include "../mwworld/ptr.hpp"

namespace MyGUI
{
    class Button;
    class ProgressBar;
    class TextBox;
    class Widget;
}

namespace MWGui
{
    class DragAndDrop;
    class ItemWidget;
    class QuickKeysMenu;

    /// Isowind: the quick keys (1 to 0) as a row of HUD boxes at the bottom of the screen, showing what each holds
    /// (the item or spell, whether it's equipped or selected, its condition or charge). In menus a slot can be
    /// clicked to use it, an item dragged from the inventory dropped on it to assign it, and right-clicked for
    /// Morrowind's own assign dialog.
    class ActionBar
    {
    public:
        ActionBar(MyGUI::Widget* parent, DragAndDrop* dragAndDrop);

        void setQuickKeysMenu(QuickKeysMenu* quickKeysMenu) { mQuickKeysMenu = quickKeysMenu; }

        void setVisible(bool visible);

        /// Shows what the quick keys hold now
        void update();

    private:
        static constexpr int sSlots = 10;

        struct Slot
        {
            MyGUI::Button* mButton = nullptr;
            ItemWidget* mIcon = nullptr;
            // Condition of worn items, charge of enchanted ones (two bars: MyGUI can't change a bar's skin safely)
            MyGUI::ProgressBar* mCondition = nullptr;
            MyGUI::ProgressBar* mCharge = nullptr;
            MyGUI::TextBox* mLabel = nullptr;
            MyGUI::TextBox* mCountText = nullptr;

            // What it shows, to update only when that changes
            ESM::QuickKeys::Type mType = ESM::QuickKeys::Type::Unassigned;
            ESM::RefId mId;
            MWWorld::Ptr mItem;
            int mCount = -1;
            int mStatusValue = -1;
            bool mActive = false;
            bool mInitialised = false;
        };

        void showSlot(int index, Slot& slot);
        void onSlotClicked(MyGUI::Widget* sender);
        void onSlotRightClicked(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
        int indexOf(MyGUI::Widget* sender) const;

        MyGUI::Widget* mParent;
        DragAndDrop* mDragAndDrop;
        QuickKeysMenu* mQuickKeysMenu = nullptr;
        std::array<Slot, sSlots> mSlots;
    };
}

#endif
