// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatCharacter.h"

#include "ItemPickup.h"
#include "Backpack.h"
#include "BandagePickup.h"
#include "EnemyCharacter.h"
#include "InventoryComponent.h"
#include "ItemDefinition.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"




// ============================================================================
// Initialization
// ============================================================================

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = DefaultCameraArmLength;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 240.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchWalkSpeed;

	GetCharacterMovement()->MaxAcceleration = 1800.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1800.0f;
	GetCharacterMovement()->AirControl = 0.3f;
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->JumpZVelocity = 560.0f;
	GetCharacterMovement()->GravityScale = 1.25f;


	EquippedHeadMesh =
		CreateDefaultSubobject<USkeletalMeshComponent>(
			TEXT("EquippedHeadMesh")
		);

	EquippedHeadMesh->SetupAttachment(GetMesh());

	EquippedHeadMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	EquippedHeadMesh->SetVisibility(false);

	EquippedTorsoMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("EquippedTorsoMesh")
		);

	EquippedTorsoMesh->SetupAttachment(GetMesh());

	EquippedTorsoMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	EquippedTorsoMesh->SetVisibility(false);

}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
				PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(PlayerMappingContext, 0);
		}
	}

	SetMovementSpeed(WalkSpeed);

	bIsRunning = false;
	bIsSprinting = false;
	bShiftHeld = false;

	BaseMaxHealth = MaxHealth;
	CurrentHealth = MaxHealth;

}

void ACombatCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float TargetArmLength =
		bIsAiming ? AimCameraArmLength : DefaultCameraArmLength;

	FVector TargetSocketOffset =
		bIsAiming ? AimSocketOffset : DefaultSocketOffset;

	if (bIsAiming)
	{
		if (CurrentPeekDirection < 0.0f)
		{
			TargetSocketOffset.Y = -PeekLeftSideOffset;
		}
		else if (CurrentPeekDirection > 0.0f)
		{
			TargetSocketOffset.Y =
				FMath::Abs(AimSocketOffset.Y) + PeekRightSideOffset;
		}
	}

	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength,
		TargetArmLength,
		DeltaTime,
		CameraInterpSpeed
	);

	CameraBoom->SocketOffset = FMath::VInterpTo(
		CameraBoom->SocketOffset,
		TargetSocketOffset,
		DeltaTime,
		CameraInterpSpeed
	);

	const float TargetPeekRoll =
		bIsAiming ? CurrentPeekDirection * PeekRollDegrees : 0.0f;

	FRotator CameraRelativeRotation = FollowCamera->GetRelativeRotation();

	CameraRelativeRotation.Roll = FMath::FInterpTo(
		CameraRelativeRotation.Roll,
		TargetPeekRoll,
		DeltaTime,
		PeekInterpSpeed
	);

	FollowCamera->SetRelativeRotation(CameraRelativeRotation);
}


// ============================================================================
// Input
// ============================================================================

void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (EnhancedInputComponent == nullptr)
	{
		return;
	}

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ACombatCharacter::HandleSprintPressed);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ACombatCharacter::HandleSprintReleased);

	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ACombatCharacter::HandleCrouchPressed);
	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ACombatCharacter::Interact);

	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ACombatCharacter::AimPressed);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ACombatCharacter::AimReleased);

	EnhancedInputComponent->BindAction(PeekLeftAction, ETriggerEvent::Started, this, &ACombatCharacter::PeekLeftPressed);
	EnhancedInputComponent->BindAction(PeekRightAction, ETriggerEvent::Started, this, &ACombatCharacter::PeekRightPressed);

	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ACombatCharacter::FirePressed);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ACombatCharacter::FireReleased);

	EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ACombatCharacter::ReloadWeapon);
	EnhancedInputComponent->BindAction(UnequipAction, ETriggerEvent::Started, this, &ACombatCharacter::ToggleEquipWeapon);

	EnhancedInputComponent->BindAction(PrimaryWeaponAction, ETriggerEvent::Started, this, &ACombatCharacter::EquipPrimaryWeapon);
	EnhancedInputComponent->BindAction(SecondaryWeaponAction, ETriggerEvent::Started, this, &ACombatCharacter::EquipSecondaryWeapon);

	EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &ACombatCharacter::ToggleInventory);
	EnhancedInputComponent->BindAction(UseBandageAction, ETriggerEvent::Started, this, &ACombatCharacter::UseInventoryBandage);
}

