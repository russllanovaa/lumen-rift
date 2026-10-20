// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enemy.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UWidgetComponent;
class UAnimSequence;
class ASpawnEnemyTargetPoint;
class UEnemyHealthWidget;
class UNiagaraSystem;

/**
 * Базовий клас ворога (методичка, п.1). Успадкований напряму від Actor,
 * БЕЗ AI Controller / Behavior Tree / NavMesh - рух по сцені є повністю
 * детермінованим, заданим програмно в Tick() (вимога завдання, п.2):
 * ворог просто по черзі йде до точок свого маршруту.
 */
UCLASS()
class MYPROJECT_API AEnemy : public AActor
{
	GENERATED_BODY()

public:
	AEnemy();

	// Викликається SpawnEnemiesManager одразу після спавну - задає маршрут
	// патрулювання (спільний список точок) і з якої точки почати рух.
	void SetPatrolRoute(const TArray<ASpawnEnemyTargetPoint*>& InPoints, int32 StartIndex);

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	// Статичний алгоритм руху: йти по прямій до поточної точки маршруту,
	// по досягненні - переходити до наступної (по колу)
	void MoveAlongRoute(float DeltaTime);

	// Перевірка дистанції до гравця й нанесення шкоди фізичним дотиком
	void TryAttackPlayer(float DeltaTime);

	// Розвертає плаваючу шкалу здоров'я обличчям до камери гравця
	void UpdateHealthWidgetFacing();

	// Передає поточний % здоров'я у віджет (і на старті, і після кожного влучання)
	void RefreshHealthWidget();

	void Die();

	UPROPERTY(VisibleDefaultsOnly, Category = "Enemy|Components")
	UCapsuleComponent* CollisionComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Enemy|Components")
	USkeletalMeshComponent* MeshComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Enemy|Components")
	UWidgetComponent* HealthWidgetComponent;

	// --- Рух ---
	UPROPERTY(EditAnywhere, Category = "Enemy|Movement")
	float MoveSpeed = 200.0f;

	// На якій відстані від точки маршруту вважати її "досягнутою"
	UPROPERTY(EditAnywhere, Category = "Enemy|Movement")
	float PointReachRadius = 50.0f;

	// --- Атака (фізичний дотик) ---
	UPROPERTY(EditAnywhere, Category = "Enemy|Attack")
	float AttackRange = 120.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Attack")
	float AttackDamage = 5.0f;

	// Період змахів: ворог б'є кожні N секунд ЗАВЖДИ, навіть коли гравця поруч нема
	UPROPERTY(EditAnywhere, Category = "Enemy|Attack")
	float AttackCooldownSeconds = 2.0f;

	// Чи шкодить змах гравцю, що випадково опинився в AttackRange у момент удару
	UPROPERTY(EditAnywhere, Category = "Enemy|Attack")
	bool bDamageIfPlayerInRange = true;

	// --- Здоров'я ---
	UPROPERTY(EditAnywhere, Category = "Enemy|Health")
	float MaxHealth = 50.0f;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|Health")
	float Health = 50.0f;

	// --- Анімації - програються напряму на MeshComponent (без AnimBP) ---
	UPROPERTY(EditAnywhere, Category = "Enemy|Animation")
	UAnimSequence* WalkAnim;

	UPROPERTY(EditAnywhere, Category = "Enemy|Animation")
	UAnimSequence* AttackAnim;

	UPROPERTY(EditAnywhere, Category = "Enemy|Animation")
	UAnimSequence* DeathAnim;

	// --- Ефекти (Niagara - той самий Niagara Examples Pack, що й для снарядів) ---
	UPROPERTY(EditAnywhere, Category = "Enemy|Effects")
	UNiagaraSystem* DeathEffect;

private:
	UPROPERTY()
	TArray<ASpawnEnemyTargetPoint*> PatrolPoints;

	int32 CurrentPatrolIndex = 0;
	float TimeSinceLastAttack = 0.0f;
	bool bIsAttacking = false;
	float SwingTimeLeft = 0.0f;
	bool bIsDead = false;
};
