// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SRToolKitWindow.h"
#include "Runtime/SlateCore/Public/Styling/SlateBrush.h"
#include "Engine/Engine.h"
#include "Runtime/SlateCore/Public/Widgets/Images/SImage.h"
#include "Runtime/Slate/Public/Widgets/Images/SThrobber.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Engine/Texture2D.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBorder.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScaleBox.h"
#include "Runtime/Slate/Public/Widgets/Notifications/SProgressBar.h"
#include "SymbolRecognizer.h"
#include "SRToolManager.h"
#include "SRCanvasHandler.h"
#include "SRPluginEditorStyle.h"
#include "SRToolWidget.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBox.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScrollBox.h"



class SRTrainingInfoBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRTrainingInfoBox) {}
	SLATE_END_ARGS()

	TWeakObjectPtr<USRToolManager> ToolKit;
	float AnimatedProgress = 0.0f;
	int32 EpochsValue = 0;
	float AccuracyValue = 0.0f;
	TSharedPtr<STextBlock> EpochsText;
	TArray<TSharedPtr<STextBlock>> AccuracyTexts;
	TSharedPtr<SScrollBox> ScoresScrollBox;

	EVisibility EstimateVisibility() const
	{
		if (ToolKit.IsValid())
		{
			if (ToolKit->GetIsTraningNetwork())
			{
				return EVisibility::Visible;
			}
		}

		return EVisibility::Collapsed;
	}

	void Construct(const FArguments& InArgs, class USRToolManager* InToolKit)
	{
		ToolKit = InToolKit;
		ToolKit->OnStartedTraining.AddLambda([this]() {
			AnimatedProgress = 0.0f;
			AccuracyTexts.Empty();
			AccuracyValue = -1.0f;

			if (ScoresScrollBox.IsValid())
			{
				ScoresScrollBox->ClearChildren();
			}
			int32 SymbolsCount = ToolKit->GetCurrentProfileRef().SymbolsAmount;
			for (int32 I = 0; I < SymbolsCount + 1; ++I)
			{
				TSharedPtr<STextBlock> ScoreTextBlock;
				ScoresScrollBox->AddSlot().Padding(2)
				[
					SAssignNew(ScoreTextBlock, STextBlock)
				];

				AccuracyTexts.Add(ScoreTextBlock);
			}
		});
		Visibility.Bind(this, &SRTrainingInfoBox::EstimateVisibility);

		
		
		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5)
			[
				SNew(SBorder)
				.Padding(5)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.Padding(5)
					.AutoHeight()
					[
						SAssignNew(EpochsText, STextBlock)
					]
					+ SVerticalBox::Slot()
					.MaxHeight(250.0f)
					[
						SAssignNew(ScoresScrollBox, SScrollBox)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5)
			[
				SNew(SBox)
				.WidthOverride(700)
				.HeightOverride(50)
				[
					SNew(SProgressBar)
					.BorderPadding(FVector2D::ZeroVector)
					.Percent(this, &SRTrainingInfoBox::GetTrainingProgress)
					.BackgroundImage(FEditorStyle::GetBrush("ProgressBar.ThinBackground"))
					.FillImage(FEditorStyle::GetBrush("ProgressBar.ThinFill"))
				]		
			]

			+ SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SThrobber)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.HeightOverride(40)
				.WidthOverride(200)
				[
					SNew(SBorder)
					.BorderImage(FSRPluginEditorStyle::Get().GetBrush("FrameBG"))
					[
						SNew(SButton)
						.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
						.TextStyle(FEditorStyle::Get(), "NormalText.Important")
						.VAlign(EVerticalAlignment::VAlign_Center)
						.HAlign(EHorizontalAlignment::HAlign_Center)
						.ToolTip(SNew(SToolTip).Text(FText::FromString("For some reason you decided its enough of training,\n so break here and save the result.")))
						.Text(FText::FromString("STOP AND SAVE"))
						.OnClicked(this, &SRTrainingInfoBox::OnCancelTraining, true)
					]
				]
				
			]
			+ SVerticalBox::Slot()
			.Padding(10)
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.HeightOverride(40)
				.WidthOverride(200)
				[
					SNew(SBorder)
					.BorderImage(FSRPluginEditorStyle::Get().GetBrush("FrameBG"))
					[
						SNew(SButton)
						.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
						.TextStyle(FEditorStyle::Get(), "NormalText.Important")
						.VAlign(EVerticalAlignment::VAlign_Center)
						.HAlign(EHorizontalAlignment::HAlign_Center)
						.Text(FText::FromString("CANCEL"))
						.OnClicked(this, &SRTrainingInfoBox::OnCancelTraining, false)
					]
				]
			]
			
		];
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		if (GetVisibility() == EVisibility::Visible)
		{
			if (ToolKit.IsValid())
			{
				const float Progress = ToolKit->GetSymbolTrainingProgress();
				if (AnimatedProgress < Progress)
				{
					AnimatedProgress = FMath::Lerp(AnimatedProgress, Progress, 20.0f * InDeltaTime);
				}
				else if (AnimatedProgress < 0.9f)
				{
					
					const float SpeedRadio = FMath::Pow(1.0f - FMath::Abs(Progress - AnimatedProgress), 5);
					AnimatedProgress = FMath::Min(AnimatedProgress + InDeltaTime * SpeedRadio * 0.01f, 1.0f);
				}

				if (EpochsValue != ToolKit->GetSymbolTrainingEpochs())
				{
					EpochsValue = ToolKit->GetSymbolTrainingEpochs();
					EpochsText->SetText(FText::FromString("Epochs: " + FString::FromInt(EpochsValue)));
				}
				
				
				float GlobalTrainingScore;
				TArray<float> ScoresPerSymbol;
				ToolKit->GetSymbolTrainingScores(ScoresPerSymbol, GlobalTrainingScore);

				if (FMath::IsNearlyEqual(AccuracyValue, GlobalTrainingScore, 0.0001f) == false)
				{
					AccuracyValue = GlobalTrainingScore;

					if (AccuracyTexts.Num() > 0)
					{
						AccuracyTexts[0]->SetText(FText::FromString(FString::Printf(TEXT("Global Score: %f"), GlobalTrainingScore)));
						for (int32 I = 0; I < ScoresPerSymbol.Num(); ++I)
						{
							AccuracyTexts[I + 1]->SetText(FText::FromString(FString::Printf(TEXT("Symbol ID: %i | Accuracy: %f"), I, ScoresPerSymbol[I])));
						}
					}

				}

				
			}
		}

		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}

	FReply OnCancelTraining(bool bShouldSave)
	{
		ToolKit->CancelTrainingTask(bShouldSave);

		return FReply::Handled();
	}

	TOptional<float> GetTrainingProgress() const
	{
		return TOptional<float>(AnimatedProgress);
	}
		
};



