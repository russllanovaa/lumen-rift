// Лабораторні роботи 5-6: NPC ворог з ШІ (AIController + AIPerception + Behavior Tree), анімація зброї

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyNPC.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UWidgetComponent;
class UAnimSequence;
class UNiagaraSystem;
class UPointLightComponent;

// Куди відскакує NPC при ухиленні (щоб збігалося з анімацією DodgeAnim)
UENUM(BlueprintType)
enum class ENPCDodgeDirection : uint8
{
	Right    UMETA(DisplayName = "Вправо"),
	Left     UMETA(DisplayName = "Вліво"),
	Backward UMETA(DisplayName = "Назад"),
	Random   UMETA(DisplayName = "Випадково вліво/вправо")
};

/**
 * NPC ворог на шаблонному скелеті (Skeleton Necromancer). На відміну від AEnemy
 * (лаб. 3) це ACharacter, яким керує AEnemyAIController: пошук гравця зором,
 * патрулювання з зупинками й оглядом, біг при виявленні, удар посохом,
 * ухилення від влучань з імовірністю, що росте з дальністю атаки гравця.
 * Зброя (SM_Staff) - окремий асет, прикріплений до сокета скелета.
 */
UCLASS()
class MYPROJECT_API AEnemyNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyNPC();

	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// Контролер вмикає біг (гравця помічено) або ходьбу (гравця втрачено)
	void SetChasing(bool bInChasing);

	// Зона сповільнення (отруйна калюжа тощо) множить швидкість; 1 = норма
	void SetSpeedMultiplier(float InMultiplier);

	// Починає удар посохом. false - ще триває кулдаун, удар вже йде, або NPC мертвий
	bool StartAttack();

	bool IsAttacking() const { return bIsAttacking; }
	bool IsDead() const { return bIsDead; }
	float GetAttackRange() const { return AttackRange; }

