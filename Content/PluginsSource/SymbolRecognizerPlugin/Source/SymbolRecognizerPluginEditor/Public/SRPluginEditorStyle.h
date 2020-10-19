// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Runtime/Engine/Public/Slate/SlateGameResources.h"

/**  */
class SYMBOLRECOGNIZERPLUGINEDITOR_API FSRPluginEditorStyle
{
public:

	static void Initialize();

	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** @return The Slate style set for the Shooter game */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

private:

	static TSharedRef< class FSlateStyleSet > Create();

private:

	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};
