#ifndef MGUI_CONTAINER_H
#define MGUI_CONTAINER_H

#include <utility>
#include <vector>

#include "itemmodel.hpp"
#include "referenceinterface.hpp"
#include "windowbase.hpp"

#include <components/misc/notnullptr.hpp>

namespace MyGUI
{
    class Gui;
    class Widget;
    class ImageBox;
    class Window;
}

namespace MWGui
{
    class ContainerWindow;
    class ContainerItemModel;
    class ItemView;
    class SortFilterItemModel;
    class ItemTransfer;

    class ContainerWindow : public WindowBase, public ReferenceInterface
    {
    public:
        explicit ContainerWindow(DragAndDrop& dragAndDrop, ItemTransfer& itemTransfer);

        void setPtr(const MWWorld::Ptr& container) override;

        void onOpen() override;

        void onClose() override;

        void clear() override { resetReference(); }

        void onFrame(float dt) override;

        void resetReference() override;

        void onDeleteCustomData(const MWWorld::Ptr& ptr) override;

        void treatNextOpenAsLoot() { mTreatNextOpenAsLoot = true; }

        void onInventoryUpdate(const MWWorld::Ptr& ptr) override;

        std::string_view getWindowIdForLua() const override { return "Container"; }

        ControllerButtons* getControllerButtons() override;
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        void setActiveControllerWindow(bool active) override;

        MWGui::ItemView* getItemView() { return mItemView; }

        // Isowind multiplayer
        bool isOnDragAndDrop() const;
        /// Starts dragging this item, once the server has approved it (its index may have changed meanwhile).
        bool dragItemByPtr(const MWWorld::Ptr& itemPtr, int dragCount);
        ItemModel* getModel() { return mModel; }
        /// The server changed this container, which the window shows: shows the window's containers afresh (the
        /// one it was opened on, and those opened with it)
        void onContainerChanged(const MWWorld::Ptr& container);
        /// Whether the window stays open once the last Take All is done (it was from one container's tab, not "All")
        bool keepsOpenAfterTakeAll() const { return mTakeAllKeepsOpen; }

    private:
        using ItemCounts = std::vector<std::pair<MWWorld::Ptr, int>>;

        Misc::NotNullPtr<DragAndDrop> mDragAndDrop;
        Misc::NotNullPtr<ItemTransfer> mItemTransfer;

        MWGui::ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        ItemModel* mModel;
        /// Isowind: containers right by the one opened that open with it, as one (see setPtr), and the model showing
        /// them all (null when there are none)
        std::vector<MWWorld::Ptr> mNearbyContainers;
        ContainerItemModel* mMergedModel;

        /// Isowind: tabs, when several containers open together: "All" first, then one per container (the one opened
        /// first, then the others nearest first)
        MyGUI::Widget* mTabBar;
        MyGUI::ImageBox* mPrevTabImage; ///< The controller's buttons for the tabs, at either end
        MyGUI::ImageBox* mNextTabImage;
        MyGUI::Button* mPrevTabButton; ///< Shown when the tabs don't all fit
        MyGUI::Button* mNextTabButton;
        std::vector<MyGUI::Button*> mTabButtons;
        std::vector<MWWorld::Ptr> mTabContainers; ///< The containers of the tabs after "All"
        size_t mTab; ///< The tab shown (0: "All")
        MWWorld::Ptr mTabContainer; ///< The container of the tab shown (none: "All"), kept when the window rebuilds
        int mItemViewTop; ///< Where the items start without the tabs
        bool mTakeAllKeepsOpen = false;
        int mSelectedItem;
        bool mUpdateNextFrame;
        bool mTreatNextOpenAsLoot;
        MyGUI::Button* mDisposeCorpseButton;
        MyGUI::Button* mTakeButton;
        MyGUI::Button* mCloseButton;

        void onItemSelected(int index);
        void onBackgroundSelected();
        void dragItem(MyGUI::Widget* sender, std::size_t count);
        void transferItem(MyGUI::Widget* sender, std::size_t count);
        void dropItem();
        void onCloseButtonClicked(MyGUI::Widget* sender);
        void onTakeAllButtonClicked(MyGUI::Widget* sender);
        void onDisposeCorpseButtonClicked(MyGUI::Widget* sender);

        /// Isowind multiplayer: items taken, by the container each comes out of
        std::vector<std::pair<MWWorld::Ptr, ItemCounts>> splitBySource(const ItemCounts& items);

        /// Isowind: the tabs for the containers opened together (none for one container), on the tab shown before if
        /// \a keepTab and its container is still there, else on "All"
        void updateTabs(bool keepTab);
        void clearTabs();
        void selectTab(size_t tab);
        void layoutTabs();
        void setTabsVisible(bool visible);
        void onTabClicked(MyGUI::Widget* sender);
        void onPrevTabClicked(MyGUI::Widget* sender);
        void onNextTabClicked(MyGUI::Widget* sender);
        void onTabWheel(MyGUI::Widget* sender, int rel);
        void onWindowResize(MyGUI::Window* window);

        void onReferenceUnavailable() override;
    };
}
#endif // CONTAINER_H