// ============================================================================
// Movement
// ============================================================================

void ACombatCharacter::Move(const FInputActionValue& Value)
{
	if (Controller == nullptr)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (bIsUsingBandage && !MovementVector.IsNearlyZero())
	{
		CancelUsingBandage();
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ACombatCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y * -1.0f);
}

void ACombatCharacter::HandleSprintPressed()
{
	if (bIsAiming)
	{
		return;
	}

	bShiftHeld = true;

	if (bIsCrouched)
	{
		bIsRunning = false;
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchFastSpeed;
		return;
	}

	// Armed characters use the normal run speed instead of double-tap sprint.
	if (bIsWeaponEquipped)
	{
		bIsRunning = true;
		bIsSprinting = false;
		SetMovementSpeed(RunSpeed);
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float TimeSinceLastTap = CurrentTime - LastShiftTapTime;

	if (TimeSinceLastTap <= DoubleTapTime)
	{
		bIsSprinting = true;
		bIsRunning = false;
		SetMovementSpeed(SprintSpeed);
	}
	else
	{
		bIsRunning = true;
		bIsSprinting = false;
		SetMovementSpeed(RunSpeed);
	}

	LastShiftTapTime = CurrentTime;
}

void ACombatCharacter::HandleSprintReleased()
{
	bShiftHeld = false;

	if (bIsCrouched)
	{
		bIsRunning = false;
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchWalkSpeed;
		return;
	}

	if (bIsAiming)
	{
		return;
	}

	bIsRunning = false;
	bIsSprinting = false;
	SetMovementSpeed(WalkSpeed);
}

void ACombatCharacter::HandleCrouchPressed()
{
	if (bIsAiming)
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ACombatCharacter::SetMovementSpeed(float NewSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}


// ============================================================================
// Aiming / Peek
// ============================================================================

void ACombatCharacter::AimPressed()
{
	if (bIsUsingBandage)
	{
		CancelUsingBandage();
	}

	if (EquippedWeapon == nullptr || !bIsWeaponEquipped)
	{
		return;
	}

	if (bIsCrouched)
	{
		bReturnToCrouchAfterAim = true;
		UnCrouch();
	}
	else
	{
		bReturnToCrouchAfterAim = false;
	}

	bIsAiming = true;
	bIsRunning = false;
	bIsSprinting = false;

	SetMovementSpeed(AimWalkSpeed);
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void ACombatCharacter::AimReleased()
{
	bIsAiming = false;
	CurrentPeekDirection = 0.0f;

	if (bShiftHeld)
	{
		bIsRunning = true;
		SetMovementSpeed(RunSpeed);
	}
	else
	{
		bIsRunning = false;
		SetMovementSpeed(WalkSpeed);
	}

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	if (bReturnToCrouchAfterAim)
	{
		Crouch();
		bReturnToCrouchAfterAim = false;
	}
}

void ACombatCharacter::PeekLeftPressed()
{
	if (!bIsAiming)
	{
		return;
	}

	CurrentPeekDirection =
		CurrentPeekDirection == -1.0f ? 0.0f : -1.0f;
}

void ACombatCharacter::PeekRightPressed()
{
	if (!bIsAiming)
	{
		return;
	}

	CurrentPeekDirection =
		CurrentPeekDirection == 1.0f ? 0.0f : 1.0f;
}


// ============================================================================
// Interaction
// ============================================================================

void ACombatCharacter::SetOverlappingWeapon(ACombatWeapon* Weapon)
{
	OverlappingWeapon = Weapon;
}

void ACombatCharacter::Interact()
{
	if (!CanStartWeaponAction())
	{
		return;
	}

	// Backpack has first interaction priority.
	if (EquippedBackpack == nullptr)
	{
		TArray<AActor*> OverlappingBackpacks;

		GetOverlappingActors(
			OverlappingBackpacks,
			ABackpack::StaticClass()
		);

		for (AActor* Actor : OverlappingBackpacks)
		{
			ABackpack* Backpack = Cast<ABackpack>(Actor);

			if (Backpack == nullptr)
			{
				continue;
			}

			if (PickupMontage)
			{
				PlayAnimMontage(PickupMontage);
			}

			EquipBackpack(Backpack);
			return;
		}
	}

	// ------------------------------------------------------------
	// GENERIC RPG ITEM PICKUP
	// ------------------------------------------------------------

	if (EquippedBackpack != nullptr)
	{
		UInventoryComponent* Inventory =
			EquippedBackpack->GetInventoryComponent();

		if (Inventory != nullptr)
		{
			TArray<AActor*> OverlappingItems;

			GetOverlappingActors(
				OverlappingItems,
				AItemPickup::StaticClass()
			);

			for (AActor* Actor : OverlappingItems)
			{
				AItemPickup* ItemPickup =
					Cast<AItemPickup>(Actor);

				if (ItemPickup == nullptr)
				{
					continue;
				}

				UItemDefinition* ItemDefinition =
					ItemPickup->GetItemDefinition();

				if (ItemDefinition == nullptr)
				{
					continue;
				}

				if (Inventory->AddItemDefinition(
					ItemDefinition,
					1))
				{
					if (PickupMontage)
					{
						PlayAnimMontage(PickupMontage);
					}

					ItemPickup->Destroy();
					return;
				}
			}
		}
	}

	// Bandages require an equipped backpack.
	if (EquippedBackpack != nullptr)
	{
		UInventoryComponent* Inventory =
			EquippedBackpack->GetInventoryComponent();

		if (Inventory != nullptr)
		{
			TArray<AActor*> OverlappingBandages;

			GetOverlappingActors(
				OverlappingBandages,
				ABandagePickup::StaticClass()
			);

			for (AActor* Actor : OverlappingBandages)
			{
				ABandagePickup* Bandage =
					Cast<ABandagePickup>(Actor);

				if (Bandage == nullptr)
				{
					continue;
				}

				const bool bAddedBandage = Inventory->AddItem(
					EInventoryItemType::Bandage,
					Bandage->GetBandageAmount()
				);

				if (!bAddedBandage)
				{
					continue;
				}

				if (PickupMontage)
				{
					PlayAnimMontage(PickupMontage);
				}

				Bandage->Destroy();
				return;
			}
		}
	}

	// If no backpack, RPG item, or bandage was handled, try weapon pickup.
	if (OverlappingWeapon == nullptr)
	{
		return;
	}

	StartWeaponAction(EWeaponActionState::PickingUp);
	FireReleased();

	if (PickupMontage)
	{
		PlayAnimMontage(PickupMontage);
	}
	else
	{
		FinishWeaponPickup();
	}
}

// ============================================================================
// Equipment
// ============================================================================

void ACombatCharacter::ToggleEquipItem(
	UItemDefinition* ItemDefinition)
{
	if (ItemDefinition == nullptr)
	{
		return;
	}

	if (ItemDefinition->ItemType != EItemType::Equipment)
	{
		return;
	}

	// Player must own the item.
	if (EquippedBackpack == nullptr)
	{
		return;
	}

	UInventoryComponent* Inventory =
		EquippedBackpack->GetInventoryComponent();

	if (Inventory == nullptr ||
		Inventory->GetItemDefinitionCount(ItemDefinition) <= 0)
	{
		return;
	}

	// ------------------------------------------------------------
	// Unequip
	// ------------------------------------------------------------

	if (IsItemEquipped(ItemDefinition))
	{
		if (ItemDefinition->EquipmentSlot == EEquipmentSlot::Head)
		{
			EquippedHeadItem = nullptr;

			EquippedHeadMesh->SetSkeletalMeshAsset(nullptr);
			EquippedHeadMesh->SetVisibility(false);
		}
		else if (ItemDefinition->EquipmentSlot == EEquipmentSlot::Torso)
		{
			EquippedTorsoItem = nullptr;

			EquippedTorsoMesh->SetStaticMesh(nullptr);
			EquippedTorsoMesh->SetVisibility(false);
		}

		RefreshMaxHealthFromEquipment();
		return;
	}

	// ------------------------------------------------------------
	// Equip Head Item
	// ------------------------------------------------------------

	if (ItemDefinition->EquipmentSlot == EEquipmentSlot::Head)
	{
		EquippedHeadItem = ItemDefinition;

		EquippedHeadMesh->SetSkeletalMeshAsset(
			ItemDefinition->EquippedSkeletalMesh
		);

		EquippedHeadMesh->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			ItemDefinition->EquipSocketName
		);

		EquippedHeadMesh->SetVisibility(true);

		RefreshMaxHealthFromEquipment();
		return;
	}

	// ------------------------------------------------------------
	// Equip Torso Item
	// ------------------------------------------------------------

	if (ItemDefinition->EquipmentSlot == EEquipmentSlot::Torso)
	{
		EquippedTorsoItem = ItemDefinition;

		EquippedTorsoMesh->SetStaticMesh(
			ItemDefinition->EquippedStaticMesh
		);

		EquippedTorsoMesh->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			ItemDefinition->EquipSocketName
		);

		EquippedTorsoMesh->SetVisibility(true);

		RefreshMaxHealthFromEquipment();
		return;
	}
}

bool ACombatCharacter::IsItemEquipped(
	UItemDefinition* ItemDefinition) const
{
	if (ItemDefinition == nullptr)
	{
		return false;
	}

	if (ItemDefinition->EquipmentSlot == EEquipmentSlot::Head)
	{
		return EquippedHeadItem == ItemDefinition;
	}

	if (ItemDefinition->EquipmentSlot == EEquipmentSlot::Torso)
	{
		return EquippedTorsoItem == ItemDefinition;
	}

	return false;
}

float ACombatCharacter::GetEquipmentDamageMultiplier() const
{
	float Multiplier = 1.0f;

	if (EquippedHeadItem != nullptr)
	{
		Multiplier *= EquippedHeadItem->DamageMultiplier;
	}

	if (EquippedTorsoItem != nullptr)
	{
		Multiplier *= EquippedTorsoItem->DamageMultiplier;
	}

	return Multiplier;
}

float ACombatCharacter::GetEquipmentMaxHealthMultiplier() const
{
	float Multiplier = 1.0f;

	if (EquippedHeadItem != nullptr)
	{
		Multiplier *= EquippedHeadItem->MaxHealthMultiplier;
	}

	if (EquippedTorsoItem != nullptr)
	{
		Multiplier *= EquippedTorsoItem->MaxHealthMultiplier;
	}

	return Multiplier;
}

void ACombatCharacter::RefreshMaxHealthFromEquipment()
{
	const float OldMaxHealth = MaxHealth;

	float HealthPercent = 1.0f;

	if (OldMaxHealth > 0.0f)
	{
		HealthPercent = CurrentHealth / OldMaxHealth;
	}

	MaxHealth =
		BaseMaxHealth *
		GetEquipmentMaxHealthMultiplier();

	CurrentHealth = FMath::Clamp(
		MaxHealth * HealthPercent,
		0.0f,
		MaxHealth
	);
}


// ============================================================================
// Backpack / Inventory
// ============================================================================

void ACombatCharacter::EquipBackpack(ABackpack* Backpack)
{
	if (Backpack == nullptr || EquippedBackpack != nullptr)
	{
		return;
	}

	EquippedBackpack = Backpack;

	Backpack->SetOwner(this);
	Backpack->SetActorEnableCollision(false);

	Backpack->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		Backpack->EquipSocketName
	);

	// Move an already-holstered rifle onto the backpack.
	if (PrimaryWeapon != nullptr)
	{
		const bool bRifleIsInHands =
			EquippedWeapon == PrimaryWeapon &&
			bIsWeaponEquipped;

		if (!bRifleIsInHands)
		{
			PrimaryWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				TEXT("RifleBackpackSocket")
			);
		}
	}
}

