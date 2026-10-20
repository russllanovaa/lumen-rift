// Лабораторна робота 5: NPC ворог з ШІ

#include "EnemyNPC.h"
#include "EnemyAIController.h"
#include "EnemyHealthWidget.h"
#include "SabrinaCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimSequence.h"
#include "BrainComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

AEnemyNPC::AEnemyNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// Контролер ШІ, який керуватиме цим персонажем
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// Зброя на сокеті скелета
	WeaponComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponComponent"));
	WeaponComponent->SetupAttachment(GetMesh(), WeaponSocketName);
	WeaponComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AuraLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AuraLight"));
	AuraLight->SetupAttachment(RootComponent);
	AuraLight->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
	AuraLight->SetLightColor(AuraColor);
	AuraLight->SetIntensity(AuraIntensity);
	AuraLight->SetAttenuationRadius(AuraRadius);
	AuraLight->SetCastShadows(false);

	HealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidgetComponent"));
	HealthWidgetComponent->SetupAttachment(RootComponent);
	HealthWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	HealthWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	HealthWidgetComponent->SetDrawSize(FVector2D(160.0f, 32.0f));

	Health = MaxHealth;
}

void AEnemyNPC::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	RefreshHealthWidget();
	UpdateSpeed();

	// Параметри аури можна змінити в Details BP, тому застосовуємо їх тут
	if (AuraLight)
	{
		AuraLight->SetLightColor(AuraColor);
		AuraLight->SetIntensity(AuraIntensity);
		AuraLight->SetAttenuationRadius(AuraRadius);
	}

	// Підстраховка: якщо сокет у Blueprint названо інакше, ніж у WeaponSocketName
	if (WeaponComponent && WeaponComponent->GetAttachSocketName() != WeaponSocketName)
	{
		WeaponComponent->AttachToComponent(GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
	}
}

void AEnemyNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	UpdateLocomotionAnimation();
	UpdateHealthWidgetFacing();
}

void AEnemyNPC::UpdateLocomotionAnimation()
{
	if (bIsAttacking || bIsDodging || !GetMesh())
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	UAnimSequence* Wanted = IdleAnim;
	if (Speed > 5.0f)
	{
		Wanted = (Speed > (WalkSpeed + RunSpeed) * 0.5f * SpeedMultiplier) ? RunAnim : WalkAnim;
	}

	if (Wanted && Wanted != CurrentAnim)
	{
		GetMesh()->PlayAnimation(Wanted, true);
		CurrentAnim = Wanted;
	}
}

void AEnemyNPC::SetChasing(bool bInChasing)
{
	bChasing = bInChasing;
	UpdateSpeed();
}

void AEnemyNPC::SetSpeedMultiplier(float InMultiplier)
{
	SpeedMultiplier = FMath::Max(InMultiplier, 0.05f);
	UpdateSpeed();
}

void AEnemyNPC::UpdateSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = (bChasing ? RunSpeed : WalkSpeed) * SpeedMultiplier;
}

// ---------------- Атака ----------------

bool AEnemyNPC::StartAttack()
{
	UWorld* World = GetWorld();
	if (!World || bIsDead || bIsAttacking || bIsDodging)
	{
		return false;
	}
	if (World->GetTimeSeconds() - LastAttackTime < AttackCooldown)
	{
		return false;
	}

	LastAttackTime = World->GetTimeSeconds();
	bIsAttacking = true;

	const float Length = AttackAnim ? FMath::Max(AttackAnim->GetPlayLength(), 0.3f) : 0.8f;
	if (AttackAnim && GetMesh())
	{
		GetMesh()->PlayAnimation(AttackAnim, false);
		CurrentAnim = AttackAnim;
	}

	GetWorldTimerManager().SetTimer(AttackHitTimer, this, &AEnemyNPC::ApplyAttackHit, Length * AttackHitFraction, false);
	GetWorldTimerManager().SetTimer(AttackEndTimer, this, &AEnemyNPC::EndAttack, Length, false);
	return true;
}

void AEnemyNPC::ApplyAttackHit()
{
	if (bIsDead)
	{
		return;
	}

	ASabrinaCharacter* Player = Cast<ASabrinaCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
	{
		return;
	}

	// Гравець у радіусі дії зброї: міряємо від кінця посоха і від самого NPC
	const FVector Reach = WeaponComponent ? WeaponComponent->GetComponentLocation() : GetActorLocation();
	const float Dist = FMath::Min(FVector::Dist(Reach, Player->GetActorLocation()),
		FVector::Dist(GetActorLocation(), Player->GetActorLocation()));

	if (Dist <= AttackRange)
	{
		UGameplayStatics::ApplyDamage(Player, AttackDamage, GetController(), this, UDamageType::StaticClass());
	}
}

