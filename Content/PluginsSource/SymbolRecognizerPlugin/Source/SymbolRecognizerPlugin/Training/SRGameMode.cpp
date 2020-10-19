// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#include "SRGameMode.h"
#include "SRHUD.h"
#include "SRPlayerController.h"


ASRGameMode::ASRGameMode()
{
	PlayerControllerClass = ASRPlayerController::StaticClass();
	HUDClass = ASRHUD::StaticClass();
}
