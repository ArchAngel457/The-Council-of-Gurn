// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SRPreviewPanel.h"
#include "SRToolManager.h"
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include "MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Runtime/SlateCore/Public/Widgets/SWindow.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Runtime/Slate/Public/Widgets/Layout/SSeparator.h"
#include "Runtime/Slate/Public/Widgets/Input/SEditableText.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBox.h"
#include "Runtime/SlateCore/Public/Widgets/Images/SImage.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Runtime/Core/Public/HAL/FileManager.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScrollBox.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBorder.h"
#include "Runtime/SlateCore/Public/Brushes/SlateImageBrush.h"
#include "SRCanvasHandler.h"
#include "SymbolRecognizer.h"
#include "Runtime/Slate/Public/Widgets/Views/SListView.h"
#include "Kismet/KismetMathLibrary.h"
#include "Editor/EditorStyle/Public/EditorStyleSet.h"
#include "SRPluginEditorStyle.h"
#include "Runtime/Slate/Public/Widgets/Input/SComboBox.h"
#include "Runtime/Slate/Public/Widgets/Input/SEditableTextBox.h"
#include "SRPopupHandler.h"
#include "Runtime/Slate/Public/Widgets/SToolTip.h"
#include "SRPulseIndicator.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"

#define AUTOHIDE_WHEN_TRAINING Visibility_Lambda([this]() -> EVisibility { return (ToolKit.IsValid() && !ToolKit->GetIsTraningNetwork()) ? EVisibility::Visible : EVisibility::Collapsed;})
#define ADD_SECTION_SEPARATION +SVerticalBox::Slot().Padding(0,5).AutoHeight()\
								[\
									SNew(SSeparator)\
									.Orientation(Orient_Horizontal)\
								]
#define ADD_SECTION_NAME(SectionName) + SVerticalBox::Slot().VAlign(VAlign_Top).HAlign(HAlign_Left).Padding(5)[\
										SNew(STextBlock)	\
										.Font(SectionFont)	\
										.Text(FText::FromString(SectionName))]


