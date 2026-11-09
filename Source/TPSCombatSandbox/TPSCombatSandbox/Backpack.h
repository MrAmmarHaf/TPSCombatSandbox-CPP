#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Backpack.generated.h"


class USphereComponent;
class USkeletalMeshComponent;
class UInventoryComponent;

UCLASS()
class TPSCOMBATSANDBOX_API ABackpack : public AActor
{
    GENERATED_BODY()

public:

    ABackpack();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backpack")
    FName EquipSocketName = TEXT("BackpackSocket");

    UFUNCTION(BlueprintPure, Category = "Backpack")
    UInventoryComponent* GetInventoryComponent() const
    {
        return InventoryComponent;
    }

protected:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backpack")
    USkeletalMeshComponent* BackpackMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backpack")
    USphereComponent* PickupSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backpack")
    UInventoryComponent* InventoryComponent;
};