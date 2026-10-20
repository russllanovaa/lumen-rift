// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SabrinaAnimInstance.generated.h"

class ASabrinaCharacter;
class UCharacterMovementComponent;

/**
 * Кастомний AnimInstance персонажа. Вся логіка руху (швидкість, напрямок,
 * чи в повітрі) рахується тут, в C++. Сам AnimGraph (Blend Space, State
 * Machine) будується в Animation Blueprint, який успадковує цей клас, -
 * Unreal не дозволяє побудувати AnimGraph кодом, лише прив'язати до нього
 * ці змінні.
 */
UCLASS()
class MYPROJECT_API USabrinaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// Горизонтальна швидкість персонажа - вісь для Blend Space Idle/Walk/Run
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Speed = 0.0f;

	// Кут напрямку руху відносно погляду (0 = вперед, 90 = вправо, -90 = вліво, 180 = назад)
	// Потрібен, якщо захочете 2D Blend Space зі стрейфом замість простого 1D
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction = 0.0f;

	// Персонаж у повітрі (стрибок/падіння) - для переходу в стан Jump
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsInAir = false;

	// Чи є прискорення руху зараз (для чіткого Idle, коли гравець відпустив клавіші)
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsAccelerating = false;

private:
	UPROPERTY()
	ASabrinaCharacter* SabrinaCharacter = nullptr;

	UPROPERTY()
	UCharacterMovementComponent* MovementComponent = nullptr;

	void CacheOwner();
};
