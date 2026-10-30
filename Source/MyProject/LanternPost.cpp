// Лабораторна робота 6: ліхтар на стовпі, що гойдається на вітрі (анімація засобами UE)

#include "LanternPost.h"
#include "WeatherManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"

ALanternPost::ALanternPost()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PostMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PostMesh"));
	PostMesh->SetupAttachment(Root);

	SwingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("SwingPivot"));
	SwingPivot->SetupAttachment(Root);
	SwingPivot->SetRelativeLocation(HookOffset);

	LanternMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LanternMesh"));
	LanternMesh->SetupAttachment(SwingPivot);
	LanternMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(SwingPivot);
	Glow->SetRelativeLocation(FVector(0.0f, 0.0f, -63.0f)); // всередині ліхтаря
	Glow->SetCastShadows(false);
}

void ALanternPost::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PostAsset)
	{
		PostMesh->SetStaticMesh(PostAsset);
	}
	if (LanternAsset)
	{
		LanternMesh->SetStaticMesh(LanternAsset);
	}
	SwingPivot->SetRelativeLocation(HookOffset);

	Glow->SetLightColor(GlowColor);
	Glow->SetIntensity(GlowIntensity);
	Glow->SetAttenuationRadius(GlowRadius);
}

void ALanternPost::BeginPlay()
{
	Super::BeginPlay();

	Weather = Cast<AWeatherManager>(UGameplayStatics::GetActorOfClass(this, AWeatherManager::StaticClass()));
	Phase = FMath::FRandRange(0.0f, 2.0f * PI);
}

void ALanternPost::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	// Вітер: у бурі ліхтар гойдається дужче й швидше
	const float Storm = Weather ? Weather->GetStormIntensity() : 0.0f;
	const float Angle = FMath::Lerp(CalmSwingAngle, StormSwingAngle, Storm);
	const float Speed = FMath::Lerp(CalmSwingSpeed, StormSwingSpeed, Storm);

	SwingPhase += 2.0f * PI * Speed * DeltaTime;
	const float Rad = FMath::DegreesToRadians(Angle) * FMath::Sin(SwingPhase + Phase);
	SwingPivot->SetRelativeRotation(FQuat(SwingAxis.GetSafeNormal(), Rad));

	const float Flicker = 1.0f + GlowFlicker * FMath::Sin(2.0f * PI * 3.1f * Time + Phase) * FMath::Sin(2.0f * PI * 0.7f * Time);
	Glow->SetIntensity(GlowIntensity * Flicker);
}
