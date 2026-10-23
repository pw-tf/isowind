#include "container.hpp"

#include <algorithm>
#include <cmath>

#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_TextIterator.h>
#include <MyGUI_Window.h>

#include <osg/Vec2f>
#include <osg/Vec3f>

#include <components/esm3/loadcont.hpp>
#include <components/settings/values.hpp>
#include <components/widgets/box.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmp/Hooks.hpp"
#include "../mwmp/LocalPlayer.hpp"
#include "../mwmp/Main.hpp"
#include "../mwmp/Networking.hpp"
#include "../mwmp/ObjectList.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/aipackage.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/summoning.hpp"

#include "../mwscript/interpretercontext.hpp"

#include "containeritemmodel.hpp"
#include "countdialog.hpp"
#include "draganddrop.hpp"
#include "inventoryitemmodel.hpp"
#include "inventorywindow.hpp"
#include "itemtransfer.hpp"
#include "itemview.hpp"
#include "pickpocketitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "tooltips.hpp"

namespace
{
    // Isowind: how near other containers have to be to the one opened (from where each stands) to open with it as
    // one: crates stacked on it or right beside it, a row of barrels
    constexpr float sNearbyContainerRange = 128.f; // across
    constexpr float sNearbyContainerHeight = 256.f; // above or below

    // Isowind: the tabs for them, above the items
    constexpr int sTabHeight = 24;
    constexpr int sTabSpacing = 4;

    /// Isowind: the ownership rule for containers opening together: taking from \a other must be no different, as far
    /// as the law goes, from taking from \a opened (the same owner, or both unowned, and the same faction and rank)
    bool haveSameOwner(const MWWorld::Ptr& opened, const MWWorld::Ptr& other)
    {
        const MWWorld::CellRef& openedRef = opened.getCellRef();
        const MWWorld::CellRef& otherRef = other.getCellRef();
        if (openedRef.getOwner() != otherRef.getOwner() || openedRef.getFaction() != otherRef.getFaction()
            || openedRef.getGlobalVariable() != otherRef.getGlobalVariable())
            return false;
        if (!openedRef.getFaction().empty() && openedRef.getFactionRank() != otherRef.getFactionRank())
            return false;
        // The evidence chest is never free to take from, whoever owns it (see MechanicsManager::isAllowedToUse)
        return (openedRef.getRefId() == "stolen_goods") == (otherRef.getRefId() == "stolen_goods");
    }

    /// Isowind: whether \a other opens with \a opened, as one container
    bool opensWith(const MWWorld::Ptr& opened, const MWWorld::Ptr& other)
    {
        if (other == opened || other.getType() != ESM::REC_CONT || other.getCell() != opened.getCell())
            return false;
        if (other.mRef->isDeleted() || !other.getRefData().isEnabled())
            return false;

        const osg::Vec3f offset
            = other.getRefData().getPosition().asVec3() - opened.getRefData().getPosition().asVec3();
        if (std::abs(offset.z()) > sNearbyContainerHeight
            || osg::Vec2f(offset.x(), offset.y()).length() > sNearbyContainerRange)
            return false;

        // Ones that have to be opened by themselves: locked, trapped, or with a script (a quest's OnActivate)
        const MWWorld::CellRef& cellRef = other.getCellRef();
        if (cellRef.isLocked() || !cellRef.getTrap().empty() || !other.getClass().getScript(other).empty())
            return false;
        // Plants (harvested by themselves) and ones that refill
        if (other.get<ESM::Container>()->mBase->mFlags & (ESM::Container::Organic | ESM::Container::Respawn))
            return false;

        return haveSameOwner(opened, other);
    }

    /// Isowind: the containers that open with \a opened (nearest first)
    std::vector<MWWorld::Ptr> findNearbyContainers(const MWWorld::Ptr& opened)
    {
        std::vector<MWWorld::Ptr> found;
        // Only a container (not a body), and not a plant
        if (opened.getType() != ESM::REC_CONT || !opened.isInCell()
            || (opened.get<ESM::Container>()->mBase->mFlags & ESM::Container::Organic))
            return found;

        opened.getCell()->forEachType<ESM::Container>([&](const MWWorld::Ptr& other) {
            if (opensWith(opened, other))
                found.push_back(other);
            return true;
        });

        // (the same order each time the window rebuilds: the tabs are in it)
        const osg::Vec3f position = opened.getRefData().getPosition().asVec3();
        std::stable_sort(found.begin(), found.end(), [&](const MWWorld::Ptr& left, const MWWorld::Ptr& right) {
            return (left.getRefData().getPosition().asVec3() - position).length2()
                < (right.getRefData().getPosition().asVec3() - position).length2();
        });
        return found;
    }
}

