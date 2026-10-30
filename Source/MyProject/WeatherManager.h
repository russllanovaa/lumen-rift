// Лабораторна робота 6: природні явища - зміна дня/ночі та магічна буря з блискавками

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "WeatherManager.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class AStaticMeshActor;
class UMaterialInstanceDynamic;
class USoundBase;

/**
 * Менеджер природних явищ. Покладіть один екземпляр на рівень і вкажіть Sun / SkyLightActor.
 *  1) День/ніч: сонце (Directional Light) обертається по колу за DayLengthSeconds,
 *     яскравість сонця, неба й скайбокса плавно падає вночі.
 *  2) Магічна буря: триває, поки гравець у AStormZone (і/або з випадковою періодичністю, якщо
 *     увімкнено bRandomStorm): густішає туман, темніє небо, у випадкові моменти б'ють блискавки.
 * Обидва явища можна вимикати: прапорці bDayNightEnabled / bStormEnabled у Details
 * або клавіші під час гри (за замовчуванням N і M). Коли явище вимкнене, сцена виглядає
 * так само, як без менеджера погоди (лаб. 5).
 */
// Сповіщення: менеджер вирішив, що зараз вдарить блискавка (її матеріалізує AStormZone)
DECLARE_MULTICAST_DELEGATE(FOnLightningStrike);

UCLASS()
class MYPROJECT_API AWeatherManager : public AActor
{
	GENERATED_BODY()

public:
	AWeatherManager();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetDayNightEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetStormEnabled(bool bEnabled);

	// Примусово почати/припинити бурю (випадковий розклад після цього продовжується)
	UFUNCTION(BlueprintCallable, Category = "Weather|Storm")
	void StartStorm();

	UFUNCTION(BlueprintCallable, Category = "Weather|Storm")
	void StopStorm();

	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetTimeOfDay() const { return TimeOfDay; }

	// 0 - ясно, 1 - буря в повну силу (плавно змінюється)
	UFUNCTION(BlueprintPure, Category = "Weather|Storm")
	float GetStormIntensity() const { return StormIntensity; }

	FOnLightningStrike OnLightningStrike;

	// Викликає AStormZone, коли гравець заходить у зону/виходить із неї
	void EnterStormZone();
	void ExitStormZone();

	UFUNCTION(BlueprintPure, Category = "Weather|Storm")
	bool IsStorming() const { return bStorming || StormZoneCount > 0; }

protected:
	virtual void BeginPlay() override;

	void UpdateStormState(float DeltaTime);
	void UpdateLightning(float DeltaTime);
	void UpdateLighting(float DeltaTime);
	void PlayThunder();
	float RandomFlashDelay() const;

	// --- Вимикачі явищ ---
	UPROPERTY(EditAnywhere, Category = "Weather|Switches")
	bool bDayNightEnabled = true;

	UPROPERTY(EditAnywhere, Category = "Weather|Switches")
	bool bStormEnabled = true;

	// Клавіші для вмикання/вимикання під час гри (нічого не робить, якщо клавіша None)
	UPROPERTY(EditAnywhere, Category = "Weather|Switches")
	FKey ToggleDayNightKey = EKeys::N;

	UPROPERTY(EditAnywhere, Category = "Weather|Switches")
	FKey ToggleStormKey = EKeys::M;

	// --- Об'єкти сцени (вибрати в Details) ---
	UPROPERTY(EditAnywhere, Category = "Weather|Scene")
	ADirectionalLight* Sun = nullptr;

	UPROPERTY(EditAnywhere, Category = "Weather|Scene")
	ASkyLight* SkyLightActor = nullptr;

	// Туман сцени. Якщо не вказано - береться Exponential Height Fog з рівня, а якщо його немає,
	// створюється новий (UE використовує лише один такий туман на рівень)
	UPROPERTY(EditAnywhere, Category = "Weather|Scene")
	AExponentialHeightFog* Fog = nullptr;

	// Скайбокс (Static Mesh актор). Його матеріал має мати Scalar Parameter SkyBrightness
	UPROPERTY(EditAnywhere, Category = "Weather|Scene")
	AStaticMeshActor* SkyboxActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Weather|Scene")
	FName SkyBrightnessParam = TEXT("SkyBrightness");

	// --- День / ніч ---
	// Тривалість повної доби в секундах реального часу
	UPROPERTY(EditAnywhere, Category = "Weather|DayNight", meta = (ClampMin = "10.0"))
	float DayLengthSeconds = 120.0f;

