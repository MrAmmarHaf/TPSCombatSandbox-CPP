#include "Backpack.h"

#include "InventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

ABackpack::ABackpack()
{
    PrimaryActorTick.bCanEverTick = false;


    InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));


    // Area where the player can interact with the backpack.
    PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));

    RootComponent = PickupSphere;

    PickupSphere->SetSphereRadius(100.0f);

    PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    PickupSphere->SetCollisionResponseToAllChannels( ECR_Ignore);

    PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // Visible backpack.
    BackpackMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BackpackMesh"));

    BackpackMesh->SetupAttachment(PickupSphere);

    BackpackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}
