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

#include "virtualization.h"
#include <stack>
#include <iostream>
#include "stream.h"
#include <Windows.h>
#include "cryptography.h"
#include "cryptography_hashes.h"
#include <array>
#include <random>
#include "log.h"
#include <Psapi.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <algorithm>

using namespace std;


typedef std::mt19937 MyRNG;
MyRNG rng;


bool RageVirtualizationManager::instanceFlag = false;
RageVirtualizationManager* RageVirtualizationManager::instance;




// virtualizes an rpf file...
HANDLE RageVirtualizationManager::Open(std::string rpfFileName)
{

	std::ifstream *rpfFile = new std::ifstream{ rpfFileName , std::ifstream::binary };
	return Open(rpfFile, rpfFileName);


}

HANDLE RageVirtualizationManager::Open(std::istream *rpfFile, std::string rpfFileName)
{
	log_write("virtualizing " + rpfFileName);
	
	boost::replace_all(rpfFileName, "\\", "/");

	rpfFile->seekg(0, std::istream::end);
	std::streamsize archSize = rpfFile->tellg();
	rpfFile->seekg(0, std::istream::beg);

	std::vector<std::string> strs;
	boost::trim(rpfFileName);
	boost::split(strs, rpfFileName, boost::is_any_of("\\/"), boost::algorithm::token_compress_on);

	std::string namex = strs[strs.size() - 1];
	auto q = hash_ng(namex.c_str(), namex.size());
	int keyIdx = ((unsigned int)(q + archSize + (101 - 40))) % 0x65;

	RageArchive *arch = new RageArchive();
	arch->readHeader(*rpfFile, AESKey, ngKeys[keyIdx]);

	stack<RageArchiveDirectory *> dirs;
	dirs.push(arch->getRoot());
	while (dirs.size() > 0)
	{
		auto srcDir = dirs.top();
		dirs.pop();

		// directories...
		for (auto d : srcDir->Directories)
		{
			dirs.push(d);			
		}

		// files...
		for (auto f : srcDir->Files)
		{
			if (dynamic_cast<RageArchiveBinaryFile *>(f) != nullptr)
			{
				auto oldFile = (RageArchiveBinaryFile *)f;

				if(oldFile->FileSize != 0)
				allVirtualizedEntries[oldFile] = new partial_streambuf( rpfFile ,oldFile->FileOffset * 512 ,oldFile->FileSize );
				allVirtualizedEntries[oldFile] = new partial_streambuf(rpfFile, oldFile->FileOffset * 512, oldFile->FileUncompressedSize);
			}
			else
			{
				auto oldFile = (RageArchiveResourceFile *)f;

				allVirtualizedEntries[oldFile] = new partial_streambuf( rpfFile ,oldFile->FileOffset * 512 ,oldFile->FileSize );
			}
		}

	}

	virtualizedArchives[rpfFileName] = arch;


	// TODO: return handle
	std::uniform_int_distribution<uint32_t> uint_dist;
	auto xyz = uint_dist(rng);
	
	log_write("virtualized " + rpfFileName + " : " + std::to_string(xyz));

	return (HANDLE)xyz;

}








