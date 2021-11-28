// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

#include "BacktraceStyle.h"
#include "CoreMinimal.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include <Runtime/Engine/Public/Slate/SlateGameResources.h>
#include <Runtime/Projects/Public/Interfaces/IPluginManager.h>
#include "IBacktraceIntegration.h"

TSharedPtr< FSlateStyleSet > FBacktraceStyle::StyleInstance = NULL;

void FBacktraceStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FBacktraceStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FBacktraceStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("BacktraceStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D settingscog(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FBacktraceStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("BacktraceStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("BacktraceIntegration")->GetBaseDir() / TEXT("Resources"));

	Style->Set("Backtrace.PluginAction", new IMAGE_BRUSH(TEXT("Icon128"), settingscog));

	const FTextBlockStyle NormalText = FTextBlockStyle()
		.SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 9))
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
		.SetHighlightShape(BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f / 8.f)));

	const FTextBlockStyle NormalRichTextStyle = FTextBlockStyle(NormalText);

	Style->Set("RichText.Text", NormalRichTextStyle);

	Style->Set("RichText.Header", FTextBlockStyle(NormalText)
		.SetFontSize(12)
	);

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FBacktraceStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FBacktraceStyle::Get()
{
	return *StyleInstance;
}
