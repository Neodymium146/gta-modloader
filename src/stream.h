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
#include <vector>
#include <algorithm>
#include "log.h"

class membuf : public std::streambuf
{
public:
	char *baseptr;

	membuf(char* base, std::size_t size);

	std::size_t written() const;
	std::size_t read() const;

	virtual pos_type seekoff(off_type off,
		std::ios_base::seekdir way,
		std::ios_base::openmode which);

	virtual pos_type seekpos(pos_type sp,
		std::ios_base::openmode which);

};

class partial_streambuf : public std::streambuf
{
private:
	std::vector<char> buffer_;
	std::streampos internalPosition;

public:

	std::istream* source_;
	std::streamoff offset_;
	std::size_t length_;

	partial_streambuf();
	partial_streambuf(std::istream* source, std::streamoff offset, std::size_t length);

	// Get character on underflow
	int_type underflow();

	// Set internal position pointer to relative position
	virtual pos_type seekoff(off_type off,
		std::ios_base::seekdir way,
		std::ios_base::openmode which);

	// Set internal position pointer to absolute position
	virtual pos_type seekpos(pos_type sp,
		std::ios_base::openmode which);

};

class virtual_stream_part
{
public:
	std::istream *baseStream;
	long offset;
	long offset_in_file;
	long length;
};

class virtual_streambuf : public std::streambuf
{
public:
	std::vector<virtual_stream_part > parts;

private:
	std::vector<char> buffer_;
	std::streampos internalPosition;

	// Get character on underflow
	int_type underflow();

	// Set internal position pointer to relative position
	virtual pos_type seekoff(off_type off,
		std::ios_base::seekdir way,
		std::ios_base::openmode which);

	// Set internal position pointer to absolute position
	virtual pos_type seekpos(pos_type sp,
		std::ios_base::openmode which);

public:

	virtual_streambuf(std::vector<virtual_stream_part> parts);

};