#define ADD_SPECIAL_BUTTON(InTitle, InWidth, InHeight, MethodRef, InToolTip) \
SNew(SBox)\
.HeightOverride(InHeight)\
.WidthOverride(InWidth)\
[\
	SNew(SBorder)\
	.BorderImage(FSRPluginEditorStyle::Get().GetBrush("FrameBG"))\
	[\
		SNew(SButton)\
		.ToolTip(SNew(SToolTip).Text(FText::FromString(InToolTip)))\
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")\
		.TextStyle(FEditorStyle::Get(), "NormalText.Important")\
		.VAlign(EVerticalAlignment::VAlign_Center)\
		.HAlign(EHorizontalAlignment::HAlign_Center)\
		.Text(FText::FromString(InTitle))\
		.OnClicked(this, MethodRef)\
	]\
]

FSRSymbolDataItem CurrentSymbolData;
FSRImageDataItem CurrentImageItem;


class SSymbolsDirectorySwitcher : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSymbolsDirectorySwitcher){}
	SLATE_END_ARGS()

	typedef TSharedPtr<FString> FComboItemType;
	TWeakObjectPtr<USRToolManager> ToolKit;
	FComboItemType CurrentItem;
	TArray<FComboItemType> Options;
	TSharedPtr<SComboBox<FComboItemType>> ProfilesComboList;

	void Construct(const FArguments& InArgs, USRToolManager* InToolKit)
	{
		ToolKit = InToolKit;

		for (auto& ProfilesItem : ToolKit->Profiles)
		{
			Options.Add(MakeShareable(new FString(ProfilesItem.GetProfileName())));
		}

		CurrentItem = Options[ToolKit->CurrentProfileDataID];

		ChildSlot
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SAssignNew(ProfilesComboList, SComboBox<FComboItemType>)
					.OptionsSource(&Options)
					.OnSelectionChanged(this, &SSymbolsDirectorySwitcher::OnSelectionChanged)
					.OnGenerateWidget(this, &SSymbolsDirectorySwitcher::MakeWidgetForOption)
					.InitiallySelectedItem(CurrentItem)
					[
						SNew(STextBlock)
						.Text(this, &SSymbolsDirectorySwitcher::GetCurrentItemLabel)
					]
				]
				+SHorizontalBox::Slot()
				.Padding(5)
				.AutoWidth()
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						ADD_SPECIAL_BUTTON("Add New", 80,30, &SSymbolsDirectorySwitcher::ShowOnAddPopup, "Creates New Profile with empty directory to store data for symbols recognition mechanism.")
					]
					+SOverlay::Slot()
					[
						SNew(SRPulseIndicator)
						.ShouldPulse(TAttribute<bool>(this, &SSymbolsDirectorySwitcher::IsCustomProfileTutorial))
					]
				]
				
			];
	}

	bool IsCustomProfileTutorial() const
	{
		return SRPopup::CurrentPopupType == ESRPopupType::PT_CustomProfile;
	}

	TSharedRef<SWidget> MakeWidgetForOption(FComboItemType InOption)
	{
		int32 SelectedIdx = Options.Find(InOption);

		TSharedRef<SHorizontalBox> WidgetOption = SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(2,0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(*InOption))
				.Justification(ETextJustify::Center)
			];


		WidgetOption->AddSlot()
		.Padding(5)
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.OnClicked(this, &SSymbolsDirectorySwitcher::OnShowRemovePopup, InOption)
			.Text(FText::FromString("Remove"))
		];
		

		return WidgetOption;
	}

	void RebuildOptionsAndSelectCurrent()
	{
		Options.Empty();

		for (auto& ProfilesItem : ToolKit->Profiles)
		{
			Options.Add(MakeShareable(new FString(ProfilesItem.GetProfileName())));
		}

		CurrentItem = Options.Last();
		ProfilesComboList->SetSelectedItem(CurrentItem);
	}

	FReply ShowOnAddPopup()
	{
		TSharedPtr<SEditableTextBox> ProfileTextBox;

		TSharedRef<SHorizontalBox> TypeProfileBox = SNew(SHorizontalBox)
			+SHorizontalBox::Slot().Padding(5,0)
			.AutoWidth()
			[
				SNew(SBox).WidthOverride(32).HeightOverride(32)
				[
					SNew(SImage).Image(FSRPluginEditorStyle::Get().GetBrush("SRLogo"))
				]
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			[
				SAssignNew(ProfileTextBox, SEditableTextBox)
				.HintText(FText::FromString(TEXT("Enter new profile name")))
				.ClearKeyboardFocusOnCommit(false)
				.OnTextCommitted_Lambda(
					[&](const FText& InText, ETextCommit::Type InCommitType)
					{
						
						FString NewProfileName = InText.ToString();

						if (InCommitType == ETextCommit::OnEnter && NewProfileName.Len() > 0)
						{
							bool bSuccess = ToolKit->AddNewProfileInDefaultDirectory(NewProfileName);
							RebuildOptionsAndSelectCurrent();
							ProfilesComboList->RefreshOptions();
							SRPopup::HidePopup();
						}
					})
				];

			TSharedRef<SBox> AddProfilesPopupContent = SNew(SBox)
				.VAlign(VAlign_Center)
				.WidthOverride(500)
				.HeightOverride(200)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.Padding(10)
					[
						TypeProfileBox
					]
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Bottom)
					.HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(5)
						[
							SNew(SBox).WidthOverride(80)
							[
								SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center)
								.Text(FText::FromString("CONFIRM"))
								.OnClicked(this, &SSymbolsDirectorySwitcher::OnAddNew, ProfileTextBox)
							]
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(5)
						[
							SNew(SBox)
							.WidthOverride(80)
							[
								SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center)
								.Text(FText::FromString("CANCEL"))
								.OnClicked_Lambda([this]()
								{
									SRPopup::HidePopup();
									return(FReply::Handled());
								})
							]
						]
					]
				];
			
			SRPopup::ShowPopup(AddProfilesPopupContent);

		return FReply::Handled();
	}

	FReply OnAddNew(TSharedPtr<SEditableTextBox> ProfileTextBox)
	{
		FString NewProfileName = ProfileTextBox->GetText().ToString();

		if (NewProfileName.Len() > 0)
		{
			bool bSuccess = ToolKit->AddNewProfileInDefaultDirectory(NewProfileName);
			RebuildOptionsAndSelectCurrent();
			ProfilesComboList->RefreshOptions();
			SRPopup::HidePopup();
		}		

		return FReply::Handled();
	}

	FReply OnShowRemovePopup(FComboItemType InOption)
	{
		FText RemoveInfo = FText::FromString("You will delete " + *InOption + " profile with its content (drawn images per symbol) \n Are You Sure?");
		FOnClicked ConfirmDel = FOnClicked::CreateSP(this, &SSymbolsDirectorySwitcher::OnRemove, InOption);
		FOnClicked CancelDel = FOnClicked::CreateLambda([&]() {
			SRPopup::HidePopup();
			return FReply::Handled(); });

		SRPopup::ShowPopup(RemoveInfo, ConfirmDel, CancelDel);

		return FReply::Handled();
	}

	FReply OnRemove(FComboItemType InOption)
	{
		int32 OptionToRemoveIdx = Options.Find(InOption);
		
		if (ToolKit->DeleteProfile(OptionToRemoveIdx))
		{
			Options.RemoveAt(OptionToRemoveIdx);

			if (CurrentItem == InOption)
			{
				RebuildOptionsAndSelectCurrent();
			}

			ProfilesComboList->RefreshOptions();
		}

		SRPopup::HidePopup();

		return FReply::Handled();
	}

	void OnSelectionChanged(FComboItemType NewValue, ESelectInfo::Type)
	{
		int32 SelectedIdx = Options.Find(NewValue);
		if (CurrentItem != NewValue)
		{
			if (SelectedIdx > -1)
			{
				CurrentItem = NewValue;
				ToolKit->CallProfileSelected(SelectedIdx);
			}
		}
	}

	FText GetCurrentItemLabel() const
	{
		if (CurrentItem.IsValid())
		{
			return FText::FromString("Profile: " + *CurrentItem);
		}

		return  FText::FromString("INVALID ITEM");
	}
};

