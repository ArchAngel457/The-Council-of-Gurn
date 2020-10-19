// Copyright 2019 Piotr Macharzewski. All Rights Reserved.


#include "SRPluginEditorStyle.h"
#include "Runtime/SlateCore/Public/Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Runtime/Engine/Public/Slate/SlateGameResources.h"
#include "Runtime/Projects/Public/Interfaces/IPluginManager.h"
#include "Runtime/SlateCore/Public/Styling/SlateTypes.h"

TSharedPtr< FSlateStyleSet > FSRPluginEditorStyle::StyleInstance = NULL;

void FSRPluginEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSRPluginEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSRPluginEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SRPluginEditorStyle"));
	return StyleSetName;
}


#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon64x64(64.0f, 64.0f);

TSharedRef< FSlateStyleSet > FSRPluginEditorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SRPluginEditorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("SymbolRecognizerPlugin")->GetBaseDir() / TEXT("Resources"));
	
	Style->Set("SymbolRecognizerPluginEditor.OpenPluginWindow", new IMAGE_BRUSH(TEXT("Icon128"), Icon40x40));
	Style->Set("NeedsDrawingIcon", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("FrameBG", new IMAGE_BRUSH(TEXT("FrameBG"), Icon64x64));
	Style->Set("FrameBGLight", new IMAGE_BRUSH(TEXT("FrameBGLight"), Icon64x64));
	Style->Set("SoftBrush", new IMAGE_BRUSH(TEXT("SoftBrush64x64"), Icon64x64));
	Style->Set("SRLogo", new IMAGE_BRUSH(TEXT("SRLogo"), Icon64x64 * 2));
	
	Style->Set("SymbolRecognizerPlugin.ArrowRight", FButtonStyle()
			.SetNormal(BOX_BRUSH("ButtonArrowRight64_Default", Icon64x64, FMargin(2 / 64.f), FLinearColor::White))
			.SetHovered(BOX_BRUSH("ButtonArrowRight64_Hover", Icon64x64, FMargin(2 / 64.f), FLinearColor::White))
			.SetPressed(BOX_BRUSH("ButtonArrowRight64_Press", Icon64x64, FMargin(2 / 64.f), FLinearColor::White))
			
		);

	Style->Set("SymbolRecognizerPlugin.Frame64", new BOX_BRUSH("Frame64", Icon64x64, FMargin(5 / 64.f)));


	// Normal Text
	const FTextBlockStyle NormalRichTextStyle = FTextBlockStyle()
		.SetFont(DEFAULT_FONT("Regular", 11))
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f));
		//.SetHighlightShape(BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f / 8.f)));


	Style->Set("RichText.Text", NormalRichTextStyle);
	Style->Set("Highlight", FTextBlockStyle(NormalRichTextStyle)
		.SetFont(DEFAULT_FONT("Bold", 11))
		.SetColorAndOpacity(FLinearColor(FColor(255, 219, 20)))
	);

	Style->Set("BL", FTextBlockStyle(NormalRichTextStyle)
		.SetFont(DEFAULT_FONT("BlackItalic", 11))
		.SetColorAndOpacity(FLinearColor(FColor(204, 0, 0, 255)))
	);

	Style->Set("H1", FTextBlockStyle(NormalRichTextStyle)
		.SetFont(DEFAULT_FONT("Bold", 14))
		.SetColorAndOpacity(FLinearColor(FColor(255, 219, 20)))
	);
	Style->Set("H2", FTextBlockStyle(NormalRichTextStyle)
		.SetFont(DEFAULT_FONT("Bold", 24))
		.SetColorAndOpacity(FLinearColor(FColor(255, 219, 20)))
	);

	Style->Set("CustomProfile", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("CustomProfile", FVector2D(380, 90) * 0.8f))
		.SetBaseline(0)
	);
	Style->Set("CanvasPreview", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("CanvasPreview", FVector2D(374, 283) * 0.8f))
		.SetBaseline(0)
	);
	Style->Set("Learning", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("Learning", FVector2D(449, 83) * 0.7f))
		.SetBaseline(0)
	);
	Style->Set("LeftBarTiles", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("LeftBarTiles", FVector2D(302, 81)))
		.SetBaseline(0)
	);
	Style->Set("SymbolsAndImages", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("SymbolsAndImages", FVector2D(302, 81)))
		.SetBaseline(0)
	);
	Style->Set("NavBar", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("NavBar", FVector2D(629, 38) * 0.85))
		.SetBaseline(0)
	);
	Style->Set("SimpleDrawingBP", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("SimpleDrawingBP", FVector2D(1131, 571) * 0.75))
		.SetBaseline(0)
	);
	Style->Set("ChangeProfile", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("ChangeProfile", FVector2D(489, 233) * 0.85))
		.SetBaseline(0)
	);
	Style->Set("TestDrawing", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("TestDrawing", FVector2D(485, 245) * 0.85))
		.SetBaseline(0)
	);
	Style->Set("WidgetHelpers", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("WidgetHelpers", FVector2D(605, 481) * 0.85))
		.SetBaseline(0)
	);
	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
#undef DEFAULT_FONT

void FSRPluginEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSRPluginEditorStyle::Get()
{
	return *StyleInstance;
}
