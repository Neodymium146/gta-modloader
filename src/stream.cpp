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

#include "stream.h"

//////////////////////////////////////////////////////
// membuf
//////////////////////////////////////////////////////

membuf::membuf(char* base, std::size_t size) {
	this->setp(base, base + size);
	this->setg(base, base, base + size);
	baseptr = base;
}

std::size_t membuf::written() const { return this->pptr() - this->pbase(); }
std::size_t membuf::read() const { return this->gptr() - this->eback(); }

std::streambuf::pos_type membuf::seekoff(off_type off,
	std::ios_base::seekdir way,
	std::ios_base::openmode which)
{	// change position by offset, according to way and mode
	if (which == std::ios_base::out)
		return -1;

	if (way == std::ios_base::beg)
	{
		setg(eback(), baseptr + off, egptr());
	}
	else if (way == std::ios_base::cur)
	{
		setg(eback(), gptr() + off, egptr());
	}
	else if (way == std::ios_base::end)
	{
		setg(eback(), egptr() - off, egptr());
	}
}

std::streambuf::pos_type membuf::seekpos(pos_type sp,
	std::ios_base::openmode which)
{	// change to specified position, according to mode
	return seekoff(sp, std::ios_base::beg, which);
}

//////////////////////////////////////////////////////
// partial_streambuf
//////////////////////////////////////////////////////

partial_streambuf::partial_streambuf() {

	internalPosition = 0;
	buffer_.resize(8192);
	char *end = &buffer_.front() + buffer_.size();
	setg(end, end, end);

}

partial_streambuf::partial_streambuf(std::istream* source, std::streamoff offset, std::size_t length) :
	source_{ source }, offset_{ offset }, length_{ length }
{

	internalPosition = 0;
	buffer_.resize(8192);
	char *end = &buffer_.front() + buffer_.size();
	setg(end, end, end);

}

// Get character on underflow
std::streambuf::int_type partial_streambuf::underflow()
{
	auto g1 = gptr();
	auto g2 = egptr();

	if (gptr() < egptr()) // buffer not exhausted
		return traits_type::to_int_type(*gptr()); // return current character

												  // refill buffer...

	char *base = &buffer_.front();
	char *start = base;


	int maxLen = (int)length_ - internalPosition;
	int count = (std::min)(8192, maxLen);



	source_->seekg(offset_ + internalPosition, std::ios_base::beg);
	source_->read(&buffer_.front(), count);
	internalPosition += count;

	auto y1 = &buffer_.front() - 10;
	auto y2 = &buffer_.front();
	auto y3 = &buffer_.front() + count;
	setg(y1, y2, y3);

	if (count == 0)
		return traits_type::eof();

	return traits_type::to_int_type(*gptr()); // return current character
}

// Set internal position pointer to relative position
std::streambuf::pos_type partial_streambuf::seekoff(off_type off,
	std::ios_base::seekdir way,
	std::ios_base::openmode which)
{	// change position by offset, according to way and mode
	if (which == std::ios_base::out)
		return internalPosition;

	if (way == std::ios_base::beg)
	{
		internalPosition = off;
		//log_write("stream: set ios_base::beg position at " + std::to_string(internalPosition));
	}
	else if (way == std::ios_base::cur)
	{
		internalPosition += off;
		//log_write("stream: set ios_base::cur position at " + std::to_string(internalPosition));
	}
	else if (way == std::ios_base::end)
	{
		internalPosition = length_ - off;
		//log_write("stream: set ios_base::end position at " + std::to_string(internalPosition));
	}

	// reset buf
	char *end = &buffer_.front() + buffer_.size();
	setg(end, end, end);

	return internalPosition;
}

// Set internal position pointer to absolute position
std::streambuf::pos_type partial_streambuf::seekpos(pos_type sp,
	std::ios_base::openmode which)
{	// change to specified position, according to mode
	return seekoff(sp, std::ios_base::beg, which);
}


//////////////////////////////////////////////////////
// virtual_stream
//////////////////////////////////////////////////////

