#include "actionbar.hpp"

#include <algorithm>

#include <MyGUI_Button.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>

#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "draganddrop.hpp"
#include "inventorywindow.hpp"
#include "itemmodel.hpp"
#include "itemwidget.hpp"
#include "quickkeysmenu.hpp"

namespace MWGui
{
    namespace
    {
        // Laid out like the HUD's weapon box: a 36 pixel box with a status bar under it
        constexpr int sBoxSize = 36;
        constexpr int sSpacing = 4;
        constexpr int sStatusHeight = 6;
        // The key ready to use: its border's width and colour; while one is, how far the others dim (empty ones more)
        constexpr int sBorderWidth = 2;
        const MyGUI::Colour sBorderColour(0.88f, 0.88f, 0.85f);
        constexpr float sDimmed = 0.55f;
        constexpr float sEmptyDimmed = 0.35f;
        constexpr float sEmpty = 0.5f;

        // Frames are drawn on 64 pixel textures at 44 pixels; bigger texture packs scale that
        int frameSize(const std::string& texture)
        {
            float scale = 1.f;
            if (MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().getTexture(texture))
                scale = tex->getHeight() / 64.f;
            return static_cast<int>(44 * scale);
        }

        // How worn an item is, or how charged an enchanted one, in percent; -1 if neither applies
        int statusOf(const MWWorld::Ptr& item, bool magicItem)
        {
            if (item.isEmpty())
                return -1;
            if (magicItem)
            {
                const ESM::RefId& enchantmentId = item.getClass().getEnchantment(item);
                const ESM::Enchantment* enchantment
                    = MWBase::Environment::get().getESMStore()->get<ESM::Enchantment>().search(enchantmentId);
                if (!enchantment)
                    return -1;
                const int maxCharge = MWMechanics::getEnchantmentCharge(*enchantment);
                if (maxCharge <= 0)
                    return -1;
                const float charge = item.getCellRef().getEnchantmentCharge();
                return charge < 0 ? 100 : static_cast<int>(100.f * charge / maxCharge);
            }
            if (!item.getClass().hasItemHealth(item))
                return -1;
            const int maxHealth = item.getClass().getItemMaxHealth(item);
            return maxHealth > 0 ? static_cast<int>(100.f * item.getClass().getItemHealth(item) / maxHealth) : -1;
        }
    }

    ActionBar::ActionBar(MyGUI::Widget* parent, DragAndDrop* dragAndDrop)
        : mParent(parent)
        , mDragAndDrop(dragAndDrop)
    {
        for (int i = 0; i < sSlots; ++i)
        {
            Slot& slot = mSlots[i];
            const int left = i * (sBoxSize + sSpacing);

            slot.mButton = mParent->createWidget<MyGUI::Button>(
                "", MyGUI::IntCoord(left, 0, sBoxSize, sBoxSize + sStatusHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            slot.mButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ActionBar::onSlotClicked);
            slot.mButton->eventMouseButtonPressed += MyGUI::newDelegate(this, &ActionBar::onSlotRightClicked);

            MyGUI::Widget* box = slot.mButton->createWidget<MyGUI::Widget>(
                "HUD_Box", MyGUI::IntCoord(0, 0, sBoxSize, sBoxSize), MyGUI::Align::Default);
            box->setNeedMouseFocus(false);
            MyGUI::Widget* inner = box->createWidget<MyGUI::Widget>(
                "", MyGUI::IntCoord(2, 2, sBoxSize - 4, sBoxSize - 4), MyGUI::Align::Default);
            inner->setNeedMouseFocus(false);
            slot.mIcon = inner->createWidget<ItemWidget>(
                "MW_ItemIconNoShadow", MyGUI::IntCoord(-5, -5, 42, 42), MyGUI::Align::Default);
            slot.mIcon->setNeedMouseFocus(false);

            for (auto [bar, skin] : { std::pair{ &slot.mCondition, "MW_EnergyBar_Weapon" },
                     std::pair{ &slot.mCharge, "MW_EnergyBar_Magic" } })
            {
                *bar = slot.mButton->createWidget<MyGUI::ProgressBar>(
                    skin, MyGUI::IntCoord(0, sBoxSize, sBoxSize, sStatusHeight), MyGUI::Align::Default);
                (*bar)->setNeedMouseFocus(false);
                (*bar)->setProgressRange(100);
                (*bar)->setVisible(false);
            }

            // The border, over the box's own edge
            const int far = sBoxSize - sBorderWidth;
            const std::array<MyGUI::IntCoord, 4> edges = { MyGUI::IntCoord(0, 0, sBoxSize, sBorderWidth),
                MyGUI::IntCoord(0, far, sBoxSize, sBorderWidth), MyGUI::IntCoord(0, 0, sBorderWidth, sBoxSize),
                MyGUI::IntCoord(far, 0, sBorderWidth, sBoxSize) };
            for (size_t e = 0; e < edges.size(); ++e)
            {
                MyGUI::ImageBox* edge
                    = slot.mButton->createWidget<MyGUI::ImageBox>("ImageBox", edges[e], MyGUI::Align::Default);
                edge->setImageTexture("white");
                edge->setColour(sBorderColour);
                edge->setNeedMouseFocus(false);
                edge->setVisible(false);
                slot.mBorder[e] = edge;
            }

            // How many there are, in the corner like the inventory shows it
            slot.mCountText = slot.mButton->createWidget<MyGUI::TextBox>(
                "CountText", MyGUI::IntCoord(2, sBoxSize - 18, sBoxSize - 5, 16), MyGUI::Align::Default);
            slot.mCountText->setTextAlign(MyGUI::Align::Right);
            slot.mCountText->setTextShadow(true);
            slot.mCountText->setNeedMouseFocus(false);

            // The key's number, in the corner like the game's quick keys menu shows it
            slot.mLabel = slot.mButton->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(3, 0, 16, 18), MyGUI::Align::Default);
            slot.mLabel->setCaption(std::to_string((i + 1) % 10));
            slot.mLabel->setTextShadow(true);
            slot.mLabel->setTextShadowColour(MyGUI::Colour::Black);
            slot.mLabel->setNeedMouseFocus(false);
        }
        mParent->setSize(sSlots * (sBoxSize + sSpacing) - sSpacing, sBoxSize + sStatusHeight);
    }

