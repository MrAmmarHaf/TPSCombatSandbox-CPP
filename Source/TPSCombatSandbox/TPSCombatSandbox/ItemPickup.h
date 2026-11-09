#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UItemDefinition;

UCLASS()
class TPSCOMBATSANDBOX_API AItemPickup : public AActor
{
    GENERATED_BODY()

public:
    AItemPickup();

    UFUNCTION(BlueprintPure, Category = "Item")
    UItemDefinition* GetItemDefinition() const
    {
        return ItemDefinition;
    }

protected:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    USphereComponent* PickupSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    USkeletalMeshComponent* ItemMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    UItemDefinition* ItemDefinition = nullptr;
};
