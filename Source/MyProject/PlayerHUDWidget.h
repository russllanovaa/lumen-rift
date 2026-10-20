// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

class UImage;
class UTextBlock;
class USizeBox;
class UTexture2D;

/**
 * HUD персонажа, ПОВНІСТЮ побудований у C++ (WidgetBlueprint більше не потрібен).
 *
 * Що малює:
 *   - зліва внизу: шкала здоров'я (червона) і шкала мани (синя), кожна з іконкою,
 *     числом "80 / 100", плавним заповненням і "хвостом" втраченого здоров'я
 *   - по центру внизу: два слоти здібностей - Сфера (ЛКМ) і Промінь (ПКМ),
 *     з іконкою, затемненням-перезарядкою, таймером і підсвіткою готовності
 *
 * Створюється з SabrinaCharacter через CreateWidget<UPlayerHUDWidget>(PC, UPlayerHUDWidget::StaticClass()).
 */
UCLASS()
class MYPROJECT_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Іконки (текстури) передає персонаж - вони задаються в його Class Defaults
	void SetIcons(UTexture2D* HealthTex, UTexture2D* ManaTex, UTexture2D* OrbTex, UTexture2D* BeamTex);

	void SetHealth(float Current, float Max);
	void SetMana(float Current, float Max);

	// CooldownRemaining/Total - секунди; bAffordable - чи вистачає мани
	void SetOrbState(float CooldownRemaining, float CooldownTotal, bool bAffordable);
	void SetBeamState(float CooldownRemaining, float CooldownTotal, bool bAffordable, int32 Kills, int32 KillsNeeded);

	// Мигає шкалою мани, коли гравець намагається вдарити без мани
	void FlashNotEnoughMana();

	// Напис "KEY" у правому верхньому куті, коли ключ підібрано
	void SetHasKey(bool bHasKey);

	// Короткий напис зверху по центру (згасає за Duration секунд)
	void ShowMessage(const FString& Message, float Duration = 2.5f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	struct FHUDBar
	{
		USizeBox* FillBox = nullptr;
		USizeBox* TrailBox = nullptr;
		UImage* Fill = nullptr;
		UImage* Trail = nullptr;
		UImage* Icon = nullptr;
		UTextBlock* Badge = nullptr; // буква "HP"/"MP" на значку, поки нема іконки
		UTextBlock* Text = nullptr;
		FString Label;

		float TargetPct = 1.0f;
		float ShownPct = 1.0f;
		float TrailPct = 1.0f;
		float TrailDelay = 0.0f;
	};

	struct FHUDAbility
	{
		UImage* Border = nullptr;
		UImage* Icon = nullptr;
		USizeBox* CooldownBox = nullptr;
		UTextBlock* TimerText = nullptr;
		UTextBlock* ExtraText = nullptr;
		FLinearColor Accent = FLinearColor::White;
	};

	FHUDBar BuildBar(class UVerticalBox* Parent, const FString& Label, const FString& BadgeLetters,
		const FLinearColor& BadgeColor, const FLinearColor& TrailColor);
	FHUDAbility BuildAbility(class UHorizontalBox* Parent, const FString& KeyLabel, const FString& Name,
		const FLinearColor& Accent);
	void SetBarTarget(FHUDBar& Bar, float Current, float Max);
	void TickBar(FHUDBar& Bar, float DeltaTime);
	void ApplyAbility(FHUDAbility& Ability, float Remaining, float Total, bool bAffordable);

	FHUDBar HealthBar;
	FHUDBar ManaBar;
	FHUDAbility OrbSlot;
	FHUDAbility BeamSlot;

	UTextBlock* KeyText = nullptr;
	UTextBlock* MessageText = nullptr;
	float MessageTimer = 0.0f;

	float ManaFlashTimer = 0.0f;
	float HealthPulseTime = 0.0f;
};
