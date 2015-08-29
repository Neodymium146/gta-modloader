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

#pragma once

#include <Windows.h>
#include <iostream>
#include <map>
#include <algorithm> 
#include <vector>
#include "archive.h"
#include <boost/algorithm/string.hpp>
#include "cryptography.h"
#include "stream.h"
#include <iostream>
#include <set>

/*
	...
*/
class VirtualizationManager
{

protected:
	

public:

	std::map<std::string, HANDLE> handles;
	std::map<HANDLE, std::istream*> streams;

	// check if file is virtualized
	bool IsVirtualized(std::string fileName);
	bool IsVirtualized(HANDLE handle);

	std::streampos tell(HANDLE handle);
	void seek(HANDLE handle, std::streampos offset);
	void read(HANDLE handle, char *buffer, int count);

};

/*
	...
*/
class RageVirtualizationManager: public VirtualizationManager
{
private:
	static bool instanceFlag;
	static RageVirtualizationManager* instance;

	RageVirtualizationManager()
	{
		instanceFlag = true;
	}

	std::map<std::string, RageArchive *> virtualizedArchives;

	// for each virtual
	std::map<RageArchiveFileEntry*, std::streambuf*> allVirtualizedEntries;

	std::map<RageArchiveFileEntry *, RageArchive* > virtualiedRPFFiles;


public:
	static RageVirtualizationManager* GetInstance()
	{
		if (instanceFlag == false)
			instance = new RageVirtualizationManager();

		return instance;
	}

	HANDLE Open(std::string rpfFile);
	HANDLE Open(std::istream *rpfFile, std::string rpfFileName);

	RageArchiveDirectory* Navigate(std::string targetDirectory);
	void Import(std::string targetDirectory, std::string importFileName);
	void Delete(std::string targetDirectory, std::string fileName);
	void Rebuild();

};














/*
	Initializes the virtualization.
*/
void virtualization_initialize(LPCTSTR lpFileName);

/*
	Checks if a file is virtualized using the file name.
*/
bool virtualization_check(LPCTSTR lpFileName);

/*
	Checks if a file is virtualized using the file handle.
*/
bool virtualization_check(HANDLE hFile);





/*************************************************************************************
	Virtualized WinAPI calls...
/*************************************************************************************/

DWORD __stdcall Virtualized_GetFileAttributesW(
	LPCTSTR lpFileName
);

BOOL __stdcall Virtualized_GetFileAttributesExW(
	LPCTSTR lpFileName,
	GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation
);

HANDLE __stdcall Virtualized_CreateFileW(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
);

DWORD __stdcall Virtualized_SetFilePointer(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod
);

DWORD __stdcall Virtualized_SetFilePointerEx(
	HANDLE hFile,
	LARGE_INTEGER liDistanceToMove,
	PLARGE_INTEGER lpNewFilePointer,
	DWORD dwMoveMethod
);

BOOL __stdcall Virtualized_ReadFile(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
);
