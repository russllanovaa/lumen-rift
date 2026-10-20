// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseMagicProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;

// Тип траєкторії польоту снаряда (вимога лаб. роботи: пряма / хвилеподібна / спіральна)
UENUM(BlueprintType)
enum class EProjectileTrajectory : uint8
{
	Straight	UMETA(DisplayName = "Пряма"),
	Wave		UMETA(DisplayName = "Хвилеподібна"),
	Spiral		UMETA(DisplayName = "Спіральна")
};

/**
 * Базовий клас магічного снаряда.
 * Рух реалізовано вручну в Tick() (а не через ProjectileMovementComponent),
 * бо потрібні нелінійні траєкторії (хвиля, спіраль), а не лише прямолінійний рух.
 */
UCLASS()
class MYPROJECT_API ABaseMagicProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABaseMagicProjectile();

	// Задає напрямок пострілу та фіксує базис (вперед/вправо/вгору) для розрахунку траєкторії
	void FireInDirection(const FVector& Direction);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	// Викликається при зіткненні снаряда з об'єктом сцени. Перевизначається в нащадках
	// (AMagicOrbProjectile — вибух, AMagicBeamProjectile — опалювання).
	virtual void OnProjectileHit(const FHitResult& Hit);

	// Напрямок польоту на момент пострілу (потрібен нащадкам для damage-подій)
	FVector GetShotDirection() const { return ForwardDir; }

	// Розраховує бічне зміщення відносно прямої траєкторії для заданого часу польоту
	FVector ComputeTrajectoryOffset(float Time) const;

	UPROPERTY(VisibleDefaultsOnly, Category = "Magic|Collision")
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Magic|Visual")
	UStaticMeshComponent* ProjectileMeshComponent;

	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	EProjectileTrajectory Trajectory = EProjectileTrajectory::Straight;

	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	float Speed = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	float LifeSpanSeconds = 3.0f;

	// Максимальна дистанція польоту від точки пострілу - якщо снаряд пролетів
	// стільки, не влучивши ні в що, вважаємо, що він вилетів за межі сцени
	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	float MaxTravelDistance = 10000.0f;

	// Амплітуда/частота для хвилеподібної траєкторії
	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	float WaveAmplitude = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	float WaveFrequency = 6.0f;

	// Радіус/кутова швидкість для спіральної траєкторії
	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	float SpiralRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	float SpiralAngularSpeed = 10.0f;

	// Якщо true — радіус спіралі зростає з часом (розкручується "розтруб")
	UPROPERTY(EditAnywhere, Category = "Magic|Movement")
	bool bSpiralExpanding = false;

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector ForwardDir = FVector::ForwardVector;
	FVector RightDir = FVector::RightVector;
	FVector UpDir = FVector::UpVector;
	float ElapsedTime = 0.0f;
	bool bLaunched = false;
};
