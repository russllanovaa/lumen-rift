// Лабораторна робота 5: контролер ШІ для EnemyNPC

#include "EnemyAIController.h"
#include "EnemyNPC.h"
#include "SabrinaCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "GenericTeamAgentInterface.h"

AEnemyAIController::AEnemyAIController()
{
	// Ця команда - NPC (ворог гравця)
	SetGenericTeamId(FGenericTeamId(NPCTeam::Enemy));

	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception")));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngle;
	SightConfig->SetMaxAge(0.0f);
	// Лише ворожі (за Affiliation) - тобто гравець
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->SetDominantSense(SightConfig->GetSenseImplementation());
}

ETeamAttitude::Type AEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	// Усе, що належить команді гравця, - ворог
	if (const IGenericTeamAgentInterface* OtherTeam = Cast<const IGenericTeamAgentInterface>(&Other))
	{
		if (OtherTeam->GetGenericTeamId() == FGenericTeamId(NPCTeam::Player))
		{
			return ETeamAttitude::Hostile;
		}
	}
	return ETeamAttitude::Neutral;
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Параметри зору можна змінити в Details контролера, тому застосовуємо їх тут
	if (SightConfig)
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngle;
		GetPerceptionComponent()->ConfigureSense(*SightConfig);
	}

	GetPerceptionComponent()->OnTargetPerceptionUpdated.RemoveDynamic(this, &AEnemyAIController::OnTargetPerceived);
	GetPerceptionComponent()->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetPerceived);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: не задано BehaviorTreeAsset"));
	}
}

void AEnemyAIController::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Cast<ASabrinaCharacter>(Actor))
	{
		return;
	}

	UBlackboardComponent* BB = GetBlackboardComponent();
	AEnemyNPC* NPC = Cast<AEnemyNPC>(GetPawn());
	if (!BB || !NPC || NPC->IsDead())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// Гравця помічено: ціль у Blackboard, переходимо на біг, дивимось на нього
		BB->SetValueAsObject(NPCBB::TargetActor, Actor);
		BB->SetValueAsBool(NPCBB::HasTarget, true);
		SetFocus(Actor);
		NPC->SetChasing(true);
	}
	else
	{
		// Гравця втрачено з очей: запам'ятовуємо, де бачили востаннє
		BB->ClearValue(NPCBB::TargetActor);
		BB->SetValueAsBool(NPCBB::HasTarget, false);
		BB->SetValueAsVector(NPCBB::LastKnownLocation, Stimulus.StimulusLocation);
		ClearFocus(EAIFocusPriority::Gameplay);
		NPC->SetChasing(false);
	}
}
