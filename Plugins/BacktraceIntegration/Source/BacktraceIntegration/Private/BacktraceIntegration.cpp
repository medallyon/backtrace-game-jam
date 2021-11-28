// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IBacktraceIntegration.h"
#include <Runtime/Projects/Public/Interfaces/IPluginManager.h>

#include <Editor/LevelEditor/Public/LevelEditor.h>
#include <Editor/MainFrame/Public/Interfaces/IMainFrameModule.h>
#include <Editor/UnrealEd/Classes/Editor/EditorEngine.h>
#include <Editor/UnrealEd/Public/Editor.h>
#include <Framework/MultiBox/MultiBoxBuilder.h>
#include <Framework/Application/SlateApplication.h>
#include <Runtime/Core/Public/GenericPlatform/GenericPlatformCrashContext.h>
#include <Runtime/Core/Public/GenericPlatform/GenericPlatformMisc.h>
#include <Runtime/Core/Public/Misc/FileHelper.h>
#include <Runtime/Core/Public/Misc/ConfigCacheIni.h>
#include <Runtime/Engine/Classes/Engine/UserInterfaceSettings.h>
#include <Runtime/Json/Public/Serialization/JsonSerializer.h>
#include <Runtime/Json/Public/Dom/JsonObject.h>
#include <Runtime/Slate/Public/Widgets/Layout/SBox.h>
#include <Runtime/Slate/Public/Widgets/Input/SButton.h>
#include <Runtime/Slate/Public/Widgets/Input/SCheckBox.h>
#include <Runtime/Slate/Public/Widgets/Input/SEditableText.h>
#include <Runtime/Slate/Public/Widgets/Input/SEditableTextBox.h>
#include <Runtime/Slate/Public/Widgets/Input/SHyperlink.h>
#include <Runtime/Slate/Public/Widgets/Text/SRichTextBlock.h>
#include <Runtime/SlateCore/Public/Styling/SlateTypes.h>
#include <Widgets/SVirtualWindow.h>

#include "BacktraceStyle.h"
#include "BacktraceCommands.h"
#include "BacktraceWinapiWrapper.hpp"
#include "BacktraceSettings.h"

#include <cmath>

#define LOCTEXT_NAMESPACE "Backtrace"

#if defined(NDEBUG) && defined(BACKTRACE_EXTENDED_LOGGING)
#define BACKTRACE_LOG UE_LOG
#else
#define BACKTRACE_LOG(...)
#endif

class FBacktraceIntegration : public IBacktraceIntegration
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void PluginButtonClicked();
	void AddMenuExtension(FMenuBuilder& Builder);

private:

	bool LoadProjectFile(FString&);
	void SaveProjectFile(FString const&);

	void LoadConfig();
	void SaveConfig();

	void UpdateUserSettingsHeader();

	void SetupPostBuildStep();

	void DisplaySetupDialog();

	void DisplayWelcomeDialog();

	void SetWelcomeStep0();
	void SetWelcomeStep1();
	void SetWelcomeStep2();
	void SetWelcomeStep3();
	void SetWelcomeStep4();

	void UpdateDataRouterUrl(FString const& path, bool preserve_old);
	void RestoreDataRouterUrl(FString const& path);

	bool IsFirstRun();
	void SetFirstRunFinished(bool);

	void OpenWebPage(FString const& url);

	int ScaledSize(int value);

	struct BacktraceConfig
	{
		FString token;

		FString symbolsToken;

		FString realm;

		FString projectName;

		bool shouldSendDebugBulids;
		bool shouldSendReleaseBulids;
		bool overrideGlobalCrashReporterSettings;
	};

	TSharedPtr<class FUICommandList> PluginCommands;
	BacktraceConfig config;
	TSharedPtr<SWindow> window;
	float scale;
};

IMPLEMENT_MODULE(FBacktraceIntegration, BacktraceIntegration)

DEFINE_LOG_CATEGORY(BacktraceLog);

void FBacktraceIntegration::DisplaySetupDialog()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Settings clicked"));

	window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Backtrace - Configuration")))
		.ClientSize(FVector2D(400, 400) * scale)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.MaxWidth(ScaledSize(400))
		.MaxHeight(ScaledSize(400));

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));

	SetWelcomeStep4();

	FSlateApplication::Get().AddModalWindow(window.ToSharedRef(), MainFrameModule.GetParentWindow().ToSharedRef());

	window.Reset();
}

