// Fill out your copyright notice in the Description page of Project Settings.

#include "MagicBeamProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/DamageType.h"

AMagicBeamProjectile::AMagicBeamProjectile()
{
	// Промінь швидший за сферу - виглядає як миттєвий магічний спалах
	Speed = 3500.0f;
}

void AMagicBeamProjectile::OnProjectileHit(const FHitResult& Hit)
{
	if (BurnEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BurnEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	if (ScorchDecalMaterial)
	{
		UGameplayStatics::SpawnDecalAtLocation(
			GetWorld(), ScorchDecalMaterial, ScorchDecalSize,
			Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), ScorchDecalLifeSpan);
	}

	if (Hit.GetActor())
	{
		UGameplayStatics::ApplyPointDamage(
			Hit.GetActor(), BurnDamage, -Hit.ImpactNormal, Hit,
			GetInstigatorController(), this, UDamageType::StaticClass());
	}

	Destroy();
}
