// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatWeapon.h"

#include "CombatCharacter.h"

// Sets default values
ACombatWeapon::ACombatWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.

	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(Root);




	//Sphere PickUp
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(Root);
	PickupSphere->SetSphereRadius(150.0f);
	
	//Ignore everything and only pay attention to player
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);



	//Debugging PickupSphere
	PickupSphere->SetHiddenInGame(false);
	PickupSphere->ShapeColor = FColor::Green;

}

// Called when the game starts or when spawned
void ACombatWeapon::BeginPlay()
{
	Super::BeginPlay();
	

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ACombatWeapon::OnSphereBeginOverlap);
	PickupSphere->OnComponentEndOverlap.AddDynamic(this, &ACombatWeapon::OnSphereEndOverlap);
	
}

// Called every frame
void ACombatWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


//Sphere collision for pickup
void ACombatWeapon::OnSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(OtherActor);

	if (CombatCharacter)
	{
		CombatCharacter->SetOverlappingWeapon(this);
	}
}

void ACombatWeapon::OnSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(OtherActor);

	if (CombatCharacter)
	{
		CombatCharacter->SetOverlappingWeapon(nullptr);
	}
}

//Fire Animation
void ACombatWeapon::PlayFireAnimation()
{
	//Pistol firing debugging
	UE_LOG(LogTemp, Warning, TEXT("PlayFireAnimation called"));
	
	if (WeaponMesh && FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
}

//Ammo
bool ACombatWeapon::CanFire() const
{
	return CurrentAmmo > 0;
}

bool ACombatWeapon::CanReload() const {
	return CurrentAmmo < MagazineSize && ReserveMagazines > 0;
}

void ACombatWeapon::ConsumeAmmo() {
	if (CurrentAmmo > 0) {
		CurrentAmmo--;
	}
	UE_LOG(LogTemp, Warning, TEXT("Ammo: %d / %d mags left"), CurrentAmmo, ReserveMagazines);
}

void ACombatWeapon::ReloadWeapon() {
	
	if (!CanReload()) return;

	ReserveMagazines--;
	CurrentAmmo = MagazineSize;
	

}

void ACombatWeapon::AddReserveMagazines(int32 Amount) {
	ReserveMagazines += Amount;

	UE_LOG(LogTemp, Warning, TEXT("Picked up ammo. Reserve mags: %d"), ReserveMagazines);
}
