// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "Components/SphereComponent.h"
#include "CombatCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/TargetPoint.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Animation/AnimSequence.h"

#include "DrawDebugHelpers.h"


// ======================================================
// LIFECYCLE
// ======================================================

AEnemyCharacter::AEnemyCharacter()
{
    //TickDisabled until Actor enters detection sphere
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(false);

    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetupAttachment(GetCapsuleComponent());
    DetectionSphere->SetSphereRadius(DetectionRadius);

    DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    EnemyWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EnemyWeaponMesh"));
    EnemyWeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
    EnemyWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    

    GetCapsuleComponent()->SetCollisionResponseToChannel(
        ECC_Visibility,
        ECR_Ignore
    );
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GetMesh()->SetCollisionResponseToChannel(
        ECC_Visibility,
        ECR_Block
    );

}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;

    DefaultMoveSpeed = GetCharacterMovement()->MaxWalkSpeed;

    DetectionSphere->OnComponentBeginOverlap.AddDynamic(
        this,
        &AEnemyCharacter::OnDetectionSphereBeginOverlap
    );

    DetectionSphere->OnComponentEndOverlap.AddDynamic(
        this,
        &AEnemyCharacter::OnDetectionSphereEndOverlap
    );

    StartPatrolling();
}

void AEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (IsEnemyCombatState(EEnemyCombatState::Attacking) ||
        IsEnemyCombatState(EEnemyCombatState::InCover))
    {
        FaceTarget(DeltaTime);
    }

    if (IsEnemyCombatState(EEnemyCombatState::Searching))
    {
        if (HasReachedSearchDestination())
        {
            MoveToNextSearchPoint();
        }
    }
}


// ======================================================
// DETECTION / PERCEPTION
// ======================================================

void AEnemyCharacter::OnDetectionSphereBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    ACombatCharacter* PlayerCharacter = Cast<ACombatCharacter>(OtherActor);
    if (bIsDead) return;

    if (PlayerCharacter)
    {
        TargetPlayer = PlayerCharacter;
        GetWorldTimerManager().SetTimer(
            CombatCheckTimerHandle,
            this,
            &AEnemyCharacter::CheckCombatState,
            0.25f,
            true
        );

        SetActorTickEnabled(true);

        bCanSeePlayer = IsPlayerInFieldOfView() && CanSeePlayer();

        if (bCanSeePlayer)
        {
            LastKnownPlayerLocation = TargetPlayer->GetActorLocation();

            if (!bHasEnemyEnteredCombat)
            {
                bHasEnemyEnteredCombat = true;

                GetWorldTimerManager().ClearTimer(PatrolWaitTimerHandle);

                CheckCombatState();
                return;
            }
        }
        else
        {
            CheckCombatState();
        }
    }
}

void AEnemyCharacter::OnDetectionSphereEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex
)
{
    ACombatCharacter* PlayerCharacter = Cast<ACombatCharacter>(OtherActor);

    if (PlayerCharacter && PlayerCharacter == TargetPlayer)
    {
        bCanSeePlayer = false;
        bIsPlayerInCombatRange = false;

        //The player was never detected or the previous search has ended
        //Losing the overlap must not cancel the active patrol
        if (IsEnemyCombatState(EEnemyCombatState::Patrolling) && 
            !bHasEnemyEnteredCombat && 
            LastKnownPlayerLocation.IsNearlyZero()) {

            TargetPlayer = nullptr;

            GetWorldTimerManager().ClearTimer(CombatCheckTimerHandle);
            SetActorTickEnabled(false);

            return;
        }

        // If the enemy is already investigating or searching,
        // leaving the detection sphere should not interrupt it.
        if (IsEnemyCombatState(EEnemyCombatState::Investigating) ||
            IsEnemyCombatState(EEnemyCombatState::Searching))
        {
            return;
        }

        // If the enemy remembers where the player was,
        // begin investigating instead of instantly forgetting them.
        if (!LastKnownPlayerLocation.IsNearlyZero())
        {
            StopChasing();
            StopFiring();

            bIsInCover = false;
            bIsPeekingFromCover = false;
            bIsRepositioning = false;
            bRepositioningToCover = false;

            GetWorldTimerManager().ClearTimer(CoverHoldTimerHandle);
            GetWorldTimerManager().ClearTimer(CoverPeekTimerHandle);
            GetWorldTimerManager().ClearTimer(CoverBurstTimerHandle);

            SetEnemyCombatState(EEnemyCombatState::Investigating);

            InvestigateLastKnownLocation();

            return;
        }

        // Enemy never actually knew where the player was.
        TargetPlayer = nullptr;

        GetWorldTimerManager().ClearTimer(CombatCheckTimerHandle);

        StopChasing();
        StopFiring();

        SetEnemyCombatState(EEnemyCombatState::Idle);
        SetActorTickEnabled(false);

        bHasEnemyEnteredCombat = false;
        bIsInCover = false;
        bIsRepositioning = false;
        bRepositioningToCover = false;

        GetWorldTimerManager().ClearTimer(CoverHoldTimerHandle);
    }
}


