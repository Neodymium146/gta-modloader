/*
	Copyright(c) 2015 Neodymium

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include <Windows.h>
#include <string>
#include <fstream>
#include "log.h"
#include "virtualization.h"


// mhook imports
BOOL Mhook_SetHook(PVOID *ppSystemFunction, PVOID pHookFunction);
BOOL Mhook_Unhook(PVOID *ppHookedFunction);


typedef DWORD(__stdcall *GetFileAttributesW_t)(
	LPCTSTR lpFileName
);

typedef BOOL(__stdcall *GetFileAttributesExW_t)(
	LPCTSTR lpFileName,
	GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation
);

typedef HANDLE(__stdcall *CreateFileW_t)(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
);

typedef DWORD(__stdcall *SetFilePointer_t)(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod
);

typedef BOOL(__stdcall *SetFilePointerEx_t)(
	HANDLE hFile,
	LARGE_INTEGER liDistanceToMove,
	PLARGE_INTEGER lpNewFilePointer,
	DWORD dwMoveMethod
);

typedef BOOL(__stdcall *ReadFile_t)(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
);

GetFileAttributesW_t Original_GetFileAttributesW;
GetFileAttributesExW_t Original_GetFileAttributesExW;
CreateFileW_t Original_CreateFileW;
SetFilePointer_t Original_SetFilePointer;
SetFilePointerEx_t Original_SetFilePointerEx;
ReadFile_t Original_ReadFile;

DWORD __stdcall Hooked_GetFileAttributesW(
	LPCTSTR lpFileName)
{
	virtualization_initialize(lpFileName);
	if (virtualization_check(lpFileName))
	{
		return Virtualized_GetFileAttributesW(
			lpFileName);
	}
	else
	{
		return Original_GetFileAttributesW(
			lpFileName);
	}
}

BOOL __stdcall Hooked_GetFileAttributesExW(
	LPCTSTR lpFileName,
	GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation)
{
	virtualization_initialize(lpFileName);
	if (virtualization_check(lpFileName))
	{
		return Virtualized_GetFileAttributesExW(
			lpFileName,
			fInfoLevelId,
			lpFileInformation);
	}
	else
	{
		return Original_GetFileAttributesExW(
			lpFileName,
			fInfoLevelId,
			lpFileInformation);
	}
}

HANDLE __stdcall Hooked_CreateFileW(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	virtualization_initialize(lpFileName);
	if (virtualization_check(lpFileName))
	{
		return Virtualized_CreateFileW(
			lpFileName,
			dwDesiredAccess,
			dwShareMode,
			lpSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile);
	}
	else
	{
		return Original_CreateFileW(
			lpFileName,
			dwDesiredAccess,
			dwShareMode,
			lpSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile);
	}
}

DWORD __stdcall Hooked_SetFilePointer(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod)
{
	if (virtualization_check(hFile))
	{
		return Virtualized_SetFilePointer(
			hFile,
			lDistanceToMove,
			lpDistanceToMoveHigh,
			dwMoveMethod);
	}
	else
	{
		return Original_SetFilePointer(
			hFile,
			lDistanceToMove,
			lpDistanceToMoveHigh,
			dwMoveMethod);
	}
}

DWORD __stdcall Hooked_SetFilePointerEx(
	HANDLE hFile,
	LARGE_INTEGER liDistanceToMove,
	PLARGE_INTEGER lpNewFilePointer,
	DWORD dwMoveMethod)
{
	if (virtualization_check(hFile))
	{
		return Virtualized_SetFilePointerEx(
			hFile,
			liDistanceToMove,
			lpNewFilePointer,
			dwMoveMethod);
	}
	else
	{
		return Original_SetFilePointerEx(
			hFile,
			liDistanceToMove,
			lpNewFilePointer,
			dwMoveMethod);
	}
}

BOOL __stdcall Hooked_ReadFile(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped)
{
	if (virtualization_check(hFile))
	{
		return Virtualized_ReadFile(
			hFile,
			lpBuffer,
			nNumberOfBytesToRead,
			lpNumberOfBytesRead,
			lpOverlapped);
	}
	else
	{
		return Original_ReadFile(
			hFile,
			lpBuffer,
			nNumberOfBytesToRead,
			lpNumberOfBytesRead,
			lpOverlapped);
	}
}

void hook_initialize()
{
	log_write("initializing hooks...");

	Original_GetFileAttributesW = (GetFileAttributesW_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetFileAttributesW");
	Original_GetFileAttributesExW = (GetFileAttributesExW_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetFileAttributesExW");
	Original_CreateFileW = (CreateFileW_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "CreateFileW");
	Original_SetFilePointer = (SetFilePointer_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "SetFilePointer");
	Original_SetFilePointerEx = (SetFilePointerEx_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "SetFilePointerEx");
	Original_ReadFile = (ReadFile_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "ReadFile");

	Mhook_SetHook((PVOID*)&Original_GetFileAttributesW, Hooked_GetFileAttributesW);
	Mhook_SetHook((PVOID*)&Original_GetFileAttributesExW, Hooked_GetFileAttributesExW);
	Mhook_SetHook((PVOID*)&Original_CreateFileW, Hooked_CreateFileW);
	Mhook_SetHook((PVOID*)&Original_SetFilePointer, Hooked_SetFilePointer);
	Mhook_SetHook((PVOID*)&Original_SetFilePointerEx, Hooked_SetFilePointerEx);
	Mhook_SetHook((PVOID*)&Original_ReadFile, Hooked_ReadFile);

	log_write("GetFileAttributesW: " + std::to_string((long)Original_GetFileAttributesW));
	log_write("GetFileAttributesExW: " + std::to_string((long)Original_GetFileAttributesExW));
	log_write("CreateFileW: " + std::to_string((long)Original_CreateFileW));
	log_write("SetFilePointer: " + std::to_string((long)Original_SetFilePointer));
	log_write("SetFilePointerEx: " + std::to_string((long)Original_SetFilePointerEx));
	log_write("ReadFile: " + std::to_string((long)Original_ReadFile));
}