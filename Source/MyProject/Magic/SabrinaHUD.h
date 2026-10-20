// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SabrinaHUD.generated.h"

UCLASS()
class MYPROJECT_API ASabrinaHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void DrawHUD() override;

private:
	// Текстура прицілу - в BP_SabrinaHUD задайте щось у стилі Сабріни (зірочка/іскра)
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	class UTexture2D* CrosshairTex;
};