bool AEnemyCharacter::CanSeePlayer() const
{
    if (TargetPlayer == nullptr) return false;

    FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
    
    //Code changed after crouch visibility bug. Bug resolved.
    UCapsuleComponent* PlayerCapsule = TargetPlayer->GetCapsuleComponent();
    if (PlayerCapsule == nullptr) {
        return false;
    }
    const FVector CapsuleCenter = PlayerCapsule->GetComponentLocation();
    const float CapsuleHalfHeight = PlayerCapsule->GetScaledCapsuleHalfHeight();
    //Aim near the top of players cappsule, this automatically moves lower when player crouches.
    FVector End = CapsuleCenter + FVector(0.0f, 0.0f, CapsuleHalfHeight - 5.0f);


    FHitResult HitResult;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (EnemyWeaponMesh)
    {
        QueryParams.AddIgnoredComponent(EnemyWeaponMesh);
    }

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_Visibility,
        QueryParams
    );

    if (!bHit)
    {
        return false;
    }

    return HitResult.GetActor() == TargetPlayer;
}

bool AEnemyCharacter::IsPlayerInFieldOfView()const {
    if (TargetPlayer == nullptr) {
        return false;
    }

    FVector DirectionToPlayer = TargetPlayer->GetActorLocation() - GetActorLocation();
    DirectionToPlayer.Z = 0.0f;
    DirectionToPlayer.Normalize();

    FVector EnemyForward = GetActorForwardVector();

    const float DotProduct = FVector::DotProduct(EnemyForward, DirectionToPlayer);
    const float HalfFOV = FieldOfViewDegrees * 0.5f;
    const float FOVThreshold = FMath::Cos(FMath::DegreesToRadians(HalfFOV));

    return DotProduct >= FOVThreshold;


}

bool AEnemyCharacter::HasClearShot() const
{
    if (TargetPlayer == nullptr)
    {
        return false;
    }

    //Start from the enemy's weapon muzzle
    const FVector Start = EnemyWeaponMesh->GetSocketLocation(MuzzleSocketName);

    //Aim roughly at the players upper body
    const FVector End = TargetPlayer->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);

    FHitResult HitResult;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    //Trace using visibility so enviromental objects can block the shot
    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_Visibility,
        QueryParams
    );

    if (!bHit)
    {
        return false;
    }

    //A shot is only considered clear if the first thing the trace hits is the playeer
    return HitResult.GetActor() == TargetPlayer;
}

bool AEnemyCharacter::IsPlayerInCombatRange() const
{
    if (TargetPlayer == nullptr) return false;

    float DistanceToPlayer = FVector::Dist(
        GetActorLocation(),
        TargetPlayer->GetActorLocation()
    );

    return DistanceToPlayer <= CombatRange;
}


// ======================================================
// AI STATE / DECISION MAKING
// ======================================================

void AEnemyCharacter::CheckCombatState()
{
    if (bIsDead)
    {
        SetEnemyCombatState(EEnemyCombatState::Dead);
        return;
    }

    if (IsEnemyCombatState(EEnemyCombatState::Suppressed))
    {
        return;
    }

    if (TargetPlayer == nullptr)
    {
        SetEnemyCombatState(EEnemyCombatState::Idle);
        StopChasing();
        StopFiring();
        bIsPlayerInCombatRange = false;
        return;
    }

    //bCanSeePlayer = IsPlayerInFieldOfView() && CanSeePlayer();
    if (bHasEnemyEnteredCombat) {
        bCanSeePlayer = CanSeePlayer();
    }
    else {
        bCanSeePlayer =
            IsPlayerInFieldOfView() && CanSeePlayer();
    }


    if (bCanSeePlayer)
    {
        LastKnownPlayerLocation = TargetPlayer->GetActorLocation();
        LastSeenPlayerTime = GetWorld()->GetTimeSeconds();

        //Player is visible again so allow another. Lost sight cover attempt in the future
        bHasTriedLostSightCover = false;
    }

    if (IsEnemyCombatState(EEnemyCombatState::InCover))
    {
        StopFiring();
        StopChasing();

        // We can still see the player.
        // Let the existing cover peek/fire system control combat.
        if (bCanSeePlayer)
        {
            return;
        }

        // We lost sight of the player, but don't immediately abandon cover.
        const float TimeSincePlayerSeen =
            GetWorld()->GetTimeSeconds() - LastSeenPlayerTime;

        if (TimeSincePlayerSeen < LostSightInvestigationDelay)
        {
            return;
        }

        // Player has remained hidden long enough.
        // Now leave cover and investigate the last known position.
        bIsInCover = false;
        bIsPeekingFromCover = false;

        GetWorldTimerManager().ClearTimer(CoverHoldTimerHandle);
        GetWorldTimerManager().ClearTimer(CoverPeekTimerHandle);
        GetWorldTimerManager().ClearTimer(CoverBurstTimerHandle);

        SetEnemyCombatState(EEnemyCombatState::Investigating);
        InvestigateLastKnownLocation();

        return;
    }
    

    //Repositioning Check
    if (IsEnemyCombatState(EEnemyCombatState::Repositioning))
    {
        return;
    }

    if (bCanSeePlayer &&
        IsEnemyCombatState(EEnemyCombatState::Searching))
    {
        CancelSearching();
    }

    //Let the search state continue without normal combat logic overriding it
    if (IsEnemyCombatState(EEnemyCombatState::Searching) && !bCanSeePlayer)
    {
        return;
    }

    if (IsPlayerInCombatRange() && bCanSeePlayer)
    {
        bIsPlayerInCombatRange = true;
        StopChasing();

        if (!IsEnemyCombatState(EEnemyCombatState::Repositioning) &&
            !IsEnemyCombatState(EEnemyCombatState::InCover))
        {
            SetEnemyCombatState(EEnemyCombatState::Attacking);
            StartFiring();
        }

        return;
    }

    if (bCanSeePlayer)
    {
        SetEnemyCombatState(EEnemyCombatState::Chasing);

        bIsPlayerInCombatRange = false;
        StopFiring();
        ChasePlayer();

        return;
    }

    //Walk Around Looking and investigating (Searching!!)
    if (IsEnemyCombatState(EEnemyCombatState::Investigating) &&
        HasReachedLastKnownLocation())
    {
        StartSearching();
        return;
    }

    if (LastKnownPlayerLocation.IsNearlyZero())
    {
        if (!IsEnemyCombatState(EEnemyCombatState::Patrolling)) {
            SetEnemyCombatState(EEnemyCombatState::Idle);
        }

        StopFiring();
        return;
    }


    if (bHasEnemyEnteredCombat)
    {
        const float TimeSincePlayerSeen =
            GetWorld()->GetTimeSeconds() - LastSeenPlayerTime;

        if (TimeSincePlayerSeen < LostSightInvestigationDelay)
        {
            bIsPlayerInCombatRange = false;

            StopFiring();
            StopChasing();

            // Try to find tactical cover once after losing sight.
            if (!bHasTriedLostSightCover)
            {
                bHasTriedLostSightCover = true;
                TryEnterCombatCover(false);
            }

            return;
        }
    }


    SetEnemyCombatState(EEnemyCombatState::Investigating);

    bIsPlayerInCombatRange = false;
    StopFiring();
    InvestigateLastKnownLocation();
}