void FBacktraceIntegration::DisplayWelcomeDialog()
{
	window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Backtrace - Configuration")))
		.ClientSize(FVector2D(400, 400) * scale)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.MaxWidth(ScaledSize(400))
		.MaxHeight(ScaledSize(400));

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));

	FSlateApplication::Get().AddWindow(window.ToSharedRef());
}

void FBacktraceIntegration::SetWelcomeStep0()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("SetWelcomeStep0()"));

	constexpr const char* info = "Welcome to Backtrace!\n\n"
		"Backtrace provides plug-and-play support for the built-in crash "
		"reporting infrastructure provided by the Unreal Engine. Initial setup will guide you through "
		"the required steps to have crashes in your project automatically reported to you.\n\n"
		"To begin, either use an existing account or create an account below. If you are a "
		"power user, you are also able to configure crash reporting by directly modifying "
		"your configuration files, <a id=\"foo\">as seen here</>";

	auto onClicked = [this] {
		OpenWebPage(TEXT("https://documentation.backtrace.io/product_integration_ue4/"));
	};

	auto onCreateAccount = [this] {
		OpenWebPage(TEXT("https://backtrace.io/createue4marketplace/"));
	};

	auto handler = FSlateHyperlinkRun::FOnClick();
	handler.BindLambda([=](auto) { onClicked(); });
	TSharedRef< ITextDecorator > dec = SRichTextBlock::HyperlinkDecorator(TEXT("foo"), handler);
	auto infoBox = SNew(SRichTextBlock).Text(FText::FromString(info)) + dec;
	infoBox->SetAutoWrapText(true);

	auto haveAccount = [this] {
		SetWelcomeStep1();
		return FReply::Handled();
	};

	auto box = SNew(SBox)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		.FillHeight(ScaledSize(9))
		[
			SNew(SBox).MinDesiredHeight(ScaledSize(270))[infoBox]
		]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		.MaxHeight(ScaledSize(20))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		[SNew(SBox).MaxDesiredWidth(ScaledSize(200)).MinDesiredWidth(ScaledSize(130))[
			SNew(SButton).Text(FText::FromString("I have an account")).OnClicked_Lambda(haveAccount)
		]]

	+ SHorizontalBox::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		[SNew(SBox).MaxDesiredWidth(ScaledSize(200)).MinDesiredWidth(ScaledSize(130))[
			SNew(SHyperlink).Text(FText::FromString("Create an account")).OnNavigate_Lambda(onCreateAccount)
		]]
		]
		];

	window->SetContent(box);
}

void FBacktraceIntegration::SetWelcomeStep1()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("SetWelcomeStep1()"));

	auto realm = SNew(SEditableTextBox)
		.Text(FText::FromString(config.realm))
		.MinDesiredWidth(ScaledSize(130));// .TextShapingMethod

	auto projectName = SNew(SEditableTextBox)
		.Text(FText::FromString(config.projectName))
		.MinDesiredWidth(ScaledSize(130));

	constexpr const char* info = "Specify the Backtrace instance you created below. "
		"If you don't have one, go back to create an account.";

	auto onBack = [this] {
		SetWelcomeStep0();
		return FReply::Handled();
	};

	auto onContinue = [this, realm, projectName] {
		config.realm = realm.Get().GetText().ToString();
		config.projectName = projectName.Get().GetText().ToString();
		SetWelcomeStep2();
		return FReply::Handled();
	};

	auto infoBox = SNew(SRichTextBlock).Text(FText::FromString(info));
	infoBox->SetAutoWrapText(true);

	auto box = SNew(SBox)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.FillHeight(1)
		[
			infoBox
		]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		.MaxHeight(0)
		.FillHeight(1)
		[SNew(STextBlock).Text(FText::FromString("Instance sub-domain "))]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.MaxHeight(0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.AutoWidth()
		[realm]

	+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[SNew(STextBlock).Text(FText::FromString(".sp.backtrace.io"))]
		].FillHeight(1)


			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			.MaxHeight(0)
			.FillHeight(1)
			[SNew(STextBlock).Text(FText::FromString("Project name "))]

		+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.MaxHeight(0)
			.FillHeight(5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[projectName]

		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoWidth()
			[SNew(STextBlock).Text(FText::FromString(""))]
			]


		+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[SNew(SBox).MaxDesiredWidth(ScaledSize(130)).MinDesiredWidth(ScaledSize(130))[
				SNew(SButton).Text(FText::FromString("< Back")).OnClicked_Lambda(onBack)
			]]

		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			[SNew(SBox).MinDesiredWidth(ScaledSize(130))[
				SNew(SButton).Text(FText::FromString("Continue >")).OnClicked_Lambda(onContinue)
			]]
			]


		+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			[SNew(STextBlock).Text(FText::FromString("Step 1 of 3."))]
		.VAlign(VAlign_Bottom)

		];

	window->SetContent(box);
}

