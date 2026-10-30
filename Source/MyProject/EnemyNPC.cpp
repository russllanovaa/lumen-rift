// Лабораторні роботи 5-6: NPC ворог з ШІ, анімація зброї

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

	// Точка за спиною на кістці спини (spine_03 у скелеті UE5; якщо в твоєму скелеті інакше -
	// змініть Parent Socket у BP). Положення можна підкрутити у вʼюпорті BP
	WeaponBackAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponBackAnchor"));
	WeaponBackAnchor->SetupAttachment(GetMesh(), TEXT("spine_03"));

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

	// Піднята поза = поточне положення зброї в BP. Старт - з опущеною зброєю
	if (WeaponComponent)
	{
		WeaponRaisedRelative = WeaponComponent->GetRelativeTransform();
		UpdateWeaponPose(0.0f);
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
	UpdateWeaponPose(DeltaTime);
}

// Анімація взаємодії зі зброєю: плавно піднімає її, коли гравець у радіусі пострілу,
// і ховає її за спину, коли той віддалився
void AEnemyNPC::UpdateWeaponPose(float DeltaTime)
{
	if (!WeaponComponent)
	{
		return;
	}

	if (!bWeaponPoseEnabled)
	{
		bWeaponRaised = true;
		WeaponAlpha = 1.0f;
		WeaponComponent->SetRelativeTransform(WeaponRaisedRelative);
		return;
	}

	// Зброя має бути піднята, якщо гравець близько або йде удар; гістерезис проти смикання на межі
	bool bShouldRaise = bWeaponRaised;
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
		if (Dist <= WeaponRaiseRadius)
		{
			bShouldRaise = true;
		}
		else if (Dist >= WeaponRaiseRadius + WeaponLowerHysteresis)
		{
			bShouldRaise = false;
		}
	}
	else
	{
		bShouldRaise = false;
	}
	if (bIsAttacking)
	{
		bShouldRaise = true;
	}

	// Анімація діставання/ховання йде: стан змінить SwitchWeaponState, тут лише ставимо позу
	const bool bUseAnims = (WeaponDrawAnim || WeaponHolsterAnim);
	if (bWeaponAnimPlaying)
	{
		bShouldRaise = bWeaponRaised;
	}
	else if (bUseAnims && bShouldRaise != bWeaponRaised && !bIsAttacking && !bIsDodging)
	{
		UAnimSequence* Needed = bShouldRaise ? WeaponDrawAnim
			: (WeaponHolsterAnim ? WeaponHolsterAnim : (bReverseDrawAsHolster ? WeaponDrawAnim : nullptr));
		if (Needed)
		{
			StartWeaponAnim(bShouldRaise);
			bShouldRaise = bWeaponRaised;
		}
	}

	if (bShouldRaise != bWeaponRaised)
	{
		bWeaponRaised = bShouldRaise;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(7103, 2.0f, FColor::Orange,
				bWeaponRaised ? TEXT("NPC: дістає зброю") : TEXT("NPC: ховає зброю за спину"));
		}
	}

	if (bWeaponAnimPlaying)
	{
		// поза не інтерполюється: зброя миттєво "перескакує" в потрібний момент анімації
		WeaponAlpha = bWeaponRaised ? 1.0f : 0.0f;
	}
	else if (bUseAnims && !(bIsAttacking && WeaponAlpha < 1.0f))
	{
		WeaponAlpha = bWeaponRaised ? 1.0f : 0.0f;
	}
	else
	{
		WeaponAlpha = FMath::FInterpConstantTo(WeaponAlpha, bWeaponRaised ? 1.0f : 0.0f, DeltaTime, 1.0f / WeaponRaiseTime);
	}
	const float S = FMath::InterpEaseInOut(0.0f, 1.0f, WeaponAlpha, 2.0f);

	// Піднята поза - у руці (відносно сокета руки). Захована - у WeaponBackAnchor: перераховуємо
	// її з світових координат у простір сокета руки, тож зброя їде за спиною разом з анімацією
	const FVector RaisedLoc = WeaponRaisedRelative.GetLocation();
	const FQuat RaisedQuat = WeaponRaisedRelative.GetRotation();

	FVector HiddenLoc = RaisedLoc;
	FQuat HiddenQuat = RaisedQuat;
	if (WeaponBackAnchor && GetMesh())
	{
		const FTransform HandWorld = GetMesh()->GetSocketTransform(WeaponComponent->GetAttachSocketName(), RTS_World);
		const FTransform BackRelative = WeaponBackAnchor->GetComponentTransform().GetRelativeTransform(HandWorld);
		HiddenLoc = BackRelative.GetLocation();
		HiddenQuat = BackRelative.GetRotation();
	}

	WeaponComponent->SetRelativeLocationAndRotation(
		FMath::Lerp(HiddenLoc, RaisedLoc, S),
		FQuat::Slerp(HiddenQuat, RaisedQuat, S));
}

