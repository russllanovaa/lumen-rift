// Лабораторна робота 6: природні явища - зміна дня/ночі та магічна буря з блискавками

#include "WeatherManager.h"
#include "Components/SceneComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

AWeatherManager::AWeatherManager()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

}

void AWeatherManager::BeginPlay()
{
	Super::BeginPlay();

	TimeOfDay = StartTimeOfDay;

	// Запам'ятовуємо початкові значення сцени, щоб при вимкненні явищ повернути їх
	if (Sun && Sun->GetLightComponent())
	{
		SunBaseRotation = Sun->GetActorRotation();
		SunDayIntensity = Sun->GetLightComponent()->Intensity;
	}
	if (SkyLightActor && SkyLightActor->GetLightComponent())
	{
		SkyDayIntensity = SkyLightActor->GetLightComponent()->Intensity;
	}
	// Туман: свій, з рівня, або новий (UE бере лише перший Exponential Height Fog на рівні)
	if (!Fog)
	{
		Fog = Cast<AExponentialHeightFog>(UGameplayStatics::GetActorOfClass(this, AExponentialHeightFog::StaticClass()));
	}
	if (!Fog)
	{
		Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (Fog && Fog->GetComponent())
		{
			Fog->GetComponent()->SetFogDensity(0.0f); // без бурі туману немає
		}
	}
	if (Fog && Fog->GetComponent())
	{
		FogBaseDensity = Fog->GetComponent()->FogDensity;
		FogBaseFalloff = Fog->GetComponent()->FogHeightFalloff;
		FogBaseColor = Fog->GetComponent()->FogInscatteringLuminance;
		FogBaseLocation = Fog->GetActorLocation();
	}

	// Скайбокс: динамічні копії матеріалів, щоб міняти яскравість
	if (SkyboxActor && SkyboxActor->GetStaticMeshComponent())
	{
		UStaticMeshComponent* SkyMesh = SkyboxActor->GetStaticMeshComponent();
		for (int32 i = 0; i < SkyMesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInstanceDynamic* MID = SkyMesh->CreateAndSetMaterialInstanceDynamic(i))
			{
				SkyboxMIDs.Add(MID);
			}
		}
	}

	if (bStartWithStorm)
	{
		StartStorm();
		StormIntensity = 1.0f;
	}
	else
	{
		StopStorm();
	}
}

void AWeatherManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Перемикачі з клавіатури
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ToggleDayNightKey.IsValid() && PC->WasInputKeyJustPressed(ToggleDayNightKey))
		{
			SetDayNightEnabled(!bDayNightEnabled);
		}
		if (ToggleStormKey.IsValid() && PC->WasInputKeyJustPressed(ToggleStormKey))
		{
			SetStormEnabled(!bStormEnabled);
		}
	}

	UpdateStormState(DeltaTime);
	UpdateLightning(DeltaTime);
	UpdateLighting(DeltaTime);

	if (bShowDebug && GEngine)
	{
		FString DayText = TEXT("день/ніч: вимкнено");
		if (bDayNightEnabled)
		{
			const int32 Hours = FMath::FloorToInt(TimeOfDay);
			const int32 Minutes = FMath::FloorToInt((TimeOfDay - Hours) * 60.0f);
			DayText = FString::Printf(TEXT("час доби: %02d:%02d"), Hours, Minutes);
		}
		const FString StormText = !bStormEnabled ? TEXT("буря: вимкнено")
			: FString::Printf(TEXT("буря: %s (сила %.0f%%)"), (bStorming || StormZoneCount > 0) ? TEXT("триває") : TEXT("немає"), StormIntensity * 100.0f);

		GEngine->AddOnScreenDebugMessage(7110, 0.0f, FColor::Cyan, FString::Printf(TEXT("%s | %s"), *DayText, *StormText));
	}
}

void AWeatherManager::SetDayNightEnabled(bool bEnabled)
{
	bDayNightEnabled = bEnabled;
}

void AWeatherManager::SetStormEnabled(bool bEnabled)
{
	bStormEnabled = bEnabled;
	if (bEnabled)
	{
		// після вмикання одразу розпочинається нова випадкова пауза
		StopStorm();
	}
}

void AWeatherManager::StartStorm()
{
	bStorming = true;
	NextToggleTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(MinStormTime, FMath::Max(MinStormTime, MaxStormTime));
	NextFlashTime = GetWorld()->GetTimeSeconds() + RandomFlashDelay();
}

void AWeatherManager::StopStorm()
{
	bStorming = false;
	NextToggleTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(MinClearTime, FMath::Max(MinClearTime, MaxClearTime));
}

float AWeatherManager::RandomFlashDelay() const
{
	// У зоні бурі блискавки б'ють частіше
	const float Min = StormZoneCount > 0 ? ZoneMinFlashInterval : MinFlashInterval;
	const float Max = StormZoneCount > 0 ? ZoneMaxFlashInterval : MaxFlashInterval;
	return FMath::FRandRange(Min, FMath::Max(Min, Max));
}

void AWeatherManager::EnterStormZone()
{
	++StormZoneCount;
	NextFlashTime = GetWorld()->GetTimeSeconds() + RandomFlashDelay();
}

void AWeatherManager::ExitStormZone()
{
	StormZoneCount = FMath::Max(0, StormZoneCount - 1);
}

void AWeatherManager::UpdateStormState(float DeltaTime)
{
	if (bStormEnabled && bRandomStorm && GetWorld()->GetTimeSeconds() >= NextToggleTime)
	{
		if (bStorming)
		{
			StopStorm();
		}
		else
		{
			StartStorm();
		}
	}

	// Вимкнена буря затихає за той самий час, що й звичайна
	const float Target = (bStormEnabled && (bStorming || StormZoneCount > 0)) ? 1.0f : 0.0f;
	StormIntensity = FMath::FInterpConstantTo(StormIntensity, Target, DeltaTime, 1.0f / StormFadeTime);
}

