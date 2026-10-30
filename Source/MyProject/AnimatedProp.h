// Лабораторна робота 6: об'єкти сцени, анімовані засобами UE (обертання, плавання, гойдання, пульсація)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnimatedProp.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UPointLightComponent;

/**
 * Універсальний анімований об'єкт сцени. Вкажіть меш (Mesh Asset) і ввімкніть потрібні рухи:
 *  - обертання навколо осей (Rotation Rate, градуси за секунду),
 *  - плавання вздовж осі (Bob Amplitude / Bob Speed),
 *  - гойдання маятником навколо точки підвісу = початку актора (Swing Angle / Swing Speed),
 *  - пульсація розміру (Pulse Amplitude / Pulse Speed),
 *  - світіння навколо об'єкта (Point Light), що мерехтить у такт.
 * Рухи можна комбінувати. Нульове значення вимикає відповідний рух.
 */
UCLASS()
class MYPROJECT_API AAnimatedProp : public AActor
{
	GENERATED_BODY()

public:
	AAnimatedProp();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Prop|Components")
	USceneComponent* AnimatedPivot;

	UPROPERTY(VisibleAnywhere, Category = "Prop|Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Prop|Components")
	UPointLightComponent* Glow;

	// --- Вигляд ---
	UPROPERTY(EditAnywhere, Category = "Prop|Look")
	UStaticMesh* MeshAsset = nullptr;

	// Матеріал для всіх слотів меша (необов'язково, наприклад MI_Crystal)
	UPROPERTY(EditAnywhere, Category = "Prop|Look")
	UMaterialInterface* MaterialOverride = nullptr;

	// Зсув, поворот і розмір меша відносно початку актора (точки підвісу/обертання).
	// Для маятника опустіть меш нижче початку актора, наприклад Z = -60
	UPROPERTY(EditAnywhere, Category = "Prop|Look")
	FVector MeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Prop|Look")
	FRotator MeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Prop|Look")
	FVector MeshScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, Category = "Prop|Look")
	bool bMeshCollision = false;

	// --- Загальне ---
	UPROPERTY(EditAnywhere, Category = "Prop|Animation")
	bool bAnimationEnabled = true;

	// Кожен екземпляр стартує з випадкової фази, щоб однакові об'єкти не рухались синхронно
	UPROPERTY(EditAnywhere, Category = "Prop|Animation")
	bool bRandomPhase = true;

	// --- Обертання ---
	// Градусів за секунду навколо осей (Pitch, Yaw, Roll)
	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Rotation")
	FRotator RotationRate = FRotator::ZeroRotator;

	// --- Плавання ---
	// Розмах у см (в обидва боки)
	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Bob")
	float BobAmplitude = 0.0f;

	// Циклів за секунду (0.4 - один цикл за 2.5 с)
	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Bob")
	float BobSpeed = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Bob")
	FVector BobAxis = FVector(0.0f, 0.0f, 1.0f);

	// --- Гойдання маятником ---
	// Максимальний кут відхилення, градуси
	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Swing")
	float SwingAngle = 0.0f;

	// Циклів за секунду
	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Swing")
	float SwingSpeed = 0.35f;

	// Вісь гойдання (X - вперед-назад гойдається вбік, Y - навпаки)
	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Swing")
	FVector SwingAxis = FVector(1.0f, 0.0f, 0.0f);

	// --- Пульсація ---
	// 0.1 = розмір змінюється на ±10%
	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Pulse")
	float PulseAmplitude = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Prop|Animation|Pulse")
	float PulseSpeed = 0.8f;

	// --- Світіння ---
	UPROPERTY(EditAnywhere, Category = "Prop|Glow")
	bool bEnableGlow = false;

	UPROPERTY(EditAnywhere, Category = "Prop|Glow")
	FLinearColor GlowColor = FLinearColor(0.7f, 0.3f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Prop|Glow")
	float GlowIntensity = 5000.0f;

	UPROPERTY(EditAnywhere, Category = "Prop|Glow")
	float GlowRadius = 500.0f;

	// Наскільки світло мерехтить у такт пульсації/гойдання (0 - не мерехтить)
	UPROPERTY(EditAnywhere, Category = "Prop|Glow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlowFlicker = 0.3f;

private:
	float Time = 0.0f;
	float Phase = 0.0f;
	FQuat SpinQuat = FQuat::Identity;
};
