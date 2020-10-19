// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FSymbolRecognizerPluginEditorModule : public IModuleInterface
{
public:
	static TSharedPtr<class SRToolWidget> ToolWidget;
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	void OnPluginTabClosed(TSharedRef<class SDockTab> InTab);
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	static FString GetPluginDir();
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	static const FString SRDirectoryToAlwaysCook;
	TSharedPtr<class FUICommandList> PluginCommands;
	void CleanupOnClosed();
	UPROPERTY()
	class USRToolManager* SRToolManager;
};
