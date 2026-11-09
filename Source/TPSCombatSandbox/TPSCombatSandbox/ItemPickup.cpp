#include "ItemPickup.h"

#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"


AItemPickup::AItemPickup()
{
    PrimaryActorTick.bCanEverTick = false;

    PickupSphere =
        CreateDefaultSubobject<USphereComponent>(
            TEXT("PickupSphere")
        );

    RootComponent = PickupSphere;

    PickupSphere->SetSphereRadius(100.0f);

    PickupSphere->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly
    );

    PickupSphere->SetCollisionResponseToAllChannels(
        ECR_Ignore
    );

    PickupSphere->SetCollisionResponseToChannel(
        ECC_Pawn,
        ECR_Overlap
    );


    ItemMesh =
        CreateDefaultSubobject<USkeletalMeshComponent>(
            TEXT("ItemMesh")
        );

    ItemMesh->SetupAttachment(PickupSphere);

    ItemMesh->SetCollisionEnabled(
        ECollisionEnabled::NoCollision
    );
}
