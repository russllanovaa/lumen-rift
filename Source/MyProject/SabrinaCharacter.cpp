// Fill out your copyright notice in the Description page of Project Settings.

#include "SabrinaCharacter.h"
#include "Magic/BaseMagicProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "PlayerHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SlidingBlock.h"

// Sets default values
ASabrinaCharacter::ASabrinaCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Камера від першого лиця (методичка, рис. 4-6)
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	check(CameraComponent != nullptr);
	CameraComponent->SetupAttachment(CastChecked<USceneComponent, UCapsuleComponent>(GetCapsuleComponent()));
	CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
	CameraComponent->bUsePawnControlRotation = true;

	// Меш персонажа лишається на капсулі (НЕ на камері!) — інакше повне тіло
	// оберталось би разом з нахилом погляду і при певних кутах опинялось
	// би "всередині" камери. Прикріплення до камери підходить лише для
	// маленьких в'юмоделей-рук, а не для повного тіла.
	GetMesh()->SetupAttachment(GetCapsuleComponent());

	WandMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WandMeshComponent"));
	WandMeshComponent->SetupAttachment(GetMesh());
	WandMeshComponent->bCastDynamicShadow = false;
	WandMeshComponent->CastShadow = false;

	// Щоб персонаж міг штовхати фізичні об'єкти (книга, квітка): сильніша тяга
	GetCharacterMovement()->bEnablePhysicsInteraction = true;
	GetCharacterMovement()->PushForceFactor = 2000000.0f;
	GetCharacterMovement()->InitialPushForceFactor = 1000.0f;
	GetCharacterMovement()->bPushForceScaledToMass = false;
	GetCharacterMovement()->bScalePushForceToVelocity = false;
}

// Called when the game starts or when spawned
void ASabrinaCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Приєднуємо паличку до сокета на руці персонажа (так само як зброю в методичці, рис. 10)
	WandMeshComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), WandSocketName);

	Health = MaxHealth;
	Mana = MaxMana;

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ASabrinaCharacter::OnCapsuleHit);

	// HUD повністю будується в C++ (клас UPlayerHUDWidget), Blueprint-віджет не потрібен
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		HUDWidgetInstance = CreateWidget<UPlayerHUDWidget>(PC, UPlayerHUDWidget::StaticClass());
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->AddToViewport();
			HUDWidgetInstance->SetIcons(HealthIconTexture, ManaIconTexture, OrbIconTexture, BeamIconTexture);
			UpdateHUDHealth();
			UpdateHUDMana();
			UpdateHUDAbilities();
		}
	}
}

// Called every frame
void ASabrinaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Відновлення мани і зменшення кулдаунів
	Mana = FMath::Min(MaxMana, Mana + ManaRegenPerSecond * DeltaTime);
	OrbCooldownRemaining = FMath::Max(0.0f, OrbCooldownRemaining - DeltaTime);
	BeamCooldownRemaining = FMath::Max(0.0f, BeamCooldownRemaining - DeltaTime);

	UpdateHUDMana();
	UpdateHUDAbilities();
}

// Called to bind functionality to input
void ASabrinaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASabrinaCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASabrinaCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &ASabrinaCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASabrinaCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASabrinaCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASabrinaCharacter::EndJump);

	// Нові дії - додайте їх в Project Settings -> Input -> Action Mappings
	PlayerInputComponent->BindAction("FireOrb", IE_Pressed, this, &ASabrinaCharacter::FireOrb);
	PlayerInputComponent->BindAction("FireBeam", IE_Pressed, this, &ASabrinaCharacter::FireBeam);
}

void ASabrinaCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.0f)
	{
		FVector Direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASabrinaCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.0f)
	{
		FVector Direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ASabrinaCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASabrinaCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASabrinaCharacter::StartJump()
{
	Jump();
}

void ASabrinaCharacter::EndJump()
{
	StopJumping();
}

void ASabrinaCharacter::FireOrb()
{
	if (!TryUseOrb())
	{
		return;
	}

	if (FireOrbMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(FireOrbMontage);
	}
	SpawnMagicProjectile(OrbProjectileClass);
}

void ASabrinaCharacter::FireBeam()
{
	if (!TryUseBeam())
	{
		return;
	}

	if (FireBeamMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(FireBeamMontage);
	}
	SpawnMagicProjectile(BeamProjectileClass);
}

void ASabrinaCharacter::SpawnMagicProjectile(TSubclassOf<ABaseMagicProjectile> ProjectileClass)
{
	if (!ProjectileClass)
	{
		return;
	}

	// Точка й напрямок огляду - так само як GetActorEyesViewPoint в методичці (рис. 29)
	FVector CameraLocation;
	FRotator CameraRotation;
	GetActorEyesViewPoint(CameraLocation, CameraRotation);

	const FVector MuzzleLocation = CameraLocation + FTransform(CameraRotation).TransformVector(MuzzleOffset);
	const FRotator MuzzleRotation = CameraRotation;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABaseMagicProjectile* Projectile = GetWorld()->SpawnActor<ABaseMagicProjectile>(
		ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams);

	if (Projectile)
	{
		Projectile->FireInDirection(MuzzleRotation.Vector());
	}
}

float ASabrinaCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
		UpdateHUDHealth();
		ShowDamageFlash();
	}

	return ActualDamage;
}