void FBacktraceIntegration::SetWelcomeStep2()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("SetWelcomeStep2()"));

	auto token = SNew(SEditableTextBox)
		.Text(FText::FromString(config.token))
		.MinDesiredWidth(ScaledSize(300));

	constexpr const char* info = "Specify a crash submission token below.\n\n"
		"This used to authenticate submissions of dumps into your instance. "
		"Find your <a id=\"token\">submission token by going here and clicking "
		"on the project dumps should be submitted to</>.";

	auto onBack = [this] {
		SetWelcomeStep1();
		return FReply::Handled();
	};

	auto onContinue = [this, token] {
		config.token = token.Get().GetText().ToString();
		SetWelcomeStep3();
		return FReply::Handled();
	};

	auto handler = FSlateHyperlinkRun::FOnClick();
	handler.BindLambda([=](auto) {
		auto link = FString::Printf(TEXT("https://%s.sp.backtrace.io/config/%s/projects/%s/tokens"),
			*config.realm, *config.realm, *config.projectName);
		OpenWebPage(link);
	});

	TSharedRef< ITextDecorator > dec = SRichTextBlock::HyperlinkDecorator(TEXT("token"), handler);
	auto infoBox = SNew(SRichTextBlock).Text(FText::FromString(info)) + dec;
	infoBox->SetAutoWrapText(true);

	auto haveAccount = [this] {
		OpenWebPage(TEXT("haveAccount"));
		return FReply::Handled();
	};

	auto box = SNew(SBox)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			infoBox
		].FillHeight(4.5)


		+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.MaxHeight(0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[token]
		].FillHeight(4.5)


			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().HAlign(HAlign_Left)
			[SNew(SBox).MinDesiredWidth(ScaledSize(130))[
				SNew(SButton).Text(FText::FromString("< Back")).OnClicked_Lambda(onBack)
			]]
		+ SHorizontalBox::Slot().HAlign(HAlign_Right)
			[SNew(SBox).MinDesiredWidth(ScaledSize(130))[
				SNew(SButton).Text(FText::FromString("Continue >")).OnClicked_Lambda(onContinue)
			]]
			]


		+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[SNew(STextBlock).Text(FText::FromString("Step 2 of 3."))]

		];

	window->SetContent(box);
}

void FBacktraceIntegration::SetWelcomeStep3()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("SetWelcomeStep3()"));

	auto symbolsToken = SNew(SEditableTextBox)
		.Text(FText::FromString(config.symbolsToken))
		.MinDesiredWidth(ScaledSize(300));

	auto projectName = SNew(SEditableTextBox)
		.Text(FText::FromString(config.projectName))
		.MinDesiredWidth(ScaledSize(130));

	constexpr const char* info = "Specify a symbol upload token below.\n\n"
		"This is different than your crash upload token. This is used to "
		"authenticate the upload of symbols to your instance. This plugin "
		"will automatically upload symbols during your builds. Find or create "
		"your <a id=\"symbols_token\">symbol token by going here, clicking on the project dumps, "
		"navigating to symbols and clicking on access tokens</>.";

	auto handler = FSlateHyperlinkRun::FOnClick();
	handler.BindLambda([=](auto) {
		auto link = FString::Printf(TEXT("https://%s.sp.backtrace.io/config/%s/projects/%s/symbol-access-tokens"),
			*config.realm, *config.realm, *config.projectName);
		OpenWebPage(link);
	});

	TSharedRef< ITextDecorator > dec = SRichTextBlock::HyperlinkDecorator(TEXT("symbols_token"), handler);
	auto infoBox = SNew(SRichTextBlock).Text(FText::FromString(info)) + dec;
	infoBox->SetAutoWrapText(true);

	auto onBack = [this] {
		SetWelcomeStep2();
		return FReply::Handled();
	};

	auto onContinue = [this, symbolsToken] {
		config.symbolsToken = symbolsToken.Get().GetText().ToString();
		SetWelcomeStep4();
		return FReply::Handled();
	};

	auto onSkip = [this] {
		SetWelcomeStep4();
		return FReply::Handled();
	};

	auto box = SNew(SBox)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			infoBox
		].FillHeight(4.5)


		+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.MaxHeight(0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[symbolsToken]
		].FillHeight(4.5)


			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().HAlign(HAlign_Left)
			[SNew(SBox).MinDesiredWidth(ScaledSize(130))[
				SNew(SButton).Text(FText::FromString("< Back")).OnClicked_Lambda(onBack)
			]]
		+ SHorizontalBox::Slot().HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().HAlign(HAlign_Right)
			[SNew(SBox).MinDesiredWidth(ScaledSize(70))[
				SNew(SButton).Text(FText::FromString("Skip")).OnClicked_Lambda(onSkip)
			]]
		+ SHorizontalBox::Slot().HAlign(HAlign_Right)
			[SNew(SBox).MinDesiredWidth(ScaledSize(130))[
				SNew(SButton).Text(FText::FromString("Continue >")).OnClicked_Lambda(onContinue)
			]]
			]
			]


		+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[SNew(STextBlock).Text(FText::FromString("Step 3 of 3."))]
		];

	window->SetContent(box);
}