void AEnemyCharacter::SetEnemyCombatState(EEnemyCombatState NewState)
{
    if (EnemyCombatState == NewState)
    {
        return;
    }

    EnemyCombatState = NewState;
}

bool AEnemyCharacter::IsEnemyCombatState(EEnemyCombatState StateToCheck) const
{
    return EnemyCombatState == StateToCheck;
}


// ======================================================
// MOVEMENT / TARGETING
// ======================================================

void AEnemyCharacter::ChasePlayer()
{
    if (TargetPlayer == nullptr) return;
    
    GetCharacterMovement()->MaxWalkSpeed = 450.0f;

    AAIController* EnemyAIController = Cast<AAIController>(GetController());

    if (EnemyAIController)
    {
        EnemyAIController->MoveToActor(TargetPlayer, CombatRange - 300.0f, false);
    }
}

void AEnemyCharacter::StopChasing()
{
    AAIController* EnemyAIController = Cast<AAIController>(GetController());

    if (EnemyAIController)
    {
        EnemyAIController->StopMovement();
    }
}

void AEnemyCharacter::FaceTarget(float DeltaTime)
{
    if (TargetPlayer == nullptr) return;

    FVector DirectionToPlayer = TargetPlayer->GetActorLocation() - GetActorLocation();
    DirectionToPlayer.Z = 0.0f;

    FRotator LookAtRotation = DirectionToPlayer.Rotation();
    FRotator SmoothRotation = FMath::RInterpTo(
        GetActorRotation(),
        LookAtRotation,
        DeltaTime,
        8.0f
    );

    SetActorRotation(SmoothRotation);
}

void AEnemyCharacter::StartPatrolling() {
    if (bIsDead) {
        return;
    }

    AAIController* EnemyAIController = Cast<AAIController>(GetController());

    if (EnemyAIController == nullptr) {
        return;
    }

    GetCharacterMovement()->MaxWalkSpeed = 150.0f;

    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());

    if (NavSystem == nullptr) {
        return;
    }

   
    if (PatrolPoints.Num() == 0) {
        return;
    }

    ATargetPoint* CurrentPatrolPoint = PatrolPoints[CurrentPatrolPointIndex];

    if (CurrentPatrolPoint == nullptr) {
        return;
    }

    EnemyCombatState = EEnemyCombatState::Patrolling;

    //Bug where enemy wasn't moving back to patrol after completing search
    EPathFollowingRequestResult::Type MoveResult =
        EnemyAIController->MoveToActor(
            CurrentPatrolPoint,
            50.0f
        );

    if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal ||
        MoveResult == EPathFollowingRequestResult::Failed)
    {
        OnPatrolMoveCompleted();
    }

}

void AEnemyCharacter::OnPatrolMoveCompleted() {

    if (!IsEnemyCombatState(EEnemyCombatState::Patrolling)) {
        return;
    }

    if (PatrolPoints.Num() == 0) {
        return;
    }

    CurrentPatrolPointIndex++;

    if (CurrentPatrolPointIndex >= PatrolPoints.Num()) {
        CurrentPatrolPointIndex = 0;
    }

    GetWorldTimerManager().SetTimer(
        PatrolWaitTimerHandle,
        this,
        &AEnemyCharacter::StartPatrolling,
        PatrolWaitTime,
        false
    );
}

// ======================================================
// NORMAL COMBAT
// ======================================================

void AEnemyCharacter::StartFiring()
{
    if (GetWorldTimerManager().IsTimerActive(FireStartDelayTimerHandle)||
        GetWorldTimerManager().IsTimerActive(BurstShotTimerHandle) ||
        GetWorldTimerManager().IsTimerActive(BurstPauseTimerHandle))
    {
        return;
    }

    
    GetWorldTimerManager().SetTimer(
        FireStartDelayTimerHandle,
        this,
        &AEnemyCharacter::StartBurst,
        FireStartDelay,
        false
    );

}

