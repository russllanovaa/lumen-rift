// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USoundBase;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EPickupType : uint8
{
	Health UMETA(DisplayName = "Health (potion)"),
	Mana UMETA(DisplayName = "Mana (crystal)"),
	Key UMETA(DisplayName = "Key (unlocks wells)")
};

/**
 * Предмет, який гравець підбирає дотиком: зілля (здоров'я), кристал або книга (мана).
 * Сфера-тригер + статичний меш, що обертається і плавно "дихає" вгору-вниз.
 * Різні предмети - це просто Blueprint-нащадки цього класу з іншим мешем,
 * типом і кількістю.
 */
UCLASS()
class MYPROJECT_API AMagicPickup : public AActor
{
	GENERATED_BODY()

public:
	AMagicPickup();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	USphereComponent* TriggerSphere;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	EPickupType PickupType = EPickupType::Health;

	// Скільки здоров'я / мани повертає
	UPROPERTY(EditAnywhere, Category = "Pickup")
	float Amount = 30.0f;

	// Градусів за секунду
	UPROPERTY(EditAnywhere, Category = "Pickup|Animation")
	float RotationSpeed = 90.0f;

	UPROPERTY(EditAnywhere, Category = "Pickup|Animation")
	float BobHeight = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Pickup|Animation")
	float BobSpeed = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Pickup|Feedback")
	USoundBase* PickupSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Pickup|Feedback")
	UNiagaraSystem* PickupEffect = nullptr;

	// Скільки секунд предмет летить до руки персонажа після підбору
	UPROPERTY(EditAnywhere, Category = "Pickup|Animation")
	float CollectDuration = 0.45f;

private:
	// Стан "предмет підбирається": летить до руки, зменшується, потім зникає
	bool bCollecting = false;
	float CollectTime = 0.0f;
	FVector CollectStartLocation = FVector::ZeroVector;
	FVector CollectStartScale = FVector::OneVector;
	TWeakObjectPtr<class ASabrinaCharacter> Collector;

	float TimeAccumulated = 0.0f;
	FVector MeshBaseLocation = FVector::ZeroVector;
};
