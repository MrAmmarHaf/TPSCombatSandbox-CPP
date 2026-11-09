// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BandagePickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class TPSCOMBATSANDBOX_API ABandagePickup : public AActor
{
	GENERATED_BODY()
	
public:	

	ABandagePickup();

    int32 GetBandageAmount() const {
        return BandageAmount;
    }


protected:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bandage")
    UStaticMeshComponent* BandageMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bandage")
    USphereComponent* PickupSphere;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bandage")
    int32 BandageAmount = 1;


};
