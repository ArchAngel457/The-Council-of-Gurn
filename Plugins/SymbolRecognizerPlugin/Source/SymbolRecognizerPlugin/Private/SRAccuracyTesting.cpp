// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SRAccuracyTesting.h"
#include "Runtime/SlateCore/Public/Widgets/SCompoundWidget.h"
#include "SymbolRecognizer.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScrollBox.h"
#include "Runtime/Slate/Public/Widgets/Input/SButton.h"
#include "Runtime/Slate/Public/Widgets/Layout/SSeparator.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBox.h"
#include "Runtime/SlateCore/Public/Widgets/Images/SImage.h"
#include "Engine/GameViewportClient.h"
#include "Runtime/SlateCore/Public/Widgets/SBoxPanel.h"
#include "SRCanvasHandler.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Runtime/Slate/Public/Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/SlateCore/Public/Styling/CoreStyle.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBorder.h"
#include "Engine/Engine.h"
#include "Runtime/CoreUObject/Public/UObject/UObjectIterator.h"
#include "Runtime/Slate/Public/Widgets/Input/SComboBox.h"


class SYMBOLRECOGNIZERPLUGIN_API SRCanvasPreview : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRCanvasPreview) {}
	SLATE_END_ARGS()

	TWeakObjectPtr<USymbolRecognizer> SymbolRecognizer;
	TSharedPtr<FSlateBrush> SymbolImgBrush;

	void Construct(const FArguments& Args, USymbolRecognizer* InSymbolRecognizer)
	{
		SymbolRecognizer = InSymbolRecognizer;
		SymbolImgBrush = MakeShareable(new FSlateBrush());

		ChildSlot
		.Padding(20)
		.VAlign(EVerticalAlignment::VAlign_Top)
		.HAlign(EHorizontalAlignment::HAlign_Right)
		[
			SNew(SBox)
			.WidthOverride(200)
			.HeightOverride(200)
			[
				SNew(SBox)
				.WidthOverride(200)
				.HeightOverride(200)
				[
					SNew(SImage)
					.Image(SymbolImgBrush.Get())
				]
			]
		];

		SymbolImgBrush->SetResourceObject(Cast<UObject>(SymbolRecognizer->GetCanvasHandler()->GetTexture()));
	}
};