TSharedRef<SWidget> SRPreviewPanel::BuildTrainingSeciton()
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot().Padding(5, 5)
		.VAlign(VAlign_Top)
		.AutoWidth()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				ADD_SPECIAL_BUTTON("LAUNCH LEARNING", 150, 30, &SRPreviewPanel::OnTrainNetwork, "- Starts training neural network based on saved textures.\n - It may take a while (5-20min), depends on how much data is given to learn.")
			]
			+SOverlay::Slot()
			[
				SNew(SRPulseIndicator)
				.ShouldPulse_Lambda([this]()->bool{return (SRPopup::CurrentPopupType == ESRPopupType::PT_DoLearning); })
			]

		]
		+ SHorizontalBox::Slot().Padding(5,5)
		.AutoWidth()
		.VAlign(VAlign_Top)
		[
			ADD_SPECIAL_BUTTON("TEST DRAWING ACCURACY", 175, 30, &SRPreviewPanel::OnShowAccuracyPanel, "- Open testing panel.\n - Works when learning was conducted only.")
		];
}

FReply SRPreviewPanel::OnShowAccuracyPanel()
{
	SRPopup::ShowAccuracyTest(ToolKit.Get());
	return FReply::Handled();
}


void SRPreviewPanel::Construct(const FArguments& InArgs, class USRToolManager* InTool)
{
	ToolKit = InTool;
	auto Font = IDetailLayoutBuilder::GetDetailFont();
	SymbolImgBrush = MakeShareable(new FSlateBrush());
	auto SectionFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);

	ChildSlot
	[
		SNew(SVerticalBox)
		ADD_SECTION_NAME("PROFILE")
		+SVerticalBox::Slot()
		.Padding(5)
		.AutoHeight()
		[
			SNew(SSymbolsDirectorySwitcher, ToolKit.Get())
		]
		ADD_SECTION_SEPARATION
		ADD_SECTION_NAME("SELECTED IMAGE")
		+SVerticalBox::Slot()
		.Padding(20,5)
		.HAlign(HAlign_Left)
		.AutoHeight()
		[
			BuildSelectedImageAndInfoSection()
		]
		ADD_SECTION_SEPARATION
		ADD_SECTION_NAME("CANVAS PREVIEW")
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildCanvasPreviewSection()
		]
		ADD_SECTION_SEPARATION
		ADD_SECTION_NAME("LEARNING")
		+SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			BuildTrainingSeciton()
		]
		ADD_SECTION_SEPARATION
		+SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			ADD_SPECIAL_BUTTON("LOAD TEST LEVEL", 150, 30, &SRPreviewPanel::OnOpenTestLevel, "Level with few simple examples how to implement plugin ingame.")
		]

	];

	InTool->ImagesSelectedEvent.AddSP(this, &SRPreviewPanel::OnImageSelected);
	SymbolImgBrush->SetResourceObject(Cast<UObject>(ToolKit->GetNeuralHandler()->GetTexture()));

	CurrentSymbolData = ToolKit->GetCurrentSymbol();
	CurrentImageItem = ToolKit->GetCurrentImage();

	UpdateImageSelected(true);	
}



