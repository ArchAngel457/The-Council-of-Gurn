// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SRToolWidget.h"
#include "SRToolKitWindow.h"
#include "SRToolManager.h"
#include "SRPreviewPanel.h"
#include "SRSymbolsCategoryList.h"
#include "SRCanvasHandler.h"
#include "SymbolRecognizer.h"
#include "SRPopupHandler.h"
#include "SRPulseIndicator.h"

#include "Runtime/Slate/Public/Widgets/Layout/SSplitter.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailCustomization.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScaleBox.h"
#include "Editor/PropertyEditor/Public/IDetailPropertyRow.h"
#include "Editor/PropertyEditor/Public/IDetailChildrenBuilder.h"
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h"
#include "Runtime/Slate/Public/Widgets/Layout/SGridPanel.h"
#include "Runtime/SlateCore/Public/Animation/CurveSequence.h"
#include "Editor/PropertyEditor/Public/DetailCategoryBuilder.h"
#include "Runtime/SlateCore/Public/Styling/CoreStyle.h"

TWeakObjectPtr<USRToolManager> ToolKit;
TSharedPtr<class SRPreviewPanel> PreviewPanel;
TSharedPtr<IDetailsView> Details;
TSharedPtr<SVerticalBox> SymbolsPanelVBox;
UPROPERTY()
TArray<UObject*> ExternalDetailsObjects;

class SRNavigationWrapper : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRNavigationWrapper)
	: _Content() {}
	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	bool bHasFocus = false;
	bool bFocusInitialized = false;

	void Construct(const FArguments& Args)
	{
		ChildSlot
		[
			Args._Content.Widget
		];

		//FSlateApplication::Get().SetAllUserFocus(AsShared(), EFocusCause::SetDirectly);
		//FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::SetDirectly);
		//FSlateApplication::Get().SetAllUserFocus(AsShared(), EFocusCause::SetDirectly);
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		//do some magic to Focus on this widget when opening because SLATE is pure magic developed by magicians.
		if (!bFocusInitialized)
		{
			FSlateApplication::Get().SetAllUserFocus(AsShared());
			FSlateApplication::Get().SetKeyboardFocus(AsShared());
			FSlateApplication::Get().SetAllUserFocus(AsShared());
			bFocusInitialized = true;	
		}
	}

	void ProcessNavigation(const FKeyEvent& InKeyEvent)
	{
		if (InKeyEvent.GetKey() == EKeys::R)
		{
			ToolKit->GetNeuralHandler()->ClearCanvas();
		}
		else if (InKeyEvent.GetKey() == EKeys::SpaceBar)
		{
			ToolKit->SaveCurrentImage();
		}
		else if (InKeyEvent.GetKey() == EKeys::W)
		{
			ToolKit->CallImageSelected(ToolKit->GetNextSymbol(ToolKit->GetCurrentSymbol(), -1).SymbolId, ToolKit->GetCurrentImage().ImgId);
		}
		else if (InKeyEvent.GetKey() == EKeys::S)
		{
			ToolKit->CallImageSelected(ToolKit->GetNextSymbol(ToolKit->GetCurrentSymbol(), 1).SymbolId, ToolKit->GetCurrentImage().ImgId);
		}
		else if (InKeyEvent.GetKey() == EKeys::A)
		{
			ToolKit->CallImageSelected(ToolKit->GetCurrentSymbol().SymbolId, ToolKit->GetNextImage(ToolKit->GetCurrentImage(), -1).ImgId);
		}
		else if (InKeyEvent.GetKey() == EKeys::D)
		{
			ToolKit->CallImageSelected(ToolKit->GetCurrentSymbol().SymbolId, ToolKit->GetNextImage(ToolKit->GetCurrentImage(), 1).ImgId);
		}
	}

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		ProcessNavigation(InKeyEvent);

		return SCompoundWidget::OnPreviewKeyDown(MyGeometry, InKeyEvent);
	}
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override
	{
		bHasFocus = true;
		return SCompoundWidget::OnFocusReceived(MyGeometry, InFocusEvent);
	}
	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override
	{
		bHasFocus = false;
		SCompoundWidget::OnFocusLost(InFocusEvent);
	}
	virtual void OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override
	{
		bHasFocus = NewWidgetPath.ContainsWidget(AsShared());
		SCompoundWidget::OnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);
	}
};

