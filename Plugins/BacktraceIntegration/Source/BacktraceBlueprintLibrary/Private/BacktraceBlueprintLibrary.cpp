// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

#include "BacktraceBlueprintLibrary.h"
#include "CoreMinimal.h"


UBacktraceBlueprintLibrary::UBacktraceBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBacktraceBlueprintLibrary::CrashException()
{
	throw "backtrace.io";
}

namespace { volatile char* ptr; }
void UBacktraceBlueprintLibrary::Crash()
{
	memset((void*)ptr, 0x42, 1024 * 1024 * 20);
}