void SRPreviewPanel::OnSymbolsListRefreshed()
{

}

FReply SRPreviewPanel::OnTrainNetwork()
{
	if (ToolKit->Validate_AllImagesDrawn() == false)
	{
		SRPopup::ShowTutorial(ToolKit.Get(), 2);
	}
	else
	{
		ToolKit->TrainNetwork();
	}
	
	return FReply::Handled();
}

FReply SRPreviewPanel::OnSaveClick()
{
	ToolKit->SaveImage(CurrentSymbolData.SymbolId, CurrentImageItem.ImgId);
	return FReply::Handled();
}

FReply SRPreviewPanel::OnClearCanvasClick()
{
	ToolKit->GetNeuralHandler()->ClearCanvas();
	return FReply::Handled();
}


FReply SRPreviewPanel::SwitchSelectedImage(ESRSwitchNavButtonType InSwitchType)
{
	switch (InSwitchType)
	{
	case SRPreviewPanel::PrevSymbol:
		ToolKit->CallImageSelected(ToolKit->GetNextSymbol(CurrentSymbolData, -1).SymbolId, CurrentImageItem.ImgId);
		break;
	case SRPreviewPanel::PrevImage:
		ToolKit->CallImageSelected(CurrentSymbolData.SymbolId, ToolKit->GetNextImage(CurrentImageItem, -1).ImgId);
		break;
	case SRPreviewPanel::NextImage:
		ToolKit->CallImageSelected(CurrentSymbolData.SymbolId, ToolKit->GetNextImage(CurrentImageItem, 1).ImgId);
		break;
	case SRPreviewPanel::NextSymbol:
		ToolKit->CallImageSelected(ToolKit->GetNextSymbol(CurrentSymbolData, 1).SymbolId, CurrentImageItem.ImgId);
		break;
	}

	return FReply::Handled();
}

