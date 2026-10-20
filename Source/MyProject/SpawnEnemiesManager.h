// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnEnemiesManager.generated.h"

class AEnemy;

/**
 * Відповідає за породження ворогів на сцені (методичка, п.2).
 * Усі знайдені на сцені ASpawnEnemyTargetPoint формують один спільний
 * маршрут патрулювання; вороги спавняться на перших N точках і йдуть
 * цим маршрутом по колу, кожен зі своїм зсувом старту.
 */
UCLASS()
class MYPROJECT_API ASpawnEnemiesManager : public AActor
{
	GENERATED_BODY()

public:
	ASpawnEnemiesManager();

protected:
	virtual void BeginPlay() override;

	// Клас ворога, що буде породжуватись (BP, успадкований від AEnemy)
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AEnemy> BaseEnemyBP;

	// Скільки ворогів заспавнити (обрізається до кількості точок на сцені)
	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 NumberOfEnemiesToSpawn = 3;
};
