// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "Widgets/Docking/SDockTab.h"

class USRToolManager;

class SYMBOLRECOGNIZERPLUGINEDITOR_API SRToolWidget : public SDockTab
{
public:
	SLATE_BEGIN_ARGS(SRToolWidget)
		: _OnTabClosed()
	{}
	SLATE_EVENT(FOnTabClosedCallback, OnTabClosed)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, USRToolManager* InSRToolManager);
	static TSharedRef<class IDetailCustomization> MakeInstance();
	void DoCleanup();

	SRToolWidget();
private:
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	TSharedRef<SWidget> BildDetails(class USRToolManager* InSRToolManager);
	TSharedRef<SWidget> BuildNavigationBar();
	TSharedPtr<class SRNavigationWrapper> NavWrapper;

};