void SRToolWidget::Construct(const FArguments& Args, USRToolManager* InSRToolManager)
{
	ToolKit = InSRToolManager;

	SDockTab::Construct(SDockTab::FArguments()
	.TabRole(ETabRole::NomadTab)
	.OnTabClosed(Args._OnTabClosed)
	[
		SAssignNew(NavWrapper, SRNavigationWrapper)
		[

			SNew(SSplitter)
			.Orientation(EOrientation::Orient_Horizontal)
			+ SSplitter::Slot()
			.Value(0.25)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().FillHeight(1.0f)
				[
					SAssignNew(SymbolsPanelVBox, SVerticalBox)
				]
			]
			+SSplitter::Slot()
			.Value(0.5)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SRToolKitWindow, InSRToolManager)
				]
				+ SOverlay::Slot()
				.VAlign(VAlign_Bottom)
				[
					BuildNavigationBar()
				]
				+ SOverlay::Slot()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString("Show Tutorial"))
					.OnClicked_Lambda([this]()
					{
						SRPopup::ShowTutorial(ToolKit.Get());
						return FReply::Handled();
					})
				]
			]
			+SSplitter::Slot()
			.Value(0.25)
			[
				BildDetails(InSRToolManager)
			]
		]
	]
	);
}

TSharedRef<SWidget> SRToolWidget::BuildNavigationBar()
{
	auto MakeNavBox = [](const FString& InPropVal, const FString& InDescValue)
	{
		return SNew(STextBlock).Text(FText::FromString("[" + InPropVal + "]: " + InDescValue));
	};

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("BoxShadow"))
		[
			SNew(SGridPanel)
			.FillColumn(1, 1.0f)
			.FillRow(1, 1.0f)
			+ SGridPanel::Slot(0, 0).Padding(2)
			[
				MakeNavBox("S", "Next Symbol")
			]
			+ SGridPanel::Slot(0, 1).Padding(2)
			[
				MakeNavBox("W", "Previous Symbol")
			]
			+ SGridPanel::Slot(1, 0).Padding(2).Nudge(FVector2D(10, 0))
			[
				MakeNavBox("D", "Next Image")
			]
			+ SGridPanel::Slot(1, 1).Padding(2).Nudge(FVector2D(10, 0))
			[
				MakeNavBox("A", "Previous Image")
			]
			+ SGridPanel::Slot(2, 0).Padding(2)
			[
				MakeNavBox("Space", "Save Drawing to Selected Image")
			]
			+ SGridPanel::Slot(2, 1).Padding(2)
			[
				MakeNavBox("R, Right Mouse Btn", "Clear Canvas")
			]
		];
}


bool SRToolWidget::SupportsKeyboardFocus() const
{
	return true;
}

FReply SRToolWidget::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (NavWrapper.IsValid())
	{
		NavWrapper->ProcessNavigation(InKeyEvent);
	}
	return SDockTab::OnPreviewKeyDown(MyGeometry, InKeyEvent);
}

TSharedRef<SWidget> SRToolWidget::BildDetails(USRToolManager* InSRToolManager)
{
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bAllowSearch = false;
	Args.bShowOptions = false;
	//Args.bShowActorLabel = false;

	SAssignNew(PreviewPanel, SRPreviewPanel, InSRToolManager);
	

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	Details = PropertyModule.CreateDetailView(Args);
	ExternalDetailsObjects.Add(InSRToolManager->GetNeuralHandler());
	
	Details->SetObject(InSRToolManager);

	TSharedRef<SVerticalBox> MyDetailsPanel = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		PreviewPanel.ToSharedRef()
	]
	+ SVerticalBox::Slot()
	.FillHeight(1.0f)
	[
		Details.ToSharedRef()
	];

	return MyDetailsPanel;
}

class FSRToolKitCustomization : public IDetailCustomization
{
public:

	IDetailLayoutBuilder* BuilderLayout = nullptr;

	template<ESRRefreshDataReason Reason>
	void RefreshDataAndWidgets()
	{
		ToolKit->RefreshSymbolDirectories(Reason);
		BuilderLayout->ForceRefreshDetails();
		PreviewPanel->OnSymbolsListRefreshed();
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{	
		TArray< TWeakObjectPtr<UObject> > OutObjects;
		DetailBuilder.GetObjectsBeingCustomized(OutObjects);
		check(OutObjects.Num());
		USRToolManager* CustomizableObject = Cast<USRToolManager>(OutObjects[0].Get());

		BuilderLayout = &DetailBuilder;
		
		check(CustomizableObject != nullptr);
		
		IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory("SETTINGS", FText::GetEmpty(), ECategoryPriority::Default);
		SettingsCategory.InitiallyCollapsed(false);
		IDetailCategoryBuilder& SaveCategory = DetailBuilder.EditCategory("Save", FText::GetEmpty(), ECategoryPriority::Default);

		SettingsCategory.HeaderContent(
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SRPulseIndicator).ShouldPulse(TAttribute<bool>(this, &FSRToolKitCustomization::IsDoParamsTutorial))
			]
		);

