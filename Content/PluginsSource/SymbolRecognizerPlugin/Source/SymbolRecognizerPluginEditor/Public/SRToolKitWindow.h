// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "Runtime/SlateCore/Public/Widgets/SCompoundWidget.h"
#include "SRToolManager.h"




class SYMBOLRECOGNIZERPLUGINEDITOR_API SRToolKitWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRToolKitWindow)  {}
	SLATE_END_ARGS()


	UPROPERTY()
	UTexture2D* BackgroundTexture;
	FSlateBrush BackgroundBrush;

	FSlateColor OnUpdateColor() const;

	FSRSymbolDataItem CurrentSymbol;

	void OnImageSelected(FSRSymbolDataItem SelectedSymbol, FSRImageDataItem SelectedImage);

	TOptional<float> GetTrainingProgress() const;

	FReply OnCancelTraining();

	void Construct(const FArguments& InArgs, class USRToolManager* InBaseTool);

	virtual bool SupportsKeyboardFocus() const override { return false; }

private:

	TWeakObjectPtr<USRToolManager> BaseTool;

	UPROPERTY()
	class UTexture2D* OpenedTexture;
	TSharedPtr<class SImage> MyBgImg;

	void UpdateBackground(UTexture2D* InBackgroundTex);
	void OnBackgroundImageChanged(int32 InSymbolId);
	
};