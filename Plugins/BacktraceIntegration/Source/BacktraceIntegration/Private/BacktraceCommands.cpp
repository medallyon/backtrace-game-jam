// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

#include "BacktraceCommands.h"
#include "CoreMinimal.h"

#define LOCTEXT_NAMESPACE "FBacktraceModule"

void FBacktraceCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Backtrace", "Execute Backtrace action", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