// Get character on underflow
std::streambuf::int_type virtual_streambuf::underflow()
{
	auto g1 = gptr();
	auto g2 = egptr();

	if (gptr() < egptr()) // buffer not exhausted
		return traits_type::to_int_type(*gptr()); // return current character

												  // refill buffer...

	char *base = &buffer_.front();
	char *start = base;


	int maxLen = (parts[parts.size() - 1].offset + parts[parts.size() - 1].length) - internalPosition;
	int count = (std::min)(8192, maxLen);
	int currentCount = count;
	int currentOffset = 0;

	while (currentCount > 0)
	{

		// find parts...
		virtual_stream_part *previousStream = nullptr;
		virtual_stream_part *nextStream = nullptr;
		for (int i = 0; i < parts.size(); i++)
		{
			if (parts[i].offset <= internalPosition)
				previousStream = &parts[i];

			if (parts[i].offset > internalPosition)
			{
				nextStream = &parts[i];
				break;
			}
		}


		if (previousStream == nullptr)
		{
			if (nextStream == nullptr)
			{
				// no data... just fill with 0
				for (int k = 0; k < currentCount; k++)
					buffer_[currentOffset + k] = 0;
				currentCount -= currentCount;
				currentOffset += currentCount;
				internalPosition += currentCount;
			}
			else
			{
				// no data yet... just fill with 0 until first block
				int maxcnt = (std::min)(currentCount, (int)(nextStream->offset - internalPosition));
				for (int k = 0; k < currentCount; k++)
					buffer_[currentOffset + k] = 0;
				currentCount -= maxcnt;
				currentOffset += maxcnt;
				internalPosition += maxcnt;
			}
		}
		else
		{
			if (((long)internalPosition - previousStream->offset) < previousStream->length)
			{
				// we are inside p1, so read until end
				int maxcnt = (std::min)(currentCount, (int)(previousStream->offset + previousStream->length - internalPosition));

				auto relOffset = ((int)internalPosition - previousStream->offset);
				auto seekPos = previousStream->offset_in_file + relOffset;
				previousStream->baseStream->seekg(seekPos, std::ios_base::beg);

				/*char xx[4];
				previousStream->baseStream->read(xx, 4);*/

				previousStream->baseStream->read(&buffer_.front() + currentOffset, maxcnt);
				currentCount -= maxcnt;
				currentOffset += maxcnt;
				internalPosition += maxcnt;
			}
			else
			{
				if (nextStream == nullptr)
				{
					// we are after the last element, so just fill with 0
					for (int k = 0; k < currentCount; k++)
						buffer_[currentOffset + k] = 0;
					currentCount -= currentCount;
					currentOffset += currentCount;
					internalPosition += currentCount;
				}
				else
				{
					// we are after the end of p1, so just fill with 0 until p2 begins
					int maxcnt = (std::min)(currentCount, (int)(nextStream->offset - internalPosition));
					for (int k = 0; k < maxcnt; k++)
						buffer_[currentOffset + k] = 0;
					currentCount -= maxcnt;
					currentOffset += maxcnt;
					internalPosition += maxcnt;
				}
			}
		}
	}

	auto y1 = &buffer_.front() - 10;
	auto y2 = &buffer_.front();
	auto y3 = &buffer_.front() + count;
	setg(y1, y2, y3);

	if (count == 0)
		return traits_type::eof();

	return traits_type::to_int_type(*gptr()); // return current character
}

// Set internal position pointer to relative position
std::streambuf::pos_type virtual_streambuf::seekoff(off_type off,
	std::ios_base::seekdir way,
	std::ios_base::openmode which)
{	// change position by offset, according to way and mode
	if (which == std::ios_base::out)
		return internalPosition;

	if (way == std::ios_base::beg)
	{
		internalPosition = off;
	}
	else if (way == std::ios_base::cur)
	{
		internalPosition += off;
	}
	else if (way == std::ios_base::end)
	{
		std::size_t totalsize = parts[parts.size() - 1].offset + parts[parts.size() - 1].length;
		internalPosition = totalsize - off;
	}

	// reset buf
	char *end = &buffer_.front() + buffer_.size();
	setg(end, end, end);

	return internalPosition;
}

// Set internal position pointer to absolute position
std::streambuf::pos_type virtual_streambuf::seekpos(std::streambuf::pos_type sp,
	std::ios_base::openmode which)
{	// change to specified position, according to mode
	return seekoff(sp, std::ios_base::beg, which);
}

virtual_streambuf::virtual_streambuf(std::vector<virtual_stream_part> parts)
	{
		this->parts = parts;
		internalPosition = 0;
		buffer_.resize(8192);
		char *end = &buffer_.front() + buffer_.size();
		setg(end, end, end);
	}