namespace MWGui
{

    ContainerWindow::ContainerWindow(DragAndDrop& dragAndDrop, ItemTransfer& itemTransfer)
        : WindowBase("openmw_container_window.layout")
        , mDragAndDrop(&dragAndDrop)
        , mItemTransfer(&itemTransfer)
        , mSortModel(nullptr)
        , mModel(nullptr)
        , mMergedModel(nullptr)
        , mTabBar(nullptr)
        , mPrevTabImage(nullptr)
        , mNextTabImage(nullptr)
        , mPrevTabButton(nullptr)
        , mNextTabButton(nullptr)
        , mTab(0)
        , mItemViewTop(0)
        , mSelectedItem(-1)
        , mUpdateNextFrame(false)
        , mTreatNextOpenAsLoot(false)
    {
        getWidget(mDisposeCorpseButton, "DisposeCorpseButton");
        getWidget(mTakeButton, "TakeButton");
        getWidget(mCloseButton, "CloseButton");

        getWidget(mItemView, "ItemView");
        mItemView->eventBackgroundClicked += MyGUI::newDelegate(this, &ContainerWindow::onBackgroundSelected);
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &ContainerWindow::onItemSelected);

        // Isowind: the tabs for containers opened together, above the items (made here rather than in the layout, so
        // an older copy of it still works)
        const MyGUI::IntCoord itemViewCoord = mItemView->getCoord();
        mItemViewTop = itemViewCoord.top;
        mTabBar = mMainWidget->createWidget<MyGUI::Widget>({},
            MyGUI::IntCoord(itemViewCoord.left, itemViewCoord.top, itemViewCoord.width, sTabHeight),
            MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
        mTabBar->setVisible(false);
        mTabBar->eventMouseWheel += MyGUI::newDelegate(this, &ContainerWindow::onTabWheel);
        auto makeArrow = [&](const char* caption, auto handler) {
            MyGUI::Button* button = mTabBar->createWidget<Gui::AutoSizedButton>(
                "MW_Button", MyGUI::IntCoord(0, 0, 24, sTabHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            button->setCaption(caption);
            button->setNeedKeyFocus(false);
            button->setVisible(false);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, handler);
            button->eventMouseWheel += MyGUI::newDelegate(this, &ContainerWindow::onTabWheel);
            return button;
        };
        mPrevTabButton = makeArrow("<", &ContainerWindow::onPrevTabClicked);
        mNextTabButton = makeArrow(">", &ContainerWindow::onNextTabClicked);
        if (Settings::gui().mControllerMenus)
        {
            // The triggers, as for the inventory's tabs (Isowind: the bumpers switch windows)
            auto makeImage = [&](SDL_GameControllerAxis axis) {
                MyGUI::ImageBox* image = mTabBar->createWidget<MyGUI::ImageBox>(
                    "ImageBox", MyGUI::IntCoord(0, 0, sTabHeight, sTabHeight), MyGUI::Align::Left | MyGUI::Align::Top);
                image->setImageTexture(MWBase::Environment::get().getInputManager()->getControllerAxisIcon(axis));
                image->setColour(MyGUI::Colour::parse("#ccb589"));
                image->setAlpha(0.7f);
                image->setNeedMouseFocus(false);
                return image;
            };
            mPrevTabImage = makeImage(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
            mNextTabImage = makeImage(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        }
        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord
            += MyGUI::newDelegate(this, &ContainerWindow::onWindowResize);

        mDisposeCorpseButton->eventMouseButtonClick
            += MyGUI::newDelegate(this, &ContainerWindow::onDisposeCorpseButtonClicked);
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onCloseButtonClicked);
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onTakeAllButtonClicked);

        setCoord(200, 0, 600, 300);

        mControllerButtons.mA = "#{Interface:Take}";
        mControllerButtons.mB = "#{Interface:Close}";
        mControllerButtons.mX = "#{Interface:TakeAll}";
        mControllerButtons.mR3 = "#{Interface:Info}";
        mControllerButtons.mL2 = "#{Interface:Inventory}";
    }

    void ContainerWindow::onItemSelected(int index)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            dropItem();
            return;
        }