void AEnemyCharacter::StopFiring()
{
    GetWorldTimerManager().ClearTimer(FireStartDelayTimerHandle);
    GetWorldTimerManager().ClearTimer(BurstShotTimerHandle);
    GetWorldTimerManager().ClearTimer(BurstPauseTimerHandle);
    

    CurrentBurstShot = 0;
}

void AEnemyCharacter::StartBurst()
{
    if (bIsDead) return;
    if (TargetPlayer == nullptr) return;
    if (!IsEnemyCombatState(EEnemyCombatState::Attacking))
    {
        return;
    }

    CurrentBurstShot = 0;
    CurrentBurstSize = FMath::RandRange(MinBurstShots, MaxBurstsShots);

    FireBurstShot();
}

void AEnemyCharacter::FireBurstShot()
{
    if (bIsDead) return;
    if (TargetPlayer == nullptr) return;
    if (!IsEnemyCombatState(EEnemyCombatState::Attacking)) return;



    FireWeapon();

    CurrentBurstShot++;

    if (CurrentBurstShot >= CurrentBurstSize)
    {
        CompletedBursts++;

        if (CompletedBursts >= BurstsBeforeReposition)
        {
            StartRepositioning();
            return;
        }

        GetWorldTimerManager().SetTimer(
            BurstPauseTimerHandle,
            this,
            &AEnemyCharacter::StartBurst,
            TimeBetweenBursts,
            false
        );

        return;
    }

    GetWorldTimerManager().SetTimer(
        BurstShotTimerHandle,
        this,
        &AEnemyCharacter::FireBurstShot,
        TimeBetweenShots,
        false
    );
}

void AEnemyCharacter::FireWeapon()
{
    if (TargetPlayer == nullptr) return;
    if (bIsDead) return;

    if (!HasClearShot())
    {
        return;
    }

    FVector Start = EnemyWeaponMesh->GetSocketLocation(MuzzleSocketName);
    FRotator MuzzleRotation = EnemyWeaponMesh->GetSocketRotation(MuzzleSocketName);

    if (MuzzleFlashClass)
    {
        GetWorld()->SpawnActor<AActor>(
            MuzzleFlashClass,
            Start,
            MuzzleRotation
        );
    }

    if (EnemyFireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            EnemyFireSound,
            Start
        );
    }

    if (FireMontage)
    {
        PlayAnimMontage(FireMontage);
    }

    //Miss 25% of shots with random generator
    FVector TargetLocation = TargetPlayer->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
    bool bAccurateShot = FMath::FRandRange(0.0f, 100.0f) <= AccuracyPercent;
    if (!bAccurateShot)
    {
        TargetLocation += FVector(
            FMath::FRandRange(-80.0f, 80.0f),
            FMath::FRandRange(-80.0f, 80.0f),
            FMath::FRandRange(-40.0f, 80.0f)
        );
    }


    FVector DirectionToPlayer = TargetLocation - Start;
    DirectionToPlayer.Normalize();

    const int32 SafePelletCount =
        FMath::Max(1, PelletsPerShot);

    const float DamagePerPellet =
        FireDamage / SafePelletCount;

    int32 PelletsHitPlayer = 0;

    bool bHasStoredHit = false;
    FHitResult StoredPlayerHit;

    for (int32 PelletIndex = 0;
        PelletIndex < SafePelletCount;
        PelletIndex++)
    {
        FVector PelletDirection = DirectionToPlayer;

        // Shotguns spread each pellet inside a cone.
        // Pistol/rifle enemies have 0 spread, so their
        // existing behavior remains unchanged.
        if (ShotSpreadDegrees > 0.0f)
        {
            PelletDirection = FMath::VRandCone(
                DirectionToPlayer,
                FMath::DegreesToRadians(ShotSpreadDegrees)
            );
        }

        const FVector End =
            Start + PelletDirection * FireRange;

        FHitResult HitResult;

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);

        const bool bHit =
            GetWorld()->LineTraceSingleByChannel(
                HitResult,
                Start,
                End,
                ECC_Pawn,
                QueryParams
            );

        if (!bHit)
        {
            continue;
        }

        ACombatCharacter* HitPlayer =
            Cast<ACombatCharacter>(
                HitResult.GetActor()
            );

        if (HitPlayer)
        {
            PelletsHitPlayer++;

            // Save one impact location for the blood effect.
            // We don't want to spawn 8 blood effects at once.
            if (!bHasStoredHit)
            {
                StoredPlayerHit = HitResult;
                bHasStoredHit = true;
            }
        }
    }

    // Apply all pellet damage as one damage event.
    if (PelletsHitPlayer > 0)
    {
        const float TotalDamage =
            DamagePerPellet * PelletsHitPlayer;

        TargetPlayer->ReceiveDamage(TotalDamage);

        if (BloodSplatterClass && bHasStoredHit)
        {
            FRotator BloodRotation =
                StoredPlayerHit.ImpactNormal.Rotation();

            GetWorld()->SpawnActor<AActor>(
                BloodSplatterClass,
                StoredPlayerHit.ImpactPoint,
                BloodRotation
            );
        }
    }
}

// ======================================================
// REPOSITIONING
// ======================================================