RageArchiveDirectory* RageVirtualizationManager::Navigate(std::string targetDirectory)
{
	std::vector<std::string> strs;
	boost::split(strs, targetDirectory, boost::is_any_of("\\/"), boost::algorithm::token_compress_on);

	int state = 0;
	std::string currentFSPath = "";

	std::istream* currentSourceStream;
	int currentSourceBase = 0;

	RageArchive *currentArchive;
	RageArchiveDirectory *currentDirectory;

	std::vector<RageArchive *> archiveStack;

	for (auto tok : strs)
	{
		boost::trim(tok);
		if (tok.compare("") == 0)
			continue;

		if (state == 0)
		{
			// state 0:
			//  we are in the real file system

			if (boost::ends_with(tok, ".rpf"))
			{
				// string ends with .rpf
				//  create or open rpf file

				if (currentFSPath.size() > 0)
					currentFSPath.append("/" + tok); else
					currentFSPath.append(tok);

				if (virtualizedArchives.find(currentFSPath) != virtualizedArchives.end())
				{
					// rpf file exists AND is virtualized
					// -> select rpf file

					currentArchive = virtualizedArchives[currentFSPath];
					currentDirectory = currentArchive->getRoot();
					archiveStack.push_back(currentArchive);

					currentSourceStream = streams[handles[currentFSPath]];
					currentSourceBase = 0;
				}
				else
				{
					std::ifstream *rpfFile = new std::ifstream{ currentFSPath , std::ifstream::binary };
					if (*rpfFile)
					{
						// rpf file exists AND is not virtualized
						// -> virtualize rpf file

						HANDLE newHandle = Open(currentFSPath);
						handles[currentFSPath] = newHandle;
						streams[newHandle] = nullptr;

						currentArchive = virtualizedArchives[currentFSPath];
						currentDirectory = currentArchive->getRoot();
						archiveStack.push_back(virtualizedArchives[currentFSPath]);
					}
					else
					{
						// rpf file does not exist
						// -> create rpf file

						RageArchive *newArch = new RageArchive();

						// add to lists...
						HANDLE h = 0; // TODO: create handle
						handles[currentFSPath] = h;
						streams[h] = nullptr;


						virtualizedArchives[currentFSPath] = newArch;

						currentArchive = newArch;
						currentDirectory = currentArchive->getRoot();
						archiveStack.push_back(newArch);
					}
				}

				state = 1;
			}
			else
			{
				// string does not end with .rpf
				//  just change directory

				if (currentFSPath.size() > 0)
					currentFSPath.append("/" + tok); else
					currentFSPath.append(tok);
			}
		}
		else if (state == 1)
		{
			// state 1:
			//  we are in the virtual file system inside an archive

			currentFSPath.append("/" + tok);

			if (boost::ends_with(tok, ".rpf"))
			{
				// string ends with .rpf
				//  if file exists: open and, if necessary, virtualize embedded rpf file
				//  if file does not exist: create embedded rpf file

				if (virtualizedArchives.find(currentFSPath) != virtualizedArchives.end())
				{

					// already virtualizes...
					RageArchive * arch = virtualizedArchives[currentFSPath];

					currentArchive = arch;
					currentDirectory = currentArchive->getRoot();
					archiveStack.push_back(arch);

				}
				else
				{

					RageArchiveBinaryFile* rpfFileEntry = nullptr;
					for (auto d : currentDirectory->Files)
						if (d->Name.compare(tok) == 0)
							rpfFileEntry = (RageArchiveBinaryFile*)d;

					if (rpfFileEntry != nullptr)
					{
						// virtualize...

						std::streambuf *streamxx_buf = allVirtualizedEntries[rpfFileEntry];
						std::istream *streamxx = new std::istream(streamxx_buf);

						Open(streamxx, currentFSPath);

						auto arch = virtualizedArchives[currentFSPath];

						currentArchive = arch;
						currentDirectory = currentArchive->getRoot();
						archiveStack.push_back(arch);

						virtualiedRPFFiles[rpfFileEntry] = arch;
					}
					else
					{
						// create...


					}

				}

			}
			else
			{
				// string does not end with .rpf
				//  just change directory

				RageArchiveDirectory* nextDir = nullptr;
				for (auto d : currentDirectory->Directories)
					if (d->Name.compare(tok) == 0)
						nextDir = d;

				if (nextDir != nullptr)
				{
					currentDirectory = nextDir;
				}
				else
				{
					nextDir = new RageArchiveDirectory();
					nextDir->Name = tok;
					currentDirectory->Directories.push_back(nextDir);

					currentDirectory = nextDir;
				}
			}

		}
	}

	return currentDirectory;
}

