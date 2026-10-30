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
#include "TimerManager.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"

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

	// Кристал для перезарядки (за замовчуванням прихований, без колізії)
	ReloadCrystalComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReloadCrystalComponent"));
	ReloadCrystalComponent->SetupAttachment(GetMesh());
	ReloadCrystalComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReloadCrystalComponent->CastShadow = false;
	ReloadCrystalComponent->SetVisibility(false);

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
	OrbCharges = MaxOrbCharges;

	if (ReloadCrystalMesh)
	{
		ReloadCrystalComponent->SetStaticMesh(ReloadCrystalMesh);
	}
	if (ReloadCrystalMaterial)
	{
		ReloadCrystalComponent->SetMaterial(0, ReloadCrystalMaterial);
	}

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

	// Авто-перезарядка, коли заряди скінчились
	if (bReloadEnabled && !bIsReloading && OrbCharges <= 0 && OrbCooldownRemaining <= 0.0f)
	{
		StartReload();
	}
	ShowAmmoOnScreen();
	UpdateReloadCrystal(DeltaTime);
}

void ASabrinaCharacter::UpdateReloadCrystal(float DeltaTime)
{
	if (!ReloadCrystalComponent || !ReloadCrystalComponent->GetStaticMesh())
	{
		return;
	}

	if (!bIsReloading)
	{
		ReloadCrystalComponent->SetVisibility(false);
		return;
	}

	ReloadElapsed += DeltaTime;
	const float F = FMath::Clamp(ReloadElapsed / FMath::Max(ReloadTotal, 0.01f), 0.0f, 1.0f);

	if (F < CrystalAppearStart || F > CrystalInsertEnd)
	{
		ReloadCrystalComponent->SetVisibility(false);
		return;
	}

	const FVector HandPos = GetMesh()->GetSocketLocation(ReloadHandBoneName) + ReloadCrystalOffset;
	// Кристал вкладається у ВЕРХІВКУ палички: беремо той кінець довгої осі меша палички,
	// який далі від руки (рука тримає низ, верх - вільний кінець)
	FVector WandPos = GetMesh()->DoesSocketExist(WandSocketName)
		? GetMesh()->GetSocketLocation(WandSocketName)
		: HandPos;
	if (WandMeshComponent && WandMeshComponent->GetStaticMesh())
	{
		const FBox LocalBox = WandMeshComponent->GetStaticMesh()->GetBoundingBox();
		const FVector Ext = LocalBox.GetExtent();
		const FVector Center = LocalBox.GetCenter();
		int32 Axis = 0;
		if (Ext.Y > Ext[Axis]) { Axis = 1; }
		if (Ext.Z > Ext[Axis]) { Axis = 2; }

		FVector EndA = Center;
		FVector EndB = Center;
		EndA[Axis] += Ext[Axis];
		EndB[Axis] -= Ext[Axis];

		const FTransform WandTM = WandMeshComponent->GetComponentTransform();
		const FVector WorldA = WandTM.TransformPosition(EndA);
		const FVector WorldB = WandTM.TransformPosition(EndB);
		const FVector HandNow = GetMesh()->GetSocketLocation(ReloadHandBoneName);
		const FVector Tip = FVector::DistSquared(WorldA, HandNow) > FVector::DistSquared(WorldB, HandNow) ? WorldA : WorldB;
		WandPos = Tip + ReloadWandTipOffset;
	}

	FVector Pos = HandPos;
	float Scale = ReloadCrystalScale;

	if (F < CrystalAppearEnd)
	{
		// Матеріалізація: росте з нуля з легким "поп"-ефектом
		const float T = (F - CrystalAppearStart) / FMath::Max(CrystalAppearEnd - CrystalAppearStart, 0.01f);
		Scale = ReloadCrystalScale * FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f);
		Pos += FVector(0.0f, 0.0f, FMath::Sin(T * PI) * 2.0f);
	}
	else
	{
		// Вкладання: летить з руки в паличку й зменшується
		const float T = (F - CrystalAppearEnd) / FMath::Max(CrystalInsertEnd - CrystalAppearEnd, 0.01f);
		const float E = FMath::InterpEaseInOut(0.0f, 1.0f, T, 2.0f);
		Pos = FMath::Lerp(HandPos, WandPos, E);
		Scale = ReloadCrystalScale * (1.0f - 0.7f * E);
	}

	ReloadCrystalComponent->SetWorldLocation(Pos);
	ReloadCrystalComponent->SetWorldRotation(FRotator(0.0f, ReloadElapsed * 180.0f, 0.0f));
	ReloadCrystalComponent->SetWorldScale3D(FVector(Scale));
	ReloadCrystalComponent->SetVisibility(true);
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

	// Перезарядка на клавішу R (прив'язка напряму, Project Settings міняти не треба)
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &ASabrinaCharacter::ReloadPressed);
}

void ASabrinaCharacter::ReloadPressed()
{
	if (bReloadEnabled && !bIsReloading && OrbCharges < MaxOrbCharges)
	{
		StartReload();
	}
}

void ASabrinaCharacter::StartReload()
{
	bIsReloading = true;

	float Duration = FallbackReloadDuration;
	if (ReloadMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		Duration = GetMesh()->GetAnimInstance()->Montage_Play(ReloadMontage);
		if (Duration <= 0.0f)
		{
			Duration = FallbackReloadDuration;
		}
	}

	ReloadElapsed = 0.0f;
	ReloadTotal = Duration;
	ShowHUDMessage(TEXT("Перезарядка..."), Duration);
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ASabrinaCharacter::FinishReload, Duration, false);
}

void ASabrinaCharacter::FinishReload()
{
	bIsReloading = false;
	OrbCharges = MaxOrbCharges;
}

void ASabrinaCharacter::ShowAmmoOnScreen() const
{
	if (bReloadEnabled && GEngine)
	{
		const FString Text = bIsReloading
			? FString(TEXT("Заряди: перезарядка..."))
			: FString::Printf(TEXT("Заряди: %d / %d  (R - перезарядка)"), OrbCharges, MaxOrbCharges);
		GEngine->AddOnScreenDebugMessage(7140, 0.1f, FColor::Cyan, Text);
	}
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
	if (bReloadEnabled && (bIsReloading || OrbCharges <= 0))
	{
		return;
	}

	if (!TryUseOrb())
	{
		return;
	}

	if (bReloadEnabled)
	{
		--OrbCharges;
	}

	if (FireOrbMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(FireOrbMontage);
	}
	SpawnMagicProjectile(OrbProjectileClass);
}

void ASabrinaCharacter::FireBeam()
{
	if (bReloadEnabled && bIsReloading)
	{
		return;
	}

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
