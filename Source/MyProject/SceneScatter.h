// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneScatter.generated.h"

class UStaticMesh;

// Один вид трави/квітки для масового розсіювання (інстансами)
USTRUCT(BlueprintType)
struct FScatterFoliageEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh = nullptr;

	// Скільки екземплярів цього мешу розкидати в колі
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 Count = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxScale = 1.3f;

	// Відстань, після якої цей вид не малюється, см (0 = брати загальну FoliageCullEnd)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CullEnd = 0.0f;

	// Чи є в екземплярів колізія (для дерев - так, для трави - ні)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCollision = false;
};

/**
 * Інструмент оздоблення сцени (лаб. 4, п.1). Ставиться на рівень, у масив Meshes
 * додаються різні Static Mesh (дерева, каміння, декор...). Кнопка "Scatter" у
 * Details розставляє Count окремих StaticMeshActor випадково в колі Radius навколо
 * себе: точка падає на землю (трейс по ландшафту), випадковий поворот і масштаб.
 * Кожен розставлений об'єкт - окремий актор у Outliner (рахується в 100+).
 * "Clear" видаляє всі об'єкти, розставлені цим скаттером.
 */
UCLASS()
class MYPROJECT_API ASceneScatter : public AActor
{
	GENERATED_BODY()

public:
	ASceneScatter();

	// Які меші розкидати (різні форми)
	UPROPERTY(EditAnywhere, Category = "Scatter")
	TArray<UStaticMesh*> Meshes;

	// Скільки об'єктів поставити за одне натискання
	UPROPERTY(EditAnywhere, Category = "Scatter", meta = (ClampMin = "1"))
	int32 Count = 30;

	// Радіус кола навколо цього актора, см
	UPROPERTY(EditAnywhere, Category = "Scatter", meta = (ClampMin = "100"))
	float Radius = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Scatter")
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Scatter")
	float MaxScale = 1.4f;

	// Випадковий поворот навколо вертикальної осі
	UPROPERTY(EditAnywhere, Category = "Scatter")
	bool bRandomYaw = true;

	// Мінімальна відстань між об'єктами, щоб не стояли один в одному
	UPROPERTY(EditAnywhere, Category = "Scatter")
	float MinSpacing = 150.0f;

	// Чи є в об'єктів колізія (для каміння/дерев - так, для дрібного декору можна вимкнути)
	UPROPERTY(EditAnywhere, Category = "Scatter")
	bool bWithCollision = true;

	// Назва папки в Outliner для розставлених об'єктів
	UPROPERTY(EditAnywhere, Category = "Scatter")
	FName OutlinerFolder = TEXT("Scattered");

	// Заповнює Meshes готовим набором із 20 різних форм (каміння, уламки, примітиви, власні моделі)
	// Локація 1: натуральний декор (каміння, уламки дерева/кори)
	UFUNCTION(CallInEditor, Category = "Scatter")
	void FillDecorDefaults();

	// Локація 2: Meshes = тільки вогняна лілія (квіти)
	UFUNCTION(CallInEditor, Category = "Scatter")
	void FillForestDefaults();

	UFUNCTION(CallInEditor, Category = "Scatter")
	void Scatter();

	UFUNCTION(CallInEditor, Category = "Scatter")
	void Clear();

	// ---- Трава й квіти (інстанси: один актор, багато екземплярів - легко для GPU) ----
	UPROPERTY(EditAnywhere, Category = "Foliage")
	TArray<FScatterFoliageEntry> Foliage;

	// Після якої відстані від камери інстанси не малюються, см
	UPROPERTY(EditAnywhere, Category = "Foliage")
	float FoliageCullEnd = 3000.0f;

	// Відступ від об'єктів, де трава не росте, см
	UPROPERTY(EditAnywhere, Category = "Foliage")
	float FoliageClearance = 40.0f;

	// Тіні від трави дуже дорогі; за замовчуванням вимкнені
	UPROPERTY(EditAnywhere, Category = "Foliage")
	bool bFoliageCastShadow = false;

	// Заповнює список мешами з пакета StylizedGrassFlowersPack (одна кнопка)
	UFUNCTION(CallInEditor, Category = "Foliage")
	void FillGrassPackDefaults();

	UFUNCTION(CallInEditor, Category = "Foliage")
	void ScatterFoliage();

private:
	FName GetOwnTag() const;
};
