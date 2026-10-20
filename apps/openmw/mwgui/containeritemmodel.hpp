#ifndef MWGUI_CONTAINER_ITEM_MODEL_H
#define MWGUI_CONTAINER_ITEM_MODEL_H

#include <utility>
#include <vector>

#include "itemmodel.hpp"

#include "../mwworld/containerstore.hpp"

namespace MWGui
{

    /// @brief The container item model supports multiple item sources, which are needed for
    /// making NPCs sell items from containers owned by them
    class ContainerItemModel : public ItemModel
    {
    public:
        ContainerItemModel(const std::vector<MWWorld::Ptr>& itemSources, const std::vector<MWWorld::Ptr>& worldItems);
        ///< @note The order of elements \a itemSources matters here. The first element has the highest priority for
        ///< removal,
        ///  while the last element will be used to add new items to.

        ContainerItemModel(const MWWorld::Ptr& source);

        /// Isowind: containers opened together as one (crates stacked on the one opened, see ContainerWindow::setPtr).
        /// The first is the one opened: items put in go to it, and items are taken from it first.
        explicit ContainerItemModel(const std::vector<MWWorld::Ptr>& itemSources);

        /// Isowind: one container's share of an item stack
        struct SourceItem
        {
            MWWorld::Ptr mContainer;
            MWWorld::Ptr mItem; ///< The container's own item (that stacks with the one asked about)
            int mCount; ///< How many of those taken come from it
        };

        /// Isowind: where \a count of \a item come from when taken (in the order removeItem takes them)
        std::vector<SourceItem> getSourceItems(const MWWorld::Ptr& item, int count);

        bool allowedToUseItems() const override;

        bool onDropItem(const MWWorld::Ptr& item, int count) override;
        bool onTakeItem(const MWWorld::Ptr& item, int count) override;

        ItemStack getItem(ModelIndex index) override;
        ModelIndex getIndex(const ItemStack& item) override;
        size_t getItemCount() override;

        void update() override;

        bool usesContainer(const MWWorld::Ptr& container) override;

    protected:
        MWWorld::Ptr addItem(const ItemStack& item, size_t count, bool allowAutoEquip = true) override;
        MWWorld::Ptr copyItem(const ItemStack& item, size_t count, bool allowAutoEquip = true) override;
        void removeItem(const ItemStack& item, size_t count) override;

    private:
        std::vector<std::pair<MWWorld::Ptr, MWWorld::ResolutionHandle>> mItemSources;
        std::vector<MWWorld::Ptr> mWorldItems;
        const bool mTrading;
        std::vector<ItemStack> mItems;
    };

}

#endif
