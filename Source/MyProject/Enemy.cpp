// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "SpawnEnemyTargetPoint.h"
#include "SabrinaCharacter.h"
#include "EnemyHealthWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimSequence.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/DamageType.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitCapsuleSize(42.0f, 96.0f);
	// Явно виставляємо профіль - без цього блокування снарядів залежить від
	// непередбачуваного дефолту компонента
	CollisionComponent->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));

	HealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidgetComponent"));
	HealthWidgetComponent->SetupAttachment(RootComponent);
	HealthWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	HealthWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	// Трохи більше місця, ніж раніше - тепер там ще іконка й число, не лише сама смужка
	HealthWidgetComponent->SetDrawSize(FVector2D(160.0f, 32.0f));

	Health = MaxHealth;
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	RefreshHealthWidget();

	// Підстраховка: якщо капсула хоч трохи "влізла" в підлогу через точку
	// спавну - це блокує будь-який горизонтальний рух через свіп-колізію.
	// Ставимо ворога точно на поверхню під ним одразу при спавні.
	if (CollisionComponent)
	{
		const float HalfHeight = CollisionComponent->GetScaledCapsuleHalfHeight();
		const FVector CurrentLocation = GetActorLocation();
		const FVector TraceStart = CurrentLocation + FVector(0.0f, 0.0f, HalfHeight);
		const FVector TraceEnd = CurrentLocation - FVector(0.0f, 0.0f, 1000.0f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		FHitResult GroundHit;
		if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
		{
			SetActorLocation(FVector(CurrentLocation.X, CurrentLocation.Y, GroundHit.ImpactPoint.Z + HalfHeight));
		}
	}

	if (WalkAnim && MeshComponent)
	{
		MeshComponent->PlayAnimation(WalkAnim, true);
	}
}

void AEnemy::SetPatrolRoute(const TArray<ASpawnEnemyTargetPoint*>& InPoints, int32 StartIndex)
{
	PatrolPoints = InPoints;
	if (PatrolPoints.Num() > 0)
	{
		CurrentPatrolIndex = StartIndex % PatrolPoints.Num();
	}
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	TimeSinceLastAttack += DeltaTime;

	TryAttackPlayer(DeltaTime);

	// Рух не залежить від атаки й гравця - завжди по маршруту
	MoveAlongRoute(DeltaTime);

	UpdateHealthWidgetFacing();
}

void AEnemy::MoveAlongRoute(float DeltaTime)
{
	if (PatrolPoints.Num() == 0)
	{
		return;
	}

	ASpawnEnemyTargetPoint* CurrentTarget = PatrolPoints[CurrentPatrolIndex];
	if (!CurrentTarget)
	{
		return;
	}

	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector CurrentLocation = GetActorLocation();

	FVector ToTarget = TargetLocation - CurrentLocation;
	ToTarget.Z = 0.0f;

	if (ToTarget.SizeSquared() <= FMath::Square(PointReachRadius))
	{
		// Досягли точки - переходимо до наступної по колу. Це і є весь
		// "алгоритм" - жодного пошуку шляху чи прийняття рішень, просто
		// заздалегідь заданий порядок точок.
		CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal();
	const FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;

	// bSweep = false: висота вже виставлена один раз при спавні (BeginPlay),
	// а капсула, що торкається підлоги впритул (без зазору), інакше блокує
	// сама себе на кожному кадрі - свіп бачить будь-який горизонтальний
	// рух як зіткнення з нуля.
	SetActorLocation(NewLocation, false);

	const FRotator FacingRotation = Direction.Rotation();
	SetActorRotation(FRotator(0.0f, FacingRotation.Yaw, 0.0f));
}

void AEnemy::TryAttackPlayer(float DeltaTime)
{
	// Поведінка повністю детермінована й НЕ залежить від гравця: ворог іде маршрутом
	// і через рівні проміжки часу (AttackCooldownSeconds) завжди змахує "в повітря".
	// Він не зупиняється, не наближається й не реагує на гравця. Шкода є лише
	// механічним наслідком: якщо в момент змаху гравець випадково опинився в зоні
	// ураження - він її отримує.

	// Закінчився змах - повертаємось до анімації ходьби
	if (bIsAttacking)
	{
		SwingTimeLeft -= DeltaTime;
		if (SwingTimeLeft <= 0.0f)
		{
			bIsAttacking = false;
			if (WalkAnim && MeshComponent)
			{
				MeshComponent->PlayAnimation(WalkAnim, true);
			}
		}
	}

	if (TimeSinceLastAttack < AttackCooldownSeconds)
	{
		return;
	}
	TimeSinceLastAttack = 0.0f;

	bIsAttacking = true;
	SwingTimeLeft = AttackAnim ? FMath::Max(AttackAnim->GetPlayLength(), 0.3f) : 0.5f;
	if (AttackAnim && MeshComponent)
	{
		MeshComponent->PlayAnimation(AttackAnim, false);
	}

	if (!bDamageIfPlayerInRange)
	{
		return;
	}

	ASabrinaCharacter* Player = Cast<ASabrinaCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (Player && FVector::Dist(GetActorLocation(), Player->GetActorLocation()) <= AttackRange)
	{
		UGameplayStatics::ApplyDamage(Player, AttackDamage, nullptr, this, UDamageType::StaticClass());
	}
}

void AEnemy::UpdateHealthWidgetFacing()
{
	if (!HealthWidgetComponent)
	{
		return;
	}

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CameraManager)
	{
		return;
	}

	const FVector ToCamera = CameraManager->GetCameraLocation() - HealthWidgetComponent->GetComponentLocation();
	HealthWidgetComponent->SetWorldRotation(ToCamera.Rotation());
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (bIsDead || ActualDamage <= 0.0f)
	{
		return ActualDamage;
	}

	Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
	RefreshHealthWidget();

	if (Health <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void AEnemy::RefreshHealthWidget()
{
	if (!HealthWidgetComponent)
	{
		return;
	}

	if (UEnemyHealthWidget* HealthWidget = Cast<UEnemyHealthWidget>(HealthWidgetComponent->GetUserWidgetObject()))
	{
		HealthWidget->SetHealth(Health, MaxHealth);
	}
}

void AEnemy::Die()
{
	bIsDead = true;

	// Вбивство наближає скидання кулдауну променя в гравця
	if (ASabrinaCharacter* Player = Cast<ASabrinaCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->NotifyEnemyKilled();
	}

	if (DeathEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathEffect, GetActorLocation(), GetActorRotation());
	}

	if (HealthWidgetComponent)
	{
		HealthWidgetComponent->SetVisibility(false);
	}

	if (DeathAnim && MeshComponent)
	{
		MeshComponent->PlayAnimation(DeathAnim, false);
	}

	// Труп лишається на сцені назавжди (не Destroy/SetLifeSpan) - просто
	// вимикаємо колізію, щоб тіло не заважало гравцю ходити, і зупиняємо
	// Tick, бо рухатись/атакувати мертвому вже нема чого
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetActorTickEnabled(false);
}
