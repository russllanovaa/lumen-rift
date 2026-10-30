// Лабораторна робота 6: зона, у якій постійно триває магічна буря (туман, видимі блискавки, рослинність)

#include "StormZone.h"
#include "WeatherManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"

AStormZone::AStormZone()
{
	PrimaryActorTick.bCanEverTick = true;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(2500.0f, 2500.0f, 1500.0f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->ShapeColor = FColor(170, 60, 255);
	Trigger->SetCanEverAffectNavigation(false);

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AStormZone::OnTriggerBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AStormZone::OnTriggerEndOverlap);
}

void AStormZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Trigger->SetHiddenInGame(!bShowDebugBox);
	RebuildVegetation();
}

void AStormZone::BeginPlay()
{
	Super::BeginPlay();

	// Пул блискавок: меш + точкове світло, спочатку приховані
	BoltTimeLeft.Init(0.0f, BoltPoolSize);
	for (int32 i = 0; i < BoltPoolSize; ++i)
	{
		UStaticMeshComponent* Mesh = nullptr;
		if (BoltMesh)
		{
			Mesh = NewObject<UStaticMeshComponent>(this);
			Mesh->SetStaticMesh(BoltMesh);
			if (BoltMaterial)
			{
				for (int32 m = 0; m < Mesh->GetNumMaterials(); ++m)
				{
					Mesh->SetMaterial(m, BoltMaterial);
				}
			}
			Mesh->SetupAttachment(GetRootComponent());
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCastShadow(false);
			Mesh->SetCanEverAffectNavigation(false);
			Mesh->SetUsingAbsoluteLocation(true);
			Mesh->SetUsingAbsoluteRotation(true);
			Mesh->SetUsingAbsoluteScale(true);
			Mesh->RegisterComponent();
			Mesh->SetVisibility(false);
		}
		BoltMeshes.Add(Mesh);

		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(GetRootComponent());
		Light->SetUsingAbsoluteLocation(true);
		Light->SetUsingAbsoluteRotation(true);
		Light->SetUsingAbsoluteScale(true);
		Light->SetLightColor(BoltLightColor);
		Light->SetAttenuationRadius(BoltLightRadius);
		Light->SetCastShadows(false);
		Light->SetIntensity(0.0f);
		Light->RegisterComponent();
		Light->SetVisibility(false);
		BoltLights.Add(Light);
	}
}

void AStormZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetPlayerInside(false);
	Super::EndPlay(EndPlayReason);
}

void AStormZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Постійні блискавки, поки гравець у зоні
	if (bContinuousBolts && bPlayerInside)
	{
		ContinuousTimer -= DeltaTime;
		if (ContinuousTimer <= 0.0f)
		{
			OnLightningStrike();
			ContinuousTimer = FMath::FRandRange(ContinuousMinInterval, FMath::Max(ContinuousMinInterval, ContinuousMaxInterval));
		}
	}

	for (int32 i = 0; i < BoltTimeLeft.Num(); ++i)
	{
		if (BoltTimeLeft[i] <= 0.0f)
		{
			continue;
		}
		BoltTimeLeft[i] = FMath::Max(0.0f, BoltTimeLeft[i] - DeltaTime);

		const float T = 1.0f - BoltTimeLeft[i] / BoltDuration;           // 0..1 за час блискавки
		const float Decay = FMath::Square(1.0f - T);                     // загасає
		const float Flicker = 0.65f + 0.35f * FMath::Sin(T * 60.0f);     // мерехтить
		const bool bAlive = BoltTimeLeft[i] > 0.0f;

		if (BoltMeshes.IsValidIndex(i) && BoltMeshes[i])
		{
			// на мить гасне й знову спалахує - як справжня блискавка
			BoltMeshes[i]->SetVisibility(bAlive && FMath::Sin(T * 70.0f) > -0.4f);
		}
		if (BoltLights.IsValidIndex(i) && BoltLights[i])
		{
			BoltLights[i]->SetIntensity(BoltLightIntensity * Decay * Flicker);
			BoltLights[i]->SetVisibility(bAlive);
		}
	}
}

// ---------------- Гравець у зоні ----------------

void AStormZone::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		SetPlayerInside(true);
	}
}

void AStormZone::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	// гравець має кілька колайдерів: виходом вважаємо, лише коли жоден не лишився в зоні
	if (Pawn && Pawn->IsPlayerControlled() && !Trigger->IsOverlappingActor(OtherActor))
	{
		SetPlayerInside(false);
	}
}

void AStormZone::SetPlayerInside(bool bInside)
{
	if (bInside == bPlayerInside || !GetWorld())
	{
		return;
	}
	bPlayerInside = bInside;

	AWeatherManager* Manager = Cast<AWeatherManager>(UGameplayStatics::GetActorOfClass(this, AWeatherManager::StaticClass()));
	if (!Manager)
	{
		return;
	}

	if (bInside)
	{
		Manager->EnterStormZone();
		// Блискавки менеджера (за розкладом) тепер матеріалізуються в цій зоні
		StrikeHandle = Manager->OnLightningStrike.AddUObject(this, &AStormZone::OnLightningStrike);
	}
	else
	{
		Manager->ExitStormZone();
		Manager->OnLightningStrike.Remove(StrikeHandle);
		StrikeHandle.Reset();
	}
}

// ---------------- Блискавка як об'єкт ----------------

