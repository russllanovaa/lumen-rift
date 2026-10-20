// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportPortal.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UArrowComponent;
class UNiagaraSystem;

/**
 * Телепорт (пункт 4 лабораторної) на механізмі тригера: коли гравець входить
 * у Trigger (UBoxComponent), його переносить до Target-порталу. Криниця - це
 * меш (з власною колізією), а тригер лежить над її отвором. Після телепорту
 * на обох порталах діє пауза, щоб гравця не перекидало туди-сюди.
 * Можна вимагати ключ (bRequiresKey).
 */
UCLASS()
class MYPROJECT_API ATeleportPortal : public AActor
{
	GENERATED_BODY()

public:
	ATeleportPortal();

protected:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* Trigger;

	// Де з'явиться гравець, коли телепортується СЮДИ (стрілка показує і напрямок погляду)
	UPROPERTY(VisibleAnywhere)
	UArrowComponent* ArrivalPoint;

	// Другий портал, куди веде цей
	UPROPERTY(EditAnywhere, Category = "Teleport")
	ATeleportPortal* TargetPortal = nullptr;

	UPROPERTY(EditAnywhere, Category = "Teleport")
	bool bRequiresKey = true;

	// Пауза між телепортаціями, с
	UPROPERTY(EditAnywhere, Category = "Teleport")
	float CooldownSeconds = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Teleport|Feedback")
	UNiagaraSystem* TeleportEffect = nullptr;

private:
	float SuppressUntil = 0.0f;
};