void AEnemyCharacter::StartRepositioning()
{
    if (bIsDead) return;

    //Aggressive enemies such as the shotgunner advance toward the player instead of cover
    if (!bUsesCover) {
        if (TargetPlayer == nullptr) {
            return;
        }

        SetEnemyCombatState(EEnemyCombatState::Repositioning);

        bIsRepositioning = true;
        bIsPlayerInCombatRange = false;
        bRepositioningToCover = false;

        StopFiring();
        CompletedBursts = 0;

        AAIController* EnemyAIController = Cast<AAIController>(GetController());

        if (EnemyAIController) {
            EnemyAIController->MoveToActor(
                TargetPlayer,
                AggressiveAdvanceDistance
            );
        }
        return;
    }

    
    GetCharacterMovement()->MaxWalkSpeed = 450.0f;

    SetEnemyCombatState(EEnemyCombatState::Repositioning);

    bIsRepositioning = true;
    bIsPlayerInCombatRange = false;

    StopFiring();
    CompletedBursts = 0;

    GenerateCoverCandidates();

    FVector RepositionPoint;
    bool bFoundCover = ChooseCoverPoint(RepositionPoint);
    bRepositioningToCover = bFoundCover;

    //Changed from strafing left <->right to just finding cover.
    if (!bFoundCover) {
        bIsRepositioning = false;
        bRepositioningToCover = false;

        SetEnemyCombatState(EEnemyCombatState::Attacking);
        StartFiring();
        return;

    }


    CurrentRepositionDestination = RepositionPoint;
    AAIController* EnemyAIController = Cast<AAIController>(GetController());
    if (EnemyAIController)
    {
        EnemyAIController->MoveToLocation(
            CurrentRepositionDestination,
            20.0f
        );
    }
}

bool AEnemyCharacter::FindRepositionPoint(FVector& OutPoint) const
{
    if (TargetPlayer == nullptr)
    {
        return false;
    }

    UNavigationSystemV1* NavSystem =
        UNavigationSystemV1::GetCurrent(GetWorld());

    if (NavSystem == nullptr)
    {
        return false;
    }

    FVector ToPlayer =
        TargetPlayer->GetActorLocation() - GetActorLocation();

    ToPlayer.Z = 0.0f;
    ToPlayer.Normalize();

    FVector RightVector =
        FVector::CrossProduct(FVector::UpVector, ToPlayer);

    RightVector.Normalize();

    const float SideMultiplier =
        FMath::RandBool() ? 1.0f : -1.0f;

    const float SideDistance =
        FMath::FRandRange(400.0f, RepositionRadius);

    FVector DesiredPoint =
        GetActorLocation() +
        RightVector * SideMultiplier * SideDistance;

    FNavLocation ProjectedLocation;

    const bool bFoundPoint =
        NavSystem->ProjectPointToNavigation(
            DesiredPoint,
            ProjectedLocation
        );

    if (!bFoundPoint)
    {
        return false;
    }

    OutPoint = ProjectedLocation.Location;

    return true;
}

bool AEnemyCharacter::HasReachedRepositionDestination() const
{
    if (CurrentRepositionDestination.IsNearlyZero())
    {
        return false;
    }

    float Distance = FVector::Dist2D(
        GetActorLocation(),
        CurrentRepositionDestination
    );

    return Distance <= 180.0f;
}


// ======================================================
// COVER
// ======================================================

void AEnemyCharacter::TryEnterCombatCover(bool bResumeAttackIfNoCover)
{
    if (bIsDead) return;
    if (TargetPlayer == nullptr) return;

    if (!bUsesCover) {
        return;
    }

    GetCharacterMovement()->MaxWalkSpeed = 350.0f;
    GenerateCoverCandidates();

    FVector CoverPoint;

    if (ChooseCoverPoint(CoverPoint))
    {
        // Stop old combat/movement FIRST
        StopFiring();
        StopChasing();

        // NOW begin the new cover movement state
        bRepositioningToCover = true;
        bIsRepositioning = true;

        SetEnemyCombatState(EEnemyCombatState::Repositioning);

        CurrentRepositionDestination = CoverPoint;

        AAIController* EnemyAIController =
            Cast<AAIController>(GetController());

        if (EnemyAIController)
        {
            EnemyAIController->MoveToLocation(
                CurrentRepositionDestination,
                20.0f
            );
        }

        return;
    }

    if (bResumeAttackIfNoCover) {
        SetEnemyCombatState(EEnemyCombatState::Attacking);
        StartFiring();
    }
}