void ACombatCharacter::ToggleInventory()
{
	if (EquippedBackpack == nullptr)
	{
		return;
	}

	if (bIsInventoryOpen)
	{
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->RemoveFromParent();
			InventoryWidgetInstance = nullptr;
		}

		if (APlayerController* PlayerController =
			Cast<APlayerController>(GetController()))
		{
			PlayerController->bShowMouseCursor = false;

			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}

		bIsInventoryOpen = false;
		return;
	}

	if (InventoryWidgetClass == nullptr)
	{
		return;
	}

	InventoryWidgetInstance = CreateWidget<UUserWidget>(
		GetWorld(),
		InventoryWidgetClass
	);


	if (InventoryWidgetInstance)
	{
		InventoryWidgetInstance->AddToViewport();
		bIsInventoryOpen = true;

		if (APlayerController* PlayerController =
			Cast<APlayerController>(GetController()))
		{
			PlayerController->bShowMouseCursor = true;

			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(
				InventoryWidgetInstance->TakeWidget()
			);

			InputMode.SetHideCursorDuringCapture(false);

			PlayerController->SetInputMode(InputMode);
		}
	}
}


// ============================================================================
// Healing
// ============================================================================

void ACombatCharacter::UseInventoryBandage()
{
	if (bIsDead || bIsUsingBandage)
	{
		return;
	}

	if (EquippedBackpack == nullptr)
	{
		return;
	}

	if (CurrentHealth >= MaxHealth)
	{
		return;
	}

	UInventoryComponent* Inventory =
		EquippedBackpack->GetInventoryComponent();

	if (Inventory == nullptr)
	{
		return;
	}

	if (Inventory->GetItemCount(EInventoryItemType::Bandage) <= 0)
	{
		return;
	}

	bIsUsingBandage = true;

	if (BandageMontage)
	{
		PlayAnimMontage(BandageMontage);
	}
	else
	{
		FinishUsingBandage();
	}
}

