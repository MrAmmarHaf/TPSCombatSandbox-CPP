// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "Navigation/PathFollowingComponent.h"

void AEnemyAIController::OnMoveCompleted(
	FAIRequestID  RequestID,
	const FPathFollowingResult& Result
)
{
	Super::OnMoveCompleted(RequestID, Result);


	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(GetPawn());

	if (EnemyCharacter == nullptr) {
		return;
	}


	if (!Result.IsSuccess()) {
		UE_LOG(LogTemp, Warning, TEXT("MOVE FAILED OR ABORTEED"));

		EnemyCharacter->OnRepositionMoveFailed();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Move Succeeded"));

	EnemyCharacter->OnRepositionMoveCompleted();

	EnemyCharacter->OnPatrolMoveCompleted();

}
