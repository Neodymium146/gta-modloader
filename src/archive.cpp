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

#include "archive.h"
#include "cryptography.h"
#include <vector>
#include <stack>
#include <typeinfo>
#include "virtualization.h"
#include "log.h"
#include <fstream>

using namespace std;

RageArchive::RageArchive()
{
	Encryption = RageArchiveEncryption::None;
	Root = new RageArchiveDirectory();
}

RageArchive::~RageArchive()
{
	delete Root;
}

void RageArchive::readHeader(std::istream &stream, char *aesKey, char *ngKey)
{
	unsigned int header_identifier;
	unsigned int header_entriesCount;
	unsigned int header_namesLength;
	unsigned int header_encryption;
	stream.read((char *)&header_identifier, 4);
	stream.read((char *)&header_entriesCount, 4);
	stream.read((char *)&header_namesLength, 4);
	stream.read((char *)&header_encryption, 4);

	char *entries_data_dec = new char[header_entriesCount * 16]{};
	char *names_data_dec = new char[header_namesLength] {};
	int ii = header_entriesCount * 16;
	stream.read(entries_data_dec, ii);
	stream.read(names_data_dec, header_namesLength);

	Encryption = RageArchiveEncryption::None;

	if (header_encryption == 0x04E45504F) // for OpenIV compatibility
	{
		// no encryption...
		//Encryption = RageArchiveEncryption::None;

		//log_write("rpf: no encryption");
	}
	else if (header_encryption == 0x0ffffff9)
	{
		// AES enceyption...     
		//Encryption = RageArchiveEncryption::AES;
		decrypt_aes((byte *)entries_data_dec, header_entriesCount * 16, (byte *)aesKey);
		decrypt_aes((byte *)names_data_dec, header_namesLength, (byte *)aesKey);

		//log_write("rpf: aes encryption");
	}
	else
	{
		// NG encryption...
		//Encryption = RageArchiveEncryption::NG;
		decrypt_ng((unsigned char *)entries_data_dec, header_entriesCount * 16, ngKey);
		decrypt_ng((unsigned char *)names_data_dec, header_namesLength, ngKey);

		//log_write("rpf: ng encryption");
	}

	vector<RageArchiveEntry *> entries;
	for (int i = 0; i < header_entriesCount; i++)
	{
		unsigned int e = *(unsigned int *)(entries_data_dec + 16 * i + 4);
		if (e == 0x7fffff00)
		{
			// directory
			RageArchiveDirectory *d = new RageArchiveDirectory();
			memcpy(&d->NameOffset, entries_data_dec + 16 * i + 0, 4);
			memcpy(&d->EntriesIndex, entries_data_dec + 16 * i + 8, 4);
			memcpy(&d->EntriesCount, entries_data_dec + 16 * i + 12, 4);
			entries.push_back(d);

			std::string name{ names_data_dec + d->NameOffset };
			d->Name = name;
		}
		else
		{
			if ((e & 0x80000000) == 0)
			{
				// binary file
				RageArchiveBinaryFile *f = new RageArchiveBinaryFile();
				memcpy(&f->NameOffset, entries_data_dec + 16 * i + 0, 2);
				memcpy(&f->FileSize, entries_data_dec + 16 * i + 2, 3);
				memcpy(&f->FileOffset, entries_data_dec + 16 * i + 5, 3);
				memcpy(&f->FileUncompressedSize, entries_data_dec + 16 * i + 8, 4);
				memcpy(&f->IsEncrypted, entries_data_dec + 16 * i + 12, 4);
				entries.push_back(f);

				std::string name{ names_data_dec + f->NameOffset };
				f->Name = name;
			}
			else
			{
				// resource file
				RageArchiveResourceFile *f = new RageArchiveResourceFile();
				memcpy(&f->NameOffset, entries_data_dec + 16 * i + 0, 2);
				memcpy(&f->FileSize, entries_data_dec + 16 * i + 2, 3);
				memcpy(&f->FileOffset, entries_data_dec + 16 * i + 5, 3);
				memcpy(&f->SystemFlags, entries_data_dec + 16 * i + 8, 4);
				memcpy(&f->GraphicsFlags, entries_data_dec + 16 * i + 12, 4);
				entries.push_back(f);

				f->FileOffset &= ~0x800000;

				std::string name{ names_data_dec + f->NameOffset };
				f->Name = name;
			}
		}
	}

	stack<RageArchiveDirectory *> stack;
	stack.push((RageArchiveDirectory *)entries[0]);
	Root = (RageArchiveDirectory *)entries[0];

	while (stack.size() > 0)
	{
		RageArchiveDirectory *item = stack.top();
		stack.pop();

		for (int i = item->EntriesIndex; i < (item->EntriesIndex + item->EntriesCount); i++)
		{
			if (dynamic_cast<RageArchiveDirectory *>(entries[i]) != nullptr)
			{
				item->Directories.push_back((RageArchiveDirectory *)entries[i]);
				stack.push((RageArchiveDirectory *)entries[i]);
			}
			else
			{
				item->Files.push_back((RageArchiveFileEntry *)entries[i]);
			}
		}
	}

	delete[] entries_data_dec;
	delete[] names_data_dec;
}