void ACombatCharacter::FinishUsingBandage()
{
	if (!bIsUsingBandage)
	{
		return;
	}

	if (EquippedBackpack == nullptr)
	{
		bIsUsingBandage = false;
		return;
	}

	UInventoryComponent* Inventory =
		EquippedBackpack->GetInventoryComponent();

	if (Inventory == nullptr)
	{
		bIsUsingBandage = false;
		return;
	}

	if (CurrentHealth >= MaxHealth)
	{
		bIsUsingBandage = false;
		return;
	}

	if (!Inventory->RemoveItem(EInventoryItemType::Bandage, 1))
	{
		bIsUsingBandage = false;
		return;
	}

	CurrentHealth = FMath::Clamp(
		CurrentHealth + BandageHealAmount,
		0.0f,
		MaxHealth
	);

	bIsUsingBandage = false;
}

void ACombatCharacter::CancelUsingBandage()
{
	if (!bIsUsingBandage)
	{
		return;
	}

	bIsUsingBandage = false;

	if (BandageMontage)
	{
		StopAnimMontage(BandageMontage);
	}
}


// ============================================================================
// Weapon Pickup / Switching
// ============================================================================

void ACombatCharacter::EquipWeapon(ACombatWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr)
	{
		return;
	}

	WeaponToEquip->GetWeaponMesh()->SetSimulatePhysics(false);
	WeaponToEquip->GetWeaponMesh()->SetEnableGravity(false);
	WeaponToEquip->GetWeaponMesh()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	EquippedWeapon = WeaponToEquip;
	CurrentWeaponType = EquippedWeapon->WeaponType;

	EquippedWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		EquippedWeapon->EquipSocketName
	);

	WeaponToEquip->GetPickupSphere()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	WeaponToEquip->GetPickupSphere()->SetHiddenInGame(true);

	bIsWeaponEquipped = true;
}

