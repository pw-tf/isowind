#ifndef MWGUI_QUICKKEYS_H
#define MWGUI_QUICKKEYS_H

#include <memory>

#include "components/esm3/quickkeys.hpp"

#include "../mwworld/manualref.hpp"

#include "itemselection.hpp"
#include "spellmodel.hpp"
#include "windowbase.hpp"

namespace MWGui
{

    class QuickKeysMenuAssign;
    class MagicSelectionDialog;
    class ItemWidget;
    class SpellView;

    class QuickKeysMenu : public WindowBase
    {
    public:
        QuickKeysMenu();

        void onResChange(int, int) override { center(); }

        void onItemButtonClicked(MyGUI::Widget* sender);
        void onMagicButtonClicked(MyGUI::Widget* sender);
        void onUnassignButtonClicked(MyGUI::Widget* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);

        void onAssignItem(MWWorld::Ptr item);
        void onAssignItemCancel();
        void onAssignMagicItem(MWWorld::Ptr item);
        void onAssignMagic(const ESM::RefId& spellId);
        void onAssignMagicCancel();
        void onOpen() override;
        void onClose() override;

        void activateQuickKey(int index);
        /// Isowind: uses the next (1) or previous (-1) key that holds something, counting from the one in use
        void stepQuickKey(int direction);

        /// Isowind's action bar: what a key (1-based) holds; item is the inventory item of an item key, if any.
        struct QuickKeyInfo
        {
            ESM::QuickKeys::Type type = ESM::QuickKeys::Type::Unassigned;
            ESM::RefId id;
            MWWorld::Ptr item;
        };
        QuickKeyInfo getQuickKey(int index);
        /// Isowind's action bar: the Item / Magic / Delete dialog for a key (1-based), as clicking it here opens.
        void openAssignDialog(int index);
        void assignItemToKey(int index, const MWWorld::Ptr& item);

        /// Isowind: the slot picked next gets this item (Item, MagicItem) or spell (Magic) instead of opening the
        /// assign dialog; then the menu closes
        void beginAssign(ESM::QuickKeys::Type type, const MWWorld::Ptr& item, const ESM::RefId& spellId);
        void unassignKey(int index);

        /// Isowind multiplayer: assigns a key (1-based slot) as the server says.
        void setQuickKey(int slot, ESM::QuickKeys::Type type, const MWWorld::Ptr& item, const ESM::RefId& spellId);
        void updateActivatedQuickKey();

        void write(ESM::ESMWriter& writer);
        void readRecord(ESM::ESMReader& reader, uint32_t type);
        void clear() override;

        std::string_view getWindowIdForLua() const override { return "QuickKeys"; }

    private:
        ESM::QuickKeys::Type mPendingType = ESM::QuickKeys::Type::Unassigned;
        MWWorld::Ptr mPendingItem;
        ESM::RefId mPendingSpell;

        struct keyData
        {
            int index = -1;
            ItemWidget* button = nullptr;
            ESM::QuickKeys::Type type = ESM::QuickKeys::Type::Unassigned;
            ESM::RefId id;
            std::string name;
        };

        std::vector<keyData> mKey;
        std::vector<MWWorld::ManualRef> mTemp;
        keyData* mSelected;
        int mLastStepped = -1;
        bool isKeyInUse(int index);
        keyData* mActivated;

        MyGUI::EditBox* mInstructionLabel;
        MyGUI::Button* mOkButton;

        std::unique_ptr<QuickKeysMenuAssign> mAssignDialog;
        std::unique_ptr<ItemSelectionDialog> mItemSelectionDialog;
        std::unique_ptr<MagicSelectionDialog> mMagicSelectionDialog;

        void onQuickKeyButtonClicked(MyGUI::Widget* sender);
        void onOkButtonClicked(MyGUI::Widget* sender);
        // Check if quick key is still valid
        inline void validate(int index);
        void unassign(keyData* key);
        /// Isowind multiplayer: tells the server about a key the player changed, so it is kept between sessions
        void sendToServer(const keyData& key);
        void assignItem(MWWorld::Ptr item);

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        size_t mControllerFocus = 0;
    };

    class QuickKeysMenuAssign : public WindowModal
    {
    public:
        QuickKeysMenuAssign(QuickKeysMenu* parent);

    private:
        MyGUI::TextBox* mLabel;
        MyGUI::Button* mItemButton;
        MyGUI::Button* mMagicButton;
        MyGUI::Button* mUnassignButton;
        MyGUI::Button* mCancelButton;

        QuickKeysMenu* mParent;

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        size_t mControllerFocus = 0;
    };

    class MagicSelectionDialog : public WindowModal
    {
    public:
        MagicSelectionDialog(QuickKeysMenu* parent);

        void onOpen() override;
        bool exit() override;

        void setActiveControllerWindow(bool active) override;

    private:
        MyGUI::Button* mCancelButton;
        SpellView* mMagicList;

        QuickKeysMenu* mParent;

        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onModelIndexSelected(SpellModel::ModelIndex index);

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
    };
}

#endif
