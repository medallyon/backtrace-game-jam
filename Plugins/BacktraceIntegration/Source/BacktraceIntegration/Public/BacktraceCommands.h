// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "BacktraceStyle.h"

class FBacktraceCommands : public TCommands<FBacktraceCommands>
{
public:

    FBacktraceCommands()
		: TCommands<FBacktraceCommands>(TEXT("Backtrace"), NSLOCTEXT("Contexts", "Backtrace", "Backtrace"), NAME_None, FBacktraceStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