int RageArchive::writeHeader(std::ostream &stream, char *aesKey, char *ngKey)
{

	vector<RageArchiveEntry *> entries;
	entries.push_back(Root);

	stack<RageArchiveDirectory *> stack;
	stack.push(Root);
	int nameOffset = 1;

	map<string, int> nameDict;
	nameDict[""] = 0;
	vector<string> names;
	string nn = "";
	names.push_back(nn);

	while (stack.size() > 0)
	{
		auto directory = stack.top();
		stack.pop();
		
		directory->EntriesIndex = entries.size();
		directory->EntriesCount = directory->Directories.size() + directory->Files.size();

		vector<RageArchiveEntry *> theList;

		for (auto x : directory->Directories)
		{
			if (nameDict.find(x->Name) == nameDict.end())
			{
				nameDict[x->Name] = nameOffset;
				nameOffset += x->Name.size() + 1;

				names.push_back(x->Name);
			}
			x->NameOffset = nameDict[x->Name];

			theList.push_back(x);
		}

		for (auto x : directory->Files)
		{
			if (nameDict.find(x->Name) == nameDict.end())
			{
				nameDict[x->Name] = nameOffset;
				nameOffset += x->Name.size() + 1;

				names.push_back(x->Name);
			}
			x->NameOffset = nameDict[x->Name];

			theList.push_back(x);
		}

		auto sortFun = [](const RageArchiveEntry* a, const RageArchiveEntry* b) {
			int z= b->Name.compare(a->Name);
			return z>0;
		};
		std::sort(theList.begin(), theList.end(), sortFun); // assume this is ordinal compare...

		for (auto x : theList)
		{
			entries.push_back(x);
		}
		for (int k = theList.size() - 1; k >= 0; k--)
		{
			if (dynamic_cast<RageArchiveDirectory *>(theList[k]) != nullptr)
			{
				stack.push((RageArchiveDirectory *)theList[k]);
			}
		}

	}



	if (nameOffset % 16 > 0)
		nameOffset += 16 - (nameOffset % 16);


	char *entries_data_dec = new char[16 * entries.size()]{};
	char *names_data_dec = new char[nameOffset] {};

	// write entries to buffer...
	membuf sbuf_entries(entries_data_dec, 16 * entries.size());
	std::ostream outstr_entries(&sbuf_entries);
	for (auto x : entries)
	{
		if (dynamic_cast<RageArchiveDirectory *>(x) != nullptr)
		{
			unsigned int d_const = 0x7FFFFF00;

			// directory...
			auto y = (RageArchiveDirectory *)x;
			outstr_entries.write((char *)&y->NameOffset, 4);
			outstr_entries.write((char *)&d_const, 4);
			outstr_entries.write((char *)&y->EntriesIndex, 4);
			outstr_entries.write((char *)&y->EntriesCount, 4);
		}
		else
		{
			if (dynamic_cast<RageArchiveBinaryFile *>(x) != nullptr)
			{
				// binary file...
				auto y = (RageArchiveBinaryFile *)x;
				outstr_entries.write((char *)&y->NameOffset, 2);
				outstr_entries.write((char *)&y->FileSize, 3);
				outstr_entries.write((char *)&y->FileOffset, 3);
				outstr_entries.write((char *)&y->FileUncompressedSize, 4);
				outstr_entries.write((char *)&y->IsEncrypted, 4);
			}
			else
			{
				// resource file...
				auto y = (RageArchiveResourceFile *)x;

				y->FileOffset |= 0x800000;
				outstr_entries.write((char *)&y->NameOffset, 2);
				outstr_entries.write((char *)&y->FileSize, 3);
				outstr_entries.write((char *)&y->FileOffset, 3);
				outstr_entries.write((char *)&y->SystemFlags, 4);
				outstr_entries.write((char *)&y->GraphicsFlags, 4);
				y->FileOffset &= ~0x800000;
			}
		}
	}
	outstr_entries.flush();

	// write names to buffer...
	membuf sbuf_names(names_data_dec, nameOffset);
	std::ostream outstr_names(&sbuf_names);

	for (auto x : names)
	{
		auto p = x.c_str(); // assume this is null-terminated!
		outstr_names.write(p, x.size() + 1);
	}
	outstr_names.flush();


	unsigned int header_identifier = 0x52504637;
	unsigned int header_entriesCount = entries.size();
	unsigned int header_namesLength = nameOffset;	
	unsigned int header_encryption = 0;

	switch (Encryption)
	{
	case RageArchiveEncryption::None:
		header_encryption = 0x04E45504F;
		break;
	case RageArchiveEncryption::AES:
		header_encryption = 0x0ffffff9;
		encrypt_aes((byte *)entries_data_dec, header_entriesCount * 16, (byte *)aesKey);
		encrypt_aes((byte *)names_data_dec, header_namesLength, (byte *)aesKey);
		break;
	case RageArchiveEncryption::NG:
		header_encryption = 0x0fefffff;
		encrypt_ng((unsigned char *)entries_data_dec, header_entriesCount * 16, ngKey);
		encrypt_ng((unsigned char *)names_data_dec, header_namesLength, ngKey);
		break;
	}
	
	//stream.seekp(0, ios::beg);
	stream.write((char *)&header_identifier, 4);
	stream.write((char *)&header_entriesCount, 4);
	stream.write((char *)&header_namesLength, 4);
	stream.write((char *)&header_encryption, 4);
	stream.write(entries_data_dec, header_entriesCount * 16);
	stream.write(names_data_dec, header_namesLength);
	stream.flush();

	delete[] entries_data_dec;
	delete[] names_data_dec;

	return 16 + header_entriesCount * 16 + header_namesLength;
}

RageArchiveDirectory* RageArchive::getRoot()
{
	return this->Root;
}

RageArchiveEncryption RageArchive::getEncryption()
{
	return this->Encryption;
}

void RageArchive::setEncryption(RageArchiveEncryption encryption)
{
	this->Encryption = encryption;
}
