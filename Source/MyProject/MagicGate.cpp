// Лабораторна робота 6: брама, що сама відчиняється при наближенні гравця (анімація засобами UE)

#include "MagicGate.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AMagicGate::AMagicGate()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	FrameMesh->SetupAttachment(Root);

	// Петлі: на них обертаються стулки. Ліва петля зліва (-X), права справа (+X)
	LeftHinge = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHinge"));
	LeftHinge->SetupAttachment(Root);
	LeftHinge->SetRelativeLocation(FVector(-120.0f, 0.0f, 0.0f));

	RightHinge = CreateDefaultSubobject<USceneComponent>(TEXT("RightHinge"));
	RightHinge->SetupAttachment(Root);
	RightHinge->SetRelativeLocation(FVector(120.0f, 0.0f, 0.0f));

	LeftDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoor"));
	LeftDoor->SetupAttachment(LeftHinge);

	RightDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoor"));
	RightDoor->SetupAttachment(RightHinge);
	RightDoor->SetRelativeScale3D(FVector(-1.0f, 1.0f, 1.0f)); // дзеркало: стулка йде від петлі в бік -X

	GateLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GateLight"));
	GateLight->SetupAttachment(Root);
	GateLight->SetRelativeLocation(FVector(0.0f, -80.0f, 260.0f));
	GateLight->SetCastShadows(false);
	GateLight->SetAttenuationRadius(900.0f);
}

void AMagicGate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (FrameAsset)
	{
		FrameMesh->SetStaticMesh(FrameAsset);
	}
	if (DoorAsset)
	{
		LeftDoor->SetStaticMesh(DoorAsset);
		RightDoor->SetStaticMesh(DoorAsset);
	}
	LeftHinge->SetRelativeLocation(FVector(-HingeOffset, 0.0f, 0.0f));
	RightHinge->SetRelativeLocation(FVector(HingeOffset, 0.0f, 0.0f));

	GateLight->SetLightColor(GlowColor);
	GateLight->SetIntensity(ClosedGlow);
}

void AMagicGate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Відчинена, коли гравець ближче OpenDistance; гістерезис проти смикання на межі
	bool bShouldOpen = bOpen;
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const float Dist = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
		if (Dist <= OpenDistance)
		{
			bShouldOpen = true;
		}
		else if (Dist >= OpenDistance + CloseHysteresis)
		{
			bShouldOpen = false;
		}
	}

	if (bShouldOpen != bOpen)
	{
		bOpen = bShouldOpen;
		if (bShowDebug && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(7120, 2.0f, FColor::Purple,
				bOpen ? TEXT("Брама відчиняється") : TEXT("Брама зачиняється"));
		}
	}

	Alpha = FMath::FInterpConstantTo(Alpha, bOpen ? 1.0f : 0.0f, DeltaTime, 1.0f / OpenTime);
	const float S = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

	// Лівa стулка йде від петлі в +X, права (дзеркальна) - у -X. Щоб обидві розчинялись в один бік,
	// ліва повертається на +кут, права на -кут (+Yaw повертає +X в бік +Y)
	const float Sign = bOpenTowardsPositiveY ? 1.0f : -1.0f;
	LeftHinge->SetRelativeRotation(FRotator(0.0f, Sign * OpenAngle * S, 0.0f));
	RightHinge->SetRelativeRotation(FRotator(0.0f, -Sign * OpenAngle * S, 0.0f));

	GateLight->SetIntensity(FMath::Lerp(ClosedGlow, OpenGlow, S));
}
