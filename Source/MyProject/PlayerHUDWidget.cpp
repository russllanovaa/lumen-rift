// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr float BarWidth = 720.0f;
	constexpr float BarHeight = 55.0f;
	constexpr float BarBorder = 2.0f;
	constexpr float BarIconSize = 92.0f;
	constexpr float SlotSize = 92.0f;
	constexpr float SlotBorder = 3.0f;

	const FLinearColor HealthColor(0.85f, 0.08f, 0.12f, 1.0f);
	const FLinearColor ManaColor(0.12f, 0.50f, 1.0f, 1.0f);
	const FLinearColor OrbAccent(0.70f, 0.30f, 1.0f, 1.0f);
	const FLinearColor BeamAccent(0.25f, 0.75f, 1.0f, 1.0f);

	UTextBlock* MakeText(UWidgetTree* Tree, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", Size);
		Font.OutlineSettings.OutlineSize = 3;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Text->SetFont(Font);
			Text->SetShadowOffset(FVector2D(2.0f, 2.0f));
			Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(ETextJustify::Center);
		return Text;
	}

	UImage* MakeRect(UWidgetTree* Tree, const FLinearColor& Color)
	{
		UImage* Image = Tree->ConstructWidget<UImage>();
		Image->SetColorAndOpacity(Color);
		return Image;
	}
}

TSharedRef<SWidget> UPlayerHUDWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("HUDWidgetTree"), RF_Transient);
	}

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Root;

		// --- Шкали здоров'я і мани: лівий нижній кут ---
		UVerticalBox* Bars = WidgetTree->ConstructWidget<UVerticalBox>();
		if (UCanvasPanelSlot* BarsSlot = Root->AddChildToCanvas(Bars))
		{
			BarsSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
			BarsSlot->SetAlignment(FVector2D(0.0f, 1.0f));
			BarsSlot->SetPosition(FVector2D(30.0f, -30.0f));
			BarsSlot->SetAutoSize(true);
		}
		HealthBar = BuildBar(Bars, TEXT("HEALTH"), TEXT("HP"), HealthColor, FLinearColor(0.95f, 0.75f, 0.2f, 0.95f));
		ManaBar = BuildBar(Bars, TEXT("MANA"), TEXT("MP"), ManaColor, FLinearColor(0.7f, 0.85f, 1.0f, 0.9f));
		HealthBar.Fill->SetColorAndOpacity(HealthColor);
		ManaBar.Fill->SetColorAndOpacity(ManaColor);

		// --- Слоти здібностей: по центру внизу ---
		UHorizontalBox* Abilities = WidgetTree->ConstructWidget<UHorizontalBox>();
		if (UCanvasPanelSlot* AbSlot = Root->AddChildToCanvas(Abilities))
		{
			AbSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
			AbSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			AbSlot->SetPosition(FVector2D(0.0f, -30.0f));
			AbSlot->SetAutoSize(true);
		}
		OrbSlot = BuildAbility(Abilities, TEXT("LMB"), TEXT("ORB"), OrbAccent);
		BeamSlot = BuildAbility(Abilities, TEXT("RMB"), TEXT("BEAM"), BeamAccent);

		// --- Індикатор ключа: правий верхній кут ---
		KeyText = MakeText(WidgetTree, 28, FLinearColor(1.0f, 0.9f, 0.2f, 1.0f));
		KeyText->SetText(FText::FromString(TEXT("KEY")));
		KeyText->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* KeySlot = Root->AddChildToCanvas(KeyText))
		{
			KeySlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
			KeySlot->SetAlignment(FVector2D(1.0f, 0.0f));
			KeySlot->SetPosition(FVector2D(-30.0f, 30.0f));
			KeySlot->SetAutoSize(true);
		}

		// --- Повідомлення: зверху по центру ---
		MessageText = MakeText(WidgetTree, 32, FLinearColor::White);
		MessageText->SetRenderOpacity(0.0f);
		if (UCanvasPanelSlot* MsgSlot = Root->AddChildToCanvas(MessageText))
		{
			MsgSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			MsgSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			MsgSlot->SetPosition(FVector2D(0.0f, 90.0f));
			MsgSlot->SetAutoSize(true);
		}

		SetBarTarget(HealthBar, 1.0f, 1.0f);
		SetBarTarget(ManaBar, 1.0f, 1.0f);
	}

	return Super::RebuildWidget();
}