void RageVirtualizationManager::Import(std::string targetDirectory, std::string importFileName)
{
	RageArchiveDirectory* currentDirectory = Navigate(targetDirectory);
	
	// now we are in the corrent directory...
	// we still need to import our file...

	std::vector<std::string> importNameTokens;
	boost::split(importNameTokens, importFileName, boost::is_any_of("\\/"), boost::algorithm::token_compress_on);

	std::string importNameOnly = importNameTokens[importNameTokens.size() - 1];

	RageArchiveBinaryFile* rpfFileEntry = nullptr;

	for (std::vector<RageArchiveFileEntry *>::iterator it = currentDirectory->Files.begin(); it != currentDirectory->Files.end(); ++it) {
		if ((*it)->Name.compare(importNameOnly) == 0)
		{
			//log_write("remove " + (*it)->Name);
			currentDirectory->Files.erase(it);		
			break;
		}
	}

	// open...
	std::ifstream *importFileStr = new std::ifstream{ importFileName , std::ifstream::binary };

	// determine file size...
	importFileStr->seekg(0, std::ios::end);
	std::streamsize impFSize = importFileStr->tellg();
	importFileStr->seekg(0, std::ios::beg);

	// determine file type...
	// read first 4 bytes to determine file type...
	unsigned int ident = 0;
	importFileStr->read((char *)&ident, 4);


	if (rpfFileEntry != nullptr)
	{
		log_write("SHOULD NOT BE POSSIBLE!!!");
	}
	else
	{
		//log_write("found NO entry -> create");

		if (ident != 0x37435352)
		{
			// binary file...

			importFileStr->seekg(0, std::ios::beg);


			RageArchiveBinaryFile* binFile = new RageArchiveBinaryFile();
			binFile->Name = importNameOnly; // name
			binFile->FileOffset = 0; // dummy
			binFile->FileSize = 0; // not compressed
			binFile->FileUncompressedSize = impFSize; // size
			binFile->IsEncrypted = 0; // not encypted

			currentDirectory->Files.push_back(binFile);


			allVirtualizedEntries[binFile] = new partial_streambuf{ importFileStr ,0 ,(size_t)impFSize };

			//log_write("imported binary file:");
		}
		else
		{
			// resource file...

			RageArchiveResourceFile* binFile = new RageArchiveResourceFile();

			binFile->Name = importNameOnly; // name
			binFile->FileOffset = 0; // dummy
			binFile->FileSize = impFSize; // size
			importFileStr->seekg(8, std::ios::beg);
			importFileStr->read((char *)&binFile->SystemFlags, 4); // system flags
			importFileStr->read((char *)&binFile->GraphicsFlags, 4); // graphic flags
			importFileStr->seekg(0, std::ios::beg);

			currentDirectory->Files.push_back(binFile);


			allVirtualizedEntries[binFile] = new partial_streambuf{ importFileStr ,0 ,(size_t)impFSize };
		}

		//log_write("created");
	}

}

void RageVirtualizationManager::Delete(std::string targetDirectory, std::string fileName)
{
	RageArchiveDirectory* currentDirectory = Navigate(targetDirectory);

	for (std::vector<RageArchiveFileEntry *>::iterator it = currentDirectory->Files.begin(); it != currentDirectory->Files.end(); ++it) {
		if ((*it)->Name.compare(fileName) == 0)
		{
			//log_write("remove " + (*it)->Name);
			currentDirectory->Files.erase(it);
			break;
		}
	}
}

