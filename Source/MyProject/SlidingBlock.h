// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SlidingBlock.generated.h"

class UStaticMeshComponent;

/**
 * Об'єкт, який гравець ПЕРЕСУВАЄ по підлозі (категорія 2 лабораторної), без фізичної симуляції.
 * Коли персонаж упирається в нього й тисне вперед, об'єкт плавно їде в тому ж напрямку
 * зі швидкістю PushSpeed, зупиняється об стіни та інші перешкоди й трохи ковзає після
 * відпускання. Він ніколи не підлітає й не перекидається.
 * Різні об'єкти - це Blueprint-нащадки з різною швидкістю: легка книга їде швидко,
 * важкий обеліск повільно.
 */
UCLASS()
class MYPROJECT_API ASlidingBlock : public AActor
{
	GENERATED_BODY()

public:
	ASlidingBlock();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Викликає персонаж, коли штовхає об'єкт. Direction - горизонтальний напрямок штовхання.
	void ReceivePush(const FVector& Direction);

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	// Швидкість руху при штовханні, см/с (книга ~250, обеліск ~120)
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float PushSpeed = 220.0f;

	// Наскільки різко розганяється й гальмує (більше = різкіше, менше = ковзає довше)
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float Responsiveness = 6.0f;

	// Притискати об'єкт до підлоги, коли він їде (щоб не висів у повітрі на схилі)
	UPROPERTY(EditAnywhere, Category = "Sliding")
	bool bSnapToGround = true;

	// Наскільки близько (см від капсули) вже вважається "упирається"
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float PushReach = 20.0f;

private:
	void DetectPlayerPush();

	FVector PushDirection = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float PushTimeLeft = 0.0f;

	void SnapToGround();
};