void AEnemyNPC::EndAttack()
{
	bIsAttacking = false;
	CurrentAnim = nullptr; // Tick сам підбере ходьбу/очікування
}

// ---------------- Шкода й ухилення ----------------

float AEnemyNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// Хто стріляє: пешка гравця, інакше сам снаряд
	AActor* Attacker = EventInstigator ? EventInstigator->GetPawn() : nullptr;
	if (!Attacker)
	{
		Attacker = DamageCauser;
	}

	// Імовірність ухилення пропорційна відстані до атакуючого
	UWorld* World = GetWorld();
	if (Attacker && World && !bIsDodging &&
		World->GetTimeSeconds() - LastDodgeTime >= DodgeCooldown)
	{
		const float Dist = FVector::Dist(GetActorLocation(), Attacker->GetActorLocation());
		const float Chance = DodgeMaxChance * FMath::Clamp(Dist / FMath::Max(DodgeFullChanceDistance, 1.0f), 0.0f, 1.0f);
		if (FMath::FRand() < Chance)
		{
			Dodge(Attacker, Chance);
			return 0.0f;
		}
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
	RefreshHealthWidget();

	if (Health <= 0.0f)
	{
		Die();
	}
	return ActualDamage;
}

void AEnemyNPC::Dodge(AActor* Attacker, float Chance)
{
	LastDodgeTime = GetWorld()->GetTimeSeconds();
	bIsDodging = true;

	// Крок убік, перпендикулярно напрямку до атакуючого, випадково вліво або вправо
	FVector ToAttacker = (Attacker->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	FVector Side = FVector::CrossProduct(FVector::UpVector, ToAttacker);
	if (FMath::RandBool())
	{
		Side = -Side;
	}
	LaunchCharacter(Side * DodgeLaunchSpeed + FVector(0.0f, 0.0f, 150.0f), true, true);

	float Duration = 0.5f;
	if (DodgeAnim && GetMesh())
	{
		GetMesh()->PlayAnimation(DodgeAnim, false);
		CurrentAnim = DodgeAnim;
		Duration = FMath::Max(DodgeAnim->GetPlayLength(), 0.3f);
	}
	GetWorldTimerManager().SetTimer(DodgeEndTimer, this, &AEnemyNPC::EndDodge, Duration, false);

	// Для скріншотів у звіт
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(7101, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("NPC ухилився (імовірність %.0f%%)"), Chance * 100.0f));
	}
	UE_LOG(LogTemp, Display, TEXT("EnemyNPC: ухилення, імовірність %.2f"), Chance);
}

void AEnemyNPC::EndDodge()
{
	bIsDodging = false;
	CurrentAnim = nullptr;
}

void AEnemyNPC::Die()
{
	bIsDead = true;

	if (ASabrinaCharacter* Player = Cast<ASabrinaCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->NotifyEnemyKilled();
	}

	if (AController* C = GetController())
	{
		if (AAIController* AI = Cast<AAIController>(C))
		{
			if (AI->GetBrainComponent())
			{
				AI->GetBrainComponent()->StopLogic(TEXT("Dead"));
			}
			AI->StopMovement();
		}
	}

	if (DeathEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathEffect, GetActorLocation(), GetActorRotation());
	}
	if (HealthWidgetComponent)
	{
		HealthWidgetComponent->SetVisibility(false);
	}
	if (DeathAnim && GetMesh())
	{
		GetMesh()->PlayAnimation(DeathAnim, false);
	}

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// ---------------- Шкала здоров'я ----------------

void AEnemyNPC::UpdateHealthWidgetFacing()
{
	if (!HealthWidgetComponent)
	{
		return;
	}
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCamera = Cam->GetCameraLocation() - HealthWidgetComponent->GetComponentLocation();
		HealthWidgetComponent->SetWorldRotation(ToCamera.Rotation());
	}
}

void AEnemyNPC::RefreshHealthWidget()
{
	if (!HealthWidgetComponent)
	{
		return;
	}
	if (UEnemyHealthWidget* W = Cast<UEnemyHealthWidget>(HealthWidgetComponent->GetUserWidgetObject()))
	{
		W->SetHealth(Health, MaxHealth);
	}
}
