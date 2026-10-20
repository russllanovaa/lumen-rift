// Лабораторна робота 5: завдання Behavior Tree - зупинка й оглядання

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_LookAround.generated.h"

struct FLookAroundMemory
{
	float Elapsed = 0.0f;
	float BaseYaw = 0.0f;
};

/** NPC зупиняється й повертає корпус вліво-вправо, оглядаючись, потім завершується успіхом */
UCLASS()
class MYPROJECT_API UBTTask_LookAround : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LookAround();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FLookAroundMemory); }
	virtual FString GetStaticDescription() const override;

protected:
	// Скільки секунд оглядається
	UPROPERTY(EditAnywhere, Category = "Look")
	float Duration = 3.0f;

	// На який кут повертається в кожен бік
	UPROPERTY(EditAnywhere, Category = "Look")
	float SwingAngle = 70.0f;

	// Скільки повних коливань за Duration
	UPROPERTY(EditAnywhere, Category = "Look")
	float Swings = 1.0f;
};
