// Лабораторна робота 6: об'єкти сцени, анімовані засобами UE (обертання, плавання, гойдання, пульсація)

#include "AnimatedProp.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AAnimatedProp::AAnimatedProp()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Усі рухи застосовуються до AnimatedPivot, а меш і світло прикріплені до нього
	AnimatedPivot = CreateDefaultSubobject<USceneComponent>(TEXT("AnimatedPivot"));
	AnimatedPivot->SetupAttachment(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(AnimatedPivot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(AnimatedPivot);
	Glow->SetCastShadows(false);
	Glow->SetVisibility(false);
}

void AAnimatedProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (MeshAsset)
	{
		Mesh->SetStaticMesh(MeshAsset);
	}
	Mesh->SetRelativeLocation(MeshOffset);
	Mesh->SetRelativeRotation(MeshRotation);
	Mesh->SetRelativeScale3D(MeshScale);
	Mesh->SetCollisionEnabled(bMeshCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

	if (MaterialOverride)
	{
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			Mesh->SetMaterial(i, MaterialOverride);
		}
	}

	Glow->SetVisibility(bEnableGlow);
	Glow->SetLightColor(GlowColor);
	Glow->SetIntensity(GlowIntensity);
	Glow->SetAttenuationRadius(GlowRadius);
	Glow->SetRelativeLocation(MeshOffset);
}

void AAnimatedProp::BeginPlay()
{
	Super::BeginPlay();
	Phase = bRandomPhase ? FMath::FRandRange(0.0f, 2.0f * PI) : 0.0f;
}

void AAnimatedProp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bAnimationEnabled)
	{
		return;
	}

	Time += DeltaTime;

	// Обертання накопичується
	SpinQuat = (SpinQuat * (RotationRate * DeltaTime).Quaternion()).GetNormalized();

	// Плавання вгору-вниз вздовж осі
	const FVector Bob = BobAxis.GetSafeNormal() * BobAmplitude * FMath::Sin(2.0f * PI * BobSpeed * Time + Phase);

	// Гойдання маятником навколо початку актора
	const float SwingRad = FMath::DegreesToRadians(SwingAngle) * FMath::Sin(2.0f * PI * SwingSpeed * Time + Phase);
	const FQuat Swing = FQuat(SwingAxis.GetSafeNormal(), SwingRad);

	// Пульсація розміру
	const float Pulse = 1.0f + PulseAmplitude * FMath::Sin(2.0f * PI * PulseSpeed * Time + Phase);

	AnimatedPivot->SetRelativeLocationAndRotation(Bob, Swing * SpinQuat);
	AnimatedPivot->SetRelativeScale3D(FVector(Pulse));

	if (bEnableGlow)
	{
		const float Flicker = 1.0f + GlowFlicker * FMath::Sin(2.0f * PI * (PulseSpeed + SwingSpeed) * Time + Phase);
		Glow->SetIntensity(GlowIntensity * Flicker);
	}
}
