// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UTexture2D;

/**
 * Плаваюча шкала здоров'я над ворогом (методичка, п.7, "Актор-мішень").
 * У WidgetBlueprint-нащадку обов'язково має бути ProgressBar з іменем
 * саме "HealthBar" - це прив'язка BindWidget.
 * Необов'язково (для кращої читабельності здалеку):
 *   HealthText (TextBlock) - число "35 / 50"
 *   HealthIcon (Image) - маленька іконка (наприклад череп) зліва від шкали;
 *     сама картинка береться з HealthIconTexture нижче (Class Defaults),
 *     в дизайнері нічого обирати вручну не треба.
 */
UCLASS()
class MYPROJECT_API UEnemyHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Оновлює і заповнення шкали, і підпис числами, і колір (тільки відтінки
	// червоного - зелений/жовтий зарезервовані за шкалою гравця)
	UFUNCTION(BlueprintCallable, Category = "Enemy|Health")
	void SetHealth(float Current, float Max);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HealthIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Health")
	UTexture2D* HealthIconTexture;

	// Власна палітра шкали (для NPC лаб. 5): якщо увімкнено, замість червоних
	// відтінків використовуються ці два кольори (повне / майже нульове здоров'я)
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Health|Colors")
	bool bUseCustomColors = false;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Health|Colors")
	FLinearColor CustomFullColor = FLinearColor(0.7f, 0.2f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Health|Colors")
	FLinearColor CustomLowColor = FLinearColor(0.15f, 0.0f, 0.35f);
};
