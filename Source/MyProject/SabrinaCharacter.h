// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "SabrinaCharacter.generated.h"

class UStaticMeshComponent;
class ABaseMagicProjectile;
class UAnimMontage;
class UPlayerHUDWidget;
class UTexture2D;

UCLASS()
class MYPROJECT_API ASabrinaCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASabrinaCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void MoveForward(float Value);

	UFUNCTION()
	void MoveRight(float Value);

	UFUNCTION()
	void Turn(float Value);

	UFUNCTION()
	void LookUp(float Value);

	UFUNCTION()
	void StartJump();

	UFUNCTION()
	void EndJump();

	// Постріл магічною сферою (за замовчуванням - ЛКМ)
	UFUNCTION()
	void FireOrb();

	// Постріл магічним променем (за замовчуванням - ПКМ)
	UFUNCTION()
	void FireBeam();

	// Команда гравця (Affiliation, лаб. 5): NPC вважають її ворожою. 1 = NPCTeam::Player
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(1); }

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// Капсула вдарилась у щось, що ми вперлись у нього (штовхаємо ковзні об'єкти)
	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

protected:
	// Камера від першого лиця (методичка, п.1)
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComponent;

	// Модель чарівної палички, приєднана до кисті персонажа
	UPROPERTY(VisibleAnywhere, Category = "Magic|Weapon")
	UStaticMeshComponent* WandMeshComponent;

	// Зміщення точки вильоту снаряда відносно точки огляду камери
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic|Weapon")
	FVector MuzzleOffset = FVector(80.0f, 30.0f, -10.0f);

	// Клас снаряда "магічна сфера" - задається в Blueprint (BP_MagicOrb)
	UPROPERTY(EditDefaultsOnly, Category = "Magic|Weapon")
	TSubclassOf<ABaseMagicProjectile> OrbProjectileClass;

	// Клас снаряда "магічний промінь" - задається в Blueprint (BP_MagicBeam)
	UPROPERTY(EditDefaultsOnly, Category = "Magic|Weapon")
	TSubclassOf<ABaseMagicProjectile> BeamProjectileClass;

	// Ім'я сокета на скелеті персонажа, до якого кріпиться паличка
	UPROPERTY(EditAnywhere, Category = "Magic|Weapon")
	FName WandSocketName = TEXT("WandPoint");

	// Монтаж анімації для пострілу магічною сферою (замах паличкою)
	UPROPERTY(EditDefaultsOnly, Category = "Magic|Weapon|Animation")
	UAnimMontage* FireOrbMontage;

	// Монтаж анімації для пострілу магічним променем
	UPROPERTY(EditDefaultsOnly, Category = "Magic|Weapon|Animation")
	UAnimMontage* FireBeamMontage;

	// Монтаж анімації підбору предмета (нахил / простягнення руки)
	UPROPERTY(EditDefaultsOnly, Category = "Magic|Weapon|Animation")
	UAnimMontage* PickupMontage;

	// --- Здоров'я (методичка, п.6) ---
	UPROPERTY(EditAnywhere, Category = "Player|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "Player|Health")
	float Health = 100.0f;

	// --- Мана: ОДИН спільний ресурс для обох ударів ---
	UPROPERTY(EditAnywhere, Category = "Player|Mana")
	float MaxMana = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "Player|Mana")
	float Mana = 100.0f;

	// Скільки мани відновлюється за секунду
	UPROPERTY(EditAnywhere, Category = "Player|Mana")
	float ManaRegenPerSecond = 4.0f;

	// --- Сфера: можна спамити, обмежує лише мана (+ маленький кулдаун) ---
	UPROPERTY(EditAnywhere, Category = "Player|Orb")
	float OrbManaCost = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Player|Orb")
	float OrbCooldownSeconds = 0.35f;

	// --- Промінь ("ульта"): дорогий, довгий кулдаун, скидається вбивствами ---
	UPROPERTY(EditAnywhere, Category = "Player|Beam")
	float BeamManaCost = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Player|Beam")
	float BeamCooldownSeconds = 30.0f;

	// Скільки вбивств ворогів миттєво скидають кулдаун променя
	UPROPERTY(EditAnywhere, Category = "Player|Beam")
	int32 KillsToResetBeam = 3;

	// --- Іконки HUD (задаються в Class Defaults BP_SabrinaCharacter) ---
	UPROPERTY(EditDefaultsOnly, Category = "Player|HUD")
	UTexture2D* HealthIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Player|HUD")
	UTexture2D* ManaIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Player|HUD")
	UTexture2D* OrbIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Player|HUD")
	UTexture2D* BeamIconTexture;

	UPROPERTY()
	UPlayerHUDWidget* HUDWidgetInstance;

public:
	// Викликається ворогом, коли той помирає (скидання кулдауну променя)
	void NotifyEnemyKilled();

	// Для підбирань (аптечка, кристал мани)
	bool RestoreHealth(float Amount);
	bool RestoreMana(float Amount);

	// Анімація підбору предмета (монтаж задається в Class Defaults)
	void PlayPickupAnimation();

	// Куди летить підібраний предмет (рука з паличкою, інакше груди)
	FVector GetPickupTargetLocation() const;

	// Ключ (відмикає криниці-телепорти)
	void GiveKey();
	bool HasKey() const { return bHasKey; }

	// Короткий напис на екрані
	void ShowHUDMessage(const FString& Message, float Duration = 2.5f);

private:
	bool bHasKey = false;

	float OrbCooldownRemaining = 0.0f;
	float BeamCooldownRemaining = 0.0f;
	int32 KillsSinceBeamUse = 0;

	bool TryUseOrb();
	bool TryUseBeam();
	void UpdateHUDHealth();
	void UpdateHUDMana();
	void UpdateHUDAbilities();
	void ShowDamageFlash();

	void SpawnMagicProjectile(TSubclassOf<ABaseMagicProjectile> ProjectileClass);
};