FReply SRPreviewPanel::OnOpenTestLevel()
{
	FString NewPackageName = L"/SymbolRecognizerPlugin/Resources/TestContent/Maps/SRTestLevel";
	FString NewFilename;
	if (FPackageName::TryConvertLongPackageNameToFilename(NewPackageName, NewFilename, FPackageName::GetMapPackageExtension()))
	{
		// Load the requested level.
		FEditorFileUtils::LoadMap(NewFilename);
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SRPreviewPanel::BuildCanvasPreviewSection()
{
	TSharedRef<SHorizontalBox> SaveAndClearBox = SNew(SHorizontalBox).AUTOHIDE_WHEN_TRAINING
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					.Padding(4,4,8,4)
					[
						ADD_SPECIAL_BUTTON("SAVE CANVAS TO IMAGE", 170, 30, &SRPreviewPanel::OnSaveClick, "Save drawing to selected image.")
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					.Padding(8, 4, 4, 4)
					[
						ADD_SPECIAL_BUTTON("CLEAR DRAWING CANVAS", 170, 30, &SRPreviewPanel::OnClearCanvasClick, "Reset current drawing so you have clean canvas again.")
					]
				];


	return SNew(SVerticalBox)
	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.0f)
		[
			SNew(SBox)
			.WidthOverride(200)
			.HeightOverride(200)
			.MaxAspectRatio(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FSRPluginEditorStyle::Get().GetBrush("SymbolRecognizerPlugin.Frame64"))
				[
					SAssignNew(SymbolImg, SImage)
					.Image(SymbolImgBrush.Get())
				]
			]	
		]
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		.Padding(5, 5, 5, 10)
		[
			SaveAndClearBox
		];	
}

TSharedRef<SWidget> SRPreviewPanel::BuildNavigationButton(ESRSwitchNavButtonType SwitchType)
{
	TSharedRef<SBox> WrapBox = SNew(SBox)
		.WidthOverride(32)
		.HeightOverride(32)
		[
			SNew(SButton)
			.ButtonStyle(&FSRPluginEditorStyle::Get().GetWidgetStyle<FButtonStyle>("SymbolRecognizerPlugin.ArrowRight"))
			.OnClicked(this, &SRPreviewPanel::SwitchSelectedImage, SwitchType)
		];

	switch (SwitchType)
	{
	case SRPreviewPanel::PrevSymbol:
	case SRPreviewPanel::PrevImage:
		WrapBox->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		WrapBox->SetRenderTransform(TransformCast<FSlateRenderTransform>(FQuat2D(FMath::DegreesToRadians(180.0f))));
		break;
	}

	return WrapBox;
}

TSharedRef<SWidget> SRPreviewPanel::BuildSelectedImage()
{
	return SNew(SBox)
			.WidthOverride(100)
			.HeightOverride(100)
			.MaxAspectRatio(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FSRPluginEditorStyle::Get().GetBrush("SymbolRecognizerPlugin.Frame64"))
				[
					SAssignNew(CentralImage, SImage)
				]
			];
}

