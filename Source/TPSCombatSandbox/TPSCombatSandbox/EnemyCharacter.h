// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class USphereComponent;
class ACombatCharacter;
class USkeletalMeshComponent;
class USoundBase;
class UAnimMontage;
class ATargetPoint;
class UAnimSequence;

UENUM(BlueprintType) 
enum class EEnemyWeaponType : uint8 {
    Pistol,
    AutomaticRifle,
    Shotgun
};


UENUM(BlueprintType)
enum class EEnemyCombatState : uint8
{
    Idle,
    Patrolling,
    Chasing,
    Attacking,
    Repositioning,
    InCover,
    Investigating,
    Searching,
    Suppressed,
    Dead
};


UCLASS()
class TPSCOMBATSANDBOX_API AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:

    // ======================================================
    // LIFECYCLE
    // ======================================================

    AEnemyCharacter();

    virtual void Tick(float DeltaTime) override;


    // ======================================================
    // HEALTH / DAMAGE
    // ======================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Health")
    float MaxHealth = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy|Health")
    float CurrentHealth = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy|Health")
    bool bIsDead = false;

    void ReceiveDamage(float DamageAmount, ACombatCharacter* DamageSource);
    void Die();


    // ======================================================
    // ANIMATION STATE
    // ======================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Animation")
    EEnemyWeaponType EnemyWeaponType = EEnemyWeaponType::Pistol;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy|Animation")
    bool bIsSupressed = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Animation")
    UAnimSequence* DeathAnimation;


    // ======================================================
    // AI MOVEMENT CALLBACKS
    // ======================================================

    void OnRepositionMoveCompleted();
    void OnRepositionMoveFailed();
    void OnPatrolMoveCompleted();