void FBacktraceIntegration::SetWelcomeStep4()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("SetWelcomeStep4()"));

	auto realm = SNew(SEditableTextBox)
		.Text(FText::FromString(config.realm))
		.MinDesiredWidth(ScaledSize(200));
	auto token = SNew(SEditableTextBox)
		.Text(FText::FromString(config.token))
		.MinDesiredWidth(ScaledSize(300));
	auto symbolsToken = SNew(SEditableTextBox)
		.Text(FText::FromString(config.symbolsToken))
		.MinDesiredWidth(ScaledSize(300));
	auto postDebug = SNew(SCheckBox).
		IsChecked(config.shouldSendDebugBulids ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	auto postRelease = SNew(SCheckBox).
		IsChecked(config.shouldSendReleaseBulids ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

	auto overrideGlobalSettings = SNew(SCheckBox).
		IsChecked(config.overrideGlobalCrashReporterSettings ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

	constexpr const char* info = "Congratulations, initial setup is complete. "
		"Below are your Backtrace settings. Please remember to enable and package "
		"the Unreal crash reporter. After applying, try crashing your game to "
		"verify your settings. If you have any issues, please e-mail us at "
		"support@backtrace.io";

	auto handler = FSlateHyperlinkRun::FOnClick();
	handler.BindLambda([=](auto) {
		auto link = FString::Printf(TEXT("https://%s.sp.backtrace.io/"), *config.realm);
		OpenWebPage(link);
	});

	TSharedRef< ITextDecorator > dec = SRichTextBlock::HyperlinkDecorator(TEXT("symbols_token"), handler);
	auto infoBox = SNew(SRichTextBlock).Text(FText::FromString(info)) + dec;
	infoBox->SetAutoWrapText(true);

	auto onCancel = [this] {
		window->RequestDestroyWindow();
		window.Reset();
		return FReply::Handled();
	};

	auto onApply = [=] {
		config.token = token.Get().GetText().ToString();
		config.realm = realm.Get().GetText().ToString();
		config.symbolsToken = symbolsToken.Get().GetText().ToString();
		config.overrideGlobalCrashReporterSettings = overrideGlobalSettings.Get().IsChecked();
		config.shouldSendDebugBulids = postDebug.Get().IsChecked();
		config.shouldSendReleaseBulids = postRelease.Get().IsChecked();

		SetupPostBuildStep();

		SaveConfig();

		UpdateUserSettingsHeader();

		auto local_path = FString::Printf(TEXT("%s/Config/DefaultEngine.ini"), *FPaths::GetPath(FPaths::GetProjectFilePath()));

		UpdateDataRouterUrl(local_path, false);

		auto profile = Backtrace::GetUserProfileDirectory();
		auto global_path = FString::Printf(TEXT("%s/Documents/Unreal Engine/Engine/Config/UserEngine.ini"), *profile);

		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("SetWelcomeStep4()::onApply() previous != current"));
		if (config.overrideGlobalCrashReporterSettings) {
			UpdateDataRouterUrl(global_path, false);
		}
		else {
			RestoreDataRouterUrl(global_path);
		}

		window->RequestDestroyWindow();
		window.Reset();
		SetFirstRunFinished(true);
		return FReply::Handled();
	};

	auto box = SNew(SBox)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.FillHeight(3)
		[
			infoBox
		]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.MaxHeight(0)
		[SNew(SRichTextBlock).Text(FText::FromString("Server Instance")).TextStyle(FBacktraceStyle::Get(), "RichText.Header")]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.MaxHeight(0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		[realm]

	+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		[SNew(STextBlock).Text(FText::FromString(".sp.backtrace.io"))]
		]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.MaxHeight(0)
		[SNew(SRichTextBlock).Text(FText::FromString("Crash Submission Token")).TextStyle(FBacktraceStyle::Get(), "RichText.Header")]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.MaxHeight(0)
		[token]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.MaxHeight(0)
		[SNew(SRichTextBlock).Text(FText::FromString("Symbol Submission Token")).TextStyle(FBacktraceStyle::Get(), "RichText.Header")]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.MaxHeight(0)
		[symbolsToken]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().HAlign(HAlign_Left).MaxWidth(ScaledSize(30))[postRelease]
		+ SHorizontalBox::Slot().HAlign(HAlign_Fill).FillWidth(ScaledSize(10))[SNew(SBox).WidthOverride(ScaledSize(370))[
			SNew(STextBlock).Text(FText::FromString(TEXT("Upload symbols for release builds"))).MinDesiredWidth(ScaledSize(370))
		]]
		]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().HAlign(HAlign_Left).MaxWidth(ScaledSize(30))[postDebug]
		+ SHorizontalBox::Slot().HAlign(HAlign_Fill).FillWidth(ScaledSize(10))[SNew(SBox).WidthOverride(ScaledSize(370))[
			SNew(STextBlock).Text(FText::FromString(TEXT("Upload symbols for editor and local builds"))).MinDesiredWidth(ScaledSize(370))
		]]
		]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().HAlign(HAlign_Left).MaxWidth(ScaledSize(30))[overrideGlobalSettings]
		+ SHorizontalBox::Slot().HAlign(HAlign_Fill).FillWidth(ScaledSize(10))[SNew(SBox).WidthOverride(ScaledSize(370))[
			SNew(STextBlock).Text(FText::FromString(TEXT("Upload crashes for non-release builds (global setting)"))).MinDesiredWidth(ScaledSize(370))
		]]
		]


	+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().HAlign(HAlign_Left)
		[
			SNew(SBox).MinDesiredWidth(ScaledSize(130))[
				SNew(SButton).Text(FText::FromString("Cancel")).OnClicked_Lambda(onCancel)
			]
		]

	+ SHorizontalBox::Slot().HAlign(HAlign_Right)
		[
			SNew(SBox).MinDesiredWidth(ScaledSize(130))[
				SNew(SButton).Text(FText::FromString("Apply")).OnClicked_Lambda(onApply)
			]
		]
		]
		];

	window->SetContent(box);
}

