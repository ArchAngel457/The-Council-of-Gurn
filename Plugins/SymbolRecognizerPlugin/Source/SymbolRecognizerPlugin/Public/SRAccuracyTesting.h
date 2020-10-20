// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

class USymbolRecognizer;
class SScrollBox;
class SWeakWidget;


class SYMBOLRECOGNIZERPLUGIN_API FSRAccuracyTesting
{
public:
	static TSharedPtr<class SRAccuracyTestingStandalone> AccuracyWidget;
	static TSharedPtr<class SRCanvasPreview> PreviewWidget;

	static void ShowAccuracyTesting(USymbolRecognizer* InSymbolRecognizer);
	static void HideAccuracyTesting();

	static void ShowPreviewWidget(USymbolRecognizer* InSymbolRecognizer);
	static void HidePreviewWidget();
};