class SRDrawingCanvas : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SRDrawingCanvas) {}
	SLATE_END_ARGS()

	TWeakObjectPtr<USRToolManager> ToolKit;
	bool bIsDrawing = false;
	void Construct(const FArguments& InArgs, class USRToolManager* InToolKit)
	{
		ToolKit = InToolKit;

		/*ChildSlot
		[
			SNew(SImage).ColorAndOpacity(FLinearColor::Black)
		];*/
	}

	
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			ToolKit->GetNeuralHandler()->Draw(true, (MouseEvent.GetScreenSpacePosition() - MyGeometry.GetAbsolutePosition()));
			bIsDrawing = true;
		}

		return FReply::Handled();
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		bool bIsNewDrawing = !bIsDrawing && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);

		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			FVector2D RelPos = (MouseEvent.GetScreenSpacePosition() - MyGeometry.GetAbsolutePosition());
			ToolKit->GetNeuralHandler()->Draw(bIsNewDrawing, RelPos);
			bIsDrawing = true;

		}

		return FReply::Handled();

	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == (EKeys::RightMouseButton))
		{
			ToolKit->GetNeuralHandler()->ClearCanvas();
		}

		return FReply::Handled();
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		int32 NewLayerId = SBorder::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		float DrawBrushSize = ToolKit->GetNeuralHandler()->GetScaledBrushSize();
		
		if (const FSlateBrush* SyleBrush = FSRPluginEditorStyle::Get().GetBrush("SoftBrush"))
		{
			ToolKit->GetNeuralHandler()->MakeDrawCallsForPoints(FDrawPointDelegate::CreateLambda([&](const FVector2D& OutDrawPoint)
			{
				FPaintGeometry DrawPG(FSlateLayoutTransform(),FSlateRenderTransform(AllottedGeometry.GetAbsolutePosition() + OutDrawPoint), FVector2D(DrawBrushSize, DrawBrushSize), false);
				FSlateDrawElement::MakeBox(OutDrawElements, NewLayerId, DrawPG, SyleBrush, ESlateDrawEffect::PreMultipliedAlpha, FLinearColor(1, 1, 1, 1));
			}));
		}

		return LayerId;
	}
};


void SRToolKitWindow::OnImageSelected(FSRSymbolDataItem SelectedSymbol, FSRImageDataItem SelectedImage)
{
	if (CurrentSymbol.SymbolId != SelectedSymbol.SymbolId)
	{
		CurrentSymbol = SelectedSymbol;
		UpdateBackground(CurrentSymbol.BackgroundHelper.LoadSynchronous());
	}

}

void SRToolKitWindow::Construct(const FArguments& InArgs, class USRToolManager* InBaseTool)
{
	BaseTool = InBaseTool;
	BaseTool->ImagesSelectedEvent.AddSP(this, &SRToolKitWindow::OnImageSelected);


	ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFit)
				[
					SNew(SImage)
					.Image(&BackgroundBrush)
				]
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SBox)
				.WidthOverride(512)
				.HeightOverride(512)
				[	
					SNew(SRDrawingCanvas, InBaseTool)
					.Clipping(EWidgetClipping::ClipToBoundsAlways)		
				]
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				//SNew(SBorder)
				//.Visibility_Lambda([this]() -> EVisibility{return (BaseTool.IsValid() && BaseTool->bIsTraining) ? EVisibility::Visible : EVisibility::Collapsed;})
				//[
					SNew(SRTrainingInfoBox, BaseTool.Get())
				//]
			]
		];
	CurrentSymbol = BaseTool->GetCurrentSymbol();
	UpdateBackground(CurrentSymbol.BackgroundHelper.LoadSynchronous());
	BaseTool->OnBackgroundImageChangedEvent.AddSP(this, &SRToolKitWindow::OnBackgroundImageChanged);

}

void SRToolKitWindow::UpdateBackground(UTexture2D* InBackgroundTex)
{
	BackgroundTexture = InBackgroundTex;
	BackgroundBrush.SetResourceObject(BackgroundTexture);
	BackgroundBrush.TintColor = FSlateColor(BackgroundTexture ? FLinearColor::White : FLinearColor::Black);
	if (BackgroundTexture)
	{
		BackgroundTexture->SetForceMipLevelsToBeResident(0, 0);
		BackgroundTexture->WaitForStreaming();
	}
	
}

void SRToolKitWindow::OnBackgroundImageChanged(int32 InSymbolId)
{
	if (InSymbolId == BaseTool->GetCurrentSymbol().SymbolId)
	{
		UpdateBackground(BaseTool->GetCurrentSymbol().BackgroundHelper.LoadSynchronous());
	}
}