		TSharedPtr<IPropertyHandle> CurrentProfileDataIDProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USRToolManager, CurrentProfileDataID));
		TSharedPtr<IPropertyHandle> ProfilesProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USRToolManager, Profiles));
		DetailBuilder.HideProperty(ProfilesProperty);
		DetailBuilder.HideProperty(CurrentProfileDataIDProperty);
		int32 CurrentProfileDataID = 0;
		CurrentProfileDataIDProperty->GetValue(CurrentProfileDataID);

		TSharedPtr<IPropertyHandleArray> ProfilesArray = ProfilesProperty->AsArray();
		TSharedPtr<IPropertyHandle> CurrentProfilesProperty = ProfilesArray->GetElement(CurrentProfileDataID);
		
		TSharedPtr<IPropertyHandle> SymbolsProperty = CurrentProfilesProperty->GetChildHandle("Symbols");
		TSharedPtr<IPropertyHandleArray> SymbolsArray = SymbolsProperty->AsArray();


		const TSharedPtr<IPropertyHandle> SymbolsAmountProperty = CurrentProfilesProperty->GetChildHandle("SymbolsAmount");
		const TSharedPtr<IPropertyHandle> ImagesPerSymbolProperty = CurrentProfilesProperty->GetChildHandle("ImagesPerSymbol");
		const TSharedPtr<IPropertyHandle> LearningCyclesProperty = CurrentProfilesProperty->GetChildHandle("LearningCycles");
		const TSharedPtr<IPropertyHandle> HiddenNodesProperty = CurrentProfilesProperty->GetChildHandle("HiddenNodes");
		const TSharedPtr<IPropertyHandle> LearningRateProperty = CurrentProfilesProperty->GetChildHandle("LearningRate");
		const TSharedPtr<IPropertyHandle> AcceptableTrainingAccuracyProperty = CurrentProfilesProperty->GetChildHandle("AcceptableTrainingAccuracy");
		const TSharedPtr<IPropertyHandle> DeltaTwoBestOutcomesProperty = CurrentProfilesProperty->GetChildHandle("DeltaTwoBestOutcomes");
		const TSharedPtr<IPropertyHandle> AutoLearningProperty = CurrentProfilesProperty->GetChildHandle("bAutoTraining");

		SettingsCategory.AddProperty(SymbolsAmountProperty);
		SettingsCategory.AddProperty(ImagesPerSymbolProperty);
		SettingsCategory.AddProperty(LearningRateProperty);
		SettingsCategory.AddProperty(HiddenNodesProperty);
		SettingsCategory.AddProperty(AutoLearningProperty);
		SettingsCategory.AddProperty(LearningCyclesProperty).ShowPropertyButtons(true);
		SettingsCategory.AddProperty(AcceptableTrainingAccuracyProperty).IsEnabled(TAttribute<bool>(this, &FSRToolKitCustomization::IsAutoTraining));
		SettingsCategory.AddProperty(DeltaTwoBestOutcomesProperty).IsEnabled(TAttribute<bool>(this, &FSRToolKitCustomization::IsAutoTraining));

		SymbolsAmountProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSRToolKitCustomization::RefreshDataAndWidgets<ESRRefreshDataReason::RDR_SymbolsCountChanged>));
		ImagesPerSymbolProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSRToolKitCustomization::RefreshDataAndWidgets<ESRRefreshDataReason::RDR_ImageCountChanged>));
		CustomizableObject->OnProfilesChangedEvent.AddSP(this, &FSRToolKitCustomization::RefreshDataAndWidgets<ESRRefreshDataReason::RDR_ProfilesChanged>);
		
		IDetailCategoryBuilder& DrawingSettings = DetailBuilder.EditCategory("DrawingSettings");
		DrawingSettings.AddExternalObjectProperty(ExternalDetailsObjects, GET_MEMBER_NAME_CHECKED(USRCanvasHandler, DrawingLayers));

		SymbolsPanelVBox->ClearChildren();
		SymbolsPanelVBox->AddSlot().FillHeight(0.5f)
		[
			SNew(SRSymbolsCategoryList, ToolKit.Get(), SymbolsArray)
		];

	}

	bool IsDoParamsTutorial() const
	{
		return SRPopup::CurrentPopupType == ESRPopupType::PT_SetupParams;
	}

	bool IsAutoTraining() const
	{
		if (ToolKit.IsValid())
		{
			return ToolKit->GetCurrentProfileRef().bAutoTraining;
		}

		return true;
	}
	
};

TSharedRef<IDetailCustomization> SRToolWidget::MakeInstance()
{
	TSharedRef<IDetailCustomization> DetailsRef = MakeShared<FSRToolKitCustomization>();
	return DetailsRef;
}

void SRToolWidget::DoCleanup()
{
	ExternalDetailsObjects.Empty();
	ToolKit.Reset();
	PreviewPanel.Reset();
	Details.Reset();
	SymbolsPanelVBox.Reset();
	NavWrapper.Reset();
}

SRToolWidget::SRToolWidget() :
	SDockTab::SDockTab()
{}