class SYMBOLRECOGNIZERPLUGIN_API SRAccuracyTestingStandalone : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRAccuracyTestingStandalone) {}
	SLATE_END_ARGS()

	
	TSharedPtr<SScrollBox> AccuracyScrollBox;
	TWeakObjectPtr<USymbolRecognizer> SymbolRecognizer;
	TSharedPtr<FSlateBrush> SymbolImgBrush;
	TSharedPtr<FSlateBrush> DrawBrush;
	TWeakObjectPtr<APlayerController> PC;
	bool bIsCanvasVisible = true;

	//Profiles Combobox
	typedef TSharedPtr<FString> FComboItemType;
	struct ProfilesCombo
	{
		FComboItemType CurrentItem;
		TArray<FComboItemType> Options;
		TSharedPtr<SComboBox<FComboItemType>> ProfilesComboList;

	} Profiles;
	

	void Construct(const FArguments& Args, USymbolRecognizer* InSymbolRecognizer)
	{
		DrawBrush = MakeShareable(new FSlateBrush());
		DrawBrush->TintColor = FSlateColor(FLinearColor::White);
		DrawBrush->DrawAs = ESlateBrushDrawType::Image;
		DrawBrush->ImageSize = FVector2D(32, 32);

		SymbolRecognizer = InSymbolRecognizer;
		SymbolImgBrush = MakeShareable(new FSlateBrush());

		auto MainFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);

		SAssignNew(AccuracyScrollBox, SScrollBox);

		ChildSlot
			.Padding(15)
			.VAlign(EVerticalAlignment::VAlign_Fill)
			.HAlign(EHorizontalAlignment::HAlign_Fill)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Left)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("ExpandableArea.Border")))
					.Padding(10)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(5)
						.VAlign(VAlign_Top)
						.HAlign(HAlign_Left)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2)
							[
								BuildProfilesCombo()
							]
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.WidthOverride(200)
								.HeightOverride(200)
								[
									SNew(SImage)
									.Image(SymbolImgBrush.Get())
								]
							]
							+ SVerticalBox::Slot()
							.Padding(5)
							.AutoHeight()
							[
								SNew(SButton)
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.Text(FText::FromString("TEST DRAWING"))
								.OnClicked(this, &SRAccuracyTestingStandalone::OnTestAnswer)
							]
							+ SVerticalBox::Slot()
							.Padding(5)
							.AutoHeight()
							[
								SNew(SButton)
								.HAlign(EHorizontalAlignment::HAlign_Center)
								.Text(FText::FromString("CLEAR CANVAS"))
								.OnClicked_Lambda([this]()->FReply {SymbolRecognizer->ClearCanvas(); return FReply::Handled(); })
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(5)
							[
								SNew(SBox)
								.WidthOverride(200)
								.HeightOverride(40)
								[
									SNew(STextBlock)
									.Font(MainFont)
									.Text(FText::FromString("[T] TEST DRAWING"))
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(5)
							[
								SNew(SBox)
								.WidthOverride(200)
								.HeightOverride(40)
								[
									SNew(STextBlock)
									.Font(MainFont)
									.Text(FText::FromString("[C] SHOW CANVAS"))
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(5)
							[
								SNew(SBox)
								.WidthOverride(200)
								.HeightOverride(40)
								[
									SNew(STextBlock)
									.Font(MainFont)
									.Text(FText::FromString("[R] CLEAR DRAWING"))
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(5)
							[
								SNew(SBox)
								.WidthOverride(200)
								.HeightOverride(40)
								[
									SNew(STextBlock)
									.Font(MainFont)
									.Text(FText::FromString("[Q] CLOSE"))
								]
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(5)
						[
							SNew(SSeparator)
							.Orientation(Orient_Vertical)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.Padding(5)
							.MaxHeight(500)
							[
								AccuracyScrollBox.ToSharedRef()
							]
						]
					]
				]
			];

		SymbolImgBrush->SetResourceObject(Cast<UObject>(SymbolRecognizer->GetCanvasHandler()->GetTexture()));
	}

	TSharedRef<SWidget> BuildProfilesCombo()
	{
		TArray<FString> ProfilesList = SymbolRecognizer->GetProfilesNames();
		for (FString& ProfilesItem : ProfilesList)
		{
			Profiles.Options.Add(MakeShareable(new FString(ProfilesItem)));
		}

		Profiles.CurrentItem = MakeShareable(new FString(SymbolRecognizer->GetCurrentProfile()));

		return SAssignNew(Profiles.ProfilesComboList, SComboBox<FComboItemType>)
			.OptionsSource(&Profiles.Options)
			.OnSelectionChanged_Lambda([this](FComboItemType NewValue, ESelectInfo::Type InType)
						{
							if (NewValue.IsValid())
							{
								SymbolRecognizer->ChangeProfile(*NewValue);
								Profiles.CurrentItem = NewValue;
							}
							
						})
			.OnGenerateWidget_Lambda([this](FComboItemType InOption)->TSharedRef<SWidget>
						{
							return SNew(SBox)
								.HeightOverride(20)
								[
									SNew(STextBlock)
									.Text(FText::FromString(*InOption))
									.Justification(ETextJustify::Center)

								];
						})
			.InitiallySelectedItem(Profiles.CurrentItem)
			[
				SNew(SBox)
				.HeightOverride(20)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()->FText {return FText::FromString(*Profiles.CurrentItem); })
				]
			];
				
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		int32 NewLayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		if (bIsCanvasVisible)
		{
			if (SymbolRecognizer.IsValid())
			{
				float DrawBrushSize = SymbolRecognizer->GetCanvasHandler()->GetScaledBrushSize();

				if (DrawBrush.IsValid())
				{
					SymbolRecognizer->GetCanvasHandler()->MakeDrawCallsForPoints(FDrawPointDelegate::CreateLambda([&](const FVector2D& OutDrawPoint)
					{
						FPaintGeometry DrawPG(FSlateLayoutTransform(), FSlateRenderTransform(AllottedGeometry.GetAbsolutePosition() + OutDrawPoint), FVector2D(DrawBrushSize, DrawBrushSize), false);
						FSlateDrawElement::MakeBox(OutDrawElements, NewLayerId, DrawPG, DrawBrush.Get(), ESlateDrawEffect::PreMultipliedAlpha, FLinearColor(1, 1, 1, 1));
					}));
				}
			}
		}
		

		return LayerId;
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		
		if (!PC.IsValid())
		{
			for (TObjectIterator<APlayerController> Itr; Itr; ++Itr)
			{
				PC = *Itr;
				break;
			}

			if (PC.IsValid())
			{
				PC->bShowMouseCursor = true;
				PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
			}
		}
		else
		{
			if (PC->WasInputKeyJustPressed(EKeys::T))
			{
				OnTestAnswer();
			}
			else if (PC->WasInputKeyJustPressed(EKeys::R))
			{
				SymbolRecognizer->ClearCanvas();
			}
			else if (PC->WasInputKeyJustPressed(EKeys::Q))
			{
				FSRAccuracyTesting::HideAccuracyTesting();
			}
			else if (PC->WasInputKeyJustPressed(EKeys::C))
			{
				bIsCanvasVisible = !bIsCanvasVisible;
			}
		}
	}

	TSharedRef<SWidget> GenerateAccuracyField(int32 InId, float InAccuracy)
	{
		auto MainFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);
		FString AnswerStr = "Symbol ID: " + FString::FromInt(InId) + " |   " + FString::SanitizeFloat(InAccuracy);
		FLinearColor AnswerColor = FLinearColor::Gray;
		float AnswerEaseVal = UKismetMathLibrary::Ease(0.0f, 1.0f, InAccuracy, EEasingFunc::EaseInOut, 3.0f);
		AnswerColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Gray, FLinearColor::Green, AnswerEaseVal);

		return
			SNew(STextBlock)
			.Font(MainFont)
			.ColorAndOpacity(FSlateColor(AnswerColor))
			.Text(FText::FromString(AnswerStr));
		
	}

	void RefreshList(const TArray<float>& AnswersList)
	{
		AccuracyScrollBox->ClearChildren();

		for (int32 SymbolIdx = 0; SymbolIdx < AnswersList.Num(); ++SymbolIdx)
		{
			AccuracyScrollBox->AddSlot()
			.Padding(5)
			[
				GenerateAccuracyField(SymbolIdx, AnswersList[SymbolIdx])
			];
		}
	}

	FReply OnTestAnswer()
	{
		TArray<float> AnswersList = SymbolRecognizer->GetAccuracyList();
		RefreshList(AnswersList);
		return FReply::Handled();
	}
};


