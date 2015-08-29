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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

enum class RageArchiveEncryption
{
	None,
	AES,
	NG
};

/*
	Represents an item in an RPF7 archive.
*/
class RageArchiveEntry
{
public:
	unsigned int NameOffset;
	std::string Name;

	virtual ~RageArchiveEntry() {}
};

/*
	Represents a file in an RPF7 archive.
*/
class RageArchiveFileEntry : public RageArchiveEntry
{
public:
	unsigned int FileOffset;
};

/*
	Represents a binary file in an RPF7 archive.
*/
class RageArchiveBinaryFile : public RageArchiveFileEntry
{
public:
	unsigned int FileSize;	
	unsigned int FileUncompressedSize;
	unsigned int IsEncrypted;
};

/*
	Represents a resource file in an RPF7 archive.
*/
class RageArchiveResourceFile : public RageArchiveFileEntry
{
public:	
	unsigned int FileSize;
	unsigned int SystemFlags;
	unsigned int GraphicsFlags;

	
};

/*
	Represents a directory in an RPF7 archive.
*/
class RageArchiveDirectory : public RageArchiveEntry
{
public:
	unsigned int EntriesIndex;
	unsigned int EntriesCount;

	std::vector<RageArchiveDirectory *> Directories;
	std::vector<RageArchiveFileEntry *> Files;
};

/*
	Represents an RPF7 archive.
*/
class RageArchive
{
private:
	RageArchiveEncryption Encryption;
	RageArchiveDirectory* Root;

public:

	RageArchive();
	~RageArchive();

	/*
		Reads the header from a stream.
	*/
	void readHeader(std::istream &stream, char *aesKey, char *ngKey);

	/*
		Writes the header to a stream.
	*/
	int writeHeader(std::ostream &stream, char *aesKey, char *ngKey);

	RageArchiveDirectory* getRoot();
	RageArchiveEncryption getEncryption();
	void setEncryption(RageArchiveEncryption encryption);

};