void AEnemyCharacter::GenerateCoverCandidates()
{
    CoverCandidates.Empty();
    CoveredCandidates.Empty();

    if (TargetPlayer == nullptr)
    {
        return;
    }

    UNavigationSystemV1* NavSystem =
        UNavigationSystemV1::GetCurrent(GetWorld());

    if (NavSystem == nullptr)
    {
        return;
    }

    const int32 NumberOfDirections = 16;

    const FVector EnemyLocation = GetActorLocation();

    for (int32 i = 0; i < NumberOfDirections; i++)
    {
        const float Angle =
            (360.0f / NumberOfDirections) * i;

        const FVector SearchDirection =
            FRotator(0.0f, Angle, 0.0f).Vector();

        const float CapsuleHalfHeight =
            GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

        const FVector EnemyFeetLocation =
            EnemyLocation - FVector(0.0f, 0.0f, CapsuleHalfHeight);

        const FVector TraceStart =
            EnemyFeetLocation + FVector(0.0f, 0.0f, 45.0f);

        const FVector TraceEnd =
            TraceStart +
            SearchDirection * CoverSearchRadius;

        FHitResult HitResult;

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        QueryParams.AddIgnoredActor(TargetPlayer);

        const bool bHit =
            GetWorld()->LineTraceSingleByChannel(
                HitResult,
                TraceStart,
                TraceEnd,
                ECC_Visibility,
                QueryParams
            );

        if (!bHit)
        {
            continue;
        }

        // Get the horizontal surface normal from the obstacle.
        FVector SurfaceNormal = HitResult.ImpactNormal;
        SurfaceNormal.Z = 0.0f;

        if (!SurfaceNormal.Normalize())
        {
            continue;
        }

        const float CapsuleRadius =
            GetCapsuleComponent()->GetScaledCapsuleRadius();

        const float CoverPadding = 5.0f;

        // Place the enemy just outside the obstacle surface.
        FVector DesiredCoverPoint =
            HitResult.ImpactPoint +
            SurfaceNormal * (CapsuleRadius + CoverPadding);

        FNavLocation ProjectedLocation;

        const FVector ProjectionExtent(150.0f, 150.0f, 200.0f);
        const bool bProjected =
            NavSystem->ProjectPointToNavigation(
                DesiredCoverPoint,
                ProjectedLocation,
                ProjectionExtent
            );

        if (!bProjected)
        {
            continue;
        }

        if (!IsCandidateCoveredFromPlayer(ProjectedLocation.Location, HitResult.GetActor()))
        {
            continue;
        }

        CoveredCandidates.Add(ProjectedLocation.Location);
        
    }
}

bool AEnemyCharacter::IsCandidateCoveredFromPlayer(const FVector& CandidatePoint, const AActor* ExpectedCoverActor) const
{
    if (TargetPlayer == nullptr)
    {
        return false;
    }

    if (ExpectedCoverActor == nullptr) {
        return false;
    }

    // Player viewing position.
    const FVector PlayerViewLocation =
        TargetPlayer->GetActorLocation() +
        FVector(0.0f, 0.0f, 50.0f);

    // Approximate crouched body height at the cover location.
    const FVector CoverCenter =
        CandidatePoint +
        FVector(0.0f, 0.0f, 35.0f);

    // Direction from the player toward the cover point.
    FVector PlayerToCover =
        CandidatePoint - TargetPlayer->GetActorLocation();

    PlayerToCover.Z = 0.0f;

    if (!PlayerToCover.Normalize())
    {
        return false;
    }

    // Get a horizontal direction pointing across the enemy's body.
    const FVector SideDirection =
        FVector::CrossProduct(
            FVector::UpVector,
            PlayerToCover
        ).GetSafeNormal();

    // Check roughly the width of the enemy's capsule.
    const float CapsuleRadius =
        GetCapsuleComponent()->GetScaledCapsuleRadius();

    const float BodyCheckWidth =
        CapsuleRadius * 0.8f;

    const FVector LeftPoint =
        CoverCenter -
        SideDirection * BodyCheckWidth;

    const FVector RightPoint =
        CoverCenter +
        SideDirection * BodyCheckWidth;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(TargetPlayer);
    QueryParams.AddIgnoredActor(this);

    // Center
    FHitResult CenterHit;

    const bool bCenterBlocked =
        GetWorld()->LineTraceSingleByChannel(
            CenterHit,
            PlayerViewLocation,
            CoverCenter,
            ECC_Visibility,
            QueryParams
        );

    // The obstacle that created this cover point must actually be between the player and the enemy.
    if (!bCenterBlocked ||
        CenterHit.GetActor() != ExpectedCoverActor)
    {
        return false;
    }

    // Left side
    FHitResult LeftHit;

    const bool bLeftBlocked =
        GetWorld()->LineTraceSingleByChannel(
            LeftHit,
            PlayerViewLocation,
            LeftPoint,
            ECC_Visibility,
            QueryParams
        );

    // Right side
    FHitResult RightHit;

    const bool bRightBlocked =
        GetWorld()->LineTraceSingleByChannel(
            RightHit,
            PlayerViewLocation,
            RightPoint,
            ECC_Visibility,
            QueryParams
        );

    // The enemy is only considered safely in cover if
    // his center, left side, and right side are all protected.
    return
        bCenterBlocked &&
        bLeftBlocked &&
        bRightBlocked;
}

bool AEnemyCharacter::ChooseCoverPoint(FVector& OutCoverPoint) const
{
    if (CoveredCandidates.Num() == 0)
    {
        return false;
    }

    const int32 RandomIndex = FMath::RandRange(0, CoveredCandidates.Num() - 1);

    OutCoverPoint = CoveredCandidates[RandomIndex];
    return true;
}

void AEnemyCharacter::StartCoverPeek()
{
    if (bIsDead) return;
    if (!bIsInCover) return;

    bIsPeekingFromCover = true;

    CurrentCoverBurstShot = 0;
    FireCoverBurstShot();

    GetWorldTimerManager().SetTimer(
        CoverPeekTimerHandle,
        this,
        &AEnemyCharacter::ReturnToCover,
        1.5f,
        false
    );
}

void AEnemyCharacter::FireCoverBurstShot()
{
    if (bIsDead) return;
    if (!bIsInCover) return;
    if (!bIsPeekingFromCover) return;
    if (TargetPlayer == nullptr) return;

    FireWeapon();

    CurrentCoverBurstShot++;

    if (CurrentCoverBurstShot >= CoverBurstSize)
    {
        return;
    }

    GetWorldTimerManager().SetTimer(
        CoverBurstTimerHandle,
        this,
        &AEnemyCharacter::FireCoverBurstShot,
        CoverTimeBetweenShots,
        false
    );
}