TSharedRef<SWidget> SRPreviewPanel::BuildSelectedImageInfo()
{

		SAssignNew(ActiveItemInfo.DrawnImagesInfoText, STextBlock);
		SAssignNew(ActiveItemInfo.SymbolNameInfoText, STextBlock);
		SAssignNew(ActiveItemInfo.ImageNameInfoText, STextBlock);
		SAssignNew(ActiveItemInfo.ImagePathInfoText, SEditableText)
			.IsReadOnly(true)
			.TextFlowDirection(ETextFlowDirection::RightToLeft);


		auto BuildNavigationSegment = [this](TSharedPtr<STextBlock> InValueBlock, ESRSwitchNavButtonType InLeftBtnSwitchType, ESRSwitchNavButtonType InRightBtnSwitchType)
		{
			
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					BuildNavigationButton(InLeftBtnSwitchType)
				]
			+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(5, 0)
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(60)
					.HAlign(EHorizontalAlignment::HAlign_Center)
					[
						InValueBlock.ToSharedRef()
					]
				]
			+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					BuildNavigationButton(InRightBtnSwitchType)
				];
		};

		
		auto AddInfoEntry = [&](auto InValueWidget, auto InDesc, float InWidth = 110.0f, float InPaddingMultiplier = 1.0f)
		{
			return SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(2.5, 0, 2.5 * InPaddingMultiplier, 0)
				.AutoWidth()
				.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(InWidth)
				[
					SNew(STextBlock).Text(FText::FromString(InDesc))
				]
			]
			+ SHorizontalBox::Slot().Padding(5 * InPaddingMultiplier,0,0,0).VAlign(VAlign_Center).HAlign(HAlign_Left)
			[
				InValueWidget
			];
		};

		
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				AddInfoEntry(BuildNavigationSegment(ActiveItemInfo.SymbolNameInfoText, ESRSwitchNavButtonType::PrevSymbol, ESRSwitchNavButtonType::NextSymbol), "SELECTED SYMBOL")
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				AddInfoEntry(BuildNavigationSegment(ActiveItemInfo.ImageNameInfoText, ESRSwitchNavButtonType::PrevImage, ESRSwitchNavButtonType::NextImage), "SELECTED IMAGE")
			]
			+ SVerticalBox::Slot()
			.Padding(0,5,0,0)
			.AutoHeight()
			[
				AddInfoEntry(ActiveItemInfo.DrawnImagesInfoText.ToSharedRef(), "Drawn Images In Symbol:", 150, 2.5f)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				AddInfoEntry(ActiveItemInfo.ImagePathInfoText.ToSharedRef(), "Source File:", 80, 0.1f)
			];
			
}

TSharedRef<SWidget> SRPreviewPanel::BuildSelectedImageAndInfoSection()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth().Padding(0,0,5,0)
		[	
			BuildSelectedImage()
		]
		+SHorizontalBox::Slot()
		[	
			BuildSelectedImageInfo()	
		];
}

void SRPreviewPanel::OnImageSelected(FSRSymbolDataItem SelectedSymbol, FSRImageDataItem SelectedImage)
{
	CurrentSymbolData = SelectedSymbol;
	CurrentImageItem = SelectedImage;

	UpdateImageSelected(true);
}

void SRPreviewPanel::UpdateImageSelected(bool bTryLoadTexture)
{
	ActiveItemInfo.ImagePathInfoText->SetText(FText::FromString(CurrentImageItem.Path));
	ActiveItemInfo.SymbolNameInfoText->SetText(FText::FromString(CurrentSymbolData.GetSymbolName()));
	ActiveItemInfo.ImageNameInfoText->SetText(FText::FromString(CurrentImageItem.GetImageName(false)));

	const int32 DrawnImages = ToolKit->GetDrawnImagesCount(CurrentSymbolData.SymbolId);
	const int32 AllImagesInSymbol = ToolKit->GetCurrentProfileRef().ImagesPerSymbol;
	const FString DrawnImagesToAll = FString::FromInt(DrawnImages) + "/" + FString::FromInt(AllImagesInSymbol);
	ActiveItemInfo.DrawnImagesInfoText->SetText(FText::FromString(DrawnImagesToAll));

	if (bTryLoadTexture)
	{
		ToolKit->TryLoadBrushForImage(ToolKit->GetCurrentImage(), false);
	}
	
	bool bHasValidBrush = ToolKit->HasImageValidBursh(ToolKit->GetCurrentImage());
	const bool bNeedsDrawing = !ToolKit->GetCurrentImage().bImageFileExists || !bHasValidBrush;
	
	CurrentImgBrush = ToolKit->GetCurrentImage().ImgBrush;//copy and store as uproperty, otherwise crash when changing images count.

	CentralImage->SetImage(bNeedsDrawing ? ToolKit->GetNeedsDrawingBrush() : &CurrentImgBrush);
}


