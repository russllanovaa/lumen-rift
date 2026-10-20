// Fill out your copyright notice in the Description page of Project Settings.

#include "TeleportPortal.h"
#include "SabrinaCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

ATeleportPortal::ATeleportPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(RootComponent);
	Trigger->SetBoxExtent(FVector(80.0f, 80.0f, 60.0f));
	Trigger->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	Trigger->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));

	ArrivalPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrivalPoint"));
	ArrivalPoint->SetupAttachment(RootComponent);
	ArrivalPoint->SetRelativeLocation(FVector(300.0f, 0.0f, 100.0f));
}

void ATeleportPortal::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ATeleportPortal::OnTriggerOverlap);
}

void ATeleportPortal::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ASabrinaCharacter* Player = Cast<ASabrinaCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	if (!TargetPortal)
	{
		Player->ShowHUDMessage(TEXT("This well has no Target Portal set!"), 3.0f);
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < SuppressUntil)
	{
		return;
	}

	if (bRequiresKey && !Player->HasKey())
	{
		Player->ShowHUDMessage(TEXT("The well is sealed. Find the key!"));
		SuppressUntil = Now + 1.5f;
		return;
	}

	SuppressUntil = Now + CooldownSeconds;
	TargetPortal->SuppressUntil = Now + CooldownSeconds;

	const FVector Destination = TargetPortal->ArrivalPoint->GetComponentLocation();
	const FRotator DestinationRotation = TargetPortal->ArrivalPoint->GetComponentRotation();

	if (TeleportEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TeleportEffect, Player->GetActorLocation());
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TeleportEffect, Destination);
	}

	const FRotator YawOnly(0.0f, DestinationRotation.Yaw, 0.0f);
	// TeleportTo мовчки відмовляє, якщо в точці призначення капсула заважає геометрії -
	// тоді ставимо гравця примусово, без перевірки зіткнень
	if (!Player->TeleportTo(Destination, YawOnly))
	{
		Player->SetActorLocationAndRotation(Destination, YawOnly, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->SetControlRotation(FRotator(0.0f, DestinationRotation.Yaw, 0.0f));
		if (PC->PlayerCameraManager)
		{
			// Білий спалах, що розсіюється - ефект "переходу"
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.7f, FLinearColor::White);
		}
	}
}
