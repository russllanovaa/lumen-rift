// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseMagicProjectile.h"
#include "MagicBeamProjectile.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

/**
 * Магічний промінь. При влучанні — "випалює" ціль: точковий урон
 * і декаль опіку на поверхні (замість вибуху, як у сфери).
 */
UCLASS()
class MYPROJECT_API AMagicBeamProjectile : public ABaseMagicProjectile
{
	GENERATED_BODY()

public:
	AMagicBeamProjectile();

protected:
	virtual void OnProjectileHit(const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere, Category = "Magic|Burn")
	float BurnDamage = 15.0f;

	// Матеріал декалі опіку/паленої плями
	UPROPERTY(EditAnywhere, Category = "Magic|Burn")
	UMaterialInterface* ScorchDecalMaterial;

	UPROPERTY(EditAnywhere, Category = "Magic|Burn")
	FVector ScorchDecalSize = FVector(32.0f, 32.0f, 32.0f);

	UPROPERTY(EditAnywhere, Category = "Magic|Burn")
	float ScorchDecalLifeSpan = 20.0f;

	// Ефект опіку (Niagara)
	UPROPERTY(EditAnywhere, Category = "Magic|Burn")
	UNiagaraSystem* BurnEffect;
};