bool FBacktraceIntegration::LoadProjectFile(FString& out)
{
	auto proj = FPaths::GetProjectFilePath();
	return FFileHelper::LoadFileToString(out, *proj);
}

void FBacktraceIntegration::SaveProjectFile(FString const& str)
{
	auto proj = FPaths::GetProjectFilePath();
	FFileHelper::SaveStringToFile(str, *proj);
}

void FBacktraceIntegration::StartupModule()
{
	if(FSlateApplication::IsInitialized()) {
		scale = FSlateApplication::Get().GetApplicationScale();
	}

	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("FBacktraceIntegration::StartupModule"));

	auto cfg = &this->config;
	cfg->realm = TEXT("yourname");
	cfg->token = TEXT("your crash submission token");
	cfg->symbolsToken = TEXT("your symbols token");
	cfg->shouldSendDebugBulids = false;
	cfg->shouldSendReleaseBulids = false;
	cfg->overrideGlobalCrashReporterSettings = false;

	LoadConfig();

	FBacktraceStyle::Initialize();
	FBacktraceStyle::ReloadTextures();

	FBacktraceCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FBacktraceCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FBacktraceIntegration::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("EditLocalTabSpawners", EExtensionHook::After, PluginCommands,
			FMenuExtensionDelegate::CreateRaw(this, &FBacktraceIntegration::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FBacktraceIntegration::ShutdownModule()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("FBacktraceIntegration::ShutdownModule"));
	// SaveConfig();
}

