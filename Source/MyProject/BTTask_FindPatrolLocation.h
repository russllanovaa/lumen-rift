// Лабораторна робота 5: завдання Behavior Tree - випадкова точка патрулю на NavMesh

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_FindPatrolLocation.generated.h"

UCLASS()
class MYPROJECT_API UBTTask_FindPatrolLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:
	// Ключ Blackboard (Vector), куди записується знайдена точка
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector LocationKey;

	// Радіус пошуку навколо NPC, см
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float Radius = 2000.0f;
};
