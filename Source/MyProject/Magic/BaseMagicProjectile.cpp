// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseMagicProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ABaseMagicProjectile::ABaseMagicProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.0f);
	// Канал колізій "Projectile" створюється в Project Settings -> Collision,
	// так само як в методичці (рис. 34-35).
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	RootComponent = CollisionComponent;

	ProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMeshComponent"));
	ProjectileMeshComponent->SetupAttachment(RootComponent);
	// Колізію обробляє тільки CollisionComponent, меш — суто візуальний
	ProjectileMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InitialLifeSpan = LifeSpanSeconds;
}

void ABaseMagicProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseMagicProjectile::FireInDirection(const FVector& Direction)
{
	ForwardDir = Direction.GetSafeNormal();

	// Будуємо ортонормований базис відносно напрямку польоту -
	// він лишається фіксованим на весь політ, щоб хвиля/спіраль не "пливли"
	RightDir = FVector::CrossProduct(FVector::UpVector, ForwardDir).GetSafeNormal();
	if (RightDir.IsNearlyZero())
	{
		RightDir = FVector::RightVector;
	}
	UpDir = FVector::CrossProduct(ForwardDir, RightDir).GetSafeNormal();

	StartLocation = GetActorLocation();
	ElapsedTime = 0.0f;
	bLaunched = true;

	SetActorRotation(ForwardDir.Rotation());
}

FVector ABaseMagicProjectile::ComputeTrajectoryOffset(float Time) const
{
	switch (Trajectory)
	{
	case EProjectileTrajectory::Wave:
	{
		// Синусоїда впоперек напрямку руху
		return RightDir * WaveAmplitude * FMath::Sin(WaveFrequency * Time);
	}
	case EProjectileTrajectory::Spiral:
	{
		// Обертання навколо осі польоту: комбінація Right/Up з кутом, що росте з часом
		const float Angle = SpiralAngularSpeed * Time;
		const float Radius = bSpiralExpanding ? SpiralRadius * (1.0f + Time) : SpiralRadius;
		return (RightDir * FMath::Cos(Angle) + UpDir * FMath::Sin(Angle)) * Radius;
	}
	case EProjectileTrajectory::Straight:
	default:
		return FVector::ZeroVector;
	}
}

void ABaseMagicProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bLaunched)
	{
		return;
	}

	ElapsedTime += DeltaTime;

	const FVector NewLocation = StartLocation
		+ ForwardDir * Speed * ElapsedTime
		+ ComputeTrajectoryOffset(ElapsedTime);

	// Вилетів занадто далеко, не влучивши ні в що - вважаємо, що покинув сцену
	if (FVector::Dist(StartLocation, NewLocation) > MaxTravelDistance)
	{
		Destroy();
		return;
	}

	// bSweep = true: рух перевіряється на зіткнення по дорозі, а не лише в кінцевій точці
	FHitResult Hit;
	SetActorLocation(NewLocation, true, &Hit);

	if (Hit.bBlockingHit)
	{
		OnProjectileHit(Hit);
	}
}

void ABaseMagicProjectile::OnProjectileHit(const FHitResult& Hit)
{
	// Базова поведінка — просто зникнути. Нащадки додають вибух/опік.
	Destroy();
}