void ACombatCharacter::FinishWeaponPickup()
{
	if (WeaponActionState != EWeaponActionState::PickingUp)
	{
		return;
	}

	if (OverlappingWeapon == nullptr)
	{
		EndWeaponAction();
		return;
	}

	if (EquippedWeapon &&
		bIsWeaponEquipped &&
		EquippedWeapon != OverlappingWeapon)
	{
		UnequipWeapon();
	}

	if (OverlappingWeapon->WeaponType == EWeaponType::Rifle)
	{
		PrimaryWeapon = OverlappingWeapon;
	}
	else if (OverlappingWeapon->WeaponType == EWeaponType::Pistol)
	{
		SecondaryWeapon = OverlappingWeapon;
	}

	EquipWeapon(OverlappingWeapon);
	OverlappingWeapon = nullptr;

	EndWeaponAction();
}

void ACombatCharacter::EquipPrimaryWeapon()
{
	SwitchToWeapon(PrimaryWeapon);
}

void ACombatCharacter::EquipSecondaryWeapon()
{
	SwitchToWeapon(SecondaryWeapon);
}

void ACombatCharacter::SwitchToWeapon(ACombatWeapon* WeaponToSwitchTo)
{
	if (WeaponToSwitchTo == nullptr)
	{
		return;
	}

	if (WeaponToSwitchTo == EquippedWeapon)
	{
		return;
	}

	if (!CanStartWeaponAction())
	{
		return;
	}

	FireReleased();

	PendingWeaponToSwitchTo = WeaponToSwitchTo;
	StartWeaponAction(EWeaponActionState::Switching);

	if (WeaponSwapMontage)
	{
		if (EquippedWeapon->WeaponType == EWeaponType::Rifle &&
			WeaponToSwitchTo->WeaponType == EWeaponType::Pistol)
		{
			PlayAnimMontage(WeaponSwapMontage, 1.0f);
		}
		else if (
			EquippedWeapon->WeaponType == EWeaponType::Pistol &&
			WeaponToSwitchTo->WeaponType == EWeaponType::Rifle)
		{
			PlayAnimMontage(WeaponSwapMontage, -1.0f);

			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_SetPosition(
					WeaponSwapMontage,
					WeaponSwapMontage->GetPlayLength()
				);
			}
		}
		else
		{
			FinishWeaponSwitch();
		}
	}
	else
	{
		FinishWeaponSwitch();
	}
}

