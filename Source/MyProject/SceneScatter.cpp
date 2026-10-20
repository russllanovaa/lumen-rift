// Fill out your copyright notice in the Description page of Project Settings.

#include "SceneScatter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"


namespace
{
	// Шукає висоту землі під точкою: спершу Visibility, потім WorldStatic.
	// Якщо нічого не влучило (ландшафт без колізії в редакторі) - повертає false.
	bool FindGroundZ(UWorld* World, const FVector& P, float RefZ, const FCollisionQueryParams& Params, float& OutZ)
	{
		const FVector Start(P.X, P.Y, RefZ + 20000.0f);
		const FVector End(P.X, P.Y, RefZ - 20000.0f);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) ||
			World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			OutZ = Hit.ImpactPoint.Z;
			return true;
		}
		return false;
	}
}

ASceneScatter::ASceneScatter()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FName ASceneScatter::GetOwnTag() const
{
	return FName(*FString::Printf(TEXT("ScatterBy_%s"), *GetName()));
}

void ASceneScatter::Scatter()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<UStaticMesh*> Valid;
	for (UStaticMesh* M : Meshes)
	{
		if (M)
		{
			Valid.Add(M);
		}
	}
	if (Valid.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SceneScatter: масив Meshes порожній"));
		return;
	}

	const FVector Center = GetActorLocation();
	TArray<FVector> Placed;
	int32 Spawned = 0;
	int32 Misses = 0;

	for (int32 Attempt = 0; Attempt < Count * 12 && Spawned < Count; ++Attempt)
	{
		// Випадкова точка в колі
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Dist = Radius * FMath::Sqrt(FMath::FRand());
		FVector Pos = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Dist;

		// Не ближче MinSpacing до вже поставлених
		bool bTooClose = false;
		for (const FVector& P : Placed)
		{
			if (FVector::DistSquared2D(P, Pos) < FMath::Square(MinSpacing))
			{
				bTooClose = true;
				break;
			}
		}
		if (bTooClose)
		{
			continue;
		}

		// Падаємо на землю; якщо земля не знайдена - лишаємось на висоті актора
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		float GroundZ = Center.Z;
		if (!FindGroundZ(World, Pos, Center.Z, Params, GroundZ))
		{
			++Misses;
		}
		Pos.Z = GroundZ;

		UStaticMesh* Mesh = Valid[FMath::RandRange(0, Valid.Num() - 1)];
		const float Scale = FMath::FRandRange(MinScale, MaxScale);
		const FRotator Rot(0.0f, bRandomYaw ? FMath::FRandRange(0.0f, 360.0f) : 0.0f, 0.0f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Pos, Rot, SpawnParams);
		if (!Actor)
		{
			continue;
		}

		Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		Actor->SetActorScale3D(FVector(Scale));
		Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
		if (!bWithCollision)
		{
			Actor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Actor->Tags.Add(GetOwnTag());
#if WITH_EDITOR
		Actor->SetActorLabel(FString::Printf(TEXT("%s_%d"), *Mesh->GetName(), Spawned));
		Actor->SetFolderPath(OutlinerFolder);
#endif
		Placed.Add(Pos);
		++Spawned;
	}

	UE_LOG(LogTemp, Display, TEXT("SceneScatter: поставлено %d з %d об'єктів (без землі під точкою: %d)"), Spawned, Count, Misses);
}

void ASceneScatter::FillDecorDefaults()
{
	static const TCHAR* Paths[] = {
		TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneA.SM_MWAM_StoneA"),
		TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneB.SM_MWAM_StoneB"),
		TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneC.SM_MWAM_StoneC"),
		TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneD.SM_MWAM_StoneD"),
		TEXT("/Game/Asetss/NiagaraExamples/StaticMesh/S_Rock_shopk.S_Rock_shopk"),
		TEXT("/Game/Asetss/NiagaraExamples/StaticMesh/S_Wood_Debris_vb2mfeliw.S_Wood_Debris_vb2mfeliw"),
		TEXT("/Game/Asetss/NiagaraExamples/StaticMesh/S_Tree_Debris_wjtuagls.S_Tree_Debris_wjtuagls"),
		TEXT("/Game/Asetss/NiagaraExamples/StaticMesh/S_Pine_Bark_Piece_wjtsdevs.S_Pine_Bark_Piece_wjtsdevs"),
	};

	Meshes.Reset();
	for (const TCHAR* Path : Paths)
	{
		if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, Path))
		{
			Meshes.Add(M);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SceneScatter: не знайдено %s"), Path);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("SceneScatter: у Meshes додано %d різних мешів"), Meshes.Num());
}

void ASceneScatter::FillForestDefaults()
{
	Meshes.Reset();
	if (UStaticMesh* Lily = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Models/Fire_lily/SM_FireLily.SM_FireLily")))
	{
		Meshes.Add(Lily);
	}

	Foliage.Reset();
	UE_LOG(LogTemp, Display, TEXT("SceneScatter: лілія додана у Meshes: %d"), Meshes.Num());
}

void ASceneScatter::Clear()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FName Tag = GetOwnTag();
	TArray<AActor*> ToDelete;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(Tag))
		{
			ToDelete.Add(*It);
		}
	}
	for (AActor* A : ToDelete)
	{
		A->Destroy();
	}
	UE_LOG(LogTemp, Display, TEXT("SceneScatter: видалено %d об'єктів"), ToDelete.Num());
}


