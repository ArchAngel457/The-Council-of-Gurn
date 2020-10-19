// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once
#include "Runtime/SlateCore/Public/Widgets/SCompoundWidget.h"

class USRToolManager;

typedef TSharedPtr<class IPropertyHandle >	SRRowDataType;
typedef SListView<SRRowDataType >			SRListDataType;


class SYMBOLRECOGNIZERPLUGINEDITOR_API SRSymbolsCategoryList : public SCompoundWidget
{
	
public:
	SLATE_BEGIN_ARGS(SRSymbolsCategoryList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class USRToolManager* InTool, TSharedPtr<class IPropertyHandleArray> InSymbolsArrayHandle);
	TSharedRef<class ITableRow > OnGenerateRow(SRRowDataType Item, const TSharedRef< STableViewBase >& OwnerTable);

private:
	TArray< SRRowDataType > SymbolItemsList;
	TWeakObjectPtr<USRToolManager> ToolKit;
};