void ACombatCharacter::FinishWeaponSwitch()
{
	if (WeaponActionState != EWeaponActionState::Switching)
	{
		return;
	}

	if (PendingWeaponToSwitchTo == nullptr)
	{
		EndWeaponAction();
		return;
	}

	if (EquippedWeapon)
	{
		FName HolsterSocket = EquippedWeapon->HolsterSocketName;

		if (EquippedBackpack != nullptr &&
			EquippedWeapon->WeaponType == EWeaponType::Rifle)
		{
			HolsterSocket = TEXT("RifleBackpackSocket");
		}

		EquippedWeapon->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			HolsterSocket
		);
	}

	EquippedWeapon = PendingWeaponToSwitchTo;

	EquippedWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		EquippedWeapon->EquipSocketName
	);

	bIsWeaponEquipped = true;
	CurrentWeaponType = EquippedWeapon->WeaponType;

	PendingWeaponToSwitchTo = nullptr;
	EndWeaponAction();
}


// ============================================================================
// Firing
// ============================================================================

void ACombatCharacter::FirePressed()
{
	FireWeapon();

	if (EquippedWeapon && EquippedWeapon->bIsAutomatic)
	{
		GetWorldTimerManager().SetTimer(
			AutoFireTimerHandle,
			this,
			&ACombatCharacter::FireWeapon,
			EquippedWeapon->FireDelay,
			true
		);
	}
}

void ACombatCharacter::FireReleased()
{
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
}

