// Лабораторна робота 5: завдання Behavior Tree - удар посохом

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_NPCAttack.generated.h"

struct FNPCAttackMemory
{
	bool bStarted = false;
};

/** Чекає кулдауну, б'є посохом і завершується після кінця анімації удару */
UCLASS()
class MYPROJECT_API UBTTask_NPCAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_NPCAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FNPCAttackMemory); }
};
