// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IBacktraceBlueprintLibrary.h"
#include <Runtime/Projects/Public/Interfaces/IPluginManager.h>
#include <Runtime/Core/Public/GenericPlatform/GenericPlatformFile.h>
#include <Runtime/Core/Public/HAL/PlatformFilemanager.h>
#include <Runtime/Core/Public/Misc/FileHelper.h>
#include <Runtime/Core/Public/Misc/ConfigCacheIni.h>
#include <Runtime/Launch/Resources/Version.h>
#include "BacktraceUserSettings.h"

#include "BacktraceSettings.h"

#define LOCTEXT_NAMESPACE "Backtrace"

#if defined(NDEBUG) && defined(BACKTRACE_EXTENDED_LOGGING)
#define BACKTRACE_LOG UE_LOG
#else
#define BACKTRACE_LOG(...)
#endif

static void UpdateIniFiles();

class FBacktraceBlueprintLibrary : public IBacktraceBlueprintLibrary
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};

IMPLEMENT_MODULE( FBacktraceBlueprintLibrary, BacktraceBlueprintLibrary )

DEFINE_LOG_CATEGORY(BacktraceLog);

void FBacktraceBlueprintLibrary::StartupModule()
{
    BACKTRACE_LOG(BacktraceLog, Warning, TEXT("FBacktraceBlueprintLibrary::StartupModule"));

	auto dbg = [](int val) {
		BACKTRACE_LOG(BacktraceLog, Warning, *FString::Printf(TEXT("FBacktraceBlueprintLibrary -> %d"), val));
	};

#if !WITH_EDITOR && PLATFORM_WINDOWS
	UpdateIniFiles();
#endif
}

void FBacktraceBlueprintLibrary::ShutdownModule()
{
    BACKTRACE_LOG(BacktraceLog, Warning, TEXT("FBacktraceBlueprintLibrary::ShutdownModule"));
}

void UpdateIniFiles()
{
	FConfigCacheIni ini(EConfigCacheType::DiskBacked);

	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("UpdateDataRouterUrl: %s"), *path);
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 26
	constexpr const auto path = TEXT("../../../Engine/Programs/CrashReportClient/Config/NoRedist/DefaultEngine.ini");
	constexpr const auto dir_path = TEXT("../../../Engine/Programs/CrashReportClient/Config/NoRedist/");
#else
	constexpr const auto path = TEXT("../../../Engine/Restricted/NotForLicensees/Programs/CrashReportClient/Config/DefaultEngine.ini");
	constexpr const auto dir_path = TEXT("../../../Engine/Restricted/NotForLicensees/Programs/CrashReportClient/Config/");
#endif

	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(dir_path);
	ini.LoadFile(path, nullptr, nullptr);

	FString value;
	if (!ini.GetString(TEXT("CrashReportClient"), TEXT("DataRouterUrl"), value, path)) {
		// return;
	}

	// auto url = FString::Printf(TEXT("https://unreal.backtrace.io/post/%s/%s"), TEXT(BACKTRACE_REALM), TEXT(BACKTRACE_TOKEN));

	auto Settings = GetDefault<UBacktraceSettings>();

	auto Url = FString::Printf(TEXT("https://unreal.backtrace.io/post/%s/%s"), *Settings->Universe, *Settings->Token);

	BACKTRACE_LOG(BacktraceLog, Warning, TEXT("UpdateDataRouterUrl: %s"), *Url);

	ini.SetString(TEXT("CrashReportClient"), TEXT("DataRouterUrl"), *Url, path);
	ini.SetDouble(TEXT("CrashReportClient"), TEXT("CrashReportClientVersion"), 1.0, path);


}
