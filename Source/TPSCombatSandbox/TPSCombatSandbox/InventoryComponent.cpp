#include "InventoryComponent.h"
#include "ItemDefinition.h"


UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}


bool UInventoryComponent::AddItem(
    EInventoryItemType ItemType,
    int32 Amount)
{
    if (Amount <= 0)
    {
        return false;
    }

    int32& CurrentAmount =
        ItemCounts.FindOrAdd(ItemType);

    // Bandages have a carrying limit.
    if (ItemType == EInventoryItemType::Bandage)
    {
        if (CurrentAmount + Amount > MaxBandages)
        {
            return false;
        }
    }

    CurrentAmount += Amount;

    return true;
}


bool UInventoryComponent::RemoveItem(
    EInventoryItemType ItemType,
    int32 Amount)
{
    if (Amount <= 0)
    {
        return false;
    }

    int32* CurrentAmount =
        ItemCounts.Find(ItemType);

    if (CurrentAmount == nullptr)
    {
        return false;
    }

    if (*CurrentAmount < Amount)
    {
        return false;
    }

    *CurrentAmount -= Amount;

    if (*CurrentAmount <= 0)
    {
        ItemCounts.Remove(ItemType);
    }

    return true;
}


int32 UInventoryComponent::GetItemCount(
    EInventoryItemType ItemType) const
{
    const int32* CurrentAmount =
        ItemCounts.Find(ItemType);

    if (CurrentAmount == nullptr)
    {
        return 0;
    }

    return *CurrentAmount;
}

// ============================================================
// NEW RPG INVENTORY SYSTEM
// ============================================================

bool UInventoryComponent::AddItemDefinition(
    UItemDefinition* ItemDefinition,
    int32 Amount)
{
    if (ItemDefinition == nullptr || Amount <= 0)
    {
        return false;
    }

    // Non-stackable equipment can only exist once.
    if (!ItemDefinition->bStackable)
    {
        if (Amount > 1)
        {
            return false;
        }

        if (GetItemDefinitionCount(ItemDefinition) > 0)
        {
            return false;
        }
    }

    // Check whether this item is already in the inventory.
    for (FInventoryEntry& Entry : Items)
    {
        if (Entry.Item == ItemDefinition)
        {
            if (!ItemDefinition->bStackable)
            {
                return false;
            }

            if (Entry.Quantity + Amount > ItemDefinition->MaxStackSize)
            {
                return false;
            }

            Entry.Quantity += Amount;
            return true;
        }
    }

    // Item does not exist yet, so create a new inventory entry.
    if (ItemDefinition->bStackable &&
        Amount > ItemDefinition->MaxStackSize)
    {
        return false;
    }

    FInventoryEntry NewEntry;

    NewEntry.Item = ItemDefinition;
    NewEntry.Quantity = Amount;

    Items.Add(NewEntry);

    return true;
}


bool UInventoryComponent::RemoveItemDefinition(
    UItemDefinition* ItemDefinition,
    int32 Amount)
{
    if (ItemDefinition == nullptr || Amount <= 0)
    {
        return false;
    }

    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        FInventoryEntry& Entry = Items[Index];

        if (Entry.Item != ItemDefinition)
        {
            continue;
        }

        if (Entry.Quantity < Amount)
        {
            return false;
        }

        Entry.Quantity -= Amount;

        // If none remain, remove the inventory entry entirely.
        if (Entry.Quantity <= 0)
        {
            Items.RemoveAt(Index);
        }

        return true;
    }

    return false;
}


int32 UInventoryComponent::GetItemDefinitionCount(
    UItemDefinition* ItemDefinition) const
{
    if (ItemDefinition == nullptr)
    {
        return 0;
    }

    for (const FInventoryEntry& Entry : Items)
    {
        if (Entry.Item == ItemDefinition)
        {
            return Entry.Quantity;
        }
    }

    return 0;
}

UItemDefinition* UInventoryComponent::GetItemDefinitionAtIndex(
    int32 Index) const
{
    if (!Items.IsValidIndex(Index))
    {
        return nullptr;
    }

    return Items[Index].Item;
}


int32 UInventoryComponent::GetItemQuantityAtIndex(
    int32 Index) const
{
    if (!Items.IsValidIndex(Index))
    {
        return 0;
    }

    return Items[Index].Quantity;
}