void ACombatCharacter::FireWeapon()
{
	if (WeaponActionState != EWeaponActionState::None)
	{
		return;
	}

	if (EquippedWeapon == nullptr ||
		!bIsWeaponEquipped ||
		!bIsAiming ||
		!bCanFire)
	{
		return;
	}

	if (PickupMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			if (AnimInstance->Montage_IsPlaying(PickupMontage))
			{
				return;
			}
		}
	}

	if (!EquippedWeapon->CanFire())
	{
		if (EquippedWeapon->EmptyClickSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquippedWeapon->EmptyClickSound,
				GetActorLocation()
			);
		}

		return;
	}

	bCanFire = false;

	EquippedWeapon->ConsumeAmmo();
	AddControllerPitchInput(-EquippedWeapon->RecoilPitch);

	if (EquippedWeapon->FireMontage)
	{
		PlayAnimMontage(EquippedWeapon->FireMontage);
	}

	if (EquippedWeapon->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->FireSound,
			GetActorLocation()
		);
	}

	if (EquippedWeapon->MuzzleFlashClass)
	{
		USkeletalMeshComponent* WeaponMesh =
			EquippedWeapon->GetWeaponMesh();

		if (WeaponMesh &&
			WeaponMesh->DoesSocketExist(TEXT("MuzzleSocket")))
		{
			const FTransform MuzzleTransform =
				WeaponMesh->GetSocketTransform(TEXT("MuzzleSocket"));

			AActor* SpawnedFlash = GetWorld()->SpawnActor<AActor>(
				EquippedWeapon->MuzzleFlashClass,
				MuzzleTransform
			);

			if (SpawnedFlash)
			{
				SpawnedFlash->AttachToComponent(
					WeaponMesh,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale,
					TEXT("MuzzleSocket")
				);
			}
		}
	}

	EquippedWeapon->PlayFireAnimation();

	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceTarget =
		TraceStart + FollowCamera->GetForwardVector() * 10000.0f;

	FHitResult HitResult;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceTarget,
		ECC_Visibility,
		QueryParams
	);

	if (HitResult.bBlockingHit)
	{
		AEnemyCharacter* HitEnemy =
			Cast<AEnemyCharacter>(HitResult.GetActor());

		if (HitEnemy)
		{
			float DamageToApply =
				EquippedWeapon->Damage *
				GetEquipmentDamageMultiplier();

			if (HitResult.BoneName == FName(TEXT("head")))
			{
				DamageToApply *= EquippedWeapon->HeadSHotMultiplier;
			}

			HitEnemy->ReceiveDamage(
				DamageToApply,
				this
			);

			if (BloodSplatterClass)
			{
				const FRotator BloodRotation =
					HitResult.ImpactNormal.Rotation();

				GetWorld()->SpawnActor<AActor>(
					BloodSplatterClass,
					HitResult.ImpactPoint,
					BloodRotation
				);
			}
		}
	}

	GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&ACombatCharacter::ResetFire,
		EquippedWeapon->FireDelay,
		false
	);
}

void ACombatCharacter::ResetFire()
{
	bCanFire = true;
}


// ============================================================================
// Reload / Ammo
// ============================================================================

void ACombatCharacter::ReloadWeapon()
{
	if (!CanStartWeaponAction() ||
		EquippedWeapon == nullptr ||
		!bIsWeaponEquipped ||
		!EquippedWeapon->CanReload())
	{
		return;
	}

	StartWeaponAction(EWeaponActionState::Reloading);
	FireReleased();

	if (EquippedWeapon->ReloadMontage)
	{
		PlayAnimMontage(EquippedWeapon->ReloadMontage);
	}
	else
	{
		FinishReload();
	}
}

void ACombatCharacter::FinishReload()
{
	if (WeaponActionState != EWeaponActionState::Reloading)
	{
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->ReloadWeapon();
	}

	EndWeaponAction();
}

void ACombatCharacter::AddAmmo(int32 MagazineAmount)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AddReserveMagazines(MagazineAmount);
	}
}


// ============================================================================
// Holster / Unholster
// ============================================================================

