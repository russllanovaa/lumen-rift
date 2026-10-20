// Fill out your copyright notice in the Description page of Project Settings.

#include "MagicOrbProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"

AMagicOrbProjectile::AMagicOrbProjectile()
{
	// Сфера трохи повільніша за промінь — виглядає як "кинутий" заряд
	Speed = 1800.0f;
}

void AMagicOrbProjectile::OnProjectileHit(const FHitResult& Hit)
{
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Hit.ImpactPoint);
	}

	if (ImpactDecalMaterial)
	{
		UGameplayStatics::SpawnDecalAtLocation(
			GetWorld(), ImpactDecalMaterial, ImpactDecalSize,
			Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), ImpactDecalLifeSpan);
	}

	// Штовхаємо всі фізичні об'єкти в радіусі вибуху
	TArray<FOverlapResult> Overlaps;
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(ExplosionRadius);
	GetWorld()->OverlapMultiByChannel(Overlaps, Hit.ImpactPoint, FQuat::Identity, ECC_PhysicsBody, SphereShape);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (UPrimitiveComponent* Comp = Overlap.GetComponent())
		{
			if (Comp->IsSimulatingPhysics())
			{
				const FVector Direction = (Comp->GetComponentLocation() - Hit.ImpactPoint).GetSafeNormal();
				Comp->AddImpulseAtLocation(Direction * ExplosionImpulse, Hit.ImpactPoint);
			}
		}
	}

	if (Hit.GetActor())
	{
		UGameplayStatics::ApplyRadialDamage(
			this, ExplosionDamage, Hit.ImpactPoint, ExplosionRadius,
			nullptr, TArray<AActor*>{}, this, GetInstigatorController());
	}

	Destroy();
}
