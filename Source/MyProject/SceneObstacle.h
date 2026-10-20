// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneObstacle.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USphereComponent;
class UCapsuleComponent;

UENUM(BlueprintType)
enum class EObstacleCollision : uint8
{
	Box UMETA(DisplayName = "Simple Box"),
	Sphere UMETA(DisplayName = "Simple Sphere"),
	Capsule UMETA(DisplayName = "Simple Capsule"),
	MeshAsset UMETA(DisplayName = "From mesh (Convex / Complex)")
};

/**
 * Нерухома перешкода (категорія 1 з лабораторної): гравець її обходить.
 * Тип колізії обирається в Details -> Obstacle: коробка, сфера, капсула або
 * власна колізія самого меша. Видима форма й форма колізії можуть відрізнятися
 * (наприклад кругле дерево з капсулою замість точної форми крони).
 */
UCLASS()
class MYPROJECT_API ASceneObstacle : public AActor
{
	GENERATED_BODY()

public:
	ASceneObstacle();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* BoxCollision;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* SphereCollision;

	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* CapsuleCollision;

	UPROPERTY(EditAnywhere, Category = "Obstacle")
	EObstacleCollision CollisionType = EObstacleCollision::Box;

	// Піврозміри коробки (для Box)
	UPROPERTY(EditAnywhere, Category = "Obstacle", meta = (EditCondition = "CollisionType == EObstacleCollision::Box"))
	FVector BoxExtent = FVector(100.0f, 100.0f, 100.0f);

	// Радіус (для Sphere)
	UPROPERTY(EditAnywhere, Category = "Obstacle", meta = (EditCondition = "CollisionType == EObstacleCollision::Sphere"))
	float SphereRadius = 100.0f;

	// Радіус і піввисота (для Capsule)
	UPROPERTY(EditAnywhere, Category = "Obstacle", meta = (EditCondition = "CollisionType == EObstacleCollision::Capsule"))
	float CapsuleRadius = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Obstacle", meta = (EditCondition = "CollisionType == EObstacleCollision::Capsule"))
	float CapsuleHalfHeight = 150.0f;

	// Показувати фігуру колізії в грі (зручно для скріншотів у звіт)
	UPROPERTY(EditAnywhere, Category = "Obstacle")
	bool bShowCollisionInGame = false;
};
