// Лабораторна робота 5: контролер ШІ для EnemyNPC

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

class UBehaviorTree;
class UAISenseConfig_Sight;

// Команди (Affiliation): гравець = 1 (див. SabrinaCharacter::GetGenericTeamId), NPC = 2
namespace NPCTeam
{
	constexpr uint8 Player = 1;
	constexpr uint8 Enemy = 2;
}

// Ключі Blackboard (створи їх з такими самими назвами в ассеті BB_EnemyNPC)
namespace NPCBB
{
	static const FName TargetActor(TEXT("TargetActor"));              // Object (Actor)
	static const FName HasTarget(TEXT("HasTarget"));                  // Bool
	static const FName PatrolLocation(TEXT("PatrolLocation"));        // Vector
	static const FName LastKnownLocation(TEXT("LastKnownLocation"));  // Vector
}

/**
 * Контролер NPC: тут живе "відчуття" (AIPerception - зір) і запуск Behavior Tree.
 * Гравець належить команді Player, а цей контролер вважає її ворожою, тому
 * в налаштуваннях зору достатньо Detect Enemies.
 */
UCLASS()
class MYPROJECT_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

	// Дерево поведінки, яке запускається при оволодінні пешкою
	UPROPERTY(EditAnywhere, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(EditAnywhere, Category = "AI|Sight")
	float SightRadius = 2500.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Sight")
	float LoseSightRadius = 3000.0f;

	// Половина кута огляду, градуси (90 = кут огляду 180)
	UPROPERTY(EditAnywhere, Category = "AI|Sight")
	float PeripheralVisionHalfAngle = 70.0f;

private:
	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;
};
