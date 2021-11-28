// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

#include "CoreMinimal.h"
#include "BacktraceWinapiWrapper.hpp"

#include <vector>

#include <Runtime/Core/Public/Windows/AllowWindowsPlatformTypes.h>
// #include "AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <shlobj.h>
#include <Runtime/Core/Public/Windows/HideWindowsPlatformTypes.h>
// #include "HideWindowsPlatformTypes.h"

namespace Backtrace
{

FString GetUserProfileDirectory()
{
    TCHAR path[MAX_PATH];

    if(!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return {};
    }

    return path;
}

namespace 
{

struct EnumWindowsCallbackArgs {
	EnumWindowsCallbackArgs(DWORD p) : pid(p) { }
	const DWORD pid;
	std::vector<HWND> handles;
};

static BOOL CALLBACK EnumWindowsCallback(HWND hnd, LPARAM lParam)
{
	EnumWindowsCallbackArgs *args = (EnumWindowsCallbackArgs *)lParam;

	DWORD windowPID;
	(void)::GetWindowThreadProcessId(hnd, &windowPID);
	if (windowPID == args->pid) {
		args->handles.push_back(hnd);
	}

	return true;
}

std::vector<HWND> getToplevelWindows()
{
	EnumWindowsCallbackArgs args(::GetCurrentProcessId());
	if (::EnumWindows(&EnumWindowsCallback, (LPARAM)&args) == false) {
		// XXX Log error here
		return std::vector<HWND>();
	}
	return args.handles;
}

}

float GetDPISettings()
{
	auto module = LoadLibrary(TEXT("user32.dll"));
	if (!module)
		return -1;

	auto addr = GetProcAddress(module, "GetDpiForWindow");
	if (!addr)
		return -2;

	using func = INT_PTR WINAPI(HWND);
	auto GetDpiForWindow = (func*)addr;

	auto windows = getToplevelWindows();

	if (windows.size() < 1)
		return -3;

	return GetDpiForWindow(windows.front());
}

}

