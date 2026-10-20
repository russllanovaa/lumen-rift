// Fill out your copyright notice in the Description page of Project Settings.

#include "SceneObstacle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

ASceneObstacle::ASceneObstacle()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetupAttachment(Root);

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->SetupAttachment(Root);

	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollision"));
	CapsuleCollision->SetupAttachment(Root);
}

void ASceneObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Вмикає лише одну потрібну форму колізії, решту вимикає
	auto Apply = [this](UPrimitiveComponent* Comp, bool bEnable)
	{
		if (bEnable)
		{
			Comp->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	};

	Apply(BoxCollision, CollisionType == EObstacleCollision::Box);
	Apply(SphereCollision, CollisionType == EObstacleCollision::Sphere);
	Apply(CapsuleCollision, CollisionType == EObstacleCollision::Capsule);
	Apply(Mesh, CollisionType == EObstacleCollision::MeshAsset);

	BoxCollision->SetBoxExtent(BoxExtent);
	SphereCollision->SetSphereRadius(SphereRadius);
	CapsuleCollision->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	const bool bShow = bShowCollisionInGame;
	BoxCollision->SetHiddenInGame(!bShow);
	SphereCollision->SetHiddenInGame(!bShow);
	CapsuleCollision->SetHiddenInGame(!bShow);
}
