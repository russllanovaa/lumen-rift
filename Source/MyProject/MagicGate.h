// Лабораторна робота 6: брама, що сама відчиняється при наближенні гравця (анімація засобами UE)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicGate.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UPointLightComponent;

/**
 * Магічна брама: нерухома рамка і дві стулки на петлях. Коли гравець ближче за Open Distance,
 * стулки плавно розчиняються (поворот навколо петель), коли віддалився - зачиняються.
 * Меші: рамка SM_MagicGate_Frame, стулка SM_MagicGate_Door (одна й та сама для обох,
 * права дзеркалиться масштабом X = -1). Початок стулки - її петля.
 */
UCLASS()
class MYPROJECT_API AMagicGate : public AActor
{
	GENERATED_BODY()

public:
	AMagicGate();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Gate|Components")
	UStaticMeshComponent* FrameMesh;

	UPROPERTY(VisibleAnywhere, Category = "Gate|Components")
	USceneComponent* LeftHinge;

	UPROPERTY(VisibleAnywhere, Category = "Gate|Components")
	USceneComponent* RightHinge;

	UPROPERTY(VisibleAnywhere, Category = "Gate|Components")
	UStaticMeshComponent* LeftDoor;

	UPROPERTY(VisibleAnywhere, Category = "Gate|Components")
	UStaticMeshComponent* RightDoor;

	// Світло над брамою, що горить яскравіше, коли вона відкрита
	UPROPERTY(VisibleAnywhere, Category = "Gate|Components")
	UPointLightComponent* GateLight;

	UPROPERTY(EditAnywhere, Category = "Gate|Meshes")
	UStaticMesh* FrameAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Gate|Meshes")
	UStaticMesh* DoorAsset = nullptr;

	// Відстань від центру брами до петель, см (у SM_MagicGate_Frame це 120)
	UPROPERTY(EditAnywhere, Category = "Gate|Meshes")
	float HingeOffset = 120.0f;

	// --- Поведінка ---
	// Гравець ближче за цю відстань - брама відчиняється, см
	UPROPERTY(EditAnywhere, Category = "Gate|Behavior")
	float OpenDistance = 450.0f;

	// Брама зачиняється, коли гравець відійшов далі за OpenDistance + цей запас, см
	UPROPERTY(EditAnywhere, Category = "Gate|Behavior")
	float CloseHysteresis = 150.0f;

	// Кут відчинення стулки, градуси
	UPROPERTY(EditAnywhere, Category = "Gate|Behavior")
	float OpenAngle = 100.0f;

	// Тривалість відчинення/зачинення, с
	UPROPERTY(EditAnywhere, Category = "Gate|Behavior", meta = (ClampMin = "0.1"))
	float OpenTime = 1.5f;

	// Відчиняти стулки в бік +Y актора (інакше в бік -Y)
	UPROPERTY(EditAnywhere, Category = "Gate|Behavior")
	bool bOpenTowardsPositiveY = true;

	// Світіння над брамою
	UPROPERTY(EditAnywhere, Category = "Gate|Glow")
	FLinearColor GlowColor = FLinearColor(0.7f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Gate|Glow")
	float ClosedGlow = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Gate|Glow")
	float OpenGlow = 12000.0f;

	UPROPERTY(EditAnywhere, Category = "Gate|Debug")
	bool bShowDebug = true;

private:
	bool bOpen = false;
	float Alpha = 0.0f;
};
