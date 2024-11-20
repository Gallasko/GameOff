#include "inventory.h"

namespace pg
{
    void InventorySystem::onEvent(const GainItem& event)
    {
        auto item = event.item;

        if (item.stacksize == 1)
        {
            items[item.type].push_back(item);
            return;
        }

        auto it = std::find(items[item.type].rbegin(), items[item.type].rend(), item);

        if (it != items[item.type].rend())
        {
            // A stack size of -1 means it is infinite
            if (it->stacksize == -1)
            {
                it->nbItems += item.nbItems;
                return;
            }

            int remainingSize = it->nbItems - it->stacksize;

            if (remainingSize > 0)
            {
                if (static_cast<size_t>(remainingSize) > item.nbItems)
                {
                    it->nbItems += item.nbItems;
                    return; 
                }
                else
                {
                    it->nbItems += remainingSize;
                    item.nbItems -= remainingSize;
                }
            }
        }

        if (item.stacksize == -1)
        {
            items[item.type].push_back(item);
            return;
        }

        do
        {
            auto itemToPush = item;

            if (item.nbItems >= static_cast<size_t>(item.stacksize))
            {
                itemToPush.nbItems = item.stacksize;
            }

            items[item.type].push_back(item);

            item.nbItems -= item.stacksize;
        }
        while (item.nbItems > 0);

    }

    void InventorySystem::onEvent(const LoseItem& event)
    {
        auto item = event.item;

        auto it = std::find(items[item.type].rbegin(), items[item.type].rend(), item);

        if (it == items[item.type].rend())
        {
            LOG_ERROR("INVENTORY", "Requested removing of an item that is not in the inventory: " << item.name);
            return;
        }

        it->nbItems -= 1;

        if (it->nbItems <= 0)
        {
            items[item.type].erase(std::next(it).base());
        }
    }
}