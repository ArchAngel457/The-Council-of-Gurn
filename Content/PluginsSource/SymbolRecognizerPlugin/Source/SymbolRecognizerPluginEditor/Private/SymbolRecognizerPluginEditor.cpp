// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SymbolRecognizerPluginEditor.h"
#include "SRPluginEditorStyle.h"
#include "SRPluginEditorCommands.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Engine/Engine.h"
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Runtime/CoreUObject/Public/UObject/Package.h"
#include "SRToolManager.h"
#include "SRToolWidget.h"
#include "Editor/UnrealEd/Classes/Settings/ProjectPackagingSettings.h"
#include "Engine/EngineTypes.h"

static const FName SymbolRecognizerPluginEditorTabName("SymbolRecognizer");
const FString FSymbolRecognizerPluginEditorModule::SRDirectoryToAlwaysCook = "/SymbolRecognizerPlugin/Content";

#define LOCTEXT_NAMESPACE "FSymbolRecognizerPluginEditorModule"

TSharedPtr<class SRToolWidget> FSymbolRecognizerPluginEditorModule::ToolWidget = nullptr;

void FSymbolRecognizerPluginEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	{//make sure we are cooking asset with trained neural network.
		UProjectPackagingSettings* ProjectPackagingSettings = GetMutableDefault<UProjectPackagingSettings>();

		bool bHasSRPluginDirToCook = false;
		for (const auto& DirToCook : ProjectPackagingSettings->DirectoriesToAlwaysCook)
		{
			if (DirToCook.Path.Contains(SRDirectoryToAlwaysCook))
			{
				bHasSRPluginDirToCook = true;
				break;
			}
		}

		if (!bHasSRPluginDirToCook)
		{
			ProjectPackagingSettings->DirectoriesToAlwaysCook.Add(FDirectoryPath{ SRDirectoryToAlwaysCook });
		}
	}
	


	FSRPluginEditorStyle::Initialize();
	FSRPluginEditorStyle::ReloadTextures();

	FSRPluginEditorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSRPluginEditorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FSymbolRecognizerPluginEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("SRToolManager", FOnGetDetailCustomizationInstance::CreateStatic(&SRToolWidget::MakeInstance));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FSymbolRecognizerPluginEditorModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FSymbolRecognizerPluginEditorModule::AddToolbarExtension));

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SymbolRecognizerPluginEditorTabName, FOnSpawnTab::CreateRaw(this, &FSymbolRecognizerPluginEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FSymbolRecognizerPluginEditorTabTitle", "SymbolRecognizerPluginEditor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

}

void FSymbolRecognizerPluginEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FSRPluginEditorStyle::Shutdown();

	FSRPluginEditorCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SymbolRecognizerPluginEditorTabName);

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	{
		PropertyModule.UnregisterCustomClassLayout("SRToolManager");
	}

	CleanupOnClosed();
}

void FSymbolRecognizerPluginEditorModule::OnPluginTabClosed(TSharedRef<class SDockTab> InTab)
{
	CleanupOnClosed();
}

TSharedRef<SDockTab> FSymbolRecognizerPluginEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	if (SRToolManager && SRToolManager->IsValidLowLevel())
	{
		SRToolManager->RemoveFromRoot();
	}

	SRToolManager = NewObject<USRToolManager>(GetTransientPackage(), TEXT("SRToolManager"), RF_Transient);
	SRToolManager->InitializeParams();
	SRToolManager->AddToRoot();
	SAssignNew(ToolWidget, SRToolWidget, SRToolManager)
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FSymbolRecognizerPluginEditorModule::OnPluginTabClosed));

	return ToolWidget.ToSharedRef();
}

void FSymbolRecognizerPluginEditorModule::CleanupOnClosed()
{
	if (ToolWidget.IsValid())
	{
		ToolWidget->DoCleanup();
		ToolWidget.Reset();
	}

	if (SRToolManager && SRToolManager->IsValidLowLevel())
	{
		SRToolManager->SetShouldStopTraining(true);

		SRToolManager->SRSaveConfig();
		SRToolManager->RemoveFromRoot();
		SRToolManager = nullptr;
	}
}

void FSymbolRecognizerPluginEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(SymbolRecognizerPluginEditorTabName);
}

FString FSymbolRecognizerPluginEditorModule::GetPluginDir()
{
	static FString PluginDir = IPluginManager::Get().FindPlugin("SymbolRecognizerPlugin")->GetBaseDir() + "/";
	return PluginDir;
}

void FSymbolRecognizerPluginEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FSRPluginEditorCommands::Get().OpenPluginWindow);
}

void FSymbolRecognizerPluginEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FSRPluginEditorCommands::Get().OpenPluginWindow);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSymbolRecognizerPluginEditorModule, SymbolRecognizerPluginEditor)