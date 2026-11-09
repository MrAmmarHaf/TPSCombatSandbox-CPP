#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefinition.generated.h"

class UTexture2D;
class USkeletalMesh;
class UStaticMesh;

UENUM(BlueprintType)
enum class EItemType : uint8
{
    Consumable,
    Equipment,
    Ammo,
    Resource,
    QuestItem
};

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    None,
    Head,
    Torso,
    Legs
};

UCLASS(BlueprintType)
class TPSCOMBATSANDBOX_API UItemDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // Name shown to the player.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    FText ItemName;

    //Item Description
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
    FText Description;

    // Inventory icon.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    UTexture2D* Icon = nullptr;

    // General category of item.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    EItemType ItemType = EItemType::Resource;

    // Whether multiple copies can share one inventory slot.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
    bool bStackable = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory",
        meta = (EditCondition = "bStackable", ClampMin = "1"))
    int32 MaxStackSize = 1;

    // --------------------
    // Equipment
    // --------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

    // Mesh shown on the character when equipped.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    USkeletalMesh* EquippedSkeletalMesh = nullptr;

    // Static Mesh equipment, such as rigid armor or accessories
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    UStaticMesh* EquippedStaticMesh = nullptr;

    // Character socket this equipment attaches to.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    FName EquipSocketName = NAME_None;

    // 1.0 = normal damage, 1.10 = +10%.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats",
        meta = (ClampMin = "0.0"))
    float DamageMultiplier = 1.0f;

    //1.0 = normal max health, 1.50 = +50% max health
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats",
        meta = (ClampMin = "1.0"))
    float MaxHealthMultiplier = 1.0f;

};