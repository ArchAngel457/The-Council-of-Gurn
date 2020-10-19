// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SRPopupHandler.h"
#include "Runtime/SlateCore/Public/Widgets/SBoxPanel.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBox.h"
#include "Runtime/Slate/Public/Widgets/Input/SButton.h"
#include "SymbolRecognizerPluginEditor.h"
#include "Runtime/Slate/Public/Framework/Application/SlateApplication.h"
#include "SRToolWidget.h"
#include "SRToolManager.h"
#include "Editor/EditorStyle/Public/EditorStyleSet.h"
#include "Runtime/Slate/Public/Widgets/Layout/SSpacer.h"
#include "Runtime/Slate/Public/Widgets/Layout/SSeparator.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBorder.h"
#include "SRPluginEditorStyle.h"
#include "Runtime/SlateCore/Public/Styling/SlateTypes.h"
#include "Runtime/SlateCore/Public/Styling/CoreStyle.h"
#include "Kismet/KismetMathLibrary.h"
#include "SymbolRecognizer.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Slate/Public/Widgets/Text/SRichTextBlock.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScrollBox.h"


TWeakPtr<class SWindow> SRPopup::SRPopupWidget = nullptr;
ESRPopupType SRPopup::CurrentPopupType = ESRPopupType::PT_None;

//////////////////////////////////////////////////////////////

class SAccuracyTestingPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAccuracyTestingPopup) {}
	SLATE_END_ARGS()

	TArray<TSharedPtr<FSRSymbolDataItem>> Items;
	TSharedPtr< SListView< TSharedPtr<FSRSymbolDataItem> > > ListViewWidget;

	TArray<float> AnswersList;
	TWeakObjectPtr<USRToolManager> BaseTool;


	void Construct(const FArguments& Args, class USRToolManager* InTool)
	{
		BaseTool = InTool;
		SRPopup::CurrentPopupType = ESRPopupType::PT_Accuracy;
		BaseTool->OnProfilesChangedEvent.AddSP(this, &SAccuracyTestingPopup::RefreshList, true);

		SAssignNew(ListViewWidget, SListView<TSharedPtr<FSRSymbolDataItem>>)
			.SelectionMode(ESelectionMode::Single)
			.ClearSelectionOnClick(true)
			.ItemHeight(32)
			.ListItemsSource(&Items)
			.OnGenerateRow(this, &SAccuracyTestingPopup::OnGenerateRowForList);

		ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5)
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Left)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("TEST DRAWING"))
				.OnClicked(this, &SAccuracyTestingPopup::OnTestAnswer)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5)
			[
				SNew(SSeparator)
				.Orientation(Orient_Vertical)
			]
			+SHorizontalBox::Slot()
			[
				ListViewWidget.ToSharedRef()
			]
		];
	}

	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FSRSymbolDataItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
	{

		FString AnswerStr = AnswersList.IsValidIndex(Item->SymbolId) ? FString::SanitizeFloat(AnswersList[Item->SymbolId]) : "--";
		FLinearColor AnswerColor = FLinearColor::Gray;
		if (AnswersList.IsValidIndex(Item->SymbolId))
		{
			float AnswerVal = UKismetMathLibrary::Ease(0.0f, 1.0f, AnswersList[Item->SymbolId], EEasingFunc::EaseInOut, 3.0f);
			AnswerColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Gray, FLinearColor::Green, AnswerVal);
		}

		int32 SymbolId = Item->SymbolId;

		return SNew(STableRow< TSharedPtr<FSRSymbolDataItem> >, OwnerTable)
			.ShowSelection(false)
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(5, 2)
				[
					SNew(SImage).Image_Lambda([this, SymbolId]() -> const FSlateBrush*{ return BaseTool->ChooseBrushFromImage(SymbolId, 0); })
				]
				+ SHorizontalBox::Slot().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString("Symbol ID: " + FString::FromInt(Item->SymbolId) + " | Accuracy: " + AnswerStr)).ColorAndOpacity(FSlateColor(AnswerColor))
				]
			];
	}

	bool ShouldRebuildList() const
	{
		if (Items.Num() == 0)
		{
			return true;
		}
		if (BaseTool->GetCurrentProfileRef().Symbols.Num() != Items.Num())
		{
			return true;
		}

		for (int32 I = 0; I < BaseTool->GetCurrentProfileRef().Symbols.Num(); ++I)
		{
			if (BaseTool->GetCurrentProfileRef().Symbols[I].Images.Num() != Items[I]->Images.Num())
			{
				return true;
			}
		}

		return false;
	}


	void RefreshList(bool bRebuildWidgets = true)
	{
		Items.Empty();
		for (auto& Symbol : BaseTool->GetCurrentProfileRef().Symbols)
		{
			TSharedPtr<FSRSymbolDataItem> Item = MakeShareable(new FSRSymbolDataItem(Symbol));
			Items.Add(Item);
		}

		if (bRebuildWidgets)
		{
			ListViewWidget->RebuildList();
		}
	}

	FReply OnTestAnswer()
	{
		BaseTool->GetSymbolRecognizer()->GetMostAccurateSymbol(0.0f);
		AnswersList = BaseTool->GetSymbolRecognizer()->GetAccuracyList();
		RefreshList(true);
		return FReply::Handled();
	}
};

