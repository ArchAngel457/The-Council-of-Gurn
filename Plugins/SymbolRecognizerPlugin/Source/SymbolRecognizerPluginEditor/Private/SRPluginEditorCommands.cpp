// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SRPluginEditorCommands.h"

#define LOCTEXT_NAMESPACE "FSymbolRecognizerPluginEditorModule"

void FSRPluginEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "SR", "Bring up SymbolRecognizer window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
