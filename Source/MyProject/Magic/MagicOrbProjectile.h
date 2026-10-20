// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseMagicProjectile.h"
#include "MagicOrbProjectile.generated.h"

class UNiagaraSystem;
class USoundBase;
class UMaterialInterface;

/**
 * Магічна сфера. При влучанні — вибухає: штовхає фізичні об'єкти навколо
 * та завдає area damage в радіусі вибуху.
 */
UCLASS()
class MYPROJECT_API AMagicOrbProjectile : public ABaseMagicProjectile
{
	GENERATED_BODY()

public:
	AMagicOrbProjectile();

protected:
	virtual void OnProjectileHit(const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere, Category = "Magic|Explosion")
	float ExplosionRadius = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Magic|Explosion")
	float ExplosionImpulse = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Magic|Explosion")
	float ExplosionDamage = 40.0f;

	// Ефект вибуху (Niagara - той самий Niagara Examples Pack)
	UPROPERTY(EditAnywhere, Category = "Magic|Explosion")
	UNiagaraSystem* ExplosionEffect;

	UPROPERTY(EditAnywhere, Category = "Magic|Explosion")
	USoundBase* ExplosionSound;

	// Пляма/декаль на поверхні після вибуху (лишається видимою довше за частинки)
	UPROPERTY(EditAnywhere, Category = "Magic|Explosion")
	UMaterialInterface* ImpactDecalMaterial;

	UPROPERTY(EditAnywhere, Category = "Magic|Explosion")
	FVector ImpactDecalSize = FVector(48.0f, 48.0f, 48.0f);

	UPROPERTY(EditAnywhere, Category = "Magic|Explosion")
	float ImpactDecalLifeSpan = 30.0f;
};
