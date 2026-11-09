// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmmoPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class TPSCOMBATSANDBOX_API AAmmoPickup : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAmmoPickup();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo Pickup")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo Pickup")
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo Pickup")
	USphereComponent* PickupSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo Pickup")
	int32 MagazinesToAdd = 1;


	UFUNCTION()
	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

public:	
	// Called every frame
	

};
