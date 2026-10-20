#include "SlowZone.h"
#include "EnemyNPC.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

ASlowZone::ASlowZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetBoxExtent(FVector(300.0f, 300.0f, 100.0f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetHiddenInGame(true);
	Trigger->ShapeColor = FColor::Green;
	RootComponent = Trigger;

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ASlowZone::OnBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &ASlowZone::OnEndOverlap);
}

void ASlowZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (Trigger)
	{
		Trigger->SetHiddenInGame(!bShowDebugBox);
	}
	RebuildBushes();
}

float ASlowZone::FindGroundZ(const FVector& XY, float TopZ, float BottomZ, bool& bFound) const
{
	bFound = false;
	UWorld* World = GetWorld();
	if (!World)
	{
		return BottomZ;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SlowZoneGround), false, this);
	const FVector Start(XY.X, XY.Y, TopZ + 500.0f);
	const FVector End(XY.X, XY.Y, BottomZ - 500.0f);
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		bFound = true;
		return Hit.ImpactPoint.Z;
	}
	return BottomZ;
}

void ASlowZone::RebuildBushes()
{
	// Прибрати старі кущі
	for (UHierarchicalInstancedStaticMeshComponent* C : BushComponents)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	BushComponents.Reset();

	if (!Trigger || BushCount <= 0)
	{
		return;
	}

	TArray<UStaticMesh*> Meshes;
	for (UStaticMesh* M : BushMeshes)
	{
		if (M)
		{
			Meshes.Add(M);
		}
	}
	if (Meshes.Num() == 0)
	{
		return;
	}

	// По одному HISM на кожен меш
	for (UStaticMesh* M : Meshes)
	{
		UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
		C->SetStaticMesh(M);
		C->SetupAttachment(RootComponent);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->CastShadow = true;
		C->RegisterComponent();
		BushComponents.Add(C);
	}

	const FVector Extent = Trigger->GetScaledBoxExtent();
	const FVector Center = Trigger->GetComponentLocation();
	const FQuat Rot = Trigger->GetComponentQuat();

	FRandomStream Rand(RandomSeed);
	for (int32 i = 0; i < BushCount; ++i)
	{
		const FVector Local(Rand.FRandRange(-Extent.X, Extent.X), Rand.FRandRange(-Extent.Y, Extent.Y), 0.0f);
		FVector WorldPos = Center + Rot.RotateVector(Local);

		if (bSnapToGround)
		{
			bool bFound = false;
			WorldPos.Z = FindGroundZ(WorldPos, Center.Z + Extent.Z, Center.Z - Extent.Z, bFound);
		}
		else
		{
			WorldPos.Z = Center.Z - Extent.Z;
		}

		const float Scale = Rand.FRandRange(FMath::Min(MinScale, MaxScale), FMath::Max(MinScale, MaxScale));
		const FRotator Yaw(0.0f, Rand.FRandRange(0.0f, 360.0f), 0.0f);

		const int32 MeshIndex = i % BushComponents.Num();
		BushComponents[MeshIndex]->AddInstance(FTransform(Yaw, WorldPos, FVector(Scale)), true);
	}
}

void ASlowZone::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AEnemyNPC* NPC = Cast<AEnemyNPC>(OtherActor))
	{
		NPC->SetSpeedMultiplier(SpeedMultiplier);
	}
}

void ASlowZone::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (AEnemyNPC* NPC = Cast<AEnemyNPC>(OtherActor))
	{
		NPC->SetSpeedMultiplier(1.0f);
	}
}