void FBacktraceIntegration::UpdateUserSettingsHeader()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("IsFirstRun()"));
	auto path = FPaths::GetPath(FPaths::GetProjectFilePath());
	path = FString::Printf(TEXT("%s/Plugins/BacktraceIntegration/Source/BacktraceBlueprintLibrary/Private/BacktraceUserSettings.h"), *path);

	FString data = TEXT("#pragma once\n\n");
	data += FString::Printf(TEXT("#define BACKTRACE_TOKEN \"%s\"\n"), *config.token);
	data += FString::Printf(TEXT("#define BACKTRACE_REALM \"%s\"\n"), *config.realm);

	FFileHelper::SaveStringToFile(data, *path);
}

void FBacktraceIntegration::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FBacktraceCommands::Get().PluginAction);
}

void FBacktraceIntegration::PluginButtonClicked()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("FBacktraceIntegration::PluginButtonClicked"));
	if (IsFirstRun()) {
		DisplayWelcomeDialog();
		SetWelcomeStep0();
	}
	else {
		DisplaySetupDialog();
		SetupPostBuildStep();
		SaveConfig();
	}
}

void FBacktraceIntegration::LoadConfig()
{
	FString projContents;
	if (LoadProjectFile(projContents)) {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("file opened"));
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(projContents);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize worked"));

			TArray<TSharedPtr<FJsonValue>> arr;
			auto const* arr_ptr = &arr;

			bool success = JsonParsed->TryGetField("Plugins").IsValid();
			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Plugins valid %d"), success);

			if (!success)
			{
				BACKTRACE_LOG(BacktraceLog, Warning, TEXT("No Plugins.."));
				return;
			}


			arr = JsonParsed->TryGetField("Plugins")->AsArray();

			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Arr size: %d"), arr.Num());

			for (auto el : arr)
			{
				BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Arr element"));
				if (!el.Get())
					continue;

				auto obj = el->AsObject();

				FString Name;
				success = obj->TryGetStringField(TEXT("Name"), Name);
				if (!success)
					continue;

				BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Name Found: %s"), *Name);

				if (Name != TEXT("BacktraceIntegration")) {
					continue;
				}

				FString token, realm, symbolsToken, projectName;
				bool send_debug_pdb, send_release_pdb, override_global_reporter;

				if (obj->TryGetStringField("Token", token)) {
					config.token = token;
				}

				if (obj->TryGetStringField("SymbolsToken", symbolsToken)) {
					config.symbolsToken = symbolsToken;
				}

				if (obj->TryGetStringField("Realm", realm)) {
					config.realm = realm;
				}

				if (obj->TryGetStringField("ProjectName", projectName)) {
					config.projectName = projectName;
				}

				if (obj->TryGetBoolField("SendDebugPdb", send_debug_pdb)) {
					config.shouldSendDebugBulids = send_debug_pdb;
				}

				if (obj->TryGetBoolField("SendReleasePdb", send_release_pdb)) {
					config.shouldSendReleaseBulids = send_release_pdb;
				}

				if (obj->TryGetBoolField("OverrideGlobalReporter", override_global_reporter)) {
					config.overrideGlobalCrashReporterSettings = override_global_reporter;
				}

			}
			SaveConfig();
		}
		else {

			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize failed"));
		}
	}
	else {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("failed to open the file"));
	}
}

void FBacktraceIntegration::SaveConfig()
{
	auto MutableSettings = GetMutableDefault<UBacktraceSettings>();
	if(MutableSettings) {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Saving token %s"), *config.token);
		MutableSettings->Token = config.token;
		MutableSettings->Universe = config.realm;
		MutableSettings->PostEditChange();
		MutableSettings->SaveConfig(CPF_Config, *MutableSettings->GetDefaultConfigFilename());
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Settings saved"));
	} else {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Failed to open settings"));
	}

	FString projContents;
	if (LoadProjectFile(projContents)) {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("file opened"));
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(projContents);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize worked"));

			TArray<TSharedPtr<FJsonValue>> arr;
			auto const* arr_ptr = &arr;

			bool success = JsonParsed->TryGetField("Plugins").IsValid();
			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Plugins valid %d"), success);

			if (!success)
			{
				BACKTRACE_LOG(BacktraceLog, Warning, TEXT("No Plugins.."));

				TArray<TSharedPtr<FJsonValue>> plugins_arr;
				TSharedPtr<FJsonObject> backtrace_plugin_item = MakeShareable(new FJsonObject);
				backtrace_plugin_item->SetStringField("Name", "BacktraceIntegration");
				auto array_item = MakeShareable(new FJsonValueObject(backtrace_plugin_item));
				plugins_arr.Add(array_item);
				JsonParsed->SetArrayField("Plugins", plugins_arr);
			}

			arr = JsonParsed->TryGetField("Plugins")->AsArray();

			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Arr size: %d"), arr.Num());

			for (auto el : arr)
			{
				BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Arr element"));
				if (!el.Get())
					continue;

				auto obj = el->AsObject();

				FString Name;
				success = obj->TryGetStringField(TEXT("Name"), Name);
				if (!success)
					continue;

				BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Name Found: %s"), *Name);

				if (Name != TEXT("BacktraceIntegration")) {
					continue;
				}

				obj->SetStringField("Token", config.token);
				obj->SetStringField("SymbolsToken", config.symbolsToken);
				obj->SetStringField("Realm", config.realm);
				obj->SetStringField("ProjectName", config.projectName);
				obj->SetBoolField("SendReleasePdb", config.shouldSendReleaseBulids);
				obj->SetBoolField("SendDebugPdb", config.shouldSendDebugBulids);
				obj->SetBoolField("OverrideGlobalReporter", config.overrideGlobalCrashReporterSettings);

			}

			JsonParsed->SetArrayField("Plugins", arr);

			FString OutputString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(JsonParsed.ToSharedRef(), Writer);

			SaveProjectFile(OutputString);
		}
		else {

			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize failed"));
		}
	}
	else {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("failed to open the file"));
	}
}