//////////////////////////////////////////////////////////////
class SRTutorialPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRTutorialPopup)
	: _CurrentTutorialId(0)
	{}
	SLATE_ARGUMENT(int32, CurrentTutorialId)
	SLATE_END_ARGS()

	TSharedPtr<STextBlock> Title;
	TSharedPtr<STextBlock> Status;
	TSharedPtr<SRichTextBlock> Description;
	TSharedPtr<STextBlock> CurrentTutIdText;
	TSharedPtr<SBorder> DescriptionContent;
	int32 CurrentTutorialId = 0;
	int32 TutorialsCount = 4;
	TArray<FString> OutDescriptionsIni;
	TArray<FString> OutTitlesIni;

	bool LoadTutorialFile(TArray<FString>& FileConent)
	{
		FString Result;
		FString TutIniPath = FSymbolRecognizerPluginEditorModule::GetPluginDir() + "Resources/Tutorial.txt";
		if (FFileHelper::LoadFileToString(Result, *TutIniPath))
		{
			Result.ParseIntoArray(FileConent, TEXT("|"));
		}

		return FileConent.Num() > 0;
		
	}

	void Construct(const FArguments& InArgs, USRToolManager* InToolKit)
	{
		TookKit = InToolKit;
		this->CurrentTutorialId = InArgs._CurrentTutorialId;
		
		if (OutDescriptionsIni.Num() == 0)
		{//parse loaded txt into Titles/Descritpions.
			TArray<FString> TutorialFile;
			LoadTutorialFile(TutorialFile);
			for (int32 I = 0; I < TutorialFile.Num() - 1; I+=2)
			{
				OutTitlesIni.Add(TutorialFile[I]);
				OutDescriptionsIni.Add(TutorialFile[I+1]);
			}
		}

		TutorialsCount = OutDescriptionsIni.Num();

		auto TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
		auto DescFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);
		auto StatusFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);

		auto CreateSideButton = [this](FOnClicked InDelegate, bool bLeftArrow = false)
		{
			TSharedRef<SBox> BoxBtn = SNew(SBox)
				.WidthOverride(40)
				.HeightOverride(40)
				[
					SNew(SButton)
					.ButtonStyle(&FSRPluginEditorStyle::Get().GetWidgetStyle<FButtonStyle>("SymbolRecognizerPlugin.ArrowRight"))
					.OnClicked(InDelegate)
				];

			if (bLeftArrow)
			{
				BoxBtn->SetRenderTransformPivot(FVector2D(0.5, 0.5));
				BoxBtn->SetRenderTransform(TransformCast<FSlateRenderTransform>(FQuat2D(FMath::DegreesToRadians(180.0f))));
			}

			return BoxBtn;
		};

		ChildSlot
		[
			SNew(SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			//STATUS
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			//.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Font(StatusFont)
					.Text(FText::FromString("Status: "))
				]
				+SHorizontalBox::Slot()
				[
					SAssignNew(Status, STextBlock)
					.Font(StatusFont)
					.Text(FText::FromString("Update Me!"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0,10)
			[
				SNew(SSpacer)
			]

			//////////TITLE
			+SVerticalBox::Slot()
			.Padding(5)
			.AutoHeight()
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(20,0,0,0)
				.HAlign(HAlign_Left)
				[
					SAssignNew(CurrentTutIdText, STextBlock)
					.Font(TitleFont)
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(-20, 0,0,0)
				.HAlign(HAlign_Center)
				[
					SAssignNew(Title, STextBlock)
					.Font(TitleFont)
					.Text(FText::FromString("Some Cool Title"))
				]
			]
			+SVerticalBox::Slot()
			.Padding(5)
			.AutoHeight()
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
			]
			////////DESCRIPTION
			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(5)
				.AutoWidth()
				[
					CreateSideButton(FOnClicked::CreateSP(this, &SRTutorialPopup::OnPreviousClicked),true)
				]
				+ SHorizontalBox::Slot()
				.Padding(10,5)
				[
					
					SNew(SBox)
					.MinDesiredWidth(550)
					.MinDesiredHeight(300)
					[
						SAssignNew(DescriptionContent, SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("BoxShadow"))
						.Visibility(EVisibility::SelfHitTestInvisible)
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						[
							SNew(SScrollBox)
							.AllowOverscroll(EAllowOverscroll::Yes)
							+SScrollBox::Slot().Padding(20,5)
							[
								SAssignNew(Description, SRichTextBlock)
								.AutoWrapText(true)
								.TextStyle(&FSRPluginEditorStyle::Get(), "RichText.Text")
								.DecoratorStyleSet(&FSRPluginEditorStyle::Get())
								+ SRichTextBlock::ImageDecorator()
							]
						]
					]
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(5)
				.AutoWidth()
				[
					CreateSideButton(FOnClicked::CreateSP(this, &SRTutorialPopup::OnNextClicked), false)
				]
			]
		];
		
		SelectTutorial(CurrentTutorialId);
	}

	FReply OnPreviousClicked()
	{
		CurrentTutorialId = FMath::Max(CurrentTutorialId - 1 , 0);
		
		SelectTutorial(CurrentTutorialId);

		return FReply::Handled();
	}

	FReply OnNextClicked()
	{
		CurrentTutorialId = FMath::Min(CurrentTutorialId + 1, TutorialsCount-1) ;
		
		SelectTutorial(CurrentTutorialId);
		
		return FReply::Handled();
	}

	void SelectTutorial(int32 InTutorialId)
	{
		CurrentTutIdText->SetText(FText::FromString(" #" + FString::FromInt(InTutorialId + 1)));

		Title->SetText(FText::FromString(OutTitlesIni[InTutorialId]));

		UpdateDescriptionContent(OutDescriptionsIni[InTutorialId]);

		switch (InTutorialId)
		{
		case 0:
			DoCustomProfile();
			break;
		case 1:
			DoSetupParams();
			break;		
		case 2:
			DoDrawImages();
			break;	
		case 3:
			DoLearning();
			break;
		case 4:
			DoInGameSetup();
			break;
		}
	}

	void UpdateDescriptionContent(const FString& DescStr)
	{
		Description->SetText(FText::FromString(DescStr));
		/*
			//for more advanced tutotorial.
			DescriptionContent->SetContent(
			Description.ToSharedRef()
			.Text(FText::FromString(DescStr))
		);
		*/
	}

	FSlateColor GetValidateColor(bool InVale) const
	{
		return InVale ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Red);
	}

	void DoCustomProfile()
	{
		SRPopup::CurrentPopupType = ESRPopupType::PT_CustomProfile;

		bool bIsValid = TookKit->Validate_CustomProfileCreated();
		Status->SetText(FText::FromString(bIsValid ? "Profile Created" : "Custom profile must be created."));
		Status->SetColorAndOpacity(GetValidateColor(bIsValid));

	}

	void DoSetupParams()
	{
		SRPopup::CurrentPopupType = ESRPopupType::PT_SetupParams;

		Status->SetText(FString::Printf(TEXT("Symbols: %i | Images Per Symbol: %i"), TookKit->GetCurrentProfileRef().SymbolsAmount, TookKit->GetCurrentProfileRef().ImagesPerSymbol));
		Status->SetColorAndOpacity(FSlateColor::UseForeground());
	}


	void DoDrawImages()
	{
		SRPopup::CurrentPopupType = ESRPopupType::PT_DrawImages;
		bool bIsValid = TookKit->Validate_AllImagesDrawn();
		Status->SetText(FText::FromString(bIsValid ? "All Images Created." : "Draw and Save missing images."));
		Status->SetColorAndOpacity(GetValidateColor(bIsValid));
	}

	void DoLearning()
	{
		SRPopup::CurrentPopupType = ESRPopupType::PT_DoLearning;
		bool bIsValid = TookKit->Validate_NeuralNetworkFileMatchesProfileParams();
		Status->SetText(FText::FromString(bIsValid ? "Profile learned to recognize symbols." : "Needs learning."));
		Status->SetColorAndOpacity(GetValidateColor(bIsValid));
	}

	void DoInGameSetup()
	{
		SRPopup::CurrentPopupType = ESRPopupType::PT_IngameSetup;
		Status->SetText(FText::FromString("--"));
	}

