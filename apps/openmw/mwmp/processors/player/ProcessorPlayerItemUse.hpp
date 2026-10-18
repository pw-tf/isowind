#ifndef OPENMW_PROCESSORPLAYERITEMUSE_HPP
#define OPENMW_PROCESSORPLAYERITEMUSE_HPP

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwgui/inventorywindow.hpp"
#include "apps/openmw/mwgui/windowmanagerimp.hpp"
#include "apps/openmw/mwworld/inventorystore.hpp"

#include "apps/openmw/mwmp/MechanicsHelper.hpp"

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerItemUse final: public PlayerProcessor
    {
    public:
        ProcessorPlayerItemUse()
        {
            BPP_INIT(ID_PLAYER_ITEM_USE)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (!isLocal()) return;

            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_ITEM_USE about LocalPlayer from server");

            if (!isRequest())
            {
                LOG_APPEND(TimedLog::LOG_INFO, "- refId: %s, count: %i, charge: %i, enchantmentCharge: %f, soul: %s",
                    player->usedItem.refId.c_str(), player->usedItem.count, player->usedItem.charge,
                    player->usedItem.enchantmentCharge, player->usedItem.soul.c_str());

                MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                MWWorld::InventoryStore &inventoryStore = playerPtr.getClass().getInventoryStore(playerPtr);

                MWWorld::Ptr itemPtr = MechanicsHelper::getItemPtrFromStore(player->usedItem, inventoryStore);

                if (!itemPtr.isEmpty())
                {
                    MWBase::Environment::get().getWindowManager()->getInventoryWindow()->useItem(itemPtr);

                    if (player->usingItemMagic)
                    {
                        MWWorld::ContainerStoreIterator storeIterator = inventoryStore.begin();
                        for (; storeIterator != inventoryStore.end(); ++storeIterator)
                        {
                            if (*storeIterator == itemPtr)
                                break;
                        }

                        inventoryStore.setSelectedEnchantItem(storeIterator);
                    }

                    if (player->itemUseDrawState != static_cast<char>(MWMechanics::DrawState::Nothing))
                        playerPtr.getClass().getNpcStats(playerPtr).setDrawState(static_cast<MWMechanics::DrawState>(player->itemUseDrawState));
                }
                else
                    LOG_MESSAGE_SIMPLE(TimedLog::LOG_ERROR, "Cannot use non-existent item %s", player->usedItem.refId.c_str());
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERITEMUSE_HPP