void FBacktraceIntegration::UpdateDataRouterUrl(FString const& path, bool preserve_old)
{
	FConfigCacheIni ini(EConfigCacheType::DiskBacked);

	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("UpdateDataRouterUrl: %s"), *path);

	ini.LoadFile(path, nullptr, nullptr);


	auto url = FString::Printf(TEXT("https://unreal.backtrace.io/post/%s/%s"), *config.realm, *config.token);

	FString old_url;
	if (preserve_old && ini.GetString(TEXT("CrashReportClient"), TEXT("DataRouterUrl"), old_url, path) && old_url != url) {
		ini.SetString(TEXT("CrashReportClient"), TEXT("DataRouterUrlOld"), *old_url, path);
	}

	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("UpdateDataRouterUrl: %s"), *url);

	ini.SetString(TEXT("CrashReportClient"), TEXT("DataRouterUrl"), *url, path);
	ini.SetDouble(TEXT("CrashReportClient"), TEXT("CrashReportClientVersion"), 1.0, path);
}

void FBacktraceIntegration::RestoreDataRouterUrl(FString const& path)
{
	FConfigCacheIni ini(EConfigCacheType::DiskBacked);

	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("RestoreDataRouterUrl: %s"), *path);

	ini.LoadFile(path, nullptr, nullptr);

	FString old_url;
	if (ini.GetString(TEXT("CrashReportClient"), TEXT("DataRouterUrlOld"), old_url, path)) {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Recovering old url: %s"), *old_url);
		ini.SetString(TEXT("CrashReportClient"), TEXT("DataRouterUrl"), *old_url, path);
		ini.RemoveKey(TEXT("CrashReportClient"), TEXT("DataRouterUrlOld"), path);
	}
	else {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("Deleting the url"));
		ini.RemoveKey(TEXT("CrashReportClient"), TEXT("DataRouterUrl"), path);
	}

}