    void ActionBar::setVisible(bool visible)
    {
        mParent->setVisible(visible);
    }

    void ActionBar::update()
    {
        if (!mQuickKeysMenu || !mParent->getVisible())
            return;
        for (int i = 0; i < sSlots; ++i)
            showSlot(i, mSlots[i]);

        // The current key stands out (the one used last, stepping counts from it: QuickKeysMenu::getCurrentKey): a
        // border, a brighter number, and the others dim (while there is none, none dims)
        const int current = mQuickKeysMenu->getCurrentKey();
        for (int i = 0; i < sSlots; ++i)
        {
            Slot& slot = mSlots[i];
            const bool isCurrent = i == current;
            for (MyGUI::ImageBox* edge : slot.mBorder)
                edge->setVisible(isCurrent);
            slot.mLabel->setTextColour(
                isCurrent ? MyGUI::Colour(1.f, 0.94f, 0.75f) : MyGUI::Colour(0.79f, 0.65f, 0.38f));
            float alpha = slot.mEmpty ? sEmpty : 1.f;
            if (current >= 0 && !isCurrent)
                alpha = slot.mEmpty ? sEmptyDimmed : sDimmed;
            slot.mButton->setAlpha(alpha);
        }
    }

    void ActionBar::showSlot(int index, Slot& slot)
    {
        const QuickKeysMenu::QuickKeyInfo key = mQuickKeysMenu->getQuickKey(index + 1);
        const MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();

        // Is it what the player has ready: the equipped item, the selected magic item or spell, bare hands?
        bool active = false;
        int count = 0;
        int status = -1;
        switch (key.type)
        {
            case ESM::QuickKeys::Type::Item:
                if (!key.item.isEmpty())
                {
                    active = store.isEquipped(key.item);
                    count = store.count(key.item.getCellRef().getRefId());
                    status = statusOf(key.item, false);
                }
                break;
            case ESM::QuickKeys::Type::MagicItem:
                if (!key.item.isEmpty())
                {
                    active = store.getSelectedEnchantItem() != store.end() && *store.getSelectedEnchantItem() == key.item;
                    count = store.count(key.item.getCellRef().getRefId());
                    status = statusOf(key.item, true);
                }
                break;
            case ESM::QuickKeys::Type::Magic:
                active = store.getSelectedEnchantItem() == store.end() && windowManager->getSelectedSpell() == key.id;
                break;
            case ESM::QuickKeys::Type::HandToHand:
                active = store.getSlot(MWWorld::InventoryStore::Slot_CarriedRight) == store.end();
                break;
            default:
                break;
        }

        if (slot.mInitialised && slot.mType == key.type && slot.mId == key.id && slot.mItem == key.item
            && slot.mCount == count && slot.mStatusValue == status && slot.mActive == active)
            return;
        const bool contentChanged
            = !slot.mInitialised || slot.mType != key.type || slot.mId != key.id || slot.mItem != key.item;
        slot.mInitialised = true;
        slot.mType = key.type;
        slot.mId = key.id;
        slot.mItem = key.item;
        slot.mCount = count;
        slot.mStatusValue = status;
        slot.mActive = active;

        if (contentChanged)
        {
            slot.mButton->clearUserStrings();
            slot.mIcon->setItem(MWWorld::Ptr());
        }

        switch (key.type)
        {
            case ESM::QuickKeys::Type::Item:
                if (!key.item.isEmpty())
                {
                    // (No gold frame when equipped: the border shows the key ready to use)
                    slot.mIcon->setItem(key.item, ItemWidget::None);
                    slot.mButton->setUserString("ToolTipType", "ItemPtr");
                    slot.mButton->setUserData(MWWorld::Ptr(key.item));
                }
                break;
            case ESM::QuickKeys::Type::MagicItem:
                if (!key.item.isEmpty())
                {
                    const std::string frame = "textures\\menu_icon_select_magic_magic.dds";
                    const int size = frameSize(frame);
                    slot.mIcon->setFrame(frame, MyGUI::IntCoord(0, 0, size, size));
                    slot.mIcon->setIcon(key.item);
                    slot.mButton->setUserString("ToolTipType", "ItemPtr");
                    slot.mButton->setUserData(MWWorld::Ptr(key.item));
                }
                break;
            case ESM::QuickKeys::Type::Magic:
            {
                const MWWorld::ESMStore& esmStore = *MWBase::Environment::get().getESMStore();
                const ESM::Spell* spell = esmStore.get<ESM::Spell>().search(key.id);
                if (!spell || spell->mEffects.mList.empty())
                    break;
                const ESM::MagicEffect* effect
                    = esmStore.get<ESM::MagicEffect>().search(spell->mEffects.mList.front().mData.mEffectID);
                if (!effect)
                    break;
                const std::string frame = "textures\\menu_icon_select_magic.dds";
                const int size = frameSize(frame);
                slot.mIcon->setFrame(frame, MyGUI::IntCoord(0, 0, size, size));
                slot.mIcon->setIcon(Misc::ResourceHelpers::correctBigIconPath(VFS::Path::toNormalized(effect->mIcon),
                    *MWBase::Environment::get().getResourceSystem()->getVFS()));
                slot.mButton->setUserString("ToolTipType", "Spell");
                slot.mButton->setUserString("Spell", key.id.serialize());
                break;
            }
            case ESM::QuickKeys::Type::HandToHand:
            {
                slot.mIcon->setFrame({}, MyGUI::IntCoord());
                slot.mIcon->setIcon("icons\\k\\stealth_handtohand.dds");
                break;
            }
            default:
                break;
        }

        // Stacks show how many there are; worn or enchanted items their condition or charge
        slot.mCountText->setCaption(count > 999 ? std::string(">999") : count > 1 ? std::to_string(count) : std::string());
        MyGUI::ProgressBar* bar = key.type == ESM::QuickKeys::Type::MagicItem ? slot.mCharge : slot.mCondition;
        MyGUI::ProgressBar* otherBar = bar == slot.mCharge ? slot.mCondition : slot.mCharge;
        otherBar->setVisible(false);
        bar->setVisible(status >= 0);
        if (status >= 0)
            bar->setProgressPosition(std::clamp(status, 0, 100));


        // Keys with nothing on them stay in the background (update() sets how much)
        slot.mEmpty = key.type == ESM::QuickKeys::Type::Unassigned
            || ((key.type == ESM::QuickKeys::Type::Item || key.type == ESM::QuickKeys::Type::MagicItem)
                && key.item.isEmpty());
    }

