// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

#include "BacktraceSettingsLibrary.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"


#define LOCTEXT_NAMESPACE "Backtrace"

#if defined(NDEBUG) && defined(BACKTRACE_EXTENDED_LOGGING)
#define BACKTRACE_LOG UE_LOG
#else
#define BACKTRACE_LOG(...)
#endif


class FBacktraceSettingsLibrary : public IBacktraceSettingsLibrary
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FBacktraceSettingsLibrary, BacktraceSettingsLibrary )

//DEFINE_LOG_CATEGORY(BacktraceLog);

void FBacktraceSettingsLibrary::StartupModule()
{
}

void FBacktraceSettingsLibrary::ShutdownModule()
{
}
