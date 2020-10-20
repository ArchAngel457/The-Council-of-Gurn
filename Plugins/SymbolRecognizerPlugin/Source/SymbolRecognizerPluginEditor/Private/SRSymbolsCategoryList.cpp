// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once
#include "SRSymbolsCategoryList.h"
#include "SRToolManager.h"
#include "Runtime/Slate/Public/Widgets/Views/STableViewBase.h"
#include "Runtime/Slate/Public/Widgets/Views/SExpanderArrow.h"
#include "Runtime/Slate/Public/Widgets/Views/STableRow.h"
#include "Runtime/Slate/Public/Widgets/Views/SListView.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBox.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Engine/Texture2D.h"
#include "Runtime/SlateCore/Public/Widgets/Images/SImage.h"
#include "Runtime/Slate/Public/Widgets/Input/SEditableText.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBorder.h"
#include "Runtime/SlateCore/Public/Brushes/SlateColorBrush.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScrollBox.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScaleBox.h"
#include "SRPulseIndicator.h"
#include "SRPopupHandler.h"
#include "SRPluginEditorStyle.h"
#include "Runtime/Slate/Public/Widgets/SToolTip.h"

FLinearColor SelectedSmallImageColor = FLinearColor(0.065961, 1, 0.0f, 1.0f);
FLinearColor DefaultSmallImageColor = FLinearColor(0.85, 0.85, 0.85, 1.0f);

class SRSymbolCategoryTableRow : public STableRow<TSharedRef<IPropertyHandle>>
{
public:
	SLATE_BEGIN_ARGS(SRSymbolCategoryTableRow) {}
	SLATE_END_ARGS()

	TWeakObjectPtr<USRToolManager> ToolKit;
	TSharedPtr<SVerticalBox> ImagesVBox;
	TSharedPtr<SScrollBox> HScrollImages;
	TArray<TSharedPtr<SBox>> HScollChildren;

