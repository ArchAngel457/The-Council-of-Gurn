// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "Runtime/Slate/Public/Framework/SlateDelegates.h"

enum class ESRPopupType : uint8
{
	PT_None,
	PT_Accuracy,
	PT_CustomProfile,
	PT_SetupParams,
	PT_DrawImages,
	PT_DoLearning,
	PT_IngameSetup,
};

class SYMBOLRECOGNIZERPLUGINEDITOR_API SRPopup
{
public:
	static ESRPopupType CurrentPopupType;

	static void ShowPopup(const FText& InInfo, FOnClicked InOnConfilm = NULL, FOnClicked InOnCancel = NULL);
	static void ShowPopup(TSharedRef<class SWidget> InContent, FVector2D InSize = FVector2D(500, 300));
	static void HidePopup();
	static void ShowTutorial(class USRToolManager* InToolKit, int32 InTutId = 0);
	static void ShowAccuracyTest(class USRToolManager* InToolKit);
private:
	static TWeakPtr<class SWindow> SRPopupWidget;	
};