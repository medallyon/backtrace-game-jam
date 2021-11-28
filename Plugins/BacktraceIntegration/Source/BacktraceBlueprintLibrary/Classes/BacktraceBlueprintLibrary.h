// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"
#include "BacktraceBlueprintLibrary.generated.h"

UCLASS()
class UBacktraceBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "Backtrace")
	static void CrashException();

	UFUNCTION(BlueprintCallable, Category = "Backtrace")
	static void Crash();
};