	// Початковий час доби, години (0..24). 12 - полудень, 0 - північ
	UPROPERTY(EditAnywhere, Category = "Weather|DayNight", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float StartTimeOfDay = 9.0f;

	// Яку частку денної яскравості сонце має вночі
	UPROPERTY(EditAnywhere, Category = "Weather|DayNight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NightLightFraction = 0.15f;

	// Яку частку денної яскравості має небо (Sky Light) вночі
	UPROPERTY(EditAnywhere, Category = "Weather|DayNight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NightSkyFraction = 0.35f;

	// Яскравість скайбокса вдень і вночі (множник, 1 = як у матеріалі)
	UPROPERTY(EditAnywhere, Category = "Weather|DayNight")
	float DaySkyboxBrightness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Weather|DayNight")
	float NightSkyboxBrightness = 0.12f;

	// Як часто оновлювати освітлення від неба (с). 0 - не оновлювати (якщо в Sky Light увімкнено Real Time Capture)
	UPROPERTY(EditAnywhere, Category = "Weather|DayNight", meta = (ClampMin = "0.0"))
	float SkyRecaptureInterval = 3.0f;

	// --- Магічна буря ---
	// Випадкові бурі по всій карті. Вимкнено за замовчуванням: буря триває лише в AStormZone
	UPROPERTY(EditAnywhere, Category = "Weather|Storm")
	bool bRandomStorm = false;

	UPROPERTY(EditAnywhere, Category = "Weather|Storm")
	bool bStartWithStorm = false;

	// Скільки триває ясна погода між бурями (випадкове значення з діапазону), с
	UPROPERTY(EditAnywhere, Category = "Weather|Storm")
	float MinClearTime = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Weather|Storm")
	float MaxClearTime = 120.0f;

	// Скільки триває буря (випадкове значення з діапазону), с
	UPROPERTY(EditAnywhere, Category = "Weather|Storm")
	float MinStormTime = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Weather|Storm")
	float MaxStormTime = 70.0f;

	// За скільки секунд буря набирає/втрачає силу
	UPROPERTY(EditAnywhere, Category = "Weather|Storm", meta = (ClampMin = "0.1"))
	float StormFadeTime = 5.0f;

	// Під час бурі сонце тьмяніє до цієї частки
	UPROPERTY(EditAnywhere, Category = "Weather|Storm", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StormSunFraction = 0.08f;

	// Під час бурі скайбокс тьмяніє до цієї частки
	UPROPERTY(EditAnywhere, Category = "Weather|Storm", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StormSkyboxFraction = 0.3f;

	// Під час бурі світло неба (Sky Light) тьмяніє до цієї частки
	UPROPERTY(EditAnywhere, Category = "Weather|Storm", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StormSkyLightFraction = 0.25f;

	// Туман бурі: до базової густоти додається ця (0.02 - легка імла, 0.1 - дуже густо)
	UPROPERTY(EditAnywhere, Category = "Weather|Storm", meta = (ClampMin = "0.0"))
	float StormFogDensity = 0.08f;

	// Колір туману під час бурі (світиться в темряві, щоб туман було видно)
	UPROPERTY(EditAnywhere, Category = "Weather|Storm")
	FLinearColor StormFogColor = FLinearColor(0.45f, 0.25f, 0.65f);

	// Під час бурі туман піднімається вище над землею (менше число = вище)
	UPROPERTY(EditAnywhere, Category = "Weather|Storm", meta = (ClampMin = "0.0"))
	float StormFogHeightFalloff = 0.02f;

	// --- Блискавки ---
	// Інтервал між блискавками під час бурі (випадковий з діапазону), с
	UPROPERTY(EditAnywhere, Category = "Weather|Storm|Lightning")
	float MinFlashInterval = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Weather|Storm|Lightning")
	float MaxFlashInterval = 7.0f;

	// Інтервал між блискавками, коли гравець у AStormZone (там вони б'ють частіше), с
	UPROPERTY(EditAnywhere, Category = "Weather|Storm|Lightning")
	float ZoneMinFlashInterval = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Weather|Storm|Lightning")
	float ZoneMaxFlashInterval = 2.5f;

	// Тривалість одного спалаху, с
	UPROPERTY(EditAnywhere, Category = "Weather|Storm|Lightning", meta = (ClampMin = "0.05"))
	float FlashDuration = 0.4f;

	// У скільки разів світло неба яскравішає в піку спалаху
	UPROPERTY(EditAnywhere, Category = "Weather|Storm|Lightning")
	float SkyFlashBoost = 2.5f;

	// У скільки разів скайбокс яскравішає в піку спалаху
	UPROPERTY(EditAnywhere, Category = "Weather|Storm|Lightning")
	float FlashSkyboxBoost = 2.0f;

	// Грім (необов'язково), лунає із затримкою після спалаху
	UPROPERTY(EditAnywhere, Category = "Weather|Storm|Lightning")
	USoundBase* ThunderSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Weather|Debug")
	bool bShowDebug = true;

private:
	float TimeOfDay = 9.0f;
	float StormIntensity = 0.0f;
	bool bStorming = false;
	int32 StormZoneCount = 0;
	float NextToggleTime = 0.0f;
	float NextFlashTime = 0.0f;
	float FlashTimeLeft = 0.0f;
	float NextRecaptureTime = 0.0f;

	FRotator SunBaseRotation = FRotator::ZeroRotator;
	float SunDayIntensity = 10.0f;
	float SkyDayIntensity = 1.0f;
	float FogBaseDensity = 0.02f;
	float FogBaseFalloff = 0.2f;
	FLinearColor FogBaseColor = FLinearColor::Black;
	FVector FogBaseLocation = FVector::ZeroVector;
	float CurrentDayFactor = 1.0f;
	float CurrentFlash = 0.0f;

	FTimerHandle ThunderTimer;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> SkyboxMIDs;
};
