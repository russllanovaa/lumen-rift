// Лабораторна робота 6: ліхтар на стовпі, що гойдається на вітрі (анімація засобами UE)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LanternPost.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UPointLightComponent;
class AWeatherManager;

/**
 * Стовп (SM_LanternPost) з рукою, на кінці якої на ланцюжку висить ліхтар (SM_HangingLantern).
 * Ліхтар гойдається маятником навколо точки підвісу; коли в сцені буря (AWeatherManager),
 * вітер сильнішає і ліхтар гойдається дужче. Усередині ліхтаря світиться точкове світло.
 */
UCLASS()
class MYPROJECT_API ALanternPost : public AActor
{
	GENERATED_BODY()

public:
	ALanternPost();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Lantern|Components")
	UStaticMeshComponent* PostMesh;

	// Точка підвісу: на неї діє маятник
	UPROPERTY(VisibleAnywhere, Category = "Lantern|Components")
	USceneComponent* SwingPivot;

	UPROPERTY(VisibleAnywhere, Category = "Lantern|Components")
	UStaticMeshComponent* LanternMesh;

	UPROPERTY(VisibleAnywhere, Category = "Lantern|Components")
	UPointLightComponent* Glow;

	UPROPERTY(EditAnywhere, Category = "Lantern|Meshes")
	UStaticMesh* PostAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Lantern|Meshes")
	UStaticMesh* LanternAsset = nullptr;

	// Матеріали для слотів ліхтаря й стовпа (необов'язково)
	UPROPERTY(EditAnywhere, Category = "Lantern|Meshes")
	UMaterialInterface* LanternGlowMaterial = nullptr;

	// Положення точки підвісу відносно стовпа, см (для SM_LanternPost це кінець руки)
	UPROPERTY(EditAnywhere, Category = "Lantern|Meshes")
	FVector HookOffset = FVector(100.0f, 0.0f, 320.0f);

	// --- Гойдання ---
	// Кут відхилення в штиль / під час бурі, градуси
	UPROPERTY(EditAnywhere, Category = "Lantern|Swing")
	float CalmSwingAngle = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Lantern|Swing")
	float StormSwingAngle = 28.0f;

	// Циклів за секунду в штиль / під час бурі
	UPROPERTY(EditAnywhere, Category = "Lantern|Swing")
	float CalmSwingSpeed = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Lantern|Swing")
	float StormSwingSpeed = 0.55f;

	// Вісь гойдання: (0,1,0) - ліхтар гойдається вздовж руки, (1,0,0) - поперек
	UPROPERTY(EditAnywhere, Category = "Lantern|Swing")
	FVector SwingAxis = FVector(0.0f, 1.0f, 0.0f);

	// --- Світло ---
	UPROPERTY(EditAnywhere, Category = "Lantern|Glow")
	FLinearColor GlowColor = FLinearColor(0.8f, 0.4f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Lantern|Glow")
	float GlowIntensity = 6000.0f;

	UPROPERTY(EditAnywhere, Category = "Lantern|Glow")
	float GlowRadius = 700.0f;

	// Наскільки світло мерехтить
	UPROPERTY(EditAnywhere, Category = "Lantern|Glow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlowFlicker = 0.25f;

private:
	UPROPERTY()
	AWeatherManager* Weather = nullptr;

	float Time = 0.0f;
	float Phase = 0.0f;
	float SwingPhase = 0.0f; // накопичена фаза, щоб зміна швидкості не рвала рух
};
