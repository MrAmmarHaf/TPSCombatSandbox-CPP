// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"


#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

//Collision Sphere
#include "Components/SphereComponent.h"

//Fire Anim
#include "Animation/AnimSequence.h"

//Sound
#include "Sound/SoundBase.h"

//Should always be last
#include "CombatWeapon.generated.h"


UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None UMETA(DisplayName = "None"),
	Pistol UMETA(DisplayName = "Pistol"),
	Rifle UMETA(DisplayName = "Rifle")
};

UCLASS()
class TPSCOMBATSANDBOX_API ACombatWeapon : public AActor
{
	GENERATED_BODY()
	





public:	
	// Sets default values for this actor's properties
	ACombatWeapon();








protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Root Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* Root;

	//Skeletal mesh component for gun
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh;


	//Collision Sphere
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USphereComponent* PickupSphere;


	//Fire Animation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	UAnimSequence* FireAnimation;



	//SphereCollision for pickup
	UFUNCTION()
	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	



public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	USphereComponent* GetPickupSphere() const { return PickupSphere; }
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }





	//Weapon Stats --> Refactoring Weapon Code 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Stats")
	float Damage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Damage")
	float HeadSHotMultiplier = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Stats")
	float FireDelay = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Stats")
	bool bIsAutomatic = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon | Stats")
	float RecoilPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Audio")
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Animation")
	UAnimMontage* FireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Animation")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::None;


	//Muzzle
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AActor> MuzzleFlashClass;

	//EmptyGun sound
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	USoundBase* EmptyClickSound;

	//Reload Sound
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	USoundBase* ReloadSound;


	void PlayFireAnimation();



	//Ammo
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 MagazineSize = 15;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 CurrentAmmo = 15;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 ReserveMagazines = 2;

	bool CanFire() const;
	bool CanReload() const;

	void ConsumeAmmo();
	void ReloadWeapon();
	void AddReserveMagazines(int32 Amount);


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName EquipSocketName = TEXT("WeaponSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName HolsterSocketName = TEXT("HolsterSocket");
	




};