void FBacktraceIntegration::SetupPostBuildStep()
{
	auto proj = FPaths::GetProjectFilePath();
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("token: %s"), *proj);

	auto debug_str = FString::Printf(TEXT("--send_debug=%s"), config.shouldSendDebugBulids ? TEXT("True") : TEXT("False"));
	auto release_str = FString::Printf(TEXT("--send_release=%s"), config.shouldSendReleaseBulids ? TEXT("True") : TEXT("False"));
	auto token_str = FString::Printf(TEXT("--token=%s"), *config.token);
	auto symbols_token_str = FString::Printf(TEXT("--symbols_token=%s"), *config.symbolsToken);
	auto realm_str = FString::Printf(TEXT("--realm=%s"), *config.realm);

	auto str_to_add = FString::Printf(TEXT("\"%s/%s\" %s %s %s %s %s %s %s %s %s %s %s"),
		TEXT("$(ProjectDir)/Plugins/BacktraceIntegration/Content/BacktraceIntegration"),
		TEXT("backtrace_post_build.exe"),
		*token_str,
		*debug_str,
		*release_str,
		*realm_str,
		*symbols_token_str,
		TEXT("--engine_dir=\"$(EngineDir)\""),
		TEXT("--project_dir=\"$(ProjectDir)\""),
		TEXT("--target_name=\"$(TargetName)\""),
		TEXT("--target_configuration=\"$(TargetConfiguration)\""),
		TEXT("--target_type=\"$(TargetType)\""),
		TEXT("--project_file=\"$(ProjectFile)\"")
	);

	FString projContents;
	if (LoadProjectFile(projContents)) {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("file opened"));
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(projContents);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize worked"));
			TArray<TSharedPtr<FJsonValue>> arr;

			TSharedPtr<FJsonObject> obj = MakeShareable(new FJsonObject);
			auto const* ptr = &obj;
			auto success = JsonParsed->TryGetObjectField(TEXT("PostBuildSteps"), ptr);
			if (success) {
				BACKTRACE_LOG(BacktraceLog, Warning, TEXT("obj is valid"));
				auto const* arr_ptr = &arr;
				success = obj->TryGetArrayField(TEXT("Win64"), arr_ptr);

				if (success) {
					auto filtered = arr.FilterByPredicate([](const TSharedPtr<FJsonValue>& el) {
						if (!el.Get())
							return false;
						FString str;
						if (!el.Get()->TryGetString(str))
							return false;
						if (str.Contains(TEXT("backtrace_post_build.exe")))
							return true;
						if (str.Contains(TEXT("backtrace_symbol_uploader.exe")))
							return true;
						return false;
					});
					arr = filtered;
				}
			}
			else {
				BACKTRACE_LOG(BacktraceLog, Warning, TEXT("obj is empty"));
			}

			auto to_add = MakeShareable(new FJsonValueString(*str_to_add));
			arr.Add(to_add);


			obj->SetArrayField(TEXT("Win64"), arr);
			JsonParsed->SetObjectField(TEXT("PostBuildSteps"), obj);

			FString OutputString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(JsonParsed.ToSharedRef(), Writer);

			SaveProjectFile(OutputString);
		}
		else {

			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize failed"));
		}
	}
	else {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("failed to open the file"));
	}
}

void FBacktraceIntegration::OpenWebPage(FString const& url)
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("FBacktraceIntegration::OpenWebPage() -> %s"), *url);
	FPlatformProcess::LaunchURL(*url, nullptr, nullptr);
}

int FBacktraceIntegration::ScaledSize(int value)
{
	static auto WindowScale = Backtrace::GetDPISettings();
	if (WindowScale < 0)
	{
		WindowScale = Backtrace::GetDPISettings();
	}
	scale = WindowScale < 0 ? 1.f : WindowScale / 96;
	return std::round(value * scale);
}

bool FBacktraceIntegration::IsFirstRun()
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("IsFirstRun()"));
	auto path = FPaths::GetPath(FPaths::GetProjectFilePath());
	path = FString::Printf(TEXT("%s/Plugins/BacktraceIntegration/backtrace_state.json"), *path);

	FString projContents;
	if (FFileHelper::LoadFileToString(projContents, *path)) {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("file opened"));
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(projContents);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize worked"));

			bool result;

			bool success = JsonParsed->TryGetField("FirstRunFinished").IsValid();

			if (!success)
			{
				return true;
			}

			result = JsonParsed->TryGetField("FirstRunFinished")->AsBool();

			return !result;
		}
		else {

			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize failed"));
			return true;
		}
	}
	else {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("failed to open the file"));
		return true;
	}
	return true;
}

void FBacktraceIntegration::SetFirstRunFinished(bool value)
{
	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("IsFirstRun()"));
	auto path = FPaths::GetPath(FPaths::GetProjectFilePath());
	path = FString::Printf(TEXT("%s/Plugins/BacktraceIntegration/backtrace_state.json"), *path);

	FString projContents;
	if (FFileHelper::LoadFileToString(projContents, *path)) {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("file opened"));
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(projContents);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize worked"));

			JsonParsed->SetBoolField("FirstRunFinished", value);

			FString OutputString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(JsonParsed.ToSharedRef(), Writer);

			FFileHelper::SaveStringToFile(OutputString, *path);
			return;
		}
		else {

			BACKTRACE_LOG(BacktraceLog, Warning, TEXT("deserialize failed"));
		}
	}
	else {
		BACKTRACE_LOG(BacktraceLog, Warning, TEXT("failed to open the file"));
	}

	auto str = FString::Printf(TEXT("{ \"FirstRunFinished\": %s }"), value ? TEXT("true") : TEXT("false"));
	FFileHelper::SaveStringToFile(*str, *path);
}