protected:

    // ======================================================
    // LIFECYCLE
    // ======================================================

    virtual void BeginPlay() override;


    // ======================================================
    // DETECTION / PERCEPTION
    // ======================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
    USphereComponent* DetectionSphere;

    UPROPERTY(EditAnywhere, Category = "Enemy")
    float DetectionRadius = 1800.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Detection")
    float FieldOfViewDegrees = 170.0f;

    UPROPERTY()
    ACombatCharacter* TargetPlayer;

    UFUNCTION()
    void OnDetectionSphereBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    UFUNCTION()
    void OnDetectionSphereEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex
    );

    UPROPERTY(BlueprintReadOnly, Category = "Enemy|AI")
    bool bCanSeePlayer = false;

    bool bHasEnemyEnteredCombat = false;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy|AI")
    FVector LastKnownPlayerLocation = FVector::ZeroVector;

    bool CanSeePlayer() const;
    bool HasClearShot() const;
    bool IsPlayerInFieldOfView() const;
    bool IsPlayerInCombatRange() const;
    


    // ======================================================
    // AI STATE / DECISION MAKING
    // ======================================================

    UPROPERTY(BlueprintReadOnly, Category = "Enemy|AI")
    EEnemyCombatState EnemyCombatState = EEnemyCombatState::Idle;

    FTimerHandle CombatCheckTimerHandle;

    void CheckCombatState();
    void SetEnemyCombatState(EEnemyCombatState NewState);
    bool IsEnemyCombatState(EEnemyCombatState StateToCheck) const;


    // ======================================================
    // MOVEMENT / TARGETING
    // ======================================================

    UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
    float CombatRange = 1200.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy|Combat")
    bool bIsPlayerInCombatRange = false;

    void ChasePlayer();
    void StopChasing();
    void FaceTarget(float DeltaTime);

    void StartPatrolling();
    UPROPERTY(EditInstanceOnly, Category = "Enemy|Patrol")
    TArray<ATargetPoint*> PatrolPoints;
    int32 CurrentPatrolPointIndex = 0;
    UPROPERTY(EditAnywhere, Category = "Enemy|Patrol")
    float PatrolWaitTime = 3.5f;
    FTimerHandle PatrolWaitTimerHandle;

    // ======================================================
    // NORMAL COMBAT
    // ======================================================

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    float AccuracyPercent = 75.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
    float FireRange = 2500.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
    float FireDamage = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
    int32 PelletsPerShot = 1;

    UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
    float ShotSpreadDegrees = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
    USoundBase* EnemyFireSound;

    UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
    UAnimMontage* FireMontage;

    UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
    UAnimMontage* HitReactionMontage;

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    USoundBase* HitSound;

    void StartFiring();
    void StopFiring();
    void FireWeapon();


    // ======================================================
    // BURST FIRE
    // ======================================================

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    float TimeBetweenShots = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    float TimeBetweenBursts = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Enemy| Combat")
    int32 MinBurstShots = 2;

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    int32 MaxBurstsShots = 4;

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    int32 BurstsBeforeReposition = 2;

    int32 CurrentBurstShot = 0;
    int32 CurrentBurstSize = 0;
    int32 CompletedBursts = 0;

    FTimerHandle BurstShotTimerHandle;
    FTimerHandle BurstPauseTimerHandle;

    void StartBurst();
    void FireBurstShot();

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    float FireStartDelay = 1.5f;
    FTimerHandle FireStartDelayTimerHandle;

    // ======================================================
    // REPOSITIONING
    // ======================================================

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    bool bCanRepositionWithoutCover = true;

    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    float RepositionRadius = 700.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy | Animation")
    bool bIsRepositioning = false;

    FVector CurrentRepositionDestination = FVector::ZeroVector;

    bool bRepositioningToCover = false;

    void StartRepositioning();
    bool FindRepositionPoint(FVector& Outpoint) const;
    bool HasReachedRepositionDestination() const;

    
    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    bool bUsesCover = true; //Added for shotgun Agression
    UPROPERTY(EditAnywhere, Category = "Enemy | Combat")
    float AggressiveAdvanceDistance = 300.0f; //Added for shotgun Agression

    // ======================================================
    // COVER
    // ======================================================

    UPROPERTY(EditAnywhere, Category = "Enemy | Cover")
    float CoverSearchRadius = 1800.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy | Cover")
    int32 CoverCandidateCount = 20;

    UPROPERTY(EditAnywhere, Category = "Enemy|Cover")
    float CoverHoldDuration = 10.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy|Animation")
    bool bIsInCover = false;

    UPROPERTY(BlueprintReadOnly, Category = "Enemy | Animation")
    bool bIsPeekingFromCover = false;

    UPROPERTY(EditAnywhere, Category = "Enemy | Cover")
    float CoverPeekDelay = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Cover")
    int32 CoverBurstSize = 3;

    UPROPERTY(EditAnywhere, Category = "Enemy|Cover")
    float CoverTimeBetweenShots = 0.2f;

    TArray<FVector> CoverCandidates;
    TArray<FVector> CoveredCandidates;

    int32 CurrentCoverBurstShot = 0;

    FTimerHandle CoverHoldTimerHandle;
    FTimerHandle CoverPeekTimerHandle;
    FTimerHandle CoverBurstTimerHandle;

    void TryEnterCombatCover(bool bResumeAttackIfNoCover = true);
    void GenerateCoverCandidates();
    bool IsCandidateCoveredFromPlayer(const FVector& CandidatePoint, const AActor* ExpectedCoverActor) const;
    bool ChooseCoverPoint(FVector& OutCoverPoint) const;

    void StartCoverPeek();
    void FireCoverBurstShot();
    void ReturnToCover();
    void ExitCover();


    // ======================================================
    // WEAPON / VFX
    // ======================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Weapon")
    USkeletalMeshComponent* EnemyWeaponMesh;

    UPROPERTY(EditAnywhere, Category = "Enemy|Weapon")
    FName MuzzleSocketName = TEXT("MuzzleSocket");

    UPROPERTY(EditAnywhere, Category = "Enemy|VFX")
    TSubclassOf<AActor> MuzzleFlashClass;

    UPROPERTY(EditAnywhere, Category = "VFX")
    TSubclassOf<AActor> BloodSplatterClass;


    // ======================================================
    // INVESTIGATION / SEARCH
    // ======================================================

    UPROPERTY(EditAnywhere, Category = "Enemy|Search")
    float SearchRotationSpeed = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Search")
    float SearchDuration = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Search")
    float SearchPointRadius = 1200.0f;

    UPROPERTY(EditAnywhere, Category = "Enenemy|Search")
    float MinSearchPointDistance = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|Search")
    float SearchMoveSpeed = 250.0f;

    float DefaultMoveSpeed = 0.0f;

    FVector CurrentSearchDestination = FVector::ZeroVector;

    FTimerHandle SearchTimerHandle;

    void InvestigateLastKnownLocation();
    bool HasReachedLastKnownLocation() const;

    void StartSearching();
    void MoveToNextSearchPoint();
    FVector GetRandomSearchPoint(bool& bOutFound) const;
    bool HasReachedSearchDestination() const;
    void CancelSearching();
    void StopSearching();

    UPROPERTY(EditAnywhere, Category = "Enemy| Search")
    float LostSightInvestigationDelay = 15.0f;
    float LastSeenPlayerTime = -1.0f;
    bool bHasTriedLostSightCover = false;


    // ======================================================
    // SUPPRESSION
    // ======================================================

    UPROPERTY(EditAnywhere, Category = "Enemy| Supression")
    int32 HitsBeforeSuppressed = 3;

    UPROPERTY(EditAnywhere, Category = "Enemy | Supression")
    int32 RecentHitCount = 0;

    UPROPERTY(EditAnywhere, Category = "Enemy | Supression")
    float SuppressedDuration = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy | Supression");
    float RecentHitResetTime = 2.0f;

    FTimerHandle SuppressionTimerHandle;
    FTimerHandle RecentHitResetTimerHandle;

    void EnterSuppressedState();
    void ExitSuppressedState();
    void ResetRecentHitCount();
};
