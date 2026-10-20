// Fill out your copyright notice in the Description page of Project Settings.

#include "MagicPickup.h"
#include "SabrinaCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

AMagicPickup::AMagicPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(80.0f);
	// Реагує тільки на перекриття з Pawn - ні з чим не стикається фізично
	TriggerSphere->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	RootComponent = TriggerSphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	// Сам меш без колізії - інакше він блокував би гравця
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMagicPickup::BeginPlay()
{
	Super::BeginPlay();

	MeshBaseLocation = Mesh->GetRelativeLocation();
	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AMagicPickup::OnPickupOverlap);
}

void AMagicPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bCollecting)
	{
		CollectTime += DeltaTime;
		const float Alpha = FMath::Clamp(CollectTime / FMath::Max(CollectDuration, 0.01f), 0.0f, 1.0f);
		const float Ease = Alpha * Alpha; // прискорюється до руки

		if (ASabrinaCharacter* Player = Collector.Get())
		{
			const FVector Target = Player->GetPickupTargetLocation();
			// Невелика дуга вгору посередині польоту
			const FVector Arc(0.0f, 0.0f, FMath::Sin(Alpha * PI) * 40.0f);
			Mesh->SetWorldLocation(FMath::Lerp(CollectStartLocation, Target, Ease) + Arc);
		}
		Mesh->SetWorldScale3D(FMath::Lerp(CollectStartScale, CollectStartScale * 0.15f, Alpha));
		Mesh->AddLocalRotation(FRotator(0.0f, RotationSpeed * 6.0f * DeltaTime, 0.0f));

		if (Alpha >= 1.0f)
		{
			if (PickupEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PickupEffect, Mesh->GetComponentLocation());
			}
			Destroy();
		}
		return;
	}

	TimeAccumulated += DeltaTime;

	// Обертання навколо вертикальної осі + плавний рух вгору-вниз
	Mesh->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	Mesh->SetRelativeLocation(MeshBaseLocation + FVector(0.0f, 0.0f, FMath::Sin(TimeAccumulated * BobSpeed) * BobHeight));
}

void AMagicPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ASabrinaCharacter* Player = Cast<ASabrinaCharacter>(OtherActor);
	if (!Player || bCollecting)
	{
		return;
	}

	bool bUsed = false;
	switch (PickupType)
	{
	case EPickupType::Health:
		bUsed = Player->RestoreHealth(Amount);
		if (bUsed) { Player->ShowHUDMessage(FString::Printf(TEXT("+%d HP"), FMath::RoundToInt(Amount))); }
		break;
	case EPickupType::Mana:
		bUsed = Player->RestoreMana(Amount);
		if (bUsed) { Player->ShowHUDMessage(FString::Printf(TEXT("+%d Mana"), FMath::RoundToInt(Amount))); }
		break;
	case EPickupType::Key:
		Player->GiveKey();
		Player->ShowHUDMessage(TEXT("Key found! The wells are unlocked"));
		bUsed = true;
		break;
	}

	if (!bUsed)
	{
		// Нічого відновлювати - предмет лишається на місці, але кажемо чому
		if (PickupType == EPickupType::Health)
		{
			Player->ShowHUDMessage(TEXT("Health is already full"), 1.5f);
		}
		return;
	}

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}
	// Не знищуємо одразу: програємо анімацію підбору персонажа,
	// а предмет летить до його руки (див. Tick)
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Player->PlayPickupAnimation();

	Collector = Player;
	CollectStartLocation = Mesh->GetComponentLocation();
	CollectStartScale = Mesh->GetComponentScale();
	CollectTime = 0.0f;
	bCollecting = true;
}
