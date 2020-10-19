// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "Runtime/SlateCore/Public/Widgets/SCompoundWidget.h"


class SYMBOLRECOGNIZERPLUGINEDITOR_API SRPreviewPanel : public SCompoundWidget
{
	enum ESRSwitchNavButtonType
	{
		PrevSymbol = 0,
		NextSymbol,
		PrevImage,
		NextImage
	};

public:
	
	SLATE_BEGIN_ARGS(SRPreviewPanel) {}
	SLATE_END_ARGS()

		UPROPERTY()
		FSlateBrush CurrentImgBrush;
	

	TSharedRef<SWidget> BuildTrainingSeciton();
	void Construct(const FArguments& InArgs, class USRToolManager* InTool);
	void OnSymbolsListRefreshed();
	FReply OnTrainNetwork();
	FReply OnShowAccuracyPanel();
	FReply OnSaveClick();
	FReply OnClearCanvasClick();
	FReply SwitchSelectedImage(ESRSwitchNavButtonType SwitchDir);
	FReply OnOpenTestLevel();
	TSharedRef<SWidget> BuildCanvasPreviewSection();
	TSharedRef<SWidget> BuildNavigationButton(ESRSwitchNavButtonType SwitchType);
	TSharedRef<SWidget> BuildSelectedImage();
	TSharedRef<SWidget> BuildSelectedImageInfo();
	TSharedRef<SWidget> BuildSelectedImageAndInfoSection();
private:

	TWeakObjectPtr<USRToolManager> ToolKit;
	TSharedPtr<class SImage> CentralImage;
private:
	struct FActiveItemInfoItem
	{
		TSharedPtr<STextBlock> SymbolNameInfoText;
		TSharedPtr<STextBlock> ImageNameInfoText;
		TSharedPtr<STextBlock> DrawnImagesInfoText;
		TSharedPtr<SEditableText> ImagePathInfoText;
	} ActiveItemInfo;
	
	
	//maybe switch to local vars.
	TSharedPtr<class SImage> SymbolImg;
	TSharedPtr<FSlateBrush> SymbolImgBrush;
	TSharedPtr<class SAccuracyTestingPanel> AccuracyTestList;

	void OnImageSelected(struct FSRSymbolDataItem SelectedSymbol,struct FSRImageDataItem SelectedImage);
	void UpdateImageSelected(bool bTryLoadTexture = false);
};