void RageVirtualizationManager::Rebuild()
{

	// we have a filesystem now...
	// there are some rpfs inside that need to be virtualized...

	log_write("started rebuilding...");

	// 1) find all rpf files that need a rebuild
	vector<RageArchive *> archivesToRebuild;
	stack<RageArchiveDirectory *> tododirs;

	map<RageArchive*, RageArchiveFileEntry*> xxentries;
	map<RageArchive*, HANDLE> xxhandles;

	for (auto x : handles)
	{
		// add all root archives to stack
		archivesToRebuild.push_back(virtualizedArchives[x.first]);
		tododirs.push(virtualizedArchives[x.first]->getRoot());

		// is a MAIN archive!!!
		xxhandles[virtualizedArchives[x.first]] = x.second;

		log_write("found real archive: " + x.first);
	}

	while (tododirs.size() > 0)
	{
		auto ddd = tododirs.top();
		tododirs.pop();

		for (auto d : ddd->Directories)
		{
			tododirs.push(d);
		}

		for (auto d : ddd->Files)
		{

			// a file could now be:
			//  virtualized rpf file -> add to 'todo-list'

			if (virtualiedRPFFiles.find(d) != virtualiedRPFFiles.end())
			{
				archivesToRebuild.push_back(virtualiedRPFFiles[d]);
				tododirs.push(virtualiedRPFFiles[d]->getRoot());
				xxentries[virtualiedRPFFiles[d]] = d;

				log_write("found embedded archive: " + d->Name);
			}

		}
	}

	// 2) now we have a list of archives that need to be rebuilt
	//    so rebuild them...
	std::reverse(archivesToRebuild.begin(), archivesToRebuild.end());
	for (auto ar : archivesToRebuild)
	{
		//log_write("rebuilding an archive...");

		// for each rpf we build a list of parts...
		std::vector<virtual_stream_part > parts;

		stack<RageArchiveDirectory *> tododirsxx;
		tododirsxx.push(ar->getRoot());

		parts.push_back(virtual_stream_part());

		int blockoffset = 4096;

		while (tododirsxx.size() > 0)
		{

			auto ddd = tododirsxx.top();
			tododirsxx.pop();

			for (auto d : ddd->Directories)
			{
				tododirsxx.push(d);
			}

			for (auto d : ddd->Files)
			{

				virtual_stream_part thepart;

				std::streambuf *thebuff = allVirtualizedEntries[d];
				thepart.baseStream = new istream{ thebuff };

				thepart.baseStream->seekg(0, std::ios::end);
				std::streamsize impFSize = thepart.baseStream->tellg();
				thepart.baseStream->seekg(0, std::ios::beg);

				thepart.offset_in_file = 0;
				thepart.length = impFSize;
				thepart.offset = blockoffset * 512;

				d->FileOffset = blockoffset;

				blockoffset += (thepart.length / 512) + 10;

				parts.push_back(thepart);
			}

		}

		// finish virtual stream
		char *theBuf = new char[1024 * 1024]{};
		membuf sbuf(theBuf, 1024 * 1024);
		std::ostream outstr(&sbuf);
		int len = ar->writeHeader(outstr, nullptr, nullptr);

		char *rpfHeader = new char[len];
		memcpy(rpfHeader, theBuf, len);
		delete[] theBuf;

		membuf *sbuf22 = new membuf(rpfHeader, len);
		std::istream *qq = new std::istream(sbuf22);
		parts[0].baseStream = qq;
		parts[0].offset = 0;
		parts[0].offset_in_file = 0;
		parts[0].length = len;


		if (xxentries.find(ar) != xxentries.end())
		{
			std::streambuf *newstreambuf = new virtual_streambuf(parts);
			//std::istream *thenewstream = new std::istream(newstreambuf);

			RageArchiveFileEntry *xxee = xxentries[ar];

			// update parent's file information
			RageArchiveBinaryFile *xee2222 = (RageArchiveBinaryFile*)xxee;
			xee2222->FileUncompressedSize = blockoffset * 512; // size

			allVirtualizedEntries[xxee] = newstreambuf;

			log_write("rebuilt embedded archive");
		}
		else
		{
			// we build a new stream for this handle...
			std::streambuf *newstreambuf = new virtual_streambuf(parts);
			std::istream *thenewstream = new std::istream(newstreambuf);

			streams[xxhandles[ar]] = thenewstream;

			log_write("rebuilt real archive");
		}


	}

}