bool AStormZone::FindGround(const FVector& XY, float TopZ, float BottomZ, FVector& OutPoint, FVector& OutNormal) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StormZoneGround), false, this);
	if (World->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, TopZ), FVector(XY.X, XY.Y, BottomZ), ECC_Visibility, Params))
	{
		OutPoint = Hit.ImpactPoint;
		OutNormal = Hit.ImpactNormal;
		return true;
	}
	return false;
}

void AStormZone::OnLightningStrike()
{
	if (BoltTimeLeft.Num() == 0)
	{
		return;
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Origin = Player ? Player->GetActorLocation() : GetActorLocation();

	// Випадкова точка навколо гравця, але не за межами зони
	const float Dist = FMath::FRandRange(MinStrikeDistance, FMath::Max(MinStrikeDistance, MaxStrikeDistance));
	// Напрям: у секторі перед камерою (щоб блискавку було видно), або в усі боки
	float ViewYaw = 0.0f;
	if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		ViewYaw = Cam->GetCameraRotation().Yaw;
	}
	const float Angle = FMath::DegreesToRadians(ViewYaw + FMath::FRandRange(-StrikeViewHalfAngle, StrikeViewHalfAngle));
	FVector Spot = Origin + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
	const FBox Box = Trigger->Bounds.GetBox();
	Spot.X = FMath::Clamp(Spot.X, Box.Min.X, Box.Max.X);
	Spot.Y = FMath::Clamp(Spot.Y, Box.Min.Y, Box.Max.Y);

	FVector Ground(Spot.X, Spot.Y, Origin.Z - 90.0f), Normal = FVector::UpVector;
	FindGround(Spot, Origin.Z + 6000.0f, Origin.Z - 8000.0f, Ground, Normal);

	if (bShowStrikeMessages && GEngine)
	{
		if (BoltMeshes.IsValidIndex(NextBolt) && BoltMeshes[NextBolt])
		{
			GEngine->AddOnScreenDebugMessage(7130, 1.5f, FColor::Yellow, FString::Printf(TEXT("Блискавка вдарила за %.0f м від гравця"), Dist / 100.0f));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(7131, 4.0f, FColor::Red, TEXT("StormZone: не вказано Bolt Mesh - блискавки не буде видно"));
		}
	}

	const int32 Index = NextBolt;
	NextBolt = (NextBolt + 1) % BoltTimeLeft.Num();
	BoltTimeLeft[Index] = BoltDuration;

	const float HeightScale = FMath::FRandRange(MinBoltHeightScale, FMath::Max(MinBoltHeightScale, MaxBoltHeightScale));
	if (BoltMeshes.IsValidIndex(Index) && BoltMeshes[Index])
	{
		BoltMeshes[Index]->SetWorldLocationAndRotation(Ground, FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f));
		BoltMeshes[Index]->SetWorldScale3D(FVector(BoltThickness, BoltThickness, HeightScale));
		BoltMeshes[Index]->SetVisibility(true);
	}
	if (BoltLights.IsValidIndex(Index) && BoltLights[Index])
	{
		// світло висить на висоті ~15 м над точкою удару, щоб освітлювати рослини навколо
		BoltLights[Index]->SetWorldLocation(Ground + FVector(0.0f, 0.0f, 1500.0f * HeightScale));
		BoltLights[Index]->SetVisibility(true);
	}
}

// ---------------- Рослинність ----------------

void AStormZone::RebuildVegetation()
{
	for (UHierarchicalInstancedStaticMeshComponent* C : VegetationComponents)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	VegetationComponents.Reset();

	if (!Trigger || !bBuildVegetation)
	{
		return;
	}

	const FVector Extent = Trigger->GetScaledBoxExtent();
	const FVector Center = Trigger->GetComponentLocation();
	const FQuat Rot = Trigger->GetComponentQuat();

	FRandomStream Rand(RandomSeed);
	for (const FStormVegetationLayer& Layer : VegetationLayers)
	{
		if (!Layer.Mesh || Layer.Count <= 0)
		{
			continue;
		}

		UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
		C->SetStaticMesh(Layer.Mesh);
		C->SetupAttachment(RootComponent);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->CastShadow = Layer.bCastShadow;
		if (VegetationCullDistance > 0.0f)
		{
			C->SetCullDistances(0, static_cast<int32>(VegetationCullDistance));
		}
		C->RegisterComponent();
		VegetationComponents.Add(C);

		for (int32 i = 0; i < Layer.Count; ++i)
		{
			const FVector Local(Rand.FRandRange(-Extent.X, Extent.X), Rand.FRandRange(-Extent.Y, Extent.Y), 0.0f);
			FVector Pos = Center + Rot.RotateVector(Local);
			FVector Normal = FVector::UpVector;

			FVector HitPoint;
			FVector HitNormal;
			if (FindGround(Pos, Center.Z + Extent.Z + 500.0f, Center.Z - Extent.Z - 500.0f, HitPoint, HitNormal))
			{
				Pos.Z = HitPoint.Z;
				Normal = HitNormal;
			}
			else
			{
				Pos.Z = Center.Z - Extent.Z;
			}

			const float Scale = Rand.FRandRange(FMath::Min(Layer.MinScale, Layer.MaxScale), FMath::Max(Layer.MinScale, Layer.MaxScale));
			FQuat Quat = FQuat(FVector::UpVector, FMath::DegreesToRadians(Rand.FRandRange(0.0f, 360.0f)));
			if (Layer.bAlignToSurface)
			{
				Quat = FQuat::FindBetweenNormals(FVector::UpVector, Normal) * Quat;
			}
			C->AddInstance(FTransform(Quat, Pos, FVector(Scale)), true);
		}
	}
}
