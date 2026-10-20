// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PushableProp.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EPushType : uint8
{
	Heavy UMETA(DisplayName = "Heavy (slides, pushed by walking)"),
	Light UMETA(DisplayName = "Light (bouncy, flies from spells)")
};

/**
 * Об'єкт, який гравець може переміщати (категорія 2 з лабораторної).
 * Два типи фізичної взаємодії:
 *   Heavy - важкий, з великим тертям і демпфуванням: його можна посунути лише
 *           наполегливо йдучи в нього, магічна сфера майже не зрушує.
 *   Light - легкий і пружний: сильно відскакує, магічна сфера (вибух) відкидає
 *           його далеко, ногою теж легко збити.
 * Фізика виставляється в BeginPlay залежно від типу.
 * ВАЖЛИВО: у меша має бути проста колізія (Static Mesh Editor -> Collision -> Auto Convex).
 */
UCLASS()
class MYPROJECT_API APushableProp : public AActor
{
	GENERATED_BODY()

public:
	APushableProp();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, Category = "Push")
	EPushType PushType = EPushType::Heavy;

	// Маса, кг
	UPROPERTY(EditAnywhere, Category = "Push|Heavy")
	float HeavyMass = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Push|Heavy")
	float HeavyLinearDamping = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Push|Heavy")
	float HeavyAngularDamping = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Push|Light")
	float LightMass = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Push|Light")
	float LightBounciness = 0.6f;
};
