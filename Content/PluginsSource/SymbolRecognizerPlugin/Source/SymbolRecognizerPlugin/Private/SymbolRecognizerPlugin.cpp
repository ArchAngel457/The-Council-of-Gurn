// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SymbolRecognizerPlugin.h"
#include "SymbolRecognizer.h"
#include "Runtime/Projects/Public/Interfaces/IPluginManager.h"


#define LOCTEXT_NAMESPACE "FSymbolRecognizerPluginModule"

FString FSymbolRecognizerPluginModule::GetPluginDir()
{
	//static FString PluginDir = FPaths::ProjectPluginsDir() + "SymbolRecognizerPlugin/";
	static FString PluginDir = IPluginManager::Get().FindPlugin("SymbolRecognizerPlugin")->GetBaseDir() + "/";
	return PluginDir;
}

void FSymbolRecognizerPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FPackageName::RegisterMountPoint(USymbolRecognizer::SymbolRecognizerMountPoint, GetPluginDir());
}

void FSymbolRecognizerPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FPackageName::UnRegisterMountPoint(USymbolRecognizer::SymbolRecognizerMountPoint, GetPluginDir());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSymbolRecognizerPluginModule, SymbolRecognizerPlugin)