UPlayerHUDWidget::FHUDBar UPlayerHUDWidget::BuildBar(UVerticalBox* Parent, const FString& Label,
	const FString& BadgeLetters, const FLinearColor& BadgeColor, const FLinearColor& TrailColor)
{
	FHUDBar Bar;
	Bar.Label = Label;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.0f, 4.0f));
	}

	// Значок: кольоровий квадрат з рамкою + іконка поверх. Якщо текстури іконки
	// нема, видно кольоровий квадрат із буквами HP / MP - плутанини все одно нема.
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
	IconBox->SetWidthOverride(BarIconSize);
	IconBox->SetHeightOverride(BarIconSize);
	UOverlay* IconLayers = WidgetTree->ConstructWidget<UOverlay>();
	IconBox->AddChild(IconLayers);

	UImage* BadgeFrame = MakeRect(WidgetTree, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	if (UOverlaySlot* S = IconLayers->AddChildToOverlay(BadgeFrame))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}
	UImage* BadgeBack = MakeRect(WidgetTree, BadgeColor * 0.6f);
	if (UOverlaySlot* S = IconLayers->AddChildToOverlay(BadgeBack))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(2.0f));
	}
	Bar.Badge = MakeText(WidgetTree, 28, FLinearColor::White);
	Bar.Badge->SetText(FText::FromString(BadgeLetters));
	if (UOverlaySlot* S = IconLayers->AddChildToOverlay(Bar.Badge))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}
	Bar.Icon = WidgetTree->ConstructWidget<UImage>();
	Bar.Icon->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* S = IconLayers->AddChildToOverlay(Bar.Icon))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(3.0f));
	}
	if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	// Сама шкала: рамка -> фон -> "хвіст" -> заповнення -> число
	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>();
	BarBox->SetWidthOverride(BarWidth);
	BarBox->SetHeightOverride(BarHeight);
	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
	BarBox->AddChild(Layers);

	UImage* Frame = MakeRect(WidgetTree, FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Frame))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}

	UImage* Back = MakeRect(WidgetTree, FLinearColor(0.08f, 0.08f, 0.1f, 0.9f));
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Back))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(BarBorder));
	}

	Bar.TrailBox = WidgetTree->ConstructWidget<USizeBox>();
	Bar.TrailBox->SetHeightOverride(BarHeight - BarBorder * 2.0f);
	Bar.TrailBox->SetWidthOverride(BarWidth - BarBorder * 2.0f);
	Bar.Trail = MakeRect(WidgetTree, TrailColor);
	Bar.TrailBox->AddChild(Bar.Trail);
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Bar.TrailBox))
	{
		S->SetHorizontalAlignment(HAlign_Left);
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(BarBorder));
	}

	Bar.FillBox = WidgetTree->ConstructWidget<USizeBox>();
	Bar.FillBox->SetHeightOverride(BarHeight - BarBorder * 2.0f);
	Bar.FillBox->SetWidthOverride(BarWidth - BarBorder * 2.0f);
	Bar.Fill = MakeRect(WidgetTree, FLinearColor::White);
	Bar.FillBox->AddChild(Bar.Fill);
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Bar.FillBox))
	{
		S->SetHorizontalAlignment(HAlign_Left);
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(BarBorder));
	}

	Bar.Text = MakeText(WidgetTree, 26, FLinearColor::White);
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Bar.Text))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}

	if (UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(BarBox))
	{
		BarSlot->SetVerticalAlignment(VAlign_Center);
	}

	return Bar;
}

UPlayerHUDWidget::FHUDAbility UPlayerHUDWidget::BuildAbility(UHorizontalBox* Parent, const FString& KeyLabel,
	const FString& Name, const FLinearColor& Accent)
{
	FHUDAbility Ability;
	Ability.Accent = Accent;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	if (UHorizontalBoxSlot* ColSlot = Parent->AddChildToHorizontalBox(Column))
	{
		ColSlot->SetPadding(FMargin(8.0f, 0.0f));
	}

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	Box->SetWidthOverride(SlotSize);
	Box->SetHeightOverride(SlotSize);
	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
	Box->AddChild(Layers);
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Box))
	{
		S->SetHorizontalAlignment(HAlign_Center);
	}

	// Рамка кольору здібності
	Ability.Border = MakeRect(WidgetTree, Accent);
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Ability.Border))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}

	// Темний фон під іконкою
	UImage* Back = MakeRect(WidgetTree, FLinearColor(Accent.R * 0.2f, Accent.G * 0.2f, Accent.B * 0.2f, 1.0f));
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Back))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(SlotBorder));
	}

	// Іконка
	Ability.Icon = WidgetTree->ConstructWidget<UImage>();
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Ability.Icon))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(SlotBorder + 6.0f));
	}

	// Затемнення перезарядки - зверху вниз, висота = залишок кулдауну
	Ability.CooldownBox = WidgetTree->ConstructWidget<USizeBox>();
	Ability.CooldownBox->SetHeightOverride(0.0f);
	UImage* Dim = MakeRect(WidgetTree, FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	Ability.CooldownBox->AddChild(Dim);
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Ability.CooldownBox))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Top);
		S->SetPadding(FMargin(SlotBorder));
	}

	// Таймер по центру
	Ability.TimerText = MakeText(WidgetTree, 26, FLinearColor::White);
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Ability.TimerText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}

	// Клавіша у верхньому лівому куті
	UTextBlock* Key = MakeText(WidgetTree, 15, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	Key->SetText(FText::FromString(KeyLabel));
	if (UOverlaySlot* S = Layers->AddChildToOverlay(Key))
	{
		S->SetHorizontalAlignment(HAlign_Left);
		S->SetVerticalAlignment(VAlign_Top);
		S->SetPadding(FMargin(6.0f, 4.0f));
	}

	// Назва здібності внизу слота: ORB / BEAM
	UTextBlock* NameText = MakeText(WidgetTree, 18, FLinearColor::White);
	NameText->SetText(FText::FromString(Name));
	if (UOverlaySlot* S = Layers->AddChildToOverlay(NameText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	// Додатковий підпис під слотом (для променя - прогрес вбивств)
	Ability.ExtraText = MakeText(WidgetTree, 17, FLinearColor(1.0f, 0.95f, 0.4f, 1.0f));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Ability.ExtraText))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
	}

	return Ability;
}

