// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SRPluginEditorStyle.h"

class SYMBOLRECOGNIZERPLUGINEDITOR_API FSRPluginEditorCommands : public TCommands<FSRPluginEditorCommands>
{
public:

	FSRPluginEditorCommands()
		: TCommands<FSRPluginEditorCommands>(TEXT("SymbolRecognizerPluginEditor"), NSLOCTEXT("Contexts", "SymbolRecognizerPluginEditor", "SymbolRecognizerPluginEditor Plugin"), NAME_None, FSRPluginEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};