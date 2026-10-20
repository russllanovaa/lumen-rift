#include "BTTask_LookAround.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_LookAround::UBTTask_LookAround()
{
	NodeName = TEXT("Look Around");
	bNotifyTick = true;
}

FString UBTTask_LookAround::GetStaticDescription() const
{
	return FString::Printf(TEXT("Огляд %.1f c, +-%.0f град"), Duration, SwingAngle);
}

EBTNodeResult::Type UBTTask_LookAround::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	AI->StopMovement();

	FLookAroundMemory* Memory = reinterpret_cast<FLookAroundMemory*>(NodeMemory);
	Memory->Elapsed = 0.0f;
	Memory->BaseYaw = Pawn->GetActorRotation().Yaw;
	return EBTNodeResult::InProgress;
}

void UBTTask_LookAround::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FLookAroundMemory* Memory = reinterpret_cast<FLookAroundMemory*>(NodeMemory);
	Memory->Elapsed += DeltaSeconds;

	const float Alpha = FMath::Clamp(Memory->Elapsed / FMath::Max(Duration, 0.1f), 0.0f, 1.0f);
	const float Offset = FMath::Sin(Alpha * 2.0f * PI * Swings) * SwingAngle;
	Pawn->SetActorRotation(FRotator(0.0f, Memory->BaseYaw + Offset, 0.0f));

	if (Alpha >= 1.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
