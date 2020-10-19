// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "Runtime/SlateCore/Public/Animation/CurveSequence.h"
#include "Runtime/SlateCore/Public/Widgets/SCompoundWidget.h"


class SYMBOLRECOGNIZERPLUGINEDITOR_API SRPulseIndicator : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRPulseIndicator){}
	SLATE_ATTRIBUTE(bool, ShouldPulse)
	SLATE_END_ARGS()

	FCurveSequence PulseAnimation;
	float MaxPulseOffset = 32.0f;
	float PulseAnimationLength = 2.0f;

	void Construct(const FArguments& InArgs);


	void StartPulse();
	void StopPulse();
	void GetAnimationValues(float InAnimationProgress, float& OutAlphaFactor0, float& OutPulseFactor0, float& OutAlphaFactor1, float& OutPulseFactor1) const;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	EActiveTimerReturnType OnActiveTimer(double InCurrentTime, float InDeltaTime);
private:
	TAttribute<bool> ShouldPulse;
};