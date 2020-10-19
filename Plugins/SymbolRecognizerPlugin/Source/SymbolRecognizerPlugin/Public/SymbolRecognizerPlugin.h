// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FSymbolRecognizerPluginModule : public FDefaultGameModuleImpl
{
public:

	static FString GetPluginDir();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};