        const ItemStack& item = mSortModel->getItem(index);

        // We can't take a conjured item from a container (some NPC we're pickpocketing, a box, etc)
        if (item.mFlags & ItemStack::Flag_Bound)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sContentsMessage1}");
            return;
        }

        MWWorld::Ptr object = item.mBase;
        size_t count = item.mCount;
        bool shift = MyGUI::InputManager::getInstance().isShiftPressed();
        if (MyGUI::InputManager::getInstance().isControlPressed())
            count = 1;

        mSelectedItem = mSortModel->mapToSource(index);

        if (count > 1 && !shift)
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            std::string name{ object.getClass().getName(object) };
            name += MWGui::ToolTips::getSoulString(object.getCellRef());
            dialog->openCountDialog(name, "#{sTake}", static_cast<int>(count));
            dialog->eventOkClicked.clear();
            if (Settings::gui().mControllerMenus || MyGUI::InputManager::getInstance().isAltPressed())
                dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerWindow::transferItem);
            else
                dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerWindow::dragItem);
        }
        else if (Settings::gui().mControllerMenus || MyGUI::InputManager::getInstance().isAltPressed())
            transferItem(nullptr, count);
        else
            dragItem(nullptr, count);
    }

    void ContainerWindow::dragItem(MyGUI::Widget* /*sender*/, std::size_t count)
    {
        if (mModel == nullptr)
            return;

        const ItemStack item = mModel->getItem(mSelectedItem);

        if (!mModel->onTakeItem(item.mBase, static_cast<int>(count)))
            return;

        // Isowind multiplayer: the server decides; the drag starts when it agrees (ObjectList::editContainers)
        // (asked of each container the items are in, when the window shows several)
        if (mwmp::hooks::isMultiplayer())
        {
            for (const auto& [container, items] : splitBySource({ { item.mBase, static_cast<int>(count) } }))
                mwmp::hooks::sendContainerChange(
                    container, mwmp::BaseObjectList::REMOVE, mwmp::BaseObjectList::DRAG, items);
            return;
        }

        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mModel, mItemView, count);
    }

    void ContainerWindow::transferItem(MyGUI::Widget* /*sender*/, std::size_t count)
    {
        if (mModel == nullptr)
            return;

        // Isowind multiplayer: taken through the server, like dragging
        if (mwmp::hooks::isMultiplayer())
        {
            dragItem(nullptr, count);
            return;
        }

        const ItemStack item = mModel->getItem(mSelectedItem);

        if (!mModel->onTakeItem(item.mBase, static_cast<int>(count)))
            return;

        mItemTransfer->apply(item, count, *mItemView);
    }

    void ContainerWindow::dropItem()
    {
        if (mModel == nullptr)
            return;

        bool success = mModel->onDropItem(mDragAndDrop->mItem.mBase, static_cast<int>(mDragAndDrop->mDraggedCount));

        // Isowind multiplayer: the server puts the items in the container (for everyone); they leave the drag
        if (success && mwmp::hooks::isMultiplayer())
        {
            // (into the container whose tab is shown, of several)
            const MWWorld::Ptr target = mMergedModel ? mMergedModel->getDropTarget() : mPtr;
            mwmp::hooks::sendContainerChange(target, mwmp::BaseObjectList::ADD, mwmp::BaseObjectList::DROP,
                { { mDragAndDrop->mItem.mBase, static_cast<int>(mDragAndDrop->mDraggedCount) } });
            mDragAndDrop->finish(true);
            return;
        }

        if (success)
            mDragAndDrop->drop(mModel, mItemView);
    }

    void ContainerWindow::onBackgroundSelected()
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
            dropItem();
    }

    void ContainerWindow::setPtr(const MWWorld::Ptr& container)
    {
        if (container.isEmpty() || (container.getType() != ESM::REC_CONT && !container.getClass().isActor()))
            throw std::runtime_error("Invalid argument in ContainerWindow::setPtr");
        bool lootAnyway = mTreatNextOpenAsLoot;
        mTreatNextOpenAsLoot = false;
        // Isowind: shown afresh (the server changed one of its containers, one went away), not opened
        const bool refresh = !mPtr.isEmpty() && mPtr == container;
        mPtr = container;
        // Isowind multiplayer: the container open here, for the server's replies
        if (mwmp::hooks::isMultiplayer())
            mwmp::Main::get().getLocalPlayer()->storeCurrentContainer(container);

        bool loot = mPtr.getClass().isActor() && mPtr.getClass().getCreatureStats(mPtr).isDead();

        mNearbyContainers.clear();
        mMergedModel = nullptr;
        std::unique_ptr<ItemModel> model;
        if (mPtr.getClass().hasInventoryStore(mPtr))
        {
            if (mPtr.getClass().isNpc() && !loot && !lootAnyway)
            {
                // we are stealing stuff
                model = std::make_unique<PickpocketItemModel>(mPtr, std::make_unique<InventoryItemModel>(container),
                    !mPtr.getClass().getCreatureStats(mPtr).getKnockedDown());
            }
            else
                model = std::make_unique<InventoryItemModel>(container);
        }
        else
        {
            // Isowind: crates stacked on this one or right beside it (hard to tell apart from above) open with it, as
            // one; which ones is up to opensWith
            mNearbyContainers = findNearbyContainers(mPtr);
            if (mNearbyContainers.empty())
                model = std::make_unique<ContainerItemModel>(container);
            else
            {
                std::vector<MWWorld::Ptr> sources{ container };
                sources.insert(sources.end(), mNearbyContainers.begin(), mNearbyContainers.end());
                auto mergedModel = std::make_unique<ContainerItemModel>(sources);
                mMergedModel = mergedModel.get();
                model = std::move(mergedModel);
            }
        }
        // (shown afresh, it stays on its tab)
        updateTabs(refresh);

        mDisposeCorpseButton->setVisible(loot);
        mModel = model.get();
        auto sortModel = std::make_unique<SortFilterItemModel>(std::move(model));
        mSortModel = sortModel.get();

        mItemView->setModel(std::move(sortModel));
        mItemView->resetScrollBars();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        setTitle(container.getClass().getName(container));
    }

    void ContainerWindow::resetReference()
    {
        ReferenceInterface::resetReference();
        mItemView->setModel(nullptr);
        mModel = nullptr;
        mSortModel = nullptr;
        mMergedModel = nullptr;
        mNearbyContainers.clear();
        clearTabs();
    }

    void ContainerWindow::onOpen()
    {
        mItemTransfer->addTarget(*mItemView);
    }

    void ContainerWindow::onClose()
    {
        // Make sure the window was actually closed and not temporarily hidden.
        if (MWBase::Environment::get().getWindowManager()->containsMode(GM_Container))
            return;

        if (mModel)
            mModel->onClose();

        if (!mPtr.isEmpty())
            MWBase::Environment::get().getMechanicsManager()->onClose(mPtr);
        resetReference();
        if (mwmp::hooks::isMultiplayer())
            mwmp::Main::get().getLocalPlayer()->clearCurrentContainer();

        mItemTransfer->removeTarget(*mItemView);
    }

    void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* /*sender*/)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onTakeAllButtonClicked(MyGUI::Widget* /*sender*/)
    {
        if (!mModel)
            return;
        if (mDragAndDrop != nullptr && mDragAndDrop->mIsOnDragAndDrop)
            return;

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        // Isowind: from one container's tab (of several opened together) it takes what that one holds, and the window
        // stays open on it
        const bool keepOpen = mTab != 0;
        mTakeAllKeepsOpen = keepOpen;

        // Isowind multiplayer: the server hands everything over (and the window closes when it does)
        if (mwmp::hooks::isMultiplayer())
        {
            mModel->update();
            std::vector<std::pair<MWWorld::Ptr, int>> items;
            for (size_t i = 0; i < mModel->getItemCount(); ++i)
            {
                const ItemStack item = mModel->getItem(static_cast<ItemModel::ModelIndex>(i));
                // (crimes for taking them, as usual)
                if (!mModel->onTakeItem(item.mBase, static_cast<int>(item.mCount)))
                    break;
                items.emplace_back(item.mBase, static_cast<int>(item.mCount));
            }
            // (from each container the window shows; the first reply closes it)
            const auto bySource = splitBySource(items);
            for (const auto& [container, containerItems] : bySource)
                mwmp::hooks::sendContainerChange(
                    container, mwmp::BaseObjectList::REMOVE, mwmp::BaseObjectList::TAKE_ALL, containerItems);
            if (bySource.empty() && !keepOpen)
                MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
            return;
        }

        // transfer everything into the player's inventory
        ItemModel* playerModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getModel();
        assert(mModel);
        mModel->update();

        // unequip all items to avoid unequipping/reequipping
        if (mPtr.getClass().hasInventoryStore(mPtr))
        {
            MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);
            for (size_t i = 0; i < mModel->getItemCount(); ++i)
            {
                const ItemStack& item = mModel->getItem(static_cast<ItemModel::ModelIndex>(i));
                if (invStore.isEquipped(item.mBase) == false)
                    continue;

                invStore.unequipItem(item.mBase);
            }
        }

        mModel->update();

        for (size_t i = 0; i < mModel->getItemCount(); ++i)
        {
            if (i == 0)
            {
                // play the sound of the first object
                MWWorld::Ptr item = mModel->getItem(static_cast<ItemModel::ModelIndex>(i)).mBase;
                const ESM::RefId& sound = item.getClass().getUpSoundId(item);
                MWBase::Environment::get().getWindowManager()->playSound(sound);
            }

            const ItemStack item = mModel->getItem(static_cast<ItemModel::ModelIndex>(i));

            if (!mModel->onTakeItem(item.mBase, static_cast<int>(item.mCount)))
                break;

            mModel->moveItem(item, item.mCount, playerModel);
        }

        if (keepOpen)
        {
            mItemView->update();
            MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updateItemView();
            return;
        }
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onDisposeCorpseButtonClicked(MyGUI::Widget* /*sender*/)
    {
        if (mDragAndDrop == nullptr || !mDragAndDrop->mIsOnDragAndDrop)
        {
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

            // Copy mPtr because onTakeAllButtonClicked closes the window which resets the reference
            MWWorld::Ptr ptr = mPtr;
            onTakeAllButtonClicked(mTakeButton);

            if (ptr.getClass().isPersistent(ptr))
                MWBase::Environment::get().getWindowManager()->messageBox("#{sDisposeCorpseFail}");
            // Isowind multiplayer: the server removes the body, for everyone
            else if (mwmp::hooks::isMultiplayer())
                mwmp::hooks::sendObjectDelete(ptr);
            else
            {
                MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);

                // If we dispose corpse before end of death animation, we should update death counter counter manually.
                // Also we should run actor's script - it may react on actor's death.
                if (creatureStats.isDead() && !creatureStats.isDeathAnimationFinished())
                {
                    creatureStats.setDeathAnimationFinished(true);
                    MWBase::Environment::get().getMechanicsManager()->notifyDied(ptr);

                    const ESM::RefId& script = ptr.getClass().getScript(ptr);
                    if (!script.empty() && MWBase::Environment::get().getWorld()->getScriptsEnabled())
                    {
                        MWScript::InterpreterContext interpreterContext(&ptr.getRefData().getLocals(), ptr);
                        MWBase::Environment::get().getScriptManager()->run(script, interpreterContext);
                    }

                    // Clean up summoned creatures as well
                    auto& creatureMap = creatureStats.getSummonedCreatureMap();
                    for (const auto& creature : creatureMap)
                        MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(creature.second);
                    creatureMap.clear();

                    // Check if we are a summon and inform our master we've bit the dust
                    for (const auto& package : creatureStats.getAiSequence())
                    {
                        if (package->followTargetThroughDoors() && !package->getTarget().isEmpty())
                        {
                            const auto& summoner = package->getTarget();
                            auto& summons = summoner.getClass().getCreatureStats(summoner).getSummonedCreatureMap();
                            auto it = std::find_if(summons.begin(), summons.end(),
                                [&](const auto& entry) { return entry.second == ptr.getCellRef().getRefNum(); });
                            if (it != summons.end())
                            {
                                auto summon = *it;
                                summons.erase(it);
                                MWMechanics::purgeSummonEffect(summoner, summon);
                                break;
                            }
                        }
                    }
                }

                MWBase::Environment::get().getWorld()->deleteObject(ptr);
            }

            mPtr = MWWorld::Ptr();
        }
    }

    void ContainerWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onDeleteCustomData(const MWWorld::Ptr& ptr)
    {
        if (mModel && mModel->usesContainer(ptr))
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    ControllerButtons* ContainerWindow::getControllerButtons()
    {
        // Isowind: the tabs of containers opened together (never with a body to dispose of)
        if (!mTabButtons.empty())
        {
            mControllerButtons.mL1 = "#{Interface:Prev}";
            mControllerButtons.mR1 = "#{Interface:Next}";
            return &mControllerButtons;
        }
        mControllerButtons.mL1.clear();
        if (mDisposeCorpseButton->getVisible())
            mControllerButtons.mR1 = "#{Interface:DisposeOfCorpse}";
        else
            mControllerButtons.mR1.clear();
        return &mControllerButtons;
    }

    bool ContainerWindow::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            int index = mItemView->getControllerFocus();
            if (index >= 0 && index < mItemView->getItemCount())
                onItemSelected(index);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onCloseButtonClicked(mCloseButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_X)
        {
            onTakeAllButtonClicked(mTakeButton);
        }
        else if (!mTabButtons.empty()
            && (arg.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER || arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
        {
            // Isowind: the tabs (the triggers, where the bumpers switch windows: they come here as the bumpers)
            stepTab(arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ? 1 : -1);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
        {
            if (mDisposeCorpseButton->getVisible())
                onDisposeCorpseButtonClicked(mDisposeCorpseButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_RIGHTSTICK || arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP
            || arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN || arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT
            || arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            mItemView->onControllerButton(arg.button);
        }

        return true;
    }

    void ContainerWindow::setActiveControllerWindow(bool active)
    {
        mItemView->setActiveControllerWindow(active);
        WindowBase::setActiveControllerWindow(active);
    }

    void ContainerWindow::onFrame(float dt)
    {
        checkReferenceAvailable();

        // Isowind: one of those opened with it went away (disabled, deleted): the window shows what's left
        if (!mPtr.isEmpty()
            && std::any_of(mNearbyContainers.begin(), mNearbyContainers.end(), [](const MWWorld::Ptr& container) {
                   return container.mRef->isDeleted() || !container.getRefData().isEnabled();
               }))
            setPtr(mPtr);

        if (mUpdateNextFrame)
        {
            mItemView->update();
            mUpdateNextFrame = false;
        }
    }

    void ContainerWindow::onInventoryUpdate(const MWWorld::Ptr& ptr)
    {
        // (or one of those opened with it)
        if (ptr == mPtr
            || std::find(mNearbyContainers.begin(), mNearbyContainers.end(), ptr) != mNearbyContainers.end())
            mUpdateNextFrame = true;
    }

    bool ContainerWindow::isOnDragAndDrop() const
    {
        return mDragAndDrop->mIsOnDragAndDrop;
    }

    void ContainerWindow::onContainerChanged(const MWWorld::Ptr& container)
    {
        // (not opened afresh on that one: it may be one of those opened with the window's own; the window may have lost
        // its own, a body being disposed of)
        setPtr(mPtr.isEmpty() ? container : mPtr);
    }

    std::vector<std::pair<MWWorld::Ptr, ContainerWindow::ItemCounts>> ContainerWindow::splitBySource(
        const ItemCounts& items)
    {
        std::vector<std::pair<MWWorld::Ptr, ItemCounts>> bySource;
        auto add = [&](const MWWorld::Ptr& container, const MWWorld::Ptr& item, int count) {
            auto found = std::find_if(
                bySource.begin(), bySource.end(), [&](const auto& entry) { return entry.first == container; });
            if (found == bySource.end())
                found = bySource.emplace(bySource.end(), container, ItemCounts());
            found->second.emplace_back(item, count);
        };

        for (const auto& [item, count] : items)
        {
            if (mMergedModel == nullptr)
                add(mPtr, item, count);
            else
            {
                // (each container's own item: the server is told how many of it there are)
                for (const ContainerItemModel::SourceItem& source : mMergedModel->getSourceItems(item, count))
                    add(source.mContainer, source.mItem, source.mCount);
            }
        }
        return bySource;
    }

    void ContainerWindow::updateTabs(bool keepTab)
    {
        if (mMergedModel == nullptr)
        {
            clearTabs();
            return;
        }

        const bool hadTabs = !mTabButtons.empty();
        std::vector<MWWorld::Ptr> containers{ mPtr };
        containers.insert(containers.end(), mNearbyContainers.begin(), mNearbyContainers.end());
        // Made again only when the containers change, not each time the server changes what one holds
        if (!hadTabs || containers != mTabContainers)
        {
            for (MyGUI::Button* button : mTabButtons)
                MyGUI::Gui::getInstance().destroyWidget(button);
            mTabButtons.clear();
            mTabContainers = containers;

            auto addTab = [&]() {
                MyGUI::Button* button = mTabBar->createWidget<Gui::AutoSizedButton>(
                    "MW_Button", MyGUI::IntCoord(0, 0, 60, sTabHeight), MyGUI::Align::Left | MyGUI::Align::Top);
                button->setNeedKeyFocus(false);
                button->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onTabClicked);
                button->eventMouseWheel += MyGUI::newDelegate(this, &ContainerWindow::onTabWheel);
                mTabButtons.push_back(button);
                return button;
            };
            addTab()->setCaptionWithReplacing("#{sAllTab}");

            // Named by their containers; those with the same name are numbered, nearest first
            std::vector<std::string> names;
            for (const MWWorld::Ptr& container : containers)
                names.emplace_back(container.getClass().getName(container));
            for (size_t i = 0; i < names.size(); ++i)
            {
                std::string caption = names[i];
                if (std::count(names.begin(), names.end(), names[i]) > 1)
                    caption += " " + std::to_string(std::count(names.begin(), names.begin() + i + 1, names[i]));
                addTab()->setCaption(MyGUI::TextIterator::toTagsString(caption));
            }
        }

        // On the tab it was on if its container is still there, else on "All"
        mTab = 0;
        if (keepTab && !mTabContainer.isEmpty())
        {
            auto found = std::find(mTabContainers.begin(), mTabContainers.end(), mTabContainer);
            if (found != mTabContainers.end())
                mTab = static_cast<size_t>(found - mTabContainers.begin()) + 1;
        }
        mTabContainer = mTab == 0 ? MWWorld::Ptr() : mTabContainers[mTab - 1];
        mMergedModel->setShownSource(static_cast<int>(mTab) - 1);
        for (size_t i = 0; i < mTabButtons.size(); ++i)
            mTabButtons[i]->setStateSelected(i == mTab);

        setTabsVisible(true);
        layoutTabs();
        // (the controller's guide shows the tabs' buttons)
        if (!hadTabs)
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
    }

    void ContainerWindow::clearTabs()
    {
        const bool hadTabs = !mTabButtons.empty();
        for (MyGUI::Button* button : mTabButtons)
            MyGUI::Gui::getInstance().destroyWidget(button);
        mTabButtons.clear();
        mTabContainers.clear();
        mTabContainer = MWWorld::Ptr();
        mTab = 0;
        mTakeAllKeepsOpen = false;
        setTabsVisible(false);
        if (hadTabs)
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
    }

    void ContainerWindow::selectTab(size_t tab)
    {
        if (mMergedModel == nullptr || tab >= mTabButtons.size() || tab == mTab)
            return;

        mTab = tab;
        mTabContainer = mTab == 0 ? MWWorld::Ptr() : mTabContainers[mTab - 1];
        mMergedModel->setShownSource(static_cast<int>(mTab) - 1);
        for (size_t i = 0; i < mTabButtons.size(); ++i)
            mTabButtons[i]->setStateSelected(i == mTab);
        layoutTabs();

        mItemView->update();
        mItemView->resetScrollBars();
    }

    void ContainerWindow::layoutTabs()
    {
        if (mTabButtons.empty())
            return;

        // As many tabs as fit, around the one shown (always that one), with arrows when some don't
        std::vector<int> widths;
        int total = 0;
        for (MyGUI::Button* button : mTabButtons)
        {
            widths.push_back(static_cast<Gui::AutoSizedButton*>(button)->getRequestedSize().width);
            total += widths.back() + (widths.size() > 1 ? sTabSpacing : 0);
        }
        int available = mTabBar->getWidth();
        if (mPrevTabImage)
            available -= 2 * (sTabHeight + sTabSpacing);

        size_t first = 0;
        size_t last = mTabButtons.size() - 1;
        const bool overflow = total > available;
        if (overflow)
        {
            available -= static_cast<Gui::AutoSizedButton*>(mPrevTabButton)->getRequestedSize().width
                + static_cast<Gui::AutoSizedButton*>(mNextTabButton)->getRequestedSize().width + 2 * sTabSpacing;
            first = last = mTab;
            int used = widths[mTab];
            bool grew = true;
            while (grew)
            {
                grew = false;
                if (last + 1 < widths.size() && used + sTabSpacing + widths[last + 1] <= available)
                {
                    used += sTabSpacing + widths[++last];
                    grew = true;
                }
                if (first > 0 && used + sTabSpacing + widths[first - 1] <= available)
                {
                    used += sTabSpacing + widths[--first];
                    grew = true;
                }
            }
        }

        int x = 0;
        auto place = [&](MyGUI::Widget* widget, int width) {
            widget->setVisible(true);
            widget->setCoord(x, 0, width, sTabHeight);
            x += width + sTabSpacing;
        };
        if (mPrevTabImage)
            place(mPrevTabImage, sTabHeight);
        mPrevTabButton->setVisible(false);
        if (overflow)
            place(mPrevTabButton, static_cast<Gui::AutoSizedButton*>(mPrevTabButton)->getRequestedSize().width);
        for (size_t i = 0; i < mTabButtons.size(); ++i)
        {
            if (i < first || i > last)
                mTabButtons[i]->setVisible(false);
            else
                place(mTabButtons[i], widths[i]);
        }
        mNextTabButton->setVisible(false);
        if (overflow)
            place(mNextTabButton, static_cast<Gui::AutoSizedButton*>(mNextTabButton)->getRequestedSize().width);
        if (mNextTabImage)
            place(mNextTabImage, sTabHeight);
    }

    void ContainerWindow::setTabsVisible(bool visible)
    {
        mTabBar->setVisible(visible);
        // The items below the tabs, or where they were
        const int top = visible ? mItemViewTop + sTabHeight + sTabSpacing : mItemViewTop;
        MyGUI::IntCoord coord = mItemView->getCoord();
        if (coord.top == top)
            return;
        coord.height += coord.top - top;
        coord.top = top;
        // (as a widget: the item view lays itself out again)
        static_cast<MyGUI::Widget*>(mItemView)->setCoord(coord);
    }

    void ContainerWindow::onTabClicked(MyGUI::Widget* sender)
    {
        auto found = std::find(mTabButtons.begin(), mTabButtons.end(), sender);
        if (found != mTabButtons.end())
            selectTab(static_cast<size_t>(found - mTabButtons.begin()));
    }

    void ContainerWindow::onPrevTabClicked(MyGUI::Widget* /*sender*/)
    {
        selectTab(wrap(mTab, mTabButtons.size(), -1));
    }

    void ContainerWindow::onNextTabClicked(MyGUI::Widget* /*sender*/)
    {
        selectTab(wrap(mTab, mTabButtons.size(), 1));
    }

    bool ContainerWindow::stepTab(int delta)
    {
        if (mTabButtons.empty())
            return false;
        selectTab(wrap(mTab, mTabButtons.size(), delta));
        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        return true;
    }

    void ContainerWindow::onTabWheel(MyGUI::Widget* /*sender*/, int rel)
    {
        if (!mTabButtons.empty() && rel != 0)
            selectTab(wrap(mTab, mTabButtons.size(), rel > 0 ? -1 : 1));
    }

    void ContainerWindow::onWindowResize(MyGUI::Window* /*window*/)
    {
        layoutTabs();
    }

    bool ContainerWindow::dragItemByPtr(const MWWorld::Ptr& itemPtr, int dragCount)
    {
        for (size_t i = 0; i < mModel->getItemCount(); ++i)
        {
            if (mModel->getItem(i).mBase == itemPtr)
            {
                mDragAndDrop->startDrag(static_cast<int>(i), mSortModel, mModel, mItemView, dragCount);
                return true;
            }
        }
        return false;
    }
}
