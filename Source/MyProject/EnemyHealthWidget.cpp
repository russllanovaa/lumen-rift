// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyHealthWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UEnemyHealthWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HealthIcon && HealthIconTexture)
	{
		HealthIcon->SetBrushFromTexture(HealthIconTexture);
	}
}

void UEnemyHealthWidget::SetHealth(float Current, float Max)
{
	if (!HealthBar)
	{
		return;
	}

	const float Percent = Max > 0.0f ? FMath::Clamp(Current / Max, 0.0f, 1.0f) : 0.0f;
	HealthBar->SetPercent(Percent);

	// Тільки червоний (зелений/жовтий зарезервовані за шкалою персонажа) -
	// світліший при повному здоров'ї, темніший ближче до смерті
	const FLinearColor FullHealthColor = bUseCustomColors ? CustomFullColor : FLinearColor(1.0f, 0.25f, 0.25f);
	const FLinearColor LowHealthColor = bUseCustomColors ? CustomLowColor : FLinearColor(0.35f, 0.0f, 0.0f);
	const FLinearColor BarColor = FLinearColor::LerpUsingHSV(LowHealthColor, FullHealthColor, Percent);

	HealthBar->SetFillColorAndOpacity(BarColor);

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Current), FMath::RoundToInt(Max))));
	}
}
