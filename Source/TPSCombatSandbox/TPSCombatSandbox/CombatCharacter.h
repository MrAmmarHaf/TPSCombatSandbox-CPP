// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "CombatWeapon.h"
#include "Sound/SoundBase.h"

#include "CombatCharacter.generated.h"

class ABackpack;
class UAnimMontage;
class UAnimSequence;
class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UItemDefinition;
class USkeletalMeshComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EWeaponActionState : uint8
{
	None,
	PickingUp,
	Holstering,
	Unholstering,
	Switching,
	Reloading
};

UCLASS()
class TPSCOMBATSANDBOX_API ACombatCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACombatCharacter();

	// ============================================================================
	// ACharacter
	// ============================================================================
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ============================================================================
	// Interaction / External API
	// ============================================================================
	void SetOverlappingWeapon(ACombatWeapon* Weapon);
	void Interact();
	void EquipWeapon(ACombatWeapon* WeaponToEquip);
	void ReceiveDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void AddAmmo(int32 MagazineAmount);

	// ============================================================================
	// Animation Notify Callbacks
	// ============================================================================
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FinishWeaponPickup();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FinishWeaponSwitch();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FinishHolster();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FinishUnholster();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FinishReload();

	UFUNCTION(BlueprintCallable, Category = "Healing")
	void FinishUsingBandage();

	// ============================================================================
	// Blueprint State Getters
	// ============================================================================
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement")
	bool IsRunning() const { return bIsRunning; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	bool IsWeaponEquipped() const { return bIsWeaponEquipped; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	bool isAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "Combat | Peek")
	float GetCurrentPeekDirection() const { return CurrentPeekDirection; }

	// ============================================================================
	// Weapon State
	// ============================================================================
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	ACombatWeapon* PrimaryWeapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	ACombatWeapon* SecondaryWeapon = nullptr;

	ACombatWeapon* PendingWeaponToSwitchTo = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EWeaponType CurrentWeaponType = EWeaponType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EWeaponActionState WeaponActionState = EWeaponActionState::None;

	void EquipPrimaryWeapon();
	void EquipSecondaryWeapon();
	void SwitchToWeapon(ACombatWeapon* WeaponToSwitchTo);

	// ============================================================================
	// Health
	// ============================================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.0f;
	float BaseMaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Health")
	USoundBase* HitSound = nullptr;

	// ============================================================================
	// Weapon Action State Helpers
	// ============================================================================
	bool CanStartWeaponAction() const;
	void StartWeaponAction(EWeaponActionState NewState);
	void EndWeaponAction();

	// ============================================================================
	// Equipment
	// ============================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	USkeletalMeshComponent* EquippedHeadMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	UItemDefinition* EquippedHeadItem = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	UStaticMeshComponent* EquippedTorsoMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	UItemDefinition* EquippedTorsoItem = nullptr;


	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ToggleEquipItem(UItemDefinition* ItemDefinition);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsItemEquipped(UItemDefinition* ItemDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	float GetEquipmentDamageMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	float GetEquipmentMaxHealthMultiplier() const;


protected:
	virtual void BeginPlay() override;

	// ============================================================================
	// Components
	// ============================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera = nullptr;

	// ============================================================================
	// Enhanced Input
	// ============================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* PlayerMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* CrouchAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* AimAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* PeekLeftAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* PeekRightAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* FireAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ReloadAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* UnequipAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* PrimaryWeaponAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* SecondaryWeaponAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* InventoryAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* UseBandageAction = nullptr;

	// ============================================================================
	// Camera / ADS / Peek
	// ============================================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float DefaultCameraArmLength = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float AimCameraArmLength = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraInterpSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector DefaultSocketOffset = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector AimSocketOffset = FVector(0.0f, 90.0f, 35.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Peek")
	float PeekLeftSideOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Peek")
	float PeekRightSideOffset = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Peek")
	float PeekRollDegrees = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Peek")
	float PeekInterpSpeed = 8.0f;

	float CurrentPeekDirection = 0.0f;

	void AimPressed();
	void AimReleased();
	void PeekLeftPressed();
	void PeekRightPressed();

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsAiming = false;

	bool bReturnToCrouchAfterAim = false;

	// ============================================================================
	// Movement
	// ============================================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float WalkSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float RunSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float SprintSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float AimWalkSpeed = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float CrouchWalkSpeed = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float CrouchFastSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float DoubleTapTime = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsRunning = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bShiftHeld = false;

	float LastShiftTapTime = -999.0f;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void HandleSprintPressed();
	void HandleSprintReleased();
	void HandleCrouchPressed();
	void SetMovementSpeed(float NewSpeed);

	// ============================================================================
	// Backpack / Inventory
	// ============================================================================
	UPROPERTY(BlueprintReadOnly, Category = "Player | Backpack")
	ABackpack* EquippedBackpack = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY()
	UUserWidget* InventoryWidgetInstance = nullptr;

	bool bIsInventoryOpen = false;

	void EquipBackpack(ABackpack* Backpack);
	void ToggleInventory();

	void RefreshMaxHealthFromEquipment();

	// ============================================================================
	// Healing
	// ============================================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Healing")
	float BandageHealAmount = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* BandageMontage = nullptr;

	bool bIsUsingBandage = false;

	void UseInventoryBandage();
	void CancelUsingBandage();

	// ============================================================================
	// Weapon References / State
	// ============================================================================
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	ACombatWeapon* EquippedWeapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	ACombatWeapon* OverlappingWeapon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bIsWeaponEquipped = false;

	bool bCanFire = true;
	bool bIsUnholstering = false;

	// ============================================================================
	// Weapon Animations
	// ============================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* PickupMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* HolsterMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* WeaponSwapMontage = nullptr;

	// ============================================================================
	// Weapon Combat
	// ============================================================================
	void FirePressed();
	void FireReleased();
	void FireWeapon();
	void ResetFire();

	void ReloadWeapon();

	void ToggleEquipWeapon();
	void UnequipWeapon();
	void ReEquipWeapon();
	void EndUnholsterState();

	FTimerHandle FireTimerHandle;
	FTimerHandle AutoFireTimerHandle;
	FTimerHandle UnholsterTimerHandle;

	// ============================================================================
	// Damage / Death
	// ============================================================================
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* DeathAnimation = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation|Combat")
	UAnimMontage* HitReactionMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation|Combat")
	UAnimMontage* UnarmedHitReactionMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "VFX")
	TSubclassOf<AActor> BloodSplatterClass;

	void Die();
};
