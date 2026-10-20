// Fill out your copyright notice in the Description page of Project Settings.

#include "PushableProp.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

APushableProp::APushableProp()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	// PhysicsActor: блокує гравця й снаряди, реагує на фізику
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
}

void APushableProp::BeginPlay()
{
	Super::BeginPlay();

	UPhysicalMaterial* PhysMat = NewObject<UPhysicalMaterial>(this);

	if (PushType == EPushType::Heavy)
	{
		PhysMat->Friction = 0.6f;
		PhysMat->Restitution = 0.0f;
		Mesh->SetMassOverrideInKg(NAME_None, HeavyMass, true);
		Mesh->SetLinearDamping(HeavyLinearDamping);
		Mesh->SetAngularDamping(HeavyAngularDamping);
	}
	else
	{
		PhysMat->Friction = 0.3f;
		PhysMat->Restitution = LightBounciness;
		Mesh->SetMassOverrideInKg(NAME_None, LightMass, true);
		Mesh->SetLinearDamping(0.1f);
		Mesh->SetAngularDamping(0.2f);
	}

	Mesh->SetPhysMaterialOverride(PhysMat);

	// Гарантуємо, що тіло справді симулюється (а персонаж його штовхає)
	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Mesh->WakeAllRigidBodies();
}