void ASceneScatter::FillGrassPackDefaults()
{
	struct FPreset { const TCHAR* Name; int32 Count; float Min; float Max; };
	static const FPreset Presets[] = {
		{ TEXT("SM_Grass_a"), 500, 0.8f, 1.4f },
		{ TEXT("SM_Grass_b"), 500, 0.8f, 1.4f },
		{ TEXT("SM_SmallFlower_a"), 60, 0.8f, 1.2f },
		{ TEXT("SM_SmallFlower_b"), 60, 0.8f, 1.2f },
		{ TEXT("SM_SmallFlower_c"), 60, 0.8f, 1.2f },
		{ TEXT("SM_SmallFlower_d"), 60, 0.8f, 1.2f },
		{ TEXT("SM_Flower_Bell_Blue_a"), 30, 0.8f, 1.2f },
		{ TEXT("SM_Flower_Bell_Blue_b"), 30, 0.8f, 1.2f },
		{ TEXT("SM_Flower_Cluster_Pink_a"), 30, 0.8f, 1.2f },
		{ TEXT("SM_Flower_Cluster_Pink_b"), 30, 0.8f, 1.2f },
		{ TEXT("SM_Flower_Daisy_White_a"), 40, 0.8f, 1.2f },
		{ TEXT("SM_Flower_Daisy_White_b"), 40, 0.8f, 1.2f },
		{ TEXT("SM_Flower_Spike_Orange_a"), 25, 0.8f, 1.2f },
		{ TEXT("SM_Flower_Spike_Orange_b"), 25, 0.8f, 1.2f },
	};

	Foliage.Reset();
	for (const FPreset& P : Presets)
	{
		const FString Path = FString::Printf(TEXT("/Game/StylizedGrassFlowersPack/StaticMesh/%s.%s"), P.Name, P.Name);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("SceneScatter: не знайдено %s"), *Path);
			continue;
		}
		FScatterFoliageEntry E;
		E.Mesh = Mesh;
		E.Count = P.Count;
		E.MinScale = P.Min;
		E.MaxScale = P.Max;
		Foliage.Add(E);
	}
	UE_LOG(LogTemp, Display, TEXT("SceneScatter: додано %d видів трави/квітів"), Foliage.Num());
}

void ASceneScatter::ScatterFoliage()
{
	UWorld* World = GetWorld();
	if (!World || Foliage.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SceneScatter: список Foliage порожній (натисни Fill Grass Pack Defaults)"));
		return;
	}

	const FVector Center = GetActorLocation();

	// Один актор-тримач на весь набір
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Holder = World->SpawnActor<AActor>(AActor::StaticClass(), Center, FRotator::ZeroRotator, SpawnParams);
	if (!Holder)
	{
		return;
	}
	USceneComponent* Root = NewObject<USceneComponent>(Holder, TEXT("Root"), RF_Transactional);
	Holder->SetRootComponent(Root);
	Holder->AddInstanceComponent(Root);
	Root->RegisterComponent();
	Holder->Tags.Add(GetOwnTag());
#if WITH_EDITOR
	Holder->SetActorLabel(TEXT("GrassAndFlowers"));
	Holder->SetFolderPath(OutlinerFolder);
#endif

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Holder);

	// Зони, де трава не росте: габарити всіх невеликих акторів сцени (не ландшафт, не скаттер)
	TArray<FBox2D> Blocked;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* A = *It;
		if (A == this || A == Holder || A->IsA<AController>() || A->ActorHasTag(GetOwnTag()))
		{
			continue;
		}
		FVector Origin, Extent;
		A->GetActorBounds(true, Origin, Extent);
		if (Extent.X > 3000.0f || Extent.Y > 3000.0f || (Extent.X < 1.0f && Extent.Y < 1.0f))
		{
			continue; // ландшафт, небо, порожні актори
		}
		const FVector2D Pad(FoliageClearance, FoliageClearance);
		const FVector2D C(Origin.X, Origin.Y);
		const FVector2D E(Extent.X, Extent.Y);
		Blocked.Add(FBox2D(C - E - Pad, C + E + Pad));
	}

	int32 Total = 0;
	int32 FoliageMisses = 0;
	for (const FScatterFoliageEntry& E : Foliage)
	{
		if (!E.Mesh)
		{
			continue;
		}

		TArray<FTransform> Transforms;
		Transforms.Reserve(E.Count);
		for (int32 i = 0; i < E.Count; ++i)
		{
			const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
			const float Dist = Radius * FMath::Sqrt(FMath::FRand());
			const FVector P = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Dist;

			bool bInBlocked = false;
			for (const FBox2D& B : Blocked)
			{
				if (B.IsInside(FVector2D(P.X, P.Y)))
				{
					bInBlocked = true;
					break;
				}
			}
			if (bInBlocked)
			{
				continue;
			}

			float GroundZ = Center.Z;
			if (!FindGroundZ(World, P, Center.Z, Params, GroundZ))
			{
				++FoliageMisses;
			}

			const float S = FMath::FRandRange(E.MinScale, E.MaxScale);
			Transforms.Add(FTransform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f),
				FVector(P.X, P.Y, GroundZ), FVector(S)));
		}
		if (Transforms.Num() == 0)
		{
			continue;
		}

		UHierarchicalInstancedStaticMeshComponent* Comp =
			NewObject<UHierarchicalInstancedStaticMeshComponent>(Holder, NAME_None, RF_Transactional);
		Comp->SetStaticMesh(E.Mesh);
		Comp->SetupAttachment(Root);
		Comp->SetCollisionEnabled(E.bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(bFoliageCastShadow);
		Comp->SetCullDistances(0, FMath::RoundToInt(E.CullEnd > 0.0f ? E.CullEnd : FoliageCullEnd));
		Holder->AddInstanceComponent(Comp);
		Comp->RegisterComponent();
		Comp->AddInstances(Transforms, false, true);

		Total += Transforms.Num();
	}

	UE_LOG(LogTemp, Display, TEXT("SceneScatter: розкидано %d екземплярів трави/квітів (без землі під точкою: %d)"), Total, FoliageMisses);
}
