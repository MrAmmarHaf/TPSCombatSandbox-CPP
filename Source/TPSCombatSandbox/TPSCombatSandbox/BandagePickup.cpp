// Fill out your copyright notice in the Description page of Project Settings.


#include "BandagePickup.h"

// Sets default values
#include "BandagePickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CombatCharacter.h"


ABandagePickup::ABandagePickup()
{
    PrimaryActorTick.bCanEverTick = false;

    // Pickup collision
    PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    RootComponent = PickupSphere;

    PickupSphere->SetSphereRadius(100.0f);

    PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // Visible bandage mesh
    BandageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BandageMesh"));
    BandageMesh->SetupAttachment(PickupSphere);

    BandageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    

}