    int ActionBar::indexOf(MyGUI::Widget* sender) const
    {
        for (int i = 0; i < sSlots; ++i)
        {
            if (mSlots[i].mButton == sender)
                return i;
        }
        return -1;
    }

    void ActionBar::onSlotClicked(MyGUI::Widget* sender)
    {
        const int index = indexOf(sender);
        if (index < 0 || !mQuickKeysMenu)
            return;

        // An item dragged from the inventory: it goes on this key, and back where it was
        if (mDragAndDrop && mDragAndDrop->mIsOnDragAndDrop)
        {
            ItemModel* playerModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getModel();
            if (mDragAndDrop->mSourceModel != playerModel)
                return;
            const MWWorld::Ptr item = mDragAndDrop->mItem.mBase;
            mDragAndDrop->drop(mDragAndDrop->mSourceModel, mDragAndDrop->mSourceView);
            mQuickKeysMenu->assignItemToKey(index + 1, item);
            mSlots[index].mInitialised = false;
            return;
        }

        mQuickKeysMenu->activateQuickKey(index + 1);
    }

    void ActionBar::onSlotRightClicked(MyGUI::Widget* sender, int /*left*/, int /*top*/, MyGUI::MouseButton id)
    {
        if (id != MyGUI::MouseButton::Right || !mQuickKeysMenu)
            return;
        const int index = indexOf(sender);
        if (index < 0)
            return;
        mQuickKeysMenu->openAssignDialog(index + 1);
        mSlots[index].mInitialised = false;
    }
}