// check if file is virtualized
bool VirtualizationManager::IsVirtualized(std::string fileName)
{
	return handles.find(fileName) != handles.end();
}

bool VirtualizationManager::IsVirtualized(HANDLE handle)
{
	return streams.find(handle) != streams.end();
}

std::streampos VirtualizationManager::tell(HANDLE handle)
{
	return streams[handle]->tellg();
}

void VirtualizationManager::seek(HANDLE handle, std::streampos offset)
{
	streams[handle]->seekg(offset);
}

void VirtualizationManager::read(HANDLE handle, char *buffer, int count)
{
	streams[handle]->read(buffer, count);
}













DWORD_PTR GetProcessBaseAddress(DWORD processID)
{
	DWORD_PTR   baseAddress = 0;
	HANDLE      processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
	HMODULE     *moduleArray;
	LPBYTE      moduleArrayBytes;
	DWORD       bytesRequired;

	if (processHandle)
	{
		if (EnumProcessModules(processHandle, NULL, 0, &bytesRequired))
		{
			if (bytesRequired)
			{
				moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

				if (moduleArrayBytes)
				{
					unsigned int moduleCount;

					moduleCount = bytesRequired / sizeof(HMODULE);
					moduleArray = (HMODULE *)moduleArrayBytes;

					if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired))
					{
						baseAddress = (DWORD_PTR)moduleArray[0];
					}

					LocalFree(moduleArrayBytes);
				}
			}
		}

		CloseHandle(processHandle);
	}

	return baseAddress;
}


std::string MBFromW(LPCWSTR pwsz, UINT cp) {
	int cch = WideCharToMultiByte(cp, 0, pwsz, -1, 0, 0, NULL, NULL);

	char* psz = new char[cch];

	WideCharToMultiByte(cp, 0, pwsz, -1, psz, cch, NULL, NULL);

	std::string st(psz);
	delete[] psz;

	return st;
}






bool isInitializing = false;
bool isInitialized = false;