void ACombatCharacter::ToggleEquipWeapon()
{
	if (!CanStartWeaponAction() || EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance == nullptr)
	{
		return;
	}

	if (bIsWeaponEquipped)
	{
		StartWeaponAction(EWeaponActionState::Holstering);
		FireReleased();

		if (HolsterMontage)
		{
			bIsUnholstering = false;
			PlayAnimMontage(HolsterMontage, 1.0f);
		}
		else
		{
			FinishHolster();
		}

		return;
	}

	StartWeaponAction(EWeaponActionState::Unholstering);

	if (HolsterMontage)
	{
		bIsUnholstering = true;

		PlayAnimMontage(HolsterMontage, -1.0f);

		AnimInstance->Montage_SetPosition(
			HolsterMontage,
			HolsterMontage->GetPlayLength()
		);

		ReEquipWeapon();

		GetWorldTimerManager().SetTimer(
			UnholsterTimerHandle,
			this,
			&ACombatCharacter::EndUnholsterState,
			HolsterMontage->GetPlayLength() + 0.1f,
			false
		);
	}
	else
	{
		FinishUnholster();
	}
}

void ACombatCharacter::UnequipWeapon()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	FName HolsterSocket = EquippedWeapon->HolsterSocketName;

	if (EquippedBackpack != nullptr &&
		EquippedWeapon->WeaponType == EWeaponType::Rifle)
	{
		HolsterSocket = TEXT("RifleBackpackSocket");
	}

	EquippedWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		HolsterSocket
	);

	bIsWeaponEquipped = false;
	bIsAiming = false;
	CurrentWeaponType = EWeaponType::None;
}

void ACombatCharacter::ReEquipWeapon()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	EquippedWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		EquippedWeapon->EquipSocketName
	);

	bIsWeaponEquipped = true;
	CurrentWeaponType = EquippedWeapon->WeaponType;
}

void ACombatCharacter::FinishHolster()
{
	if (WeaponActionState != EWeaponActionState::Holstering ||
		bIsUnholstering)
	{
		return;
	}

	UnequipWeapon();
	EndWeaponAction();
}

void ACombatCharacter::FinishUnholster()
{
	if (WeaponActionState != EWeaponActionState::Unholstering)
	{
		return;
	}

	bIsUnholstering = false;
	ReEquipWeapon();

	EndWeaponAction();
}

void ACombatCharacter::EndUnholsterState()
{
	bIsUnholstering = false;

	if (WeaponActionState == EWeaponActionState::Unholstering)
	{
		EndWeaponAction();
	}
}


// ============================================================================
// Damage / Death
// ============================================================================

void ACombatCharacter::ReceiveDamage(float DamageAmount)
{
	if (bIsDead)
	{
		return;
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			GetActorLocation()
		);
	}

	CurrentHealth = FMath::Clamp(
		CurrentHealth - DamageAmount,
		0.0f,
		MaxHealth
	);

	if (CurrentHealth <= 0.0f)
	{
		Die();
		return;
	}

	// Hit reactions can interrupt reload montages, so clear the reload state.
	if (WeaponActionState == EWeaponActionState::Reloading)
	{
		EndWeaponAction();
	}

	if (bIsWeaponEquipped)
	{
		if (HitReactionMontage)
		{
			PlayAnimMontage(HitReactionMontage);
		}
	}
	else if (UnarmedHitReactionMontage)
	{
		PlayAnimMontage(UnarmedHitReactionMontage);
	}
}

void ACombatCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	FireReleased();

	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	GetWorldTimerManager().ClearTimer(UnholsterTimerHandle);

	DisableInput(Cast<APlayerController>(GetController()));
	GetCharacterMovement()->DisableMovement();

	bIsRunning = false;
	bIsSprinting = false;
	bIsAiming = false;
	CurrentPeekDirection = 0.0f;

	GetCapsuleComponent()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	GetMesh()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	if (DeathAnimation)
	{
		GetMesh()->PlayAnimation(DeathAnimation, false);
	}
}


// ============================================================================
// Weapon Action State
// ============================================================================

bool ACombatCharacter::CanStartWeaponAction() const
{
	return WeaponActionState == EWeaponActionState::None && !bIsDead;
}

void ACombatCharacter::StartWeaponAction(
	EWeaponActionState NewState)
{
	WeaponActionState = NewState;
}

void ACombatCharacter::EndWeaponAction()
{
	WeaponActionState = EWeaponActionState::None;
}