TSharedPtr<SRAccuracyTestingStandalone> FSRAccuracyTesting::AccuracyWidget;
TSharedPtr<SRCanvasPreview> FSRAccuracyTesting::PreviewWidget;

void FSRAccuracyTesting::ShowAccuracyTesting(USymbolRecognizer* InSymbolRecognizer)
{
	if (!AccuracyWidget.IsValid())
	{
		GEngine->GameViewport->AddViewportWidgetContent(SAssignNew(AccuracyWidget, SRAccuracyTestingStandalone, InSymbolRecognizer), 1000);

	}
	else
	{
		HideAccuracyTesting();
		ShowAccuracyTesting(InSymbolRecognizer);
	}
}

void FSRAccuracyTesting::HideAccuracyTesting()
{
	if (AccuracyWidget.IsValid())
	{
		if (AccuracyWidget->PC.IsValid())
		{
			AccuracyWidget->PC->bShowMouseCursor = false;
			AccuracyWidget->PC->SetInputMode(FInputModeGameOnly());
			AccuracyWidget->PC.Reset();
		}
		GEngine->GameViewport->RemoveViewportWidgetContent(AccuracyWidget.ToSharedRef());
		AccuracyWidget.Reset();
	}
}

void FSRAccuracyTesting::ShowPreviewWidget(USymbolRecognizer* InSymbolRecognizer)
{
	if (!PreviewWidget.IsValid())
	{
		GEngine->GameViewport->AddViewportWidgetContent(SAssignNew(PreviewWidget, SRCanvasPreview, InSymbolRecognizer), 1000);

	}
	else
	{
		HidePreviewWidget();
		ShowPreviewWidget(InSymbolRecognizer);
	}
}

void FSRAccuracyTesting::HidePreviewWidget()
{
	if (PreviewWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PreviewWidget.ToSharedRef());
		PreviewWidget.Reset();
	}
}