bool ASabrinaCharacter::TryUseOrb()
{
	if (OrbCooldownRemaining > 0.0f)
	{
		return false;
	}

	if (Mana < OrbManaCost)
	{
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->FlashNotEnoughMana();
		}
		return false;
	}

	Mana -= OrbManaCost;
	OrbCooldownRemaining = OrbCooldownSeconds;
	return true;
}

bool ASabrinaCharacter::TryUseBeam()
{
	if (BeamCooldownRemaining > 0.0f)
	{
		return false;
	}

	if (Mana < BeamManaCost)
	{
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->FlashNotEnoughMana();
		}
		return false;
	}

	Mana -= BeamManaCost;
	BeamCooldownRemaining = BeamCooldownSeconds;
	KillsSinceBeamUse = 0;
	return true;
}

void ASabrinaCharacter::NotifyEnemyKilled()
{
	// Вбивства рахуються лише поки промінь на кулдауні
	if (BeamCooldownRemaining <= 0.0f)
	{
		return;
	}

	++KillsSinceBeamUse;
	if (KillsSinceBeamUse >= KillsToResetBeam)
	{
		BeamCooldownRemaining = 0.0f;
		KillsSinceBeamUse = 0;
	}
}

bool ASabrinaCharacter::RestoreHealth(float Amount)
{
	// false = здоров'я вже повне, підбирати нічого
	if (Health >= MaxHealth)
	{
		return false;
	}
	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	UpdateHUDHealth();
	return true;
}

bool ASabrinaCharacter::RestoreMana(float Amount)
{
	// Мана постійно відновлюється сама й майже завжди близька до повної,
	// тому кристал підбирається завжди (інакше його ніколи не вдалось би взяти)
	Mana = FMath::Clamp(Mana + Amount, 0.0f, MaxMana);
	UpdateHUDMana();
	return true;
}

void ASabrinaCharacter::OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ASlidingBlock* Block = Cast<ASlidingBlock>(OtherActor);
	if (!Block)
	{
		return;
	}

	// Напрямок штовхання - від гравця в бік об'єкта (проти нормалі поверхні)
	FVector PushDir = -Hit.ImpactNormal;
	PushDir.Z = 0.0f;
	if (PushDir.IsNearlyZero())
	{
		return;
	}
	PushDir.Normalize();

	// Штовхаємо лише тоді, коли гравець справді іде в об'єкт, а не просто стоїть поряд
	FVector Input = GetLastMovementInputVector();
	Input.Z = 0.0f;
	if (Input.IsNearlyZero() || FVector::DotProduct(Input.GetSafeNormal(), PushDir) < 0.3f)
	{
		return;
	}

	Block->ReceivePush(PushDir);
}

void ASabrinaCharacter::PlayPickupAnimation()
{
	if (PickupMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(PickupMontage);
	}
}

FVector ASabrinaCharacter::GetPickupTargetLocation() const
{
	if (GetMesh() && GetMesh()->DoesSocketExist(WandSocketName))
	{
		return GetMesh()->GetSocketLocation(WandSocketName);
	}
	return GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
}

void ASabrinaCharacter::GiveKey()
{
	bHasKey = true;
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->SetHasKey(true);
	}
}

void ASabrinaCharacter::ShowHUDMessage(const FString& Message, float Duration)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->ShowMessage(Message, Duration);
	}
}

void ASabrinaCharacter::ShowDamageFlash()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	// Швидкий червоний спалах на весь екран, що згасає - стандартний
	// індикатор отримання шкоди. FromAlpha 0.5 -> ToAlpha 0.0 за 1.0с.
	PC->PlayerCameraManager->StartCameraFade(0.5f, 0.0f, 1.0f, FLinearColor::Red, false, true);
}

void ASabrinaCharacter::UpdateHUDHealth()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->SetHealth(Health, MaxHealth);
	}
}

void ASabrinaCharacter::UpdateHUDMana()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->SetMana(Mana, MaxMana);
	}
}

void ASabrinaCharacter::UpdateHUDAbilities()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->SetOrbState(OrbCooldownRemaining, OrbCooldownSeconds, Mana >= OrbManaCost);
		HUDWidgetInstance->SetBeamState(BeamCooldownRemaining, BeamCooldownSeconds, Mana >= BeamManaCost,
			KillsSinceBeamUse, KillsToResetBeam);
	}
}
