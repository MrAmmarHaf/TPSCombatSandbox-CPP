// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CombatCharacter.h"

// Sets default values
AAmmoPickup::AAmmoPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(Root);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(Root);
	PickupSphere->SetSphereRadius(120.0f);

	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned
void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	
	PickupSphere->OnComponentBeginOverlap.AddDynamic(
		this,
		&AAmmoPickup::OnSphereBeginOverlap
	);
}



void AAmmoPickup::OnSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(OtherActor);

	if (CombatCharacter) {
		CombatCharacter->AddAmmo(MagazinesToAdd);
		Destroy();
	}
}