#pragma once

#include "Engine/DeveloperSettings.h"
#include "CoreMinimal.h"
#include "BacktraceSettings.generated.h"

UCLASS(Config=Plugins, defaultconfig)
class BACKTRACESETTINGSLIBRARY_API UBacktraceSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, globalconfig, Category = "Backtrace")
	FString Token;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, globalconfig, Category = "Backtrace")
	FString SymbolsToken;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, globalconfig, Category = "Backtrace")
	FString Universe;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, globalconfig, Category = "Backtrace")
	FString ProjectName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, globalconfig, Category = "Backtrace")
	bool bShouldSendDebugBulids;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, globalconfig, Category = "Backtrace")
	bool bShouldSendReleaseBulids;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, globalconfig, Category = "Backtrace")
	bool bOverrideGlobalCrashReporterSettings;

	UPROPERTY(globalconfig)
	bool bPortedOldSettingsStyle;
};