private:
	TWeakObjectPtr<USRToolManager> TookKit;
	
};



//////////////////////////////////////////////////////////////
void SRPopup::ShowPopup(const FText& InInfo, FOnClicked InOnConfilm /*= NULL*/, FOnClicked InOnCancel /*= NULL*/)
{
	TSharedRef<SVerticalBox> PopupVBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(InInfo)
		];


	if (InOnConfilm.IsBound() || InOnCancel.IsBound())
	{
		TSharedRef<SHorizontalBox> ButtonsHBox = SNew(SHorizontalBox);

		auto MakePopupBtn = [=](FOnClicked InDelegate, const FString& InBtnLabel)
		{
			return SNew(SBox)
				.WidthOverride(100)
				.HeightOverride(50)
				[
					SNew(SButton)
					.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Text(FText::FromString(InBtnLabel))
				.OnClicked(InDelegate)
				];
		};

		if (InOnConfilm.IsBound())
		{
			ButtonsHBox->AddSlot().AutoWidth()
				[
					MakePopupBtn(InOnConfilm, "Confirm")
				];
		}
		if (InOnCancel.IsBound())
		{
			ButtonsHBox->AddSlot().AutoWidth()
				[
					MakePopupBtn(InOnCancel, "Cancel")
				];
		}

		PopupVBox->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Center)
			[
				ButtonsHBox
			];
	}

	TSharedRef<SBox> PoupBox = SNew(SBox)
		.WidthOverride(500)
		.HeightOverride(300)
		[
			PopupVBox
		];

	ShowPopup(PoupBox);
}