/*
	Initializes the virtualization.
*/
void virtualization_initialize(LPCTSTR lpFileName)
{
	if (isInitializing || isInitialized)
		return;

	std::wstring wss{ lpFileName };
	if (!boost::algorithm::ends_with(wss, ".rpf"))
		return;


	log_write("initialize virtualization... ");

	rng.seed();
	isInitializing = true;



	auto pid = GetCurrentProcessId();
	log_write("process id: " + std::to_string(pid));

	auto mainmodptr = GetProcessBaseAddress(pid);
	log_write("process address: " + std::to_string(mainmodptr));



	// open key file
	string datFileName{ "gta5key.dat" };
	ifstream datFile{ datFileName, std::ifstream::binary };



	// get keys addresses
	UINT32 aesKeyAddress = 0;
	array<UINT32, 101> ngKeyAddresses{};
	array<UINT32, 272> ngTableAddresses{};
	UINT32 lutAddress;

	if (datFile)
	{
		log_write("key file found -> read");

		datFile.read((char *)&aesKeyAddress, 4);
		datFile.read((char *)ngKeyAddresses.data(), 4 * 101);
		datFile.read((char *)ngTableAddresses.data(), 4 * 272);
		datFile.read((char *)&lutAddress, 4);
	}
	else
	{
		log_write("key file not found -> create");
		int searchLength = 0x02000000;
		unsigned char *mybuf = new unsigned char[searchLength];
		size_t bytesRead;
		DWORD oldProtect;

		HANDLE      processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		VirtualProtectEx(processHandle, (LPVOID)mainmodptr, searchLength, PAGE_EXECUTE_READWRITE, &oldProtect);
		ReadProcessMemory(processHandle, (LPCVOID)mainmodptr, mybuf, searchLength, &bytesRead);

		bool b1 = find_hash((byte *)mybuf, searchLength, (const char *)PC_AES_KEY_HASH, &aesKeyAddress, 0x20);
		bool b2 = find_hashes((byte *)mybuf, searchLength, (const char *)PC_NG_KEY_HASHES, 20, 101, ngKeyAddresses.data(), 0x110);
		bool b3 = find_hashes((byte *)mybuf, searchLength, (const char *)PC_NG_DECRYPT_TABLE_HASHES, 20, 272, ngTableAddresses.data(), 0x400);
		bool b4 = find_hash((byte *)mybuf, searchLength, (const char *)PC_LUT_HASH, &lutAddress, 0x100);

		ofstream datOutFile{ datFileName,ofstream::binary };
		datOutFile.write((char *)&aesKeyAddress, 4);
		datOutFile.write((char *)ngKeyAddresses.data(), 4 * 101);
		datOutFile.write((char *)ngTableAddresses.data(), 4 * 272);
		datOutFile.write((char *)&lutAddress, 4);
		datOutFile.close();
	}



	// get keys
	AESKey = (char *)mainmodptr + aesKeyAddress;
	ngKeys = new char*[101];
	for (int i = 0; i < 101; i++)
		ngKeys[i] = (char *)mainmodptr + ngKeyAddresses[i];
	ngTables = new char*[272];
	for (int i = 0; i < 272; i++)
		ngTables[i] = (char *)mainmodptr + ngTableAddresses[i];
	LUT = (char *)mainmodptr + lutAddress;




	std::wstring packPath = L".\\packages\\*";

	WIN32_FIND_DATA fnddat;
	auto fhnd = FindFirstFileEx(packPath.c_str(), FindExInfoStandard, &fnddat, FindExSearchLimitToDirectories, NULL, FIND_FIRST_EX_CASE_SENSITIVE);
	while (fhnd != INVALID_HANDLE_VALUE)
	{
		try
		{
			using boost::property_tree::ptree;
			ptree pt;

			string xmlfilename = "packages\\" + MBFromW(fnddat.cFileName, 0) + "\\package.config";
			log_write("opening " + xmlfilename);

			read_xml(xmlfilename, pt);

			auto importsss = pt.get_child("package");
			for (auto xx : importsss)
			{
				auto theimport = xx.second;

				

				auto targetDirValue = theimport.get<string>("<xmlattr>.targetDir");
				auto fileValue = "packages\\" + MBFromW(fnddat.cFileName, 0) + "\\" + theimport.get<string>("<xmlattr>.file");
				//auto fileValue = theimport.get_child("<xmlattr>").get_value("file");

				log_write("importing " + fileValue + " into " + targetDirValue);
				RageVirtualizationManager::GetInstance()->Import(targetDirValue, fileValue);
			}
		}
		catch(...)
		{
		
		}
		
		auto ii = FindNextFile(fhnd, &fnddat);
		if (!ii)
			break;
	}

	RageVirtualizationManager::GetInstance()->Rebuild();

	isInitialized = true;
	isInitializing = false;
}

/*
	Checks if a file is virtualized using the file name.
*/
bool virtualization_check(LPCTSTR lpFileName)
{
	std::wstring x(lpFileName);
	std::string y(x.begin(), x.end());
	auto vman = RageVirtualizationManager::GetInstance();
	return vman->IsVirtualized(y);
}

/*
	Checks if a file is virtualized using the file handle.
*/
bool virtualization_check(HANDLE hFile)
{
	auto vman = RageVirtualizationManager::GetInstance();
	return vman->IsVirtualized(hFile);
}










/********************************************************************************************
	Virtualized WinAPI calls
********************************************************************************************/

DWORD __stdcall Virtualized_GetFileAttributesW(
	LPCTSTR lpFileName)
{
	return FILE_ATTRIBUTE_NORMAL;
}