void AWeatherManager::UpdateLightning(float DeltaTime)
{
	UWorld* World = GetWorld();

	// Нова блискавка: тільки коли буря вже набрала силу
	if (bStormEnabled && (bStorming || StormZoneCount > 0) && StormIntensity > 0.6f && FlashTimeLeft <= 0.0f && World->GetTimeSeconds() >= NextFlashTime)
	{
		FlashTimeLeft = FlashDuration;
		NextFlashTime = World->GetTimeSeconds() + RandomFlashDelay();

		OnLightningStrike.Broadcast();

		if (ThunderSound)
		{
			World->GetTimerManager().SetTimer(ThunderTimer, this, &AWeatherManager::PlayThunder, FMath::FRandRange(0.3f, 1.5f), false);
		}
	}

	CurrentFlash = 0.0f;
	if (FlashTimeLeft > 0.0f)
	{
		FlashTimeLeft = FMath::Max(0.0f, FlashTimeLeft - DeltaTime);
		const float T = 1.0f - FlashTimeLeft / FlashDuration;           // 0..1 за час спалаху
		const float Decay = FMath::Square(1.0f - T);                    // загасає
		const float Flicker = 0.65f + 0.35f * FMath::Sin(T * 60.0f);    // мерехтить
		CurrentFlash = Decay * Flicker;
	}
}

void AWeatherManager::PlayThunder()
{
	if (ThunderSound)
	{
		UGameplayStatics::PlaySound2D(this, ThunderSound);
	}
}

void AWeatherManager::UpdateLighting(float DeltaTime)
{
	if (bDayNightEnabled)
	{
		TimeOfDay = FMath::Fmod(TimeOfDay + DeltaTime * 24.0f / DayLengthSeconds, 24.0f);
	}

	// 12:00 -> Pitch = -90 (світить вертикально вниз), 0:00 -> +90 (знизу, сонце за горизонтом)
	const float Pitch = TimeOfDay / 24.0f * 360.0f + 90.0f;
	const float SunHeight = -FMath::Sin(FMath::DegreesToRadians(Pitch)); // 1 - в зеніті, -1 - в надирі
	// Коли день/ніч вимкнено - завжди "день", сцена як її налаштовано в редакторі
	CurrentDayFactor = bDayNightEnabled ? FMath::SmoothStep(-0.05f, 0.30f, SunHeight) : 1.0f;

	if (Sun && Sun->GetLightComponent())
	{
		Sun->SetActorRotation(bDayNightEnabled ? FRotator(Pitch, SunBaseRotation.Yaw, 0.0f) : SunBaseRotation);

		const float DayPart = NightLightFraction + (1.0f - NightLightFraction) * CurrentDayFactor;
		const float StormPart = FMath::Lerp(1.0f, StormSunFraction, StormIntensity);
		Sun->GetLightComponent()->SetIntensity(SunDayIntensity * DayPart * StormPart);
	}

	if (SkyLightActor && SkyLightActor->GetLightComponent())
	{
		USkyLightComponent* Sky = SkyLightActor->GetLightComponent();
		const float SkyPart = NightSkyFraction + (1.0f - NightSkyFraction) * CurrentDayFactor;
		const float StormPart = FMath::Lerp(1.0f, StormSkyLightFraction, StormIntensity);
		Sky->SetIntensity(SkyDayIntensity * SkyPart * StormPart * (1.0f + SkyFlashBoost * CurrentFlash));

		const bool bNeedsRecapture = bDayNightEnabled || StormIntensity > 0.0f;
		if (bNeedsRecapture && SkyRecaptureInterval > 0.0f && GetWorld()->GetTimeSeconds() >= NextRecaptureTime)
		{
			Sky->RecaptureSky();
			NextRecaptureTime = GetWorld()->GetTimeSeconds() + SkyRecaptureInterval;
		}
	}

	if (Fog && Fog->GetComponent())
	{
		UExponentialHeightFogComponent* FogComp = Fog->GetComponent();
		FogComp->SetFogDensity(FogBaseDensity + StormFogDensity * StormIntensity);
		FogComp->SetFogHeightFalloff(FMath::Lerp(FogBaseFalloff, StormFogHeightFalloff, StormIntensity));
		FogComp->SetFogMaxOpacity(FMath::Lerp(FogComp->FogMaxOpacity, 1.0f, StormIntensity));
		FogComp->FogInscatteringLuminance = FMath::Lerp(FogBaseColor, StormFogColor, StormIntensity);

		// Туман густий біля висоти його актора й рідшає вгору: під час бурі тримаємо його на рівні землі гравця
		FVector FogLocation = FogBaseLocation;
		if (StormIntensity > 0.001f)
		{
			if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
			{
				FogLocation.Z = FMath::Lerp(FogBaseLocation.Z, Player->GetActorLocation().Z - 100.0f, StormIntensity);
			}
		}
		Fog->SetActorLocation(FogLocation);
		FogComp->MarkRenderStateDirty();
	}

	// Яскравість скайбокса: вночі темніє, під час бурі тьмяніє, у спалах яскравішає
	const float DaySky = bDayNightEnabled ? FMath::Lerp(NightSkyboxBrightness, DaySkyboxBrightness, CurrentDayFactor) : 1.0f;
	const float SkyBrightness = DaySky
		* FMath::Lerp(1.0f, StormSkyboxFraction, StormIntensity)
		* (1.0f + FlashSkyboxBoost * CurrentFlash);
	for (UMaterialInstanceDynamic* MID : SkyboxMIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(SkyBrightnessParam, SkyBrightness);
		}
	}
}
