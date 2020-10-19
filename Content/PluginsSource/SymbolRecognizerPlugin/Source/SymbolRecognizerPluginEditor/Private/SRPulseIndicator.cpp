// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SRPulseIndicator.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBox.h"

void SRPulseIndicator::Construct(const FArguments& InArgs)
{
	ShouldPulse = InArgs._ShouldPulse;
	Visibility = EVisibility::HitTestInvisible;
	PulseAnimation.AddCurve(0.0f, PulseAnimationLength, ECurveEaseFunction::Linear);
	PulseAnimation.Play(this->AsShared(), true);
	RegisterActiveTimer(1.f, FWidgetActiveTimerDelegate::CreateSP(this, &SRPulseIndicator::OnActiveTimer));

	ChildSlot
		[
			SNew(SBox)
			.MaxAspectRatio(1.0f)
			.WidthOverride(16)
			.HeightOverride(16)
		];

	StopPulse();
}

void SRPulseIndicator::StartPulse()
{
	PulseAnimation.Resume();
}

void SRPulseIndicator::StopPulse()
{
	PulseAnimation.Pause();
}

void SRPulseIndicator::GetAnimationValues(float InAnimationProgress, float& OutAlphaFactor0, float& OutPulseFactor0, float& OutAlphaFactor1, float& OutPulseFactor1) const
{
	InAnimationProgress = FMath::Fmod(InAnimationProgress * 2.0f, 1.0f);

	OutAlphaFactor0 = FMath::Square(1.0f - InAnimationProgress);
	OutPulseFactor0 = 1.0f - FMath::Square(1.0f - InAnimationProgress);

	float OffsetProgress = FMath::Fmod(InAnimationProgress + 0.25f, 1.0f);
	OutAlphaFactor1 = FMath::Square(1.0f - OffsetProgress);
	OutPulseFactor1 = 1.0f - FMath::Square(1.0f - OffsetProgress);

}

int32 SRPulseIndicator::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1000;

	if (PulseAnimation.IsPlaying())
	{
		float AlphaFactor0 = 0.0f;
		float AlphaFactor1 = 0.0f;
		float PulseFactor0 = 0.0f;
		float PulseFactor1 = 0.0f;

		GetAnimationValues(PulseAnimation.GetLerp(), AlphaFactor0, PulseFactor0, AlphaFactor1, PulseFactor1);

		const FSlateBrush* PulseBrush = FEditorStyle::Get().GetBrush(TEXT("TutorialLaunch.Circle"));
		const FLinearColor PulseColor = FLinearColor(1, 0.551031, 0, 1);

		{
			FVector2D PulseOffset = FVector2D(PulseFactor0 * MaxPulseOffset, PulseFactor0 * MaxPulseOffset);

			FVector2D BorderPosition = (AllottedGeometry.AbsolutePosition - ((FVector2D(PulseBrush->Margin.Left, PulseBrush->Margin.Top) * PulseBrush->ImageSize * AllottedGeometry.Scale) + PulseOffset));
			FVector2D BorderSize = ((AllottedGeometry.GetLocalSize() * AllottedGeometry.Scale) + (PulseOffset * 2.0f) + (FVector2D(PulseBrush->Margin.Right * 2.0f, PulseBrush->Margin.Bottom * 2.0f) * PulseBrush->ImageSize * AllottedGeometry.Scale));

			FPaintGeometry BorderGeometry(BorderPosition, BorderSize, AllottedGeometry.Scale);

			// draw highlight border
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, BorderGeometry, PulseBrush, ESlateDrawEffect::None, FLinearColor(PulseColor.R, PulseColor.G, PulseColor.B, AlphaFactor0));
		}

		{
			FVector2D PulseOffset = FVector2D(PulseFactor1 * MaxPulseOffset, PulseFactor1 * MaxPulseOffset);

			FVector2D BorderPosition = (AllottedGeometry.AbsolutePosition - ((FVector2D(PulseBrush->Margin.Left, PulseBrush->Margin.Top) * PulseBrush->ImageSize * AllottedGeometry.Scale) + PulseOffset));
			FVector2D BorderSize = ((AllottedGeometry.Size * AllottedGeometry.Scale) + (PulseOffset * 2.0f) + (FVector2D(PulseBrush->Margin.Right * 2.0f, PulseBrush->Margin.Bottom * 2.0f) * PulseBrush->ImageSize * AllottedGeometry.Scale));

			FPaintGeometry BorderGeometry(BorderPosition, BorderSize, AllottedGeometry.Scale);

			// draw highlight border
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, BorderGeometry, PulseBrush, ESlateDrawEffect::None, FLinearColor(PulseColor.R, PulseColor.G, PulseColor.B, AlphaFactor1));
		}
	}

	return LayerId;
}

EActiveTimerReturnType SRPulseIndicator::OnActiveTimer(double InCurrentTime, float InDeltaTime)
{
	if (ShouldPulse.IsSet())
	{
		if (ShouldPulse.Get() && !PulseAnimation.IsPlaying())
		{
			StartPulse();
		}
		else if (!ShouldPulse.Get() && PulseAnimation.IsPlaying())
		{
			StopPulse();
		}
	}

	return EActiveTimerReturnType::Continue;
}