BOOL __stdcall Virtualized_GetFileAttributesExW(
	LPCTSTR lpFileName,
	GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation)
{
	string fileName = MBFromW(lpFileName, 0);


	auto instance = RageVirtualizationManager::GetInstance();
	
	auto xhandle = instance->handles[fileName];
	auto xstream = instance->streams[xhandle];

	std::streampos posbak = xstream->tellg();
	xstream->seekg(0, std::ios::end);
	std::streamsize length = xstream->tellg();
	xstream->seekg(posbak, std::ios::beg);

	LPWIN32_FILE_ATTRIBUTE_DATA info = (LPWIN32_FILE_ATTRIBUTE_DATA)lpFileInformation;
	info->nFileSizeLow = (DWORD)length;
	info->nFileSizeHigh = 0;
	info->ftCreationTime.dwLowDateTime = 0x209b3108;
	info->ftCreationTime.dwHighDateTime = 0x01d093af;
	info->ftLastAccessTime.dwLowDateTime = 0x209b3108;
	info->ftLastAccessTime.dwHighDateTime = 0x01d093af;
	info->ftLastWriteTime.dwLowDateTime = 0x209b3108;
	info->ftLastWriteTime.dwHighDateTime = 0x01d093af;
	info->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	return true;
}

HANDLE __stdcall Virtualized_CreateFileW(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	string fileName = MBFromW(lpFileName, 0);

	auto instance = RageVirtualizationManager::GetInstance();
	return instance->handles[fileName];
}

DWORD __stdcall Virtualized_SetFilePointer(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod)
{
	auto instance = RageVirtualizationManager::GetInstance();
	auto xstream = instance->streams[hFile];

	if (lpDistanceToMoveHigh == 0)
	{
		// 32bit value
		LONGLONG v = (LONGLONG)lDistanceToMove;
		xstream->seekg(v, std::ios::beg);
	}
	else
	{
		// 64bit value
		UINT32 v1 = (UINT32)lDistanceToMove;
		UINT32 v2 = (UINT32)(*lpDistanceToMoveHigh);
		LONGLONG v = (LONGLONG)((UINT64)v1 | (UINT64)v2 << 32);
		xstream->seekg(v, std::ios::beg);
	}

	return true;
}

DWORD __stdcall Virtualized_SetFilePointerEx(
	HANDLE hFile,
	LARGE_INTEGER liDistanceToMove,
	PLARGE_INTEGER lpNewFilePointer,
	DWORD dwMoveMethod)
{
	auto instance = RageVirtualizationManager::GetInstance();
	auto xstream = instance->streams[hFile];

	switch (dwMoveMethod)
	{
	case FILE_BEGIN:
		xstream->seekg(liDistanceToMove.QuadPart, std::ios::beg);
		break;
	case FILE_CURRENT:
		xstream->seekg(liDistanceToMove.QuadPart, std::ios::cur);
		break;
	case FILE_END:
		xstream->seekg(liDistanceToMove.QuadPart, std::ios::end);
		break;
	}

	if (lpNewFilePointer != 0)
		lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;

	return true;
}

BOOL __stdcall Virtualized_ReadFile(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped)
{
	auto instance = RageVirtualizationManager::GetInstance();
	auto xstream = instance->streams[hFile];

	if (lpOverlapped != 0)
	{
		LONGLONG position = 0;
		position = lpOverlapped->Offset;
		position |= (lpOverlapped->OffsetHigh << 32);
		xstream->seekg(position, std::ios::beg);
	}

	xstream->read((char *)lpBuffer, nNumberOfBytesToRead);

	if (lpOverlapped != 0)
	{
		lpOverlapped->Internal = 18446708893632430080;
		lpOverlapped->InternalHigh = nNumberOfBytesToRead;
	}

	if (lpNumberOfBytesRead != 0)
		*lpNumberOfBytesRead = nNumberOfBytesToRead;

	return true;
}