protected:
	virtual void BeginPlay() override;

	void UpdateLocomotionAnimation();
	void UpdateSpeed();
	void UpdateHealthWidgetFacing();
	void UpdateWeaponPose(float DeltaTime);
	void StartWeaponAnim(bool bRaise);
	void SwitchWeaponState();
	void EndWeaponAnim();
	void CancelWeaponAnim();
	void RefreshHealthWidget();

	void ApplyAttackHit();
	void EndAttack();
	void Dodge(AActor* Attacker, float Chance);
	void EndDodge();
	void Die();

	// Зброя - окремий статичний меш на сокеті скелета
	UPROPERTY(VisibleAnywhere, Category = "NPC|Components")
	UStaticMeshComponent* WeaponComponent;

	// Точка за спиною, куди ховається зброя (прикріплена до кістки спини, у BP її можна
	// пересунути/повернути у вʼюпорті, щоб посох лежав за спиною як треба)
	UPROPERTY(VisibleAnywhere, Category = "NPC|Components")
	USceneComponent* WeaponBackAnchor;

	UPROPERTY(VisibleAnywhere, Category = "NPC|Components")
	UWidgetComponent* HealthWidgetComponent;

	// Кольорова аура навколо NPC, щоб відрізняти його від ворогів лаб. 3
	UPROPERTY(VisibleAnywhere, Category = "NPC|Components")
	UPointLightComponent* AuraLight;

	UPROPERTY(EditAnywhere, Category = "NPC|Look")
	FLinearColor AuraColor = FLinearColor(0.7f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "NPC|Look")
	float AuraIntensity = 8000.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Look")
	float AuraRadius = 450.0f;

	// Назва сокета на кістці руки (створи його в Skeleton Editor)
	UPROPERTY(EditAnywhere, Category = "NPC|Weapon")
	FName WeaponSocketName = TEXT("hand_r");

	// --- Рух ---
	UPROPERTY(EditAnywhere, Category = "NPC|Movement")
	float WalkSpeed = 150.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Movement")
	float RunSpeed = 420.0f;

	// --- Атака ---
	// Радіус дії зброї: гравець у цьому радіусі отримує шкоду
	UPROPERTY(EditAnywhere, Category = "NPC|Attack")
	float AttackRange = 170.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Attack")
	float AttackDamage = 8.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Attack")
	float AttackCooldown = 2.0f;

	// В яку частку тривалості анімації удару наноситься шкода (0..1)
	UPROPERTY(EditAnywhere, Category = "NPC|Attack", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float AttackHitFraction = 0.5f;

	// --- Ухилення ---
	// Максимальна імовірність ухилення (на дальній дистанції)
	UPROPERTY(EditAnywhere, Category = "NPC|Dodge", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DodgeMaxChance = 0.8f;

	// На якій відстані атакуючого імовірність досягає максимуму, см.
	// Імовірність = DodgeMaxChance * (відстань / DodgeFullChanceDistance), пропорційно
	UPROPERTY(EditAnywhere, Category = "NPC|Dodge")
	float DodgeFullChanceDistance = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Dodge")
	float DodgeCooldown = 1.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Dodge")
	float DodgeLaunchSpeed = 700.0f;

	// Напрямок відскоку відносно NPC. Став таким, як у твоєї анімації ухилення
	// (v2_Dodging_Right = Вправо)
	UPROPERTY(EditAnywhere, Category = "NPC|Dodge")
	ENPCDodgeDirection DodgeDirection = ENPCDodgeDirection::Right;

	// --- Зброя: опущена / піднята (лаб. 6) ---
	// Вимкнути - NPC завжди тримає зброю піднятою, як у лаб. 5
	UPROPERTY(EditAnywhere, Category = "NPC|WeaponPose")
	bool bWeaponPoseEnabled = true;

	// Коли гравець ближче за цей радіус, NPC піднімає зброю й тримає її готовою, см
	UPROPERTY(EditAnywhere, Category = "NPC|WeaponPose")
	float WeaponRaiseRadius = 1200.0f;

	// Зброя ховається знову, коли гравець відійшов далі за WeaponRaiseRadius + цей запас (щоб не смикалась на межі)
	UPROPERTY(EditAnywhere, Category = "NPC|WeaponPose")
	float WeaponLowerHysteresis = 200.0f;

	// Тривалість піднімання/опускання, с
	UPROPERTY(EditAnywhere, Category = "NPC|WeaponPose", meta = (ClampMin = "0.1"))
	float WeaponRaiseTime = 0.6f;

	// АНІМАЦІЇ взаємодії зі зброєю (Mixamo / власні). Якщо задані - NPC відтворює їх, коли дістає
	// чи ховає зброю (під час анімації він стоїть на місці). Порожньо - проста плавна анімація зброї.
	UPROPERTY(EditAnywhere, Category = "NPC|WeaponPose")
	UAnimSequence* WeaponDrawAnim = nullptr;

	UPROPERTY(EditAnywhere, Category = "NPC|WeaponPose")
	UAnimSequence* WeaponHolsterAnim = nullptr;

	// Якщо Weapon Holster Anim порожня: ховати зброю, програючи Weapon Draw Anim у зворотному напрямку
	UPROPERTY(EditAnywhere, Category = "NPC|WeaponPose")
	bool bReverseDrawAsHolster = true;

	// У який момент анімації (частка 0..1) зброя "перестрибує" зі спини в руку (при діставанні)
	// або з руки на спину (при ховані): має збігатися з моментом, коли рука торкається зброї
	UPROPERTY(EditAnywhere, Category = "NPC|WeaponPose", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeaponSwitchFraction = 0.5f;

	// ПІДНЯТА поза - це те, як зброя стоїть у руці в BP зараз (Transform компонента WeaponComponent).
	// ЗАХОВАНА поза - зброя лежить у точці WeaponBackAnchor за спиною (кістка спини; назва
	// кістки в Details компонента WeaponBackAnchor -> Parent Socket)

	// --- Здоров'я ---
	UPROPERTY(EditAnywhere, Category = "NPC|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "NPC|Health")
	float Health = 100.0f;

	// --- Анімації (відтворюються напряму, без AnimBP) ---
	UPROPERTY(EditAnywhere, Category = "NPC|Animation")
	UAnimSequence* IdleAnim;

	UPROPERTY(EditAnywhere, Category = "NPC|Animation")
	UAnimSequence* WalkAnim;

	UPROPERTY(EditAnywhere, Category = "NPC|Animation")
	UAnimSequence* RunAnim;

	UPROPERTY(EditAnywhere, Category = "NPC|Animation")
	UAnimSequence* AttackAnim;

	// Необов'язково: якщо порожньо, ухилення без окремої анімації
	UPROPERTY(EditAnywhere, Category = "NPC|Animation")
	UAnimSequence* DodgeAnim;

	UPROPERTY(EditAnywhere, Category = "NPC|Animation")
	UAnimSequence* DeathAnim;

	UPROPERTY(EditAnywhere, Category = "NPC|Effects")
	UNiagaraSystem* DeathEffect;

private:
	bool bIsDead = false;
	bool bIsAttacking = false;
	bool bIsDodging = false;
	bool bChasing = false;
	float SpeedMultiplier = 1.0f;
	float LastAttackTime = -1000.0f;
	float LastDodgeTime = -1000.0f;

	UPROPERTY()
	UAnimSequence* CurrentAnim = nullptr;

	// Анімація зброї: 0 - опущена, 1 - піднята
	bool bWeaponRaised = false;
	float WeaponAlpha = 0.0f;
	FTransform WeaponRaisedRelative;

	bool bWeaponAnimPlaying = false;
	bool bPendingWeaponRaised = false;
	FTimerHandle WeaponSwitchTimer;
	FTimerHandle WeaponAnimEndTimer;

	FTimerHandle AttackHitTimer;
	FTimerHandle AttackEndTimer;
	FTimerHandle DodgeEndTimer;
};
