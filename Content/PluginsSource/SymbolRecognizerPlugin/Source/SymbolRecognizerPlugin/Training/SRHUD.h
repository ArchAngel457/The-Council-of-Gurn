// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once

#include "GameFramework/HUD.h"
#include "SRHUD.generated.h"

/**
 * 
 */
UCLASS()
class SYMBOLRECOGNIZERPLUGIN_API ASRHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void DrawHUD() override;

};