void AEnemyCharacter::ReturnToCover()
{
    if (bIsDead) return;
    if (!bIsInCover) return;

    GetWorldTimerManager().ClearTimer(CoverBurstTimerHandle);

    bIsPeekingFromCover = false;

    GetWorldTimerManager().SetTimer(
        CoverPeekTimerHandle,
        this,
        &AEnemyCharacter::StartCoverPeek,
        CoverPeekDelay,
        false
    );
}

void AEnemyCharacter::ExitCover()
{
    if (bIsDead) return;

    bIsInCover = false;

    GetWorldTimerManager().ClearTimer(CoverHoldTimerHandle);

    SetEnemyCombatState(EEnemyCombatState::Attacking);

    StartFiring();
}


// ======================================================
// AI MOVEMENT CALLBACKS
// ======================================================

void AEnemyCharacter::OnRepositionMoveCompleted()
{
    if (bIsDead)
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("REPOSITION SUCCESS - ToCover: %s | InCombatRange: %s | Distance: %.0f"),
        bRepositioningToCover ? TEXT("YES") : TEXT("NO"),
        bIsPlayerInCombatRange ? TEXT("YES") : TEXT("NO"),
        TargetPlayer
        ? FVector::Dist2D(GetActorLocation(), TargetPlayer->GetActorLocation())
        : -1.0f
    );

    if (!IsEnemyCombatState(EEnemyCombatState::Repositioning))
    {
        return;
    }

    bIsRepositioning = false;
    CurrentRepositionDestination = FVector::ZeroVector;

    CompletedBursts = 0;
    CurrentBurstShot = 0;

    if (bRepositioningToCover)
    {
        bIsInCover = true;

        bIsPeekingFromCover = false;

        SetEnemyCombatState(EEnemyCombatState::InCover);

        GetWorldTimerManager().SetTimer(
            CoverHoldTimerHandle,
            this,
            &AEnemyCharacter::StartCoverPeek,
            CoverPeekDelay,
            false
        );
    }
    else
    {
        SetEnemyCombatState(EEnemyCombatState::Attacking);
        StartFiring();
    }

    bRepositioningToCover = false;
}

void AEnemyCharacter::OnRepositionMoveFailed()
{
    if (bIsDead)
    {
        return;
    }

        if (!IsEnemyCombatState(EEnemyCombatState::Repositioning))
        {
            return;
        }

    bIsRepositioning = false;
    bRepositioningToCover = false;
    CurrentRepositionDestination = FVector::ZeroVector;

    SetEnemyCombatState(EEnemyCombatState::Attacking);
    StartFiring();
}


// ======================================================
// INVESTIGATION / SEARCH
// ======================================================

void AEnemyCharacter::InvestigateLastKnownLocation()
{
    if (LastKnownPlayerLocation.IsNearlyZero()) return;

    AAIController* EnemyAIController = Cast<AAIController>(GetController());

    if (EnemyAIController)
    {
        EnemyAIController->MoveToLocation(
            LastKnownPlayerLocation,
            100.0f
        );
    }
}

bool AEnemyCharacter::HasReachedLastKnownLocation() const
{
    if (LastKnownPlayerLocation.IsNearlyZero())
    {
        return false;
    }

    const float Distance = FVector::Dist2D(
        GetActorLocation(),
        LastKnownPlayerLocation
    );

    return Distance <= 150.0f;
}

void AEnemyCharacter::StartSearching()
{
    if (bIsDead) return;

    SetEnemyCombatState(EEnemyCombatState::Searching);

    StopChasing();
    StopFiring();

    GetCharacterMovement()->MaxWalkSpeed = SearchMoveSpeed;

    MoveToNextSearchPoint();

    GetWorldTimerManager().SetTimer(
        SearchTimerHandle,
        this,
        &AEnemyCharacter::StopSearching,
        SearchDuration,
        false
    );
}

void AEnemyCharacter::MoveToNextSearchPoint()
{
    if (bIsDead) return;

    bool bFoundPoint = false;
    FVector SearchPoint = GetRandomSearchPoint(bFoundPoint);

    if (!bFoundPoint)
    {
        return;
    }

    CurrentSearchDestination = SearchPoint;

    AAIController* EnemyAIController = Cast<AAIController>(GetController());

    if (EnemyAIController)
    {
        EnemyAIController->StopMovement();
        EPathFollowingRequestResult::Type Result =
            EnemyAIController->MoveToLocation(CurrentSearchDestination, 50.0f);
    }

}

FVector AEnemyCharacter::GetRandomSearchPoint(bool& bOutFound) const
{
    bOutFound = false;

    UNavigationSystemV1* NavSystem =
        UNavigationSystemV1::GetCurrent(GetWorld());

    if (NavSystem == nullptr)
    {
        return FVector::ZeroVector;
    }

    AAIController* EnemyAIController =
        Cast<AAIController>(GetController());

    if (EnemyAIController == nullptr)
    {
        return FVector::ZeroVector;
    }

    ANavigationData* NavData = NavSystem->GetNavDataForProps(
        EnemyAIController->GetNavAgentPropertiesRef(),
        GetActorLocation()
    );

    if (NavData == nullptr)
    {
        return FVector::ZeroVector;
    }

    const int32 MaxAttempts = 10;

    for (int32 Attempt = 0; Attempt < MaxAttempts; Attempt++)
    {
        FNavLocation ResultLocation;

        const bool bFoundReachablePoint =
            NavSystem->GetRandomReachablePointInRadius(
                LastKnownPlayerLocation,
                SearchPointRadius,
                ResultLocation,
                NavData
            );

        if (!bFoundReachablePoint)
        {
            continue;
        }

        const float DistanceFromEnemy = FVector::Dist2D(
            GetActorLocation(),
            ResultLocation.Location
        );

        if (DistanceFromEnemy >= MinSearchPointDistance)
        {
            bOutFound = true;
            return ResultLocation.Location;
        }
    }

    return FVector::ZeroVector;
}

