// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "SpawnEnemyTargetPoint.generated.h"

/**
 * Точка на сцені, що використовується і для спавну ворогів, і як
 * контрольна точка маршруту патрулювання (методичка, п.2, рис.4-5).
 */
UCLASS()
class MYPROJECT_API ASpawnEnemyTargetPoint : public ATargetPoint
{
	GENERATED_BODY()
};