void UPlayerHUDWidget::SetIcons(UTexture2D* HealthTex, UTexture2D* ManaTex, UTexture2D* OrbTex, UTexture2D* BeamTex)
{
	auto Apply = [](UImage* Image, UTexture2D* Tex, ESlateVisibility NoTexVisibility)
	{
		if (!Image)
		{
			return;
		}
		if (Tex)
		{
			Image->SetBrushFromTexture(Tex);
			Image->SetColorAndOpacity(FLinearColor::White);
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image->SetVisibility(NoTexVisibility);
		}
	};

	Apply(HealthBar.Icon, HealthTex, ESlateVisibility::Collapsed);
	Apply(ManaBar.Icon, ManaTex, ESlateVisibility::Collapsed);

	// Буквені значки HP / MP потрібні лише поки нема картинки
	if (HealthBar.Badge)
	{
		HealthBar.Badge->SetVisibility(HealthTex ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (ManaBar.Badge)
	{
		ManaBar.Badge->SetVisibility(ManaTex ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	Apply(OrbSlot.Icon, OrbTex, ESlateVisibility::Collapsed);
	Apply(BeamSlot.Icon, BeamTex, ESlateVisibility::Collapsed);
}

void UPlayerHUDWidget::SetBarTarget(FHUDBar& Bar, float Current, float Max)
{
	const float Pct = Max > 0.0f ? FMath::Clamp(Current / Max, 0.0f, 1.0f) : 0.0f;

	// Втратили - "хвіст" ще трохи висить, потім повзе вниз
	if (Pct < Bar.TargetPct - KINDA_SMALL_NUMBER)
	{
		Bar.TrailDelay = 0.5f;
	}
	Bar.TargetPct = Pct;

	if (Bar.Text)
	{
		Bar.Text->SetText(FText::FromString(
			FString::Printf(TEXT("%s   %d / %d"), *Bar.Label, FMath::RoundToInt(Current), FMath::RoundToInt(Max))));
	}
}

void UPlayerHUDWidget::SetHealth(float Current, float Max)
{
	SetBarTarget(HealthBar, Current, Max);
}

void UPlayerHUDWidget::SetMana(float Current, float Max)
{
	SetBarTarget(ManaBar, Current, Max);
}

void UPlayerHUDWidget::TickBar(FHUDBar& Bar, float DeltaTime)
{
	Bar.ShownPct = FMath::FInterpTo(Bar.ShownPct, Bar.TargetPct, DeltaTime, 10.0f);

	if (Bar.TrailDelay > 0.0f)
	{
		Bar.TrailDelay -= DeltaTime;
	}
	else
	{
		Bar.TrailPct = FMath::FInterpConstantTo(Bar.TrailPct, Bar.TargetPct, DeltaTime, 0.35f);
	}
	// Під час лікування "хвіст" не може бути нижче за заповнення
	Bar.TrailPct = FMath::Max(Bar.TrailPct, Bar.ShownPct);

	const float Inner = BarWidth - BarBorder * 2.0f;
	if (Bar.FillBox)
	{
		Bar.FillBox->SetWidthOverride(FMath::Max(0.0f, Inner * Bar.ShownPct));
	}
	if (Bar.TrailBox)
	{
		Bar.TrailBox->SetWidthOverride(FMath::Max(0.0f, Inner * Bar.TrailPct));
	}
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TickBar(HealthBar, InDeltaTime);
	TickBar(ManaBar, InDeltaTime);

	if (MessageTimer > 0.0f && MessageText)
	{
		MessageTimer -= InDeltaTime;
		MessageText->SetRenderOpacity(FMath::Clamp(MessageTimer / 0.5f, 0.0f, 1.0f));
	}

	// Пульсація здоров'я, коли лишилось менше чверті
	if (HealthBar.Fill)
	{
		if (HealthBar.TargetPct < 0.25f && HealthBar.TargetPct > 0.0f)
		{
			HealthPulseTime += InDeltaTime;
			const float Pulse = 0.65f + 0.35f * FMath::Abs(FMath::Sin(HealthPulseTime * 6.0f));
			HealthBar.Fill->SetColorAndOpacity(FLinearColor(HealthColor.R * Pulse + 0.1f, HealthColor.G, HealthColor.B, 1.0f));
		}
		else
		{
			HealthBar.Fill->SetColorAndOpacity(HealthColor);
		}
	}

	// Мигання мани, коли її не вистачило на удар
	if (ManaBar.Fill)
	{
		if (ManaFlashTimer > 0.0f)
		{
			ManaFlashTimer -= InDeltaTime;
			const float Blink = FMath::Abs(FMath::Sin(ManaFlashTimer * 20.0f));
			ManaBar.Fill->SetColorAndOpacity(FLinearColor::LerpUsingHSV(ManaColor, FLinearColor(1.0f, 0.1f, 0.1f, 1.0f), Blink));
		}
		else
		{
			ManaBar.Fill->SetColorAndOpacity(ManaColor);
		}
	}
}

void UPlayerHUDWidget::FlashNotEnoughMana()
{
	ManaFlashTimer = 0.45f;
}

void UPlayerHUDWidget::SetHasKey(bool bHasKey)
{
	if (KeyText)
	{
		KeyText->SetVisibility(bHasKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPlayerHUDWidget::ShowMessage(const FString& Message, float Duration)
{
	if (MessageText)
	{
		MessageText->SetText(FText::FromString(Message));
		MessageText->SetRenderOpacity(1.0f);
		MessageTimer = Duration;
	}
}

void UPlayerHUDWidget::ApplyAbility(FHUDAbility& Ability, float Remaining, float Total, bool bAffordable)
{
	if (!Ability.CooldownBox)
	{
		return;
	}

	const float Fraction = Total > 0.0f ? FMath::Clamp(Remaining / Total, 0.0f, 1.0f) : 0.0f;
	Ability.CooldownBox->SetHeightOverride((SlotSize - SlotBorder * 2.0f) * Fraction);

	if (Ability.TimerText)
	{
		if (Remaining > 0.05f)
		{
			Ability.TimerText->SetText(FText::FromString(
				Remaining >= 10.0f ? FString::Printf(TEXT("%d"), FMath::CeilToInt(Remaining))
				                   : FString::Printf(TEXT("%.1f"), Remaining)));
		}
		else
		{
			Ability.TimerText->SetText(FText::GetEmpty());
		}
	}

	const bool bReady = Remaining <= 0.0f;
	if (Ability.Icon)
	{
		const FLinearColor Tint = !bReady ? FLinearColor(0.45f, 0.45f, 0.5f, 1.0f)
		                        : bAffordable ? FLinearColor::White
		                        : FLinearColor(0.85f, 0.35f, 0.35f, 1.0f);
		Ability.Icon->SetColorAndOpacity(Tint);
	}
	if (Ability.Border)
	{
		// Готово й вистачає мани - яскрава рамка; інакше тьмяна
		Ability.Border->SetColorAndOpacity((bReady && bAffordable) ? Ability.Accent : Ability.Accent * 0.35f);
	}
}

void UPlayerHUDWidget::SetOrbState(float CooldownRemaining, float CooldownTotal, bool bAffordable)
{
	ApplyAbility(OrbSlot, CooldownRemaining, CooldownTotal, bAffordable);
}

void UPlayerHUDWidget::SetBeamState(float CooldownRemaining, float CooldownTotal, bool bAffordable, int32 Kills, int32 KillsNeeded)
{
	ApplyAbility(BeamSlot, CooldownRemaining, CooldownTotal, bAffordable);

	if (BeamSlot.ExtraText)
	{
		if (CooldownRemaining > 0.0f && KillsNeeded > 0)
		{
			BeamSlot.ExtraText->SetText(FText::FromString(
				FString::Printf(TEXT("Kills %d/%d"), Kills, KillsNeeded)));
		}
		else
		{
			BeamSlot.ExtraText->SetText(FText::GetEmpty());
		}
	}
}
