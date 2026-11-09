#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;


// ============================================================
// RPG INVENTORY ENTRY
// ============================================================

USTRUCT(BlueprintType)
struct FInventoryEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    UItemDefinition* Item = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    int32 Quantity = 0;
};


// ============================================================
// OLD INVENTORY TYPE
// Temporary - bandages still use this.
// We will remove this later after bandages are migrated.
// ============================================================

UENUM(BlueprintType)
enum class EInventoryItemType : uint8
{
    Bandage,
    Medicine,
    Food,
    Supplies
};


// ============================================================
// INVENTORY COMPONENT
// ============================================================

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPSCOMBATSANDBOX_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UInventoryComponent();


    // --------------------------------------------------------
    // OLD INVENTORY SYSTEM
    // --------------------------------------------------------

    bool AddItem(EInventoryItemType ItemType, int32 Amount = 1);

    bool RemoveItem(EInventoryItemType ItemType, int32 Amount = 1);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(EInventoryItemType ItemType) const;


    // --------------------------------------------------------
    // NEW RPG INVENTORY SYSTEM
    // --------------------------------------------------------

    bool AddItemDefinition(
        UItemDefinition* ItemDefinition,
        int32 Amount = 1
    );

    bool RemoveItemDefinition(
        UItemDefinition* ItemDefinition,
        int32 Amount = 1
    );

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemDefinitionCount(
        UItemDefinition* ItemDefinition
    ) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    UItemDefinition* GetItemDefinitionAtIndex(int32 Index) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemQuantityAtIndex(int32 Index) const;

protected:

    // --------------------------------------------------------
    // OLD INVENTORY DATA
    // --------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
    int32 MaxBandages = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TMap<EInventoryItemType, int32> ItemCounts;


    // --------------------------------------------------------
    // NEW RPG INVENTORY DATA
    // --------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FInventoryEntry> Items;
};