	int32 SymbolID = -1;
	int32 SmallImgSize = 24;
	FText DrawnImagesTextInfo;
	bool bAllImagesDrawn = false;
	const FName BackgroundAssetPropName = "BackgroundHelper";
	bool bMyItemIsExpanded = false;
	

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, SRRowDataType InItem, USRToolManager* InToolKit)
	{
		STableRow<TSharedRef<IPropertyHandle>>::Construct(STableRow::FArguments(), InOwnerTableView);
		bShowSelection = false;

		InItem->GetChildHandle("SymbolId", true)->GetValue(SymbolID);
		ToolKit = InToolKit;
		ToolKit->ImagesSelectedEvent.AddSP(this, &SRSymbolCategoryTableRow::OnImageUpdated);

		auto Font = IDetailLayoutBuilder::GetDetailFont();
		auto FontBold = IDetailLayoutBuilder::GetDetailFontBold();

		FText SymbolName = FText::FromString("SYMBOL ID: " + FString::FromInt(SymbolID));

		UpdateDrawnImagesInfo();

		//////////////////// SMALL IMAGES HORIZONTAL SCROLL BOX////////////
		SAssignNew(HScrollImages, SScrollBox)
			.Orientation(Orient_Horizontal)
			.ScrollBarDragFocusCause(EFocusCause::Cleared)
			.AllowOverscroll(EAllowOverscroll::Yes);

		/////////////////////////////////////////////////////////////

		TSharedRef<SBox> LoadImagexBox = SNew(SBox)
			.WidthOverride(20)
			.HeightOverride(20)
			.MaxAspectRatio(1.0f)
			[
				SNew(SButton)
				.OnClicked(this, &SRSymbolCategoryTableRow::OnChangeSymbolsDirectory)
				.ToolTipText(FText::FromString("Browse for a new source Symbol path"))
				[
					SNew(STextBlock)
					.Font(FontBold)
					.Text(FText::FromString("..."))
				]
			];

		TSharedRef<SHorizontalBox> LoadImgAndText = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(2)
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[
				LoadImagexBox
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.VAlign(EVerticalAlignment::VAlign_Center)
				.HeightOverride(20)
				[
					SNew(SEditableText)
					.Justification(ETextJustify::Right)
					.TextFlowDirection(ETextFlowDirection::RightToLeft)
					.IsReadOnly(true)
					.Font(Font)
					.Text_Lambda([this]()->FText {return FText::FromString(GetSymbolDataRef().Path); })
				]
			];

		TSharedRef<SBorder> SymbolData = SNew(SBorder)
			.BorderImage_Lambda([this]() -> const FSlateBrush*{ return CheckIsCurrentSymbol() ? FSRPluginEditorStyle::Get().GetBrush("FrameBG") : &Style->OddRowBackgroundHoveredBrush; })
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(2)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Font(FontBold)
					.Text(SymbolName)
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(5)
				[
					SNew(SBorder)
					//.BorderBackgroundColor_Lambda([=]() -> FLinearColor { return FLinearColor::White; }) TODO: adjust color when drawings needed.
					.Padding(2)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.ColorAndOpacity(FLinearColor(0.0, 0.0, 0.0, 0.5f))
						]
						+SOverlay::Slot().Padding(5,2)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Bottom)
							[
								SNew(SExpanderArrow, SharedThis(this))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.ToolTipText(FText::FromString("Drawn images counter."))
								.ColorAndOpacity_Lambda([this]()->FLinearColor {return bAllImagesDrawn ? FLinearColor::Green : FLinearColor::Red; })
								.Font(FontBold)
								.Text_Lambda([this]()->FText 
								{
									return DrawnImagesTextInfo;
								})
							]
						]
					]
				]
			];

		TSharedPtr<SOverlay> HBoxOverlay;
		TSharedRef<SVerticalBox> DataBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(2)
		.HAlign(HAlign_Left)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				SymbolData
			]
			+SHorizontalBox::Slot().MaxWidth(200)
			.Padding(2)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Left)
				[
					LoadImgAndText
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
				[
					SAssignNew(HBoxOverlay, SOverlay)
					+SOverlay::Slot()
					[
						HScrollImages.ToSharedRef()
					]
				]
			]
		];

		if (SymbolID == 0)
		{
			HBoxOverlay->AddSlot(1000)
				[
					SNew(SRPulseIndicator)
					.ShouldPulse_Lambda([this]()->bool {return SRPopup::CurrentPopupType == ESRPopupType::PT_DrawImages; })
				];
		}


		ChildSlot
		[
			SNew(SBorder)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.AutoWidth()
					[	
						SNew(SBorder)
						.Padding(5)
						[
							DataBox			
						]
					]
					//symbol name
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Left)
					.FillWidth(1.0f)
					[
						InItem->GetChildHandle(BackgroundAssetPropName)->CreatePropertyValueWidget()
					]	
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ImagesVBox, SVerticalBox)
					.Visibility_Lambda([this]() -> EVisibility { return bMyItemIsExpanded ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
				]
			]

		];
		
		ConstructImages();
	}

	FSRSymbolDataItem& GetSymbolDataRef() const
	{
		check(ToolKit.IsValid());
		return ToolKit->GetCurrentProfileRef().Symbols[SymbolID];
	}

	FSRImageDataItem& GetImageDataRef(int32 ImgId) const
	{
		return ToolKit->GetCurrentProfileRef().Symbols[SymbolID].Images[ImgId];
	}

	FReply OnImportImage(int32 InImgId, TSharedPtr<SImage> IconImg)
	{
		FSRImageDataItem& ImgData = GetImageDataRef(InImgId);

		if (ToolKit->OpenFileDialog(ImgData.Path))
		{
			if (ToolKit->TryLoadBrushForImage(SymbolID, InImgId, true))
			{
				IconImg->SetImage(&ImgData.ImgBrush); //probably not neccessary
			}

			ToolKit->CallImageSelected(SymbolID, InImgId);
		}

		return FReply::Handled();
	}

	FReply OnDeleteImage(int32 InImgId)
	{
		ToolKit->DeleteImage(SymbolID, InImgId);
		
		return FReply::Handled();
	}

	FReply OnSelectImage(int32 InImgId)
	{
		ToolKit->CallImageSelected(SymbolID, InImgId);

		return FReply::Handled();
	}

	FReply OnSaveImage(int32 InImgId)
	{
		ToolKit->SaveImage(SymbolID, InImgId);
		UpdateDrawnImagesInfo();
		return FReply::Handled();
	}

	bool CheckIsCurrentImage(int32 ImgId) const
	{
		return (ToolKit->GetCurrentProfileRef().CurrentSymbolIdx == SymbolID) && (ToolKit->GetCurrentProfileRef().CurrentImageIdx == ImgId);
	}

	bool CheckIsCurrentSymbol() const
	{
		return ((ToolKit->Profiles.Num() > 0) && (ToolKit->GetCurrentSymbol().SymbolId == SymbolID));
	}

	void AddIconImgToHorizotalSCrollBox(int32 InImgId, TSharedPtr<SImage>& IconImg)
	{
		TSharedPtr<SBox> HChild;
		 SAssignNew(HChild, SBox)
				.MinDesiredWidth(SmallImgSize)
				.MinDesiredHeight(SmallImgSize)
				.WidthOverride(SmallImgSize)
				.HeightOverride(SmallImgSize)
				[
					 SNew(SButton)
					.ButtonColorAndOpacity_Lambda([=]() -> FSlateColor { return CheckIsCurrentImage(InImgId) ? FSlateColor(SelectedSmallImageColor) : FSlateColor(DefaultSmallImageColor); })
					.ContentPadding(1)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
					.OnClicked(this, &SRSymbolCategoryTableRow::OnSelectImage, InImgId)
					[
						IconImg.ToSharedRef()
					]
				];

		HScollChildren.Add(HChild);

		HScrollImages->AddSlot()
			.Padding(1,1,1,1)
			[
				HChild.ToSharedRef()
			];	
	}

	TSharedRef<SWidget> CreateImageWidget(int32 InImgId)
	{
		TSharedPtr<SImage> IconImg;
		SAssignNew(IconImg, SImage).Image_Lambda([=]() -> const FSlateBrush* { return ToolKit->ChooseBrushFromImage(SymbolID, InImgId); });
		AddIconImgToHorizotalSCrollBox(InImgId, IconImg);
		
		TSharedRef<SBox> ImportPathText = SNew(SBox)
						[
							SNew(SBorder)
							.Padding(0)
							.BorderImage(FCoreStyle::Get().GetBrush("NotificationList.ItemBackground"))
							[
								SNew(SEditableText)
								.Justification(ETextJustify::Right)
								.TextFlowDirection(ETextFlowDirection::RightToLeft)
								.IsReadOnly(true)
								.Text_Lambda([=]() -> FText { return FText::FromString(GetSymbolDataRef().Images[InImgId].Path); })
							]
						];

		TSharedRef<SVerticalBox> ImageButtons = SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[		
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						SNew(SBox)
						.WidthOverride(60)
						[
							SNew(SButton)
							.ToolTip(SNew(SToolTip).Text(FText::FromString("Import texture into image container.")))
							.VAlign(VAlign_Center)
							.OnClicked(this, &SRSymbolCategoryTableRow::OnImportImage, InImgId, IconImg)
							[
								SNew(STextBlock)
								.Justification(ETextJustify::Center)
								.Text(FText::FromString("Import"))
							]
						]			
					]
					+ SHorizontalBox::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Left)
					.VAlign(VAlign_Center)
					[
						ImportPathText
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
					.WidthOverride(60)
					[
						SNew(SButton)
						.ToolTip(SNew(SToolTip).Text(FText::FromString("Save drawing to this image.")))
						.OnClicked(this, &SRSymbolCategoryTableRow::OnSaveImage, InImgId)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.Text(FText::FromString("Save"))
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
					.WidthOverride(60)
					[
						SNew(SButton)
						.ToolTip(SNew(SToolTip).Text(FText::FromString("Remove texture for this image.")))
						.OnClicked(this, &SRSymbolCategoryTableRow::OnDeleteImage, InImgId)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.Text(FText::FromString("Delete"))
						]
					]
				];

		TSharedRef<SHorizontalBox> ImageRowContent = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(this, &SRSymbolCategoryTableRow::OnSelectImage, InImgId)
				[
					SNew(SBox)
					.WidthOverride(64)
					.HeightOverride(64)
					.MaxAspectRatio(1.0f)
					[
						IconImg.ToSharedRef()
					]
				]
			]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				ImageButtons
			];


			return SNew(SBorder)
				.BorderImage_Lambda([=]() -> const FSlateBrush*{ return CheckIsCurrentImage(InImgId) ? FSRPluginEditorStyle::Get().GetBrush("FrameBG") : FCoreStyle::Get().GetBrush("Border"); })
				[
					ImageRowContent
				];
			
	}

	void UpdateDrawnImagesInfo()
	{
		const int32 DrawnImages = ToolKit->GetDrawnImagesCount(SymbolID);
		const int32 AllImagesInSymbol = ToolKit->GetCurrentProfileRef().ImagesPerSymbol;
		const FString DrawnImagesToAll = FString::FromInt(DrawnImages) + "/" + FString::FromInt(AllImagesInSymbol);

		bAllImagesDrawn = (DrawnImages == AllImagesInSymbol);

		DrawnImagesTextInfo = FText::FromString(DrawnImagesToAll);
	}

	void OnImageUpdated(FSRSymbolDataItem InSymbolItem, FSRImageDataItem InImageItem)
	{
		if (SymbolID >= ToolKit->GetCurrentProfileRef().SymbolsAmount)
		{
			return;
		}

		UpdateDrawnImagesInfo();

		if (HScrollImages)
		{
			const int32 NumChildren = HScollChildren.Num();
			for (int32 I = 0; I < NumChildren; ++I)
			{	
				if (SymbolID == InSymbolItem.SymbolId)
				{
					if (I == InImageItem.ImgId)
					{
						HScrollImages->ScrollDescendantIntoView(HScollChildren[I], true, EDescendantScrollDestination::IntoView);
						break;
					}
				}
			}
		}
		
	}

	void ConstructImages()
	{
		ImagesVBox->ClearChildren();
		HScrollImages->ClearChildren();
		HScollChildren.Empty();

		for (auto& Img : GetSymbolDataRef().Images)
		{
			ToolKit->TryLoadBrushForImage(SymbolID, Img.ImgId);
			ImagesVBox->AddSlot()
			.AutoHeight()
			[
				CreateImageWidget(Img.ImgId)
			];		
		}	
	}

	FReply OnChangeSymbolsDirectory()
	{	
		if (ToolKit->OpenDirectoryDialog(GetSymbolDataRef().Path, GetSymbolDataRef().Path))
		{
			ToolKit->LoadFilesForSymbol(GetSymbolDataRef(), GetSymbolDataRef().Path);

			if (bMyItemIsExpanded)
			{
				ToggleExpansion();
			}

			ConstructImages();
		}

		return FReply::Handled();
	}


	


	virtual bool IsItemExpanded() const override
	{
		return bMyItemIsExpanded;
	}

	virtual void ToggleExpansion() override
	{
		bMyItemIsExpanded = !bMyItemIsExpanded;
		
		if (bMyItemIsExpanded)
		{		
			if (ImagesVBox->GetChildren()->Num() == 0)
			{
				ConstructImages();
			}			
		}

	}

	virtual int32 DoesItemHaveChildren() const override
	{
		return 1;
	}
};


void SRSymbolsCategoryList::Construct(const FArguments& InArgs, USRToolManager* InTool, TSharedPtr<IPropertyHandleArray> InSymbolsArrayHandle)
{
	ToolKit = InTool;
	
	uint32 SymbolsCount = 0;
	InSymbolsArrayHandle->GetNumElements(SymbolsCount);

	for (uint32 I = 0; I < SymbolsCount; ++I)
	{
		SymbolItemsList.Add(InSymbolsArrayHandle->GetElement(I));
	}
	
	this->ChildSlot
		[
			SNew(SRListDataType)
			.AllowOverscroll(EAllowOverscroll::Yes)
			.SelectionMode(ESelectionMode::Single)
			.ListItemsSource(&SymbolItemsList)
			.OnGenerateRow(this, &SRSymbolsCategoryList::OnGenerateRow)


		];

	
}

TSharedRef< ITableRow > SRSymbolsCategoryList::OnGenerateRow(SRRowDataType Item, const TSharedRef< STableViewBase >& OwnerTable)
{
	TSharedRef< SRSymbolCategoryTableRow > ReturnRow = SNew(SRSymbolCategoryTableRow, OwnerTable, Item, ToolKit.Get());
	return ReturnRow;
}