bool AEnemyCharacter::HasReachedSearchDestination() const
{
    if (CurrentSearchDestination.IsNearlyZero())
    {
        return false;
    }

    const float Distance = FVector::Dist2D(
        GetActorLocation(),
        CurrentSearchDestination
    );

    return Distance <= 100.0f;
}

void AEnemyCharacter::CancelSearching()
{
    GetWorldTimerManager().ClearTimer(SearchTimerHandle);

    CurrentSearchDestination = FVector::ZeroVector;

    GetCharacterMovement()->MaxWalkSpeed = DefaultMoveSpeed;
}

void AEnemyCharacter::StopSearching()
{
    if (bIsDead) return;

    GetCharacterMovement()->MaxWalkSpeed = DefaultMoveSpeed;

    StopChasing();
    StopFiring();

    CurrentSearchDestination = FVector::ZeroVector;
    LastKnownPlayerLocation = FVector::ZeroVector;

    bCanSeePlayer = false;
    bIsPlayerInCombatRange = false;
    bHasEnemyEnteredCombat = false;

    if (TargetPlayer &&
        !DetectionSphere->IsOverlappingActor(TargetPlayer))
    {
        TargetPlayer = nullptr;

        GetWorldTimerManager().ClearTimer(CombatCheckTimerHandle);
        SetActorTickEnabled(false);
    }
    StartPatrolling();
}


// ======================================================
// SUPPRESSION
// ======================================================

void AEnemyCharacter::EnterSuppressedState()
{
    if (bIsDead) return;
    if (IsEnemyCombatState(EEnemyCombatState::Suppressed)) return;

    SetEnemyCombatState(EEnemyCombatState::Suppressed);

    bIsSupressed = true;

    StopChasing();
    StopFiring();

    bIsPlayerInCombatRange = false;

    GetWorldTimerManager().SetTimer(
        SuppressionTimerHandle,
        this,
        &AEnemyCharacter::ExitSuppressedState,
        SuppressedDuration,
        false
    );
}

void AEnemyCharacter::ExitSuppressedState()
{
    if (bIsDead) return;

    RecentHitCount = 0;

    bIsSupressed = false;

    SetEnemyCombatState(EEnemyCombatState::Idle);

    CheckCombatState();
}

void AEnemyCharacter::ResetRecentHitCount()
{
    RecentHitCount = 0;
}


// ======================================================
// HEALTH / DEATH
// ======================================================

void AEnemyCharacter::ReceiveDamage(float DamageAmount, ACombatCharacter* DamageSource)
{

    if (bIsDead) return;

    if (HitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            HitSound,
            GetActorLocation()
        );
    }

    // Apply damage FIRST
    CurrentHealth -= DamageAmount;
    CurrentHealth = FMath::Clamp(
        CurrentHealth,
        0.0f,
        MaxHealth
    );

    // If this shot killed player, do nothing else
    if (CurrentHealth <= 0.0f)
    {
        Die();
        return;
    }

    // Only living enemies react and shoot back
    if (DamageSource != nullptr)
    {
        TargetPlayer = DamageSource;

        LastKnownPlayerLocation =
            DamageSource->GetActorLocation();

        bHasEnemyEnteredCombat = true;

        GetWorldTimerManager().ClearTimer(
            PatrolWaitTimerHandle
        );

        StopChasing();

        SetActorTickEnabled(true);

        SetEnemyCombatState(
            EEnemyCombatState::Attacking
        );

        StartFiring();
    }

    if (HitReactionMontage)
    {
        PlayAnimMontage(HitReactionMontage);
    }
}

void AEnemyCharacter::Die()
{
    bIsDead = true;

    SetEnemyCombatState(EEnemyCombatState::Dead);

    // Stop all AI behavior
    StopChasing();
    StopFiring();

    GetWorldTimerManager().ClearTimer(CombatCheckTimerHandle);
    SetActorTickEnabled(false);

    // Stop character movement
    GetCharacterMovement()->DisableMovement();

    // Dead enemies no longer need gameplay collision/detection
    GetCapsuleComponent()->SetCollisionEnabled(
        ECollisionEnabled::NoCollision
    );

    DetectionSphere->SetCollisionEnabled(
        ECollisionEnabled::NoCollision
    );

    GetMesh()->SetCollisionEnabled(
        ECollisionEnabled::NoCollision
    );

    // Play death animation
    if (DeathAnimation)
    {
        GetMesh()->PlayAnimation(DeathAnimation, false);
    }

    // Drop weapon
    EnemyWeaponMesh->DetachFromComponent(
        FDetachmentTransformRules::KeepWorldTransform
    );

    EnemyWeaponMesh->SetSimulatePhysics(true);

    EnemyWeaponMesh->SetCollisionEnabled(
        ECollisionEnabled::PhysicsOnly
    );

    // Remove corpse after 10 seconds
    SetLifeSpan(10.0f);
}