void SRPopup::ShowPopup(TSharedRef<class SWidget> InContent, FVector2D InSize)
{
	HidePopup();
	TSharedPtr<SWindow> SharedPopupWindow;
	SAssignNew(SharedPopupWindow, SWindow)
		.ClientSize(InSize)
		//.SizingRule(ESizingRule::UserSized)
		.AutoCenter(EAutoCenter::PrimaryWorkArea)
		[
			SNew(SBox)
			.Padding(FMargin(5, 10))
			[
				InContent
			]
		];

	
	SharedPopupWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([](const TSharedRef<SWindow>&) {SRPopup::CurrentPopupType = ESRPopupType::PT_None; }));
	//TSharedPtr<SWindow> TopWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	TSharedPtr<SWindow> TopWindow = FSymbolRecognizerPluginEditorModule::ToolWidget->GetParentWindow();
	if (TopWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(SharedPopupWindow.ToSharedRef(), TopWindow.ToSharedRef(), true);
	}
	else
	{
		FSlateApplication::Get().AddWindow(SharedPopupWindow.ToSharedRef());
	}

	SRPopupWidget = SharedPopupWindow;
}

void SRPopup::HidePopup()
{
	if (SRPopupWidget.IsValid())
	{
		SRPopupWidget.Pin()->RequestDestroyWindow();
		SRPopupWidget.Reset();
		SRPopupWidget = nullptr;
		CurrentPopupType = ESRPopupType::PT_None;
	}
}

void SRPopup::ShowTutorial(class USRToolManager* InToolKit, int32 InTutId)
{
	ShowPopup(SNew(SRTutorialPopup, InToolKit).CurrentTutorialId(InTutId), FVector2D(1000, 500));
}

void SRPopup::ShowAccuracyTest(class USRToolManager* InToolKit)
{
	ShowPopup(SNew(SAccuracyTestingPopup, InToolKit), FVector2D(380, 500));
}