void AEnemyNPC::StartWeaponAnim(bool bRaise)
{
	UAnimSequence* Anim = bRaise ? WeaponDrawAnim : WeaponHolsterAnim;
	bool bReversed = false;
	if (!bRaise && !Anim && bReverseDrawAsHolster)
	{
		Anim = WeaponDrawAnim;
		bReversed = true;
	}
	if (!Anim || !GetMesh())
	{
		return;
	}

	bWeaponAnimPlaying = true;
	bPendingWeaponRaised = bRaise;

	// Під час анімації NPC стоїть на місці
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	const float Length = FMath::Max(Anim->GetPlayLength(), 0.3f);
	GetMesh()->PlayAnimation(Anim, false);
	if (bReversed)
	{
		// те саме "діставання", але від кінця до початку
		GetMesh()->SetPosition(Anim->GetPlayLength(), false);
		GetMesh()->SetPlayRate(-1.0f);
	}
	else
	{
		GetMesh()->SetPlayRate(1.0f);
	}
	CurrentAnim = Anim;

	// при зворотному програванні момент перемикання зеркальний
	const float SwitchFrac = FMath::Clamp(bReversed ? 1.0f - WeaponSwitchFraction : WeaponSwitchFraction, 0.0f, 1.0f);
	GetWorldTimerManager().SetTimer(WeaponSwitchTimer, this, &AEnemyNPC::SwitchWeaponState,
		Length * SwitchFrac + 0.001f, false);
	GetWorldTimerManager().SetTimer(WeaponAnimEndTimer, this, &AEnemyNPC::EndWeaponAnim, Length, false);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(7103, 2.0f, FColor::Orange,
			bRaise ? TEXT("NPC: дістає зброю (анімація)") : TEXT("NPC: ховає зброю за спину (анімація)"));
	}
}

void AEnemyNPC::SwitchWeaponState()
{
	bWeaponRaised = bPendingWeaponRaised;
}

void AEnemyNPC::EndWeaponAnim()
{
	if (!bWeaponAnimPlaying)
	{
		return;
	}
	bWeaponAnimPlaying = false;
	bWeaponRaised = bPendingWeaponRaised;
	if (GetMesh())
	{
		GetMesh()->SetPlayRate(1.0f);
	}
	if (!bIsDead)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		CurrentAnim = nullptr;
	}
}

// Атака чи ухилення перебивають анімацію зброї: зброя лишається в бойовій позі
void AEnemyNPC::CancelWeaponAnim()
{
	if (!bWeaponAnimPlaying)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(WeaponSwitchTimer);
	GetWorldTimerManager().ClearTimer(WeaponAnimEndTimer);
	bWeaponAnimPlaying = false;
	bWeaponRaised = true;
	if (GetMesh())
	{
		GetMesh()->SetPlayRate(1.0f);
	}
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	CurrentAnim = nullptr;
}

void AEnemyNPC::UpdateLocomotionAnimation()
{
	if (bIsAttacking || bIsDodging || bWeaponAnimPlaying || !GetMesh())
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

	CancelWeaponAnim();
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
	CancelWeaponAnim();
	LastDodgeTime = GetWorld()->GetTimeSeconds();
	bIsDodging = true;

	// Напрямок відскоку за налаштуванням DodgeDirection. Up x ToAttacker = вправо від NPC,
	// коли він дивиться на атакуючого
	const FVector ToAttacker = (Attacker->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToAttacker);
	FVector Side = Right;
	switch (DodgeDirection)
	{
	case ENPCDodgeDirection::Left:
		Side = -Right;
		break;
	case ENPCDodgeDirection::Backward:
		Side = -ToAttacker;
		break;
	case ENPCDodgeDirection::Random:
		Side = FMath::RandBool() ? Right : -Right;
		break;
	default:
		break;
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
