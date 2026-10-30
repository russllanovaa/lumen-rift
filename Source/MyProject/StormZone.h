// Лабораторна робота 6: зона, у якій постійно триває магічна буря (туман, видимі блискавки, рослинність)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StormZone.generated.h"

class UBoxComponent;
class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;
class UPointLightComponent;
class UHierarchicalInstancedStaticMeshComponent;

// Один шар рослинності: який меш, скільки екземплярів, який розмір
USTRUCT(BlueprintType)
struct FStormVegetationLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 Count = 300;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxScale = 1.4f;

	// Нахиляти вздовж поверхні (для схилів)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAlignToSurface = false;

	// Відкидати тінь (дерева - так, трава - ні, економить продуктивність)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCastShadow = false;
};

/**
 * Зона бурі. Поки гравець усередині Trigger-коробки:
 *  - AWeatherManager тримає бурю (густий туман, темне небо, спалахи неба);
 *  - у зоні вдаряють блискавки - справжні 3D-об'єкти (BoltMesh) з точковим світлом, між якими
 *    можна бігати. Місце удару вибирається випадково навколо гравця;
 *  - Vegetation Layers засаджують зону травою, кущами й деревами, прив'язаними до землі.
 * Розмір зони змінюється через Scale актора або Box Extent у Details.
 */
UCLASS()
class MYPROJECT_API AStormZone : public AActor
{
	GENERATED_BODY()

public:
	AStormZone();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

	// Перебудувати рослинність (також викликається автоматично при зміні актора)
	UFUNCTION(CallInEditor, Category = "Storm|Vegetation")
	void RebuildVegetation();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetPlayerInside(bool bInside);
	void OnLightningStrike();
	bool FindGround(const FVector& XY, float TopZ, float BottomZ, FVector& OutPoint, FVector& OutNormal) const;

	UPROPERTY(VisibleAnywhere, Category = "Storm|Components")
	UBoxComponent* Trigger;

	// Показувати межі зони в грі (для налаштування й скріншотів)
	UPROPERTY(EditAnywhere, Category = "Storm")
	bool bShowDebugBox = false;

	// --- Блискавки ---
	// Меш блискавки SM_LightningBolt (початок координат - точка удару в землю, висота 45 м)
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	UStaticMesh* BoltMesh = nullptr;

	// Світний матеріал блискавки (наприклад MI_Bolt: білувато-фіолетовий, Strength 20-40)
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	UMaterialInterface* BoltMaterial = nullptr;

	// Скільки блискавок може бути видно одночасно
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "1", ClampMax = "32"))
	int32 BoltPoolSize = 12;

	// Постійні блискавки: поки гравець у зоні, зона сама б'є блискавками
	// (додатково до ударів менеджера), з випадковим інтервалом
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	bool bContinuousBolts = true;

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0.02"))
	float ContinuousMinInterval = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0.02"))
	float ContinuousMaxInterval = 0.6f;

	// Блискавки вдаряють у секторі перед камерою гравця, щоб їх було видно (кут в обидва боки від напряму погляду).
	// 180 = навколо гравця в усі боки
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float StrikeViewHalfAngle = 60.0f;

	// Писати на екрані повідомлення про кожен удар (для діагностики й скріншотів)
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	bool bShowStrikeMessages = true;

	// Відстань від гравця до місця удару, см
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	float MinStrikeDistance = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	float MaxStrikeDistance = 3500.0f;

	// Розмір блискавки: висота (множник) і товщина (множник)
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	float MinBoltHeightScale = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	float MaxBoltHeightScale = 1.3f;

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	float BoltThickness = 8.0f;

	// Скільки секунд видно одну блискавку
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning", meta = (ClampMin = "0.05"))
	float BoltDuration = 0.45f;

	// Світло від блискавки (освітлює землю й рослини навколо місця удару)
	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	float BoltLightIntensity = 200000.0f;

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	float BoltLightRadius = 8000.0f;

	UPROPERTY(EditAnywhere, Category = "Storm|Lightning")
	FLinearColor BoltLightColor = FLinearColor(0.8f, 0.7f, 1.0f);

	// --- Рослинність ---
	UPROPERTY(EditAnywhere, Category = "Storm|Vegetation")
	bool bBuildVegetation = true;

	UPROPERTY(EditAnywhere, Category = "Storm|Vegetation")
	TArray<FStormVegetationLayer> VegetationLayers;

	UPROPERTY(EditAnywhere, Category = "Storm|Vegetation")
	int32 RandomSeed = 11;

	// Дальність, після якої рослини зникають (оптимізація), см. 0 - не обмежувати
	UPROPERTY(EditAnywhere, Category = "Storm|Vegetation")
	float VegetationCullDistance = 12000.0f;

private:
	bool bPlayerInside = false;
	FDelegateHandle StrikeHandle;

	UPROPERTY(Transient)
	TArray<UHierarchicalInstancedStaticMeshComponent*> VegetationComponents;

	UPROPERTY(Transient)
	TArray<UStaticMeshComponent*> BoltMeshes;

	UPROPERTY(Transient)
	TArray<UPointLightComponent*> BoltLights;

	TArray<float> BoltTimeLeft;
	int32 NextBolt = 0;
	float ContinuousTimer = 0.5f;
};
