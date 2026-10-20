#include "BTTask_NPCAttack.h"
#include "EnemyNPC.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_NPCAttack::UBTTask_NPCAttack()
{
	NodeName = TEXT("NPC Attack");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_NPCAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	AEnemyNPC* NPC = AI ? Cast<AEnemyNPC>(AI->GetPawn()) : nullptr;
	if (!NPC)
	{
		return EBTNodeResult::Failed;
	}

	AI->StopMovement();
	reinterpret_cast<FNPCAttackMemory*>(NodeMemory)->bStarted = false;
	return EBTNodeResult::InProgress;
}

void UBTTask_NPCAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	AEnemyNPC* NPC = AI ? Cast<AEnemyNPC>(AI->GetPawn()) : nullptr;
	if (!NPC || NPC->IsDead())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FNPCAttackMemory* Memory = reinterpret_cast<FNPCAttackMemory*>(NodeMemory);
	if (!Memory->bStarted)
	{
		// Чекаємо кулдауну, потім починаємо удар
		Memory->bStarted = NPC->StartAttack();
		return;
	}

	if (!NPC->IsAttacking())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
