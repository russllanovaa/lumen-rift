// Fill out your copyright notice in the Description page of Project Settings.

#include "SpawnEnemiesManager.h"
#include "SpawnEnemyTargetPoint.h"
#include "Enemy.h"
#include "Kismet/GameplayStatics.h"

ASpawnEnemiesManager::ASpawnEnemiesManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASpawnEnemiesManager::BeginPlay()
{
	Super::BeginPlay();

	if (!BaseEnemyBP)
	{
		return;
	}

	// Знаходимо всі точки на сцені - разом вони формують маршрут патрулювання
	TArray<AActor*> FoundPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnEnemyTargetPoint::StaticClass(), FoundPoints);

	if (FoundPoints.Num() == 0)
	{
		return;
	}

	TArray<ASpawnEnemyTargetPoint*> Route;
	Route.Reserve(FoundPoints.Num());
	for (AActor* Point : FoundPoints)
	{
		if (ASpawnEnemyTargetPoint* RoutePoint = Cast<ASpawnEnemyTargetPoint>(Point))
		{
			Route.Add(RoutePoint);
		}
	}

	const int32 SpawnCount = FMath::Min(NumberOfEnemiesToSpawn, Route.Num());

	for (int32 i = 0; i < SpawnCount; ++i)
	{
		ASpawnEnemyTargetPoint* SpawnPoint = Route[i];
		if (!SpawnPoint)
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AEnemy* NewEnemy = GetWorld()->SpawnActor<AEnemy>(
			BaseEnemyBP, SpawnPoint->GetActorLocation(), SpawnPoint->GetActorRotation(), SpawnParams);

		if (NewEnemy)
		{
			// Кожен ворог іде тим самим маршрутом, але з іншим стартовим
			// індексом, щоб вони не рухались одночасно однією юрбою
			NewEnemy->SetPatrolRoute(Route, i);
		}
	}
}
