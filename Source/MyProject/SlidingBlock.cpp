// Fill out your copyright notice in the Description page of Project Settings.

#include "SlidingBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"

ASlidingBlock::ASlidingBlock()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	// Рухомий, але НЕ фізичний: блокує гравця й снаряди, рухаємо його самі в Tick
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetSimulatePhysics(false);
	// Гравець не має "заходити" на невисокий об'єкт - він його штовхає
	Mesh->CanCharacterStepUpOn = ECB_No;
}

void ASlidingBlock::BeginPlay()
{
	Super::BeginPlay();

	// Примусово (Blueprint міг перезаписати): на блок не можна ні заходити, ні стояти
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetSimulatePhysics(false);
	Mesh->CanCharacterStepUpOn = ECB_No;
	Mesh->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.0f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
}

void ASlidingBlock::ReceivePush(const FVector& Direction)
{
	FVector Flat = Direction;
	Flat.Z = 0.0f;
	if (Flat.IsNearlyZero())
	{
		return;
	}
	PushDirection = Flat.GetSafeNormal();
	PushTimeLeft = 0.15f; // поки гравець тисне, цей таймер постійно поновлюється
}

void ASlidingBlock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DetectPlayerPush();

	FVector TargetVelocity = FVector::ZeroVector;
	if (PushTimeLeft > 0.0f)
	{
		PushTimeLeft -= DeltaTime;
		TargetVelocity = PushDirection * PushSpeed;
	}

	Velocity = FMath::VInterpTo(Velocity, TargetVelocity, DeltaTime, Responsiveness);

	if (Velocity.SizeSquared() < 1.0f)
	{
		Velocity = FVector::ZeroVector;
		return;
	}

	// Рух зі свіпом: об'єкт зупиняється об стіни та інші перешкоди
	// Піднімаємо на 3 см, щоб свіп не чіпляв підлогу, потім SnapToGround опускає назад
	FHitResult Hit;
	AddActorWorldOffset(Velocity * DeltaTime + FVector(0.0f, 0.0f, 3.0f), true, &Hit);
	if (Hit.bBlockingHit && FMath::Abs(Hit.ImpactNormal.Z) < 0.7f)
	{
		Velocity = FVector::ZeroVector; // справжня стіна/перешкода
	}

	if (bSnapToGround)
	{
		SnapToGround();
	}
}

void ASlidingBlock::DetectPlayerPush()
{
	ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
	{
		return;
	}

	const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	const UCharacterMovementComponent* Move = Player->GetCharacterMovement();
	if (!Capsule || !Move)
	{
		return;
	}

	const FBox Box = GetComponentsBoundingBox();
	const FVector PlayerPos = Player->GetActorLocation();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	// Чи на одній висоті (гравець не стрибає над блоком і не під ним)
	if (PlayerPos.Z - HalfHeight > Box.Max.Z - 5.0f || PlayerPos.Z + HalfHeight < Box.Min.Z + 5.0f)
	{
		return;
	}

	// Найближча до гравця точка блока по горизонталі
	const FVector Closest(
		FMath::Clamp(PlayerPos.X, Box.Min.X, Box.Max.X),
		FMath::Clamp(PlayerPos.Y, Box.Min.Y, Box.Max.Y),
		PlayerPos.Z);

	FVector ToBlock = Closest - PlayerPos;
	ToBlock.Z = 0.0f;
	const float Dist = ToBlock.Size();
	if (Dist > Radius + PushReach || ToBlock.IsNearlyZero())
	{
		return;
	}
	ToBlock.Normalize();

	// Прискорення = те, куди гравець реально тисне (клавіші), не обнуляється об стіну
	FVector Accel = Move->GetCurrentAcceleration();
	Accel.Z = 0.0f;
	if (Accel.IsNearlyZero())
	{
		return;
	}
	if (FVector::DotProduct(Accel.GetSafeNormal(), ToBlock) < 0.35f)
	{
		return;
	}

	// Їдемо по головній осі (щоб не крутилось по діагоналі)
	FVector Dir = ToBlock;
	if (FMath::Abs(Dir.X) > FMath::Abs(Dir.Y)) { Dir = FVector(FMath::Sign(Dir.X), 0, 0); }
	else { Dir = FVector(0, FMath::Sign(Dir.Y), 0); }

	ReceivePush(Dir);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(7001, 0.2f, FColor::Green,
			FString::Printf(TEXT("[%s] pushed"), *GetName()));
	}
}

void ASlidingBlock::SnapToGround()
{
	const FBox Box = GetComponentsBoundingBox();
	const FVector Center = Box.GetCenter();
	const FVector Start(Center.X, Center.Y, Box.Min.Z + 30.0f);
	const FVector End = Start - FVector(0.0f, 0.0f, 200.0f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(Player);
	}

	FHitResult Ground;
	if (GetWorld()->LineTraceSingleByChannel(Ground, Start, End, ECC_Visibility, Params))
	{
		const float DeltaZ = Ground.ImpactPoint.Z - Box.Min.Z;
		if (FMath::Abs(DeltaZ) > 0.1f)
		{
			AddActorWorldOffset(FVector(0.0f, 0.0f, DeltaZ), false);
		}
	}
}
