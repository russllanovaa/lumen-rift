// Лабораторна робота 5, п.3: перешкода для NPC через тригер (зона сповільнення)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SlowZone.generated.h"

class UBoxComponent;
class UStaticMesh;
class UHierarchicalInstancedStaticMeshComponent;

/**
 * Тригер-зона (хащі, болото): поки NPC всередині, його швидкість
 * множиться на SpeedMultiplier. Гравця не чіпає.
 * Для наочності зона сама засаджується високими кущами (BushMeshes),
 * які не мають колізії, тому нічого не блокують.
 */
UCLASS()
class MYPROJECT_API ASlowZone : public AActor
{
	GENERATED_BODY()

public:
	ASlowZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	// Перегенерувати кущі (також у меню правої кнопки в редакторі)
	UFUNCTION(CallInEditor, Category = "Slow Zone|Bushes")
	void RebuildBushes();

protected:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* Trigger;

	// 0.3 = NPC іде з 30% швидкості
	UPROPERTY(EditAnywhere, Category = "Slow Zone", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float SpeedMultiplier = 0.3f;

	// Показувати зелену рамку тригера в грі (для налагодження)
	UPROPERTY(EditAnywhere, Category = "Slow Zone")
	bool bShowDebugBox = false;

	// Меші кущів (високі). Можна вказати кілька, вони чергуються
	UPROPERTY(EditAnywhere, Category = "Slow Zone|Bushes")
	TArray<TObjectPtr<UStaticMesh>> BushMeshes;

	// Скільки кущів загалом на зону
	UPROPERTY(EditAnywhere, Category = "Slow Zone|Bushes", meta = (ClampMin = "0", ClampMax = "2000"))
	int32 BushCount = 60;

	UPROPERTY(EditAnywhere, Category = "Slow Zone|Bushes")
	float MinScale = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Slow Zone|Bushes")
	float MaxScale = 2.5f;

	// Однакове значення дає однакове розташування кущів
	UPROPERTY(EditAnywhere, Category = "Slow Zone|Bushes")
	int32 RandomSeed = 7;

	// Ставити кущі на землю (трасування вниз), інакше на дно рамки
	UPROPERTY(EditAnywhere, Category = "Slow Zone|Bushes")
	bool bSnapToGround = true;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> BushComponents;

	float FindGroundZ(const FVector& XY, float TopZ, float BottomZ, bool& bFound) const;
};
