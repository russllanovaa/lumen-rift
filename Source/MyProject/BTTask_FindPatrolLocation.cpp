#include "BTTask_FindPatrolLocation.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_FindPatrolLocation::UBTTask_FindPatrolLocation()
{
	NodeName = TEXT("Find Patrol Location");
	LocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolLocation, LocationKey));
}

void UBTTask_FindPatrolLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		LocationKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_FindPatrolLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Випадкова точка в радіусі %.0f -> %s"), Radius, *LocationKey.SelectedKeyName.ToString());
}

EBTNodeResult::Type UBTTask_FindPatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !Nav || !BB)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation Result;
	if (Nav->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), Radius, Result))
	{
		BB->SetValueAsVector(LocationKey.SelectedKeyName, Result.Location);
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::Failed;
}
