/* -*- Mode: C++; ; indent-tabs-mode: t; c-file-style: "linux" -*- */
// Copyright (C) 2013 Jake Shadle
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Original author: Jake Shadle <jake.shadle@frostbite.com>

#include "PDBParser.h"

#include "utils.h"
#include <assert.h>
#include <algorithm>
#ifdef _WIN32
#include <atlfile.h>
#include <ppl.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_TBB
#include "tbb/task_group.h"
#include "tbb/parallel_sort.h"
#include "tbb/compat/ppl.h"
#endif
#endif
#include <stdexcept>
#include <string.h>

#ifndef _WIN32
#include <errno.h>

int fopen_s(FILE** f, const char* filename, const char* mode)
{
	*f = fopen(filename, mode);
	if (*f)
		return 0;
	return errno;
}

namespace Concurrency
{
#ifdef HAVE_TBB
	using tbb::parallel_sort;
#else
	// Single-threaded implementation of the bits of PPL we're using.
	enum task_group_status {
		canceled,
		completed,
		not_complete
	};

	class task_group
	{
	public:
		task_group() {}
		task_group_status wait()
		{
			return completed;
		}
		template<typename _Function>
		void run(const _Function& _Func)
		{
			_Func();
		}
	};

	template<typename _Random_iterator>
	inline void parallel_sort(const _Random_iterator &_Begin,
		const _Random_iterator &_End)
	{
		std::sort(_Begin, _End);
	}
#endif
}
#endif

namespace google_breakpad
{

bool MMapWrapper::Map(const char* path)
{
#ifdef _WIN32
	CAtlFile file;
	HRESULT result = file.Create(path, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
	if (FAILED(result))
		return false;

	m_mapFile = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, 0);
	if (m_mapFile == nullptr)
		return false;

	m_base = (const uint8_t*)MapViewOfFile(m_mapFile, FILE_MAP_READ, 0, 0, 0);

	if (m_base == nullptr)
		return false;
#else // !_WIN32
		int fd = open(path, O_RDONLY, 0);
		if (fd == -1)
			return false;

#if defined(__x86_64__)
		struct stat st;
		if (fstat(fd, &st) == -1 || st.st_size < 0)
		{
#else
		struct stat64 st;
		if (fstat64(fd, &st) == -1 || st.st_size < 0)
		{
#endif
#if 0
		}
#endif
			close(fd);
			return false;
		}
		m_length = st.st_size;

#if defined(__x86_64__)
		void* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
#else
		void* data = mmap2(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
#endif
		close(fd);
		if (data == MAP_FAILED)
			return false;
		m_base = reinterpret_cast<const uint8_t*>(data);
#endif
		return true;
}

bool MMapWrapper::Unmap()
{
	if (!Valid())
		return false;

#ifdef _WIN32
	UnmapViewOfFile(m_base);
	CloseHandle(m_mapFile);
#else
	munmap(const_cast<uint8_t*>(m_base), m_length);
#endif
	return true;
}

PDBParser::FunctionRecord& PDBParser::FunctionRecord::operator =(FunctionRecord&& other)
{
	std::swap(name, other.name);
	std::swap(lines, other.lines);
	std::swap(lineCount, other.lineCount);
	std::swap(segment, other.segment);
	std::swap(offset, other.offset);
	std::swap(fileIndex, other.fileIndex);
	std::swap(length, other.length);
	std::swap(lineOffset, other.lineOffset);
	std::swap(typeIndex, other.typeIndex);
	std::swap(paramSize, other.paramSize);

	return *this;
}

uint32_t
getNumPages(uint32_t length, uint32_t pageSize)
{
	uint32_t numPages = length / pageSize;
	if (length % pageSize)
		numPages += 1;

	return numPages;
}

class StreamReader
{
public:
	StreamReader(const PDBParser::StreamPair& stream, const PDBParser& parser, uint32_t offset = 0)
		: m_stream(stream)
		, m_parser(parser)
		, m_data(nullptr)
		, m_seqPageEnd(nullptr)
		, m_offset(0xffffffff)
		, m_pageIndex(0xffffffff)
		, m_pageEndIndex(0)
	{
		seek(offset);
	}

	uint32_t getOffset() const { return m_offset; }
	const uint8_t* getData() const { return m_data; };

	bool isValidOffset(uint32_t offset)
	{
		uint32_t index = offset / m_parser.pageSize();
		return index < m_stream.pageIndices.size();
	}

	static const uint8_t* getData(const PDBParser::StreamPair& stream, const PDBParser& parser, uint32_t offset = 0)
	{
		int index = offset / parser.pageSize();

		const uint8_t* base = parser.data() + stream.pageIndices[index] * parser.pageSize();
		base += offset % parser.pageSize();

		return base;
	}

	void align(uint32_t align)
	{
		uint32_t diff = m_offset % align;

		if (diff)
			seek(m_offset + align - diff);
	}

	void seek(uint32_t offset)
	{
		uint32_t index = offset / m_parser.pageSize();

		if (index == m_pageIndex)
		{
			if (offset > m_offset)
				m_data += offset - m_offset;
			else
				m_data -= m_offset - offset;

			m_offset = offset;
			return;
		}

		if (index >= m_stream.pageIndices.size())
			throw std::runtime_error("Requesting offset outside of page range");

		const uint8_t* base = m_parser.data() + m_stream.pageIndices[index] * m_parser.pageSize();
		m_data = base + offset % m_parser.pageSize();
		m_offset = offset;
		m_pageIndex = index;

		/*if (m_pageIndex < m_pageEndIndex)
		return;*/

		// Check to see if we have any non-adjacent pages
		size_t count = m_stream.pageIndices.size();
		size_t last = count - 1;
		if (index == last)
		{
			m_seqPageEnd = base + m_parser.pageSize();
			m_pageEndIndex = (uint32_t)last;

			assert(m_seqPageEnd > m_data);
			return;
		}
		else
		{
			// Scan forward to find the first non-adjacent page
			uint32_t expected = m_stream.pageIndices[index] + 1;
			for (size_t i = index + 1; i < count; ++i)
			{
				if (expected++ != m_stream.pageIndices[i])
				{
					m_seqPageEnd = m_parser.data() + m_stream.pageIndices[i - 1] * m_parser.pageSize() + m_parser.pageSize();
					m_pageEndIndex = (uint32_t)i - 1;

					assert(m_seqPageEnd > m_data);
					return;
				}
			}
		}

		m_seqPageEnd = m_parser.data() + m_stream.pageIndices[last] * m_parser.pageSize() + m_parser.pageSize();
		m_pageEndIndex = (uint32_t)last;

		assert(m_seqPageEnd > m_data);
	}

	template<typename T>
	T peek()
	{
		// Verify!
		if (m_data + sizeof(T) > m_seqPageEnd)
		{
			uint32_t offset = m_offset;

			T retValue;
			uint8_t* outVal = (uint8_t*)&retValue;
			uint32_t toRead = sizeof(T);

			while (toRead > 0)
			{
				uint32_t seqRead = std::min((uint32_t)(m_seqPageEnd - m_data), toRead);

				// Hack
				if (seqRead == 0)
				{
					--m_offset;
					seek(m_offset + 1);
				}

				memcpy(outVal, m_data, seqRead);

				seek(m_offset + seqRead);

				toRead -= seqRead;
				outVal += seqRead;
			}

			// Return back to the original position
			seek(offset);

			return retValue;
		}
		else
			return *((const T*)m_data);
	}

	template<typename T>
	DataPtr<T> read(uint32_t size = 0)
	{
		uint32_t toRead = size == 0 ? sizeof(T) : size;

		// Check to see if the data is split across multiple pages
		if (m_data + toRead > m_seqPageEnd)
		{
			uint8_t* alloced = (uint8_t*)malloc(toRead);
			uint8_t* outPos = alloced;

			while (toRead > 0)
			{
				uint32_t seqRead = std::min((uint32_t)(m_seqPageEnd - m_data), toRead);

				// Hack
				if (seqRead == 0)
				{
					--m_offset;
					seek(m_offset + 1);
				}

				memcpy(outPos, m_data, seqRead);

				seek(m_offset + seqRead);

				toRead -= seqRead;
				outPos += seqRead;
			}

			return DataPtr<T>(alloced, true);
		}
		else
		{
			DataPtr<T> read(m_data);

			m_data += toRead;
			m_offset += toRead;

			return read;
		}
	}

	DataPtr<char> readString()
	{
		uint32_t origOffset = m_offset;
		const uint8_t* toCopy = m_data;
		uint32_t strLen = 0;
		bool needsSeek = false;

		do
		{
			if (toCopy == m_seqPageEnd)
			{
				needsSeek = true;
				seek(m_offset + strLen + 1);
				toCopy = m_data - 1;
			}

			if (*toCopy++ == 0)
			{
				if (needsSeek)
					seek(origOffset);

				return read<char>(strLen + 1);
			}

			++strLen;
		} while(true);
	}

private:

	const PDBParser::StreamPair&	m_stream;
	const PDBParser&				m_parser;

	const uint8_t*					m_data;
	const uint8_t*					m_seqPageEnd;
	uint32_t						m_offset;
	uint32_t						m_pageIndex;
	uint32_t						m_pageEndIndex;
};

void
PDBParser::load(const char* path)
{
	if (!m_mapping.Map(path))
		throw std::runtime_error("Failed to load PDB file");

	m_base = m_mapping.base();

	if (!readRootStream())
		throw std::runtime_error("Failed to read PDB Root Stream");

	// Find and read the executable that is paired with this PDB,
	// because unfortunately the PDB doesn't contain the necessary info
	// we need, which seems very wrong...
	m_filename = path;
	m_filename.erase(m_filename.size() - 3, 3);
	m_filename += "exe";

	m_isExe = true;

	// Get the exe header, don't use ImageLoad since it loads the entire exe into memory
	// which is completely unnecessary as we only need some info from the header
	FILE* exeFile = nullptr;
	if (fopen_s(&exeFile, m_filename.c_str(), "rb") != 0)
	{
		// Try .dll
		m_filename.erase(m_filename.size() - 3, 3);
		m_filename += "dll";
		if (fopen_s(&exeFile, m_filename.c_str(), "rb") != 0)
			fprintf(stderr, "Failed to find paired exe/dll file");

		m_isExe = false;
	}

	m_filename.erase(m_filename.size() - 3, 3);
	size_t loc = m_filename.find_last_of('\\');
	if (loc == std::string::npos)
		loc = m_filename.find_last_of('/');

	if (loc != std::string::npos)
		m_filename.erase(0, loc + 1);

	if (exeFile)
	{
		do
		{
			IMAGE_DOS_HEADER dosHeader;
			if (fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, exeFile) != 1)
				throw std::runtime_error("Error reading PE header");

			if (dosHeader.e_magic != 23117 /* "PE\0\0" */)
				throw std::runtime_error("Invalid PE header detected");

			fseek(exeFile, dosHeader.e_lfanew, SEEK_SET);

			IMAGE_NT_HEADERS64 header;
			if (fread(&header, sizeof(IMAGE_NT_HEADERS64), 1, exeFile) != 1)
				throw std::runtime_error("Error reading PE NT header");

			m_PETimeStamp = header.FileHeader.TimeDateStamp;

			if (header.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			{
				// Ignore this if it's powerPC because...xenon
				// Detect if the executable/dll is actually a CLR assembly, which is not supported
				if (header.FileHeader.Machine != 0x01F2 && header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress != 0)
					throw std::runtime_error("The image is a CLR assembly, which is not supported");

				m_PESize = header.OptionalHeader.SizeOfImage;
			}
			else if (header.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			{
				const IMAGE_NT_HEADERS32* header32 = (const IMAGE_NT_HEADERS32*)&header;
				if (header32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress != 0)
					throw std::runtime_error("The image is a CLR assembly, which is not supported");

				m_PESize = header32->OptionalHeader.SizeOfImage;
			}
			else
				throw std::runtime_error("Unknown image format");

			m_foundPE = true;
		} while(0);

		fclose(exeFile);
	}
}

bool
PDBParser::readRootStream()
{
	const PDBHeader* header = (const PDBHeader*)m_base;
	const char validSignature[] = {"Microsoft C/C++ MSF 7.00\r\n\032DS\0\0"};

	if (memcmp(header->signature, validSignature, sizeof(validSignature)) != 0)
	{
		fprintf(stderr, "Input file has an invalid signature\n");
		return false;
	}

	m_pageSize = header->pageSize;
	m_numPages = header->pagesUsed;

	uint32_t rootSize = header->directorySize;
	uint32_t numRootPages = getNumPages(rootSize, m_pageSize);
	uint32_t numRootIndexPages = getNumPages(numRootPages * 4, m_pageSize);

	const uint32_t* rootIndices = (const uint32_t*)(m_base + sizeof(PDBHeader));
	std::vector<uint32_t> rootPageList;
	for (uint32_t i = 0; i < numRootIndexPages; ++i) {
		const uint32_t* rootPages = (const uint32_t*)(m_base + rootIndices[i] * m_pageSize);
		rootPageList.insert(rootPageList.end(), rootPages, rootPages + (m_pageSize / sizeof(uint32_t)));
	}

	uint32_t pageIndex = 0;
	uint32_t pageOffset = 0;
	const uint32_t* page = (const uint32_t*)(m_base + rootPageList[pageIndex] * m_pageSize);

	// The first 4 bytes are how many streams we actually need to read
	uint32_t numStreams = *page;
	++pageOffset;

	m_streams.reserve(numStreams);

	const uint32_t numItems = m_pageSize / sizeof(uint32_t);

	// Read all of the sizes for each stream, which directly determines how many
	// page indices we need to read after them
	{
		uint32_t streamIndex = 0;
		do
		{
			for (; streamIndex < numStreams && pageOffset < numItems; ++streamIndex, ++pageOffset)
			{
				uint32_t size = page[pageOffset];
				if (size == 0xFFFFFFFF)
					m_streams.push_back(StreamPair(0));
				else
					m_streams.push_back(StreamPair(size));
			}

			// Advance to the next page
			if (pageOffset == numItems)
			{
				page = (const uint32_t*)(m_base + rootPageList[++pageIndex] * m_pageSize);
				pageOffset = 0;
			}
		} while (streamIndex < numStreams);
	}

	// For each stream, get the list of page indices associated with each one,
	// since any data associated with a stream are not necessarily adjacent
	for (uint32_t i = 0; i < numStreams; ++i)
	{
		uint32_t numPages = getNumPages(m_streams[i].size, m_pageSize);

		if (numPages != 0)
		{
			m_streams[i].pageIndices.resize(numPages);

			uint32_t numToCopy = numPages;
			do
			{
				uint32_t num = std::min(numToCopy, numItems - pageOffset);
				memcpy(m_streams[i].pageIndices.data() + numPages - numToCopy, page + pageOffset, sizeof(uint32_t) * num);
				numToCopy -= num;
				pageOffset += num;

				if (pageOffset == numItems)
				{
					page = (const uint32_t*)(m_base + rootPageList[++pageIndex] * m_pageSize);
					pageOffset = 0;
				}
			} while (numToCopy);
		}
	}

	uint32_t numOk = 0;

	{
		StreamReader nameReader(m_streams[1], *this);

		auto nameIndexHeader = nameReader.read<NameIndexHeader>();
		m_guid = nameIndexHeader->guid;

		uint32_t nameStart = nameReader.getOffset();

		StreamReader mapReader(m_streams[1], *this);
		mapReader.seek(nameStart + nameIndexHeader->names);

		numOk = *mapReader.read<uint32_t>().data;
		uint32_t count = *mapReader.read<uint32_t>().data;

		uint32_t okOffset = mapReader.getOffset();
		uint32_t skip = *mapReader.read<uint32_t>().data;

		if ((count >> 5) > skip)
		{
			fprintf(stderr, "Invalid name index\n");
			return false;
		}

		auto okBits = mapReader.read<uint32_t>((count >> 5) * sizeof(uint32_t));

		mapReader.seek(okOffset + (skip + 1) * sizeof(uint32_t));

		uint32_t verification = *mapReader.read<uint32_t>().data;
		if (verification != 0)
		{
			fprintf(stderr, "Invalid name index\n");
			return false;
		}

		struct StringVal
		{
			uint32_t id;
			uint32_t stream;
		};

		for (uint32_t i = 0; i < count; ++i)
		{
			if (!(okBits.data[i >> 5] & (1 << (i % 32))))
				continue;

			auto val = mapReader.read<StringVal>();
			nameReader.seek(nameStart + val->id);
			std::string name = nameReader.readString().data;
			strupper((char*)name.c_str());

			m_nameIndices.insert(std::make_pair(std::move(name), val->stream));

			numOk--;
		}
	}

	return numOk == 0;
}

void
PDBParser::loadNameStream(NameStream& names)
{
	auto nIter = m_nameIndices.find("/NAMES");
	if (nIter == m_nameIndices.end())
		throw std::runtime_error("Could not find /NAMES in name indices");

	struct NameStreamHeader
	{
		uint32_t sig;
		int32_t	 version;
		int32_t  offset;
	};

	auto& ns = getStream(nIter->second);

	// Die in a fire microsoft.
	// Explanation - Every pdb I have tested puts streams in sequential order
	// so I assumed that was always the case, but no, apparently incorrect!
	// This was only encountered in one PDB in this one stream, so for now, just do this once.
	// Obviously the way to always be 100% correct all the time is to copy the entire stream into sequential
	// memory, but we don't want to do that if we don't have to
	char* tempData = new char[ns.size];
	names.buffer = tempData;

	uint32_t last = (uint32_t)ns.pageIndices.size() - 1;
	for (uint32_t i = 0, end = last; i < end; ++i)
		memcpy(tempData + i * m_pageSize, m_base + ns.pageIndices[i] * m_pageSize, m_pageSize);

	// The last page may be shorter than the actual page size
	memcpy(tempData + last * m_pageSize, m_base + ns.pageIndices[last] * m_pageSize, std::min(m_pageSize, ns.size - last * m_pageSize));

	const NameStreamHeader* nsh = (NameStreamHeader*)tempData;

	if (nsh->sig != 0xeffeeffe || nsh->version != 1)
		throw std::runtime_error("Invalid name stream");

	const uint32_t* offsets = (uint32_t*)(tempData + sizeof(NameStreamHeader) + nsh->offset);
	uint32_t size = *offsets++;

	const char* nameStart = tempData + sizeof(NameStreamHeader);

	for (uint32_t i = 0; i < size; ++i)
	{
		uint32_t id = *offsets++;
		if (id != 0)
		{
			DataPtr<char> data(nameStart + id);
			if (data.data != nullptr)
				names.map.insert(std::make_pair(id, std::move(data)));
		}
	}
}

PDBParser::TypeMap
PDBParser::loadTypeStream()
{
	auto& ts = getStream(TypeInfoStream);
	if (ts.size == 0)
		throw std::runtime_error("Invalid type info stream");

	StreamReader reader(ts, *this);

	auto tih = reader.read<TypeInfoHeader>();

	TypeMap map;
	map.reserve(tih->max - tih->min);
	uint32_t end = reader.getOffset();

	for (uint32_t i = tih->min; i < tih->max; ++i)
	{
		reader.seek(end);
		reader.align(sizeof(TypeRecord));

		auto tr = reader.read<TypeRecord>();
		if (tr->length == 0)
			throw std::runtime_error("Invalid type info stream");

		// No type, which can happen rarely, do NOT adjust when encountered
		if (tr->leafType == 0)
			continue;

		end = reader.getOffset() + tr->length - sizeof(uint16_t);

		TypeInfo nfo;
		nfo.type = (LEAF::Enum)tr->leafType;

		switch (tr->leafType)
		{
		case LEAF::LF_MODIFIER:
			nfo.data = reader.read<uint8_t>(sizeof(LeafModifier));
			break;
		case LEAF::LF_POINTER:
			nfo.data = reader.read<uint8_t>(sizeof(LeafPointer));
			break;
		case LEAF::LF_PROCEDURE:
			nfo.data = reader.read<uint8_t>(sizeof(LeafProc));
			break;
		case LEAF::LF_MFUNCTION:
			nfo.data = reader.read<uint8_t>(sizeof(LeafMFunc));
			break;
		case LEAF::LF_ARGLIST:
			{
				uint32_t count = reader.peek<uint32_t>();
				nfo.data = reader.read<uint8_t>((count + 1) * sizeof(uint32_t));
			}
			break;
		case LEAF::LF_ARRAY:
			nfo.data = reader.read<uint8_t>(sizeof(LeafArray));
			break;
		case LEAF::LF_CLASS:
		case LEAF::LF_STRUCTURE:
			{
				reader.seek(reader.getOffset() + sizeof(LeafClass) + sizeof(uint16_t));
				nfo.name = reader.readString();
			}
			break;
		case LEAF::LF_UNION:
			{
				reader.seek(reader.getOffset() + sizeof(LeafUnion) + sizeof(uint16_t));
				nfo.name = reader.readString();
			}
			break;
		case LEAF::LF_ENUM:
			{
				reader.seek(reader.getOffset() + sizeof(LeafEnum));
				nfo.name = reader.readString();
			}
			break;
		case LEAF::LF_ALIAS:
			{
				reader.seek(reader.getOffset() + sizeof(LeafAlias));
				nfo.name = reader.readString();
			}
			break;
		case LEAF::LF_INDEX:
			nfo.data = reader.read<uint8_t>(sizeof(LeafIndex));
			break;
		default:
			{
				if (nfo.type < LEAF::LF_NUMERIC || nfo.type > LEAF::LF_UTF8STRING)
					continue;

				nfo.data = reader.read<uint8_t>(end - reader.getOffset());
			}
			break;
		}

		map.insert(std::make_pair(i, std::move(nfo)));
	}

	return map;
}

void
PDBParser::close()
{
  m_mapping.Unmap();
}

struct SymbolSource
{
	uint16_t numModules;
	uint16_t numModuleSources;
};

void
PDBParser::printBreakpadSymbols(FILE* of, const char* platform, FileMod* fileMod)
{
	const StreamPair& pair = getStream(DebugInfo);
	if (pair.size == 0)
		throw std::runtime_error("Invalid DebugInfo stream");

	StreamReader reader(pair, *this);
	auto header = reader.read<DBIHeader>();

	printHeader(header.data, of, platform);

	uint32_t endOffset = reader.getOffset() + header->moduleSize;

	struct Module
	{
		DataPtr<DBIModuleInfo>	info;
		SrcFileIndex			srcIndex;
		DataPtr<char>			moduleName;
		DataPtr<char>			objectName;

		Module(DataPtr<DBIModuleInfo>&& data, DataPtr<char>&& modName, DataPtr<char>&& objName)
			: info(std::move(data))
			, moduleName(std::move(modName))
			, objectName(std::move(objName))
		{}

		Module(Module&& other)
		{
			*this = std::move(other);
		}

		Module& operator=(Module&& other)
		{
			info = std::move(other.info);
			srcIndex = std::move(other.srcIndex);
			moduleName = std::move(other.moduleName);
			objectName = std::move(other.objectName);

			return *this;
		}

	private:

		Module(){}
		Module& operator=(const Module&){ return *this; }
	};

	std::vector<Module> modules;

	while (reader.getOffset() < endOffset)
	{
		auto dbInfo = reader.read<DBIModuleInfo>();
		auto modName = reader.readString();
		auto objName = reader.readString();

		if (dbInfo->stream != -1)
			modules.push_back(Module(std::move(dbInfo), std::move(modName), std::move(objName)));

		reader.align(4);
	}

	reader.seek(reader.getOffset()
				+ header->secConSize
				+ header->secMapSize
				+ header->fileInfoSize
				+ header->srcModuleSize
				+ header->ecInfoSize);

	auto debugHeader = reader.read<DBIDebugHeader>();

	// Get the PE headers so that we can offset the functions to their correct
	// addresses in the actual executable
	std::vector<IMAGE_SECTION_HEADER> sections;
	if (debugHeader->sectionHdr != 0xFFFF)
	{
		readSectionHeaders(debugHeader->sectionHdr, sections);
	}

	uint32_t id = 1;
	UniqueSrcFiles unique;
	for (auto& mod : modules)
	{
		if (mod.info.data->stream < 0)
		{
			fprintf(stderr, "Invalid module found gathering files...\n");
			continue;
		}

		getModuleFiles(mod.info.data, id, unique, mod.srcIndex);
	}

	NameStream names;

	// Start printing the module files in a separate thread
	Concurrency::task_group tg;
	tg.run(
	       [this, &unique, &modules, &names, of, fileMod]() {
			loadNameStream(names);

			auto end = names.map.end();

			for (auto& mod : modules)
			{
				for (auto& kv : mod.srcIndex)
				{
					auto& us = unique.at(kv.second);

					if (!us.visited)
					{
						auto iter = names.map.find(kv.second);

						// Handle bad file references...again...thank you microsoft
						if (iter != end)
						{
							if (fileMod)
							{
								const char* str = iter->second.data;
								fprintf(of, "FILE %d %s\n", us.id, (*fileMod)(str, strlen(str)));
							}
							else
								fprintf(of, "FILE %d %s\n", us.id, iter->second.data);
						}

						us.visited = 1;
					}
				}
			}
		});

	TypeMap tm;
	tg.run([this, &tm] { tm = loadTypeStream(); });

	// Check to see if we need to remap functions
	if (debugHeader->tokenRidMap != 0 && debugHeader->tokenRidMap != 0xffff)
		throw std::runtime_error("Implement me...");

	// Get functions from the global stream. These have mangled names that are useful.
	Globals globals;
	getGlobalFunctions(header->symRecordStream, sections, globals);

	Functions functions;
	for (auto& mod : modules)
	{
		getModuleFunctions(mod.info.data, functions);
	}

	Concurrency::parallel_sort(functions.begin(), functions.end());

	for (auto& mod : modules)
	{
		resolveFunctionLines(mod.info.data, functions, unique, mod.srcIndex);
	}

	std::map<std::pair<uint32_t, uint32_t>, DataPtr<FPO_DATA>> fpov1Data;
	std::map<std::pair<uint32_t, uint32_t>, DataPtr<FPO_DATA_V2>> fpov2Data;

	if (debugHeader->FPO != 0xffff) {
		readFPO(debugHeader->FPO, fpov1Data);
	}

	if (debugHeader->newFPO != 0xffff) {
		readFPO(debugHeader->newFPO, fpov2Data);
	}

	// We cheat in the Function < operator so that we can sort
	// first, now iterate over the functions and remove the functions that are duplicates,
	// we don't actually remove the functions, just make it so that they are skipped from printing
	FunctionRecord* current = &functions[0];
	for (uint32_t i = 1, end = (uint32_t)functions.size(); i < end; ++i)
	{
		if (*current == functions[i])
		{
			// Duplicate! Preserve whichever one is 'most' interesting
			if (current->lines.data || !functions[i].lines.data)
				functions[i].segment = 0xffffffff;
			else if (functions[i].lines.data)
			{
				current->segment = 0xffffffff;
				current = &functions[i];
			}
		}
		else
			current = &functions[i];
	}
	// Offset functions by segment address and
	// try to fill in paramSize from FPO data.
	for (auto& func : functions)
	{
		if (func.segment == 0xffffffff)
			continue;
		func.offset += sections[func.segment - 1].VirtualAddress;
		if (!updateParamSize(func, fpov2Data))
		{
			if (!updateParamSize(func, fpov1Data))
			{
				updateParamSize(func, globals);
			}
		}
	}

	// Wait for the type stream to be loaded, and all of the src files to be written
	tg.wait();

	printFunctions(functions, tm, of);

	printFPOs(fpov2Data, names, of);
	printFPOs(fpov1Data, names, of);

	fflush(of);
}

void
PDBParser::readModule(const DBIModuleInfo* module, int32_t section, ModuleReadCB cb)
{
	const StreamPair& pair = getStream(module->stream);

	StreamReader reader(pair, *this);
	auto sig = reader.read<int32_t>();

	if (*sig.data != 4)
		throw std::runtime_error("Invalid module stream signature");

	// Skip functions
	reader.seek(module->cbSyms + module->cbOldLines);
	uint32_t endOffset = reader.getOffset() + module->cbLines;

	struct SubsectionHeader
	{
		int32_t sig;
		int32_t size;
	};

	while (reader.getOffset() < endOffset)
	{
		auto header = reader.read<SubsectionHeader>();

		if (header->sig == 0 || (header->sig & Subsection::Ignore))
			continue;

		uint32_t end = reader.getOffset() + header->size;

		if (!reader.isValidOffset(end))
			throw std::runtime_error("Invalid subsection header detected");

		if (header->sig == section)
			cb(reader, header->sig, end);

		reader.seek(end);

		// AGH! It seems that regular windows compilers properly pad this, the Xenon compiler,
		// however, just says FUCK YOU
		reader.align(4);
	}
}

void
PDBParser::printHeader(const DBIHeader* header, FILE* of, const char* platform)
{
	if (!platform)
	{
		const char* machineType = nullptr;
		switch (header->machine)
		{
		case IMAGE_FILE_MACHINE_I386:
			machineType = "x86";
			break;
		case IMAGE_FILE_MACHINE_AMD64:
			machineType = "x86_64";
			break;
		case IMAGE_FILE_MACHINE_ARM:
			machineType = "arm";
			break;
		case 0x01F2: // This is the value that the Xenon uses, which isn't in the IMAGE_FILE_MACHINE list, which is awesome
			machineType = "ppc64";
			break;
		default:
			machineType = "unknown";
			break;
		}

		fprintf(of, "MODULE windows %s %08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%x %spdb\n", machineType,
			m_guid.Data1, m_guid.Data2, m_guid.Data3,
			m_guid.Data4[0], m_guid.Data4[1], m_guid.Data4[2], m_guid.Data4[3],
			m_guid.Data4[4], m_guid.Data4[5], m_guid.Data4[6], m_guid.Data4[7],
			header->age, m_filename.c_str());
	}
	else
		fprintf(of, "MODULE %s %08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%x %spdb\n", platform,
			m_guid.Data1, m_guid.Data2, m_guid.Data3,
			m_guid.Data4[0], m_guid.Data4[1], m_guid.Data4[2], m_guid.Data4[3],
			m_guid.Data4[4], m_guid.Data4[5], m_guid.Data4[6], m_guid.Data4[7],
			header->age, m_filename.c_str());

	if (m_foundPE)
		fprintf(of, "INFO CODE_ID %08X%X %s%s\n", m_PETimeStamp, m_PESize, m_filename.c_str(), m_isExe ? "exe" : "dll");
}

void
PDBParser::readSectionHeaders(uint32_t headerStream, SectionHeaders& headers)
{
	auto& hs = getStream(headerStream);

	StreamReader reader(hs, *this);

	while (reader.getOffset() < hs.size)
	{
		auto sh = reader.read<SectionHeader>();
		headers.push_back(*sh.data);
	}
}

void
PDBParser::getModuleFiles(const DBIModuleInfo* module, uint32_t& id, UniqueSrcFiles& unique, SrcFileIndex& fileIndices)
{
	readModule(module, Subsection::FileChecksums,
		[module, &id, &unique, &fileIndices](StreamReader& reader, int32_t sig, uint32_t end)
		{
			uint32_t index = reader.getOffset();

			while (reader.getOffset() < end)
			{
				uint32_t msid = reader.getOffset() - index;
				auto fileChk = reader.read<CVFileChecksum>();

				auto fiter = unique.find(fileChk->name);
				if (fiter == unique.end())
				{
					auto& fileid = unique[fileChk->name];
					fileid.id = id++;
				}
				else
					id++;

				fileIndices.insert(std::make_pair(msid, fileChk->name));

				// Skip past the actual checksum itself
				reader.seek(reader.getOffset() + fileChk->len);
				reader.align(4);
			}
		});
}

void
PDBParser::getModuleFunctions(const DBIModuleInfo* module, Functions& funcs)
{
	const StreamPair& pair = getStream(module->stream);

	StreamReader reader(pair, *this);
	auto sig = reader.read<int32_t>();

	if (*sig.data != 4)
		throw std::runtime_error("Invalid module stream signature");

	struct SymbolHeader
	{
		uint16_t size;
		uint16_t type;
	};

	uint32_t end = (uint32_t)module->cbSyms;

	while (reader.getOffset() < end)
	{
		auto header = reader.read<SymbolHeader>();

		uint32_t offsetBeg = reader.getOffset() - sizeof(uint16_t);

		switch (header->type)
		{
		case SymbolDefs::S_GPROC32:
		case SymbolDefs::S_LPROC32:
			{
				auto proc = reader.read<ProcSym32>();
				auto name = reader.readString();

				FunctionRecord rec(std::move(name));
				rec.offset = proc->off;
				rec.segment = proc->seg;
				rec.length = proc->len;
				rec.typeIndex = proc->typind;

				funcs.push_back(std::move(rec));
			}
			break;
		case SymbolDefs::S_THUNK32:
			{
				auto thunk = reader.read<ThunkSym32>();
				auto name = reader.readString();

				FunctionRecord rec(std::move(name));
				rec.offset = thunk->off;
				rec.segment = thunk->seg;
				rec.length = thunk->parent != 0 ? thunk->len : 0;

				funcs.push_back(std::move(rec));
			}
			break;
		default:
			break;
		}

		// Mycket viktigt!
		reader.seek(offsetBeg + header->size);
	}
}

void
PDBParser::getGlobalFunctions(uint16_t symRecStream, const SectionHeaders& headers, Globals& globals)
{
	const StreamPair& pair = getStream(symRecStream);
	StreamReader reader(pair, *this);

	while (reader.getOffset() < pair.size)
	{
		auto len = *reader.read<uint16_t>().data;
		if (len >= sizeof(GlobalRecord))
		{
			auto rec = reader.read<GlobalRecord>();
			auto name = reader.read<char>(len - sizeof(GlobalRecord));

			// Is function?
			if (rec->symType == 2)
			{
				globals.insert(std::make_pair(rec->offset + headers[rec->segment - 1].VirtualAddress, std::move(name)));
			}
		}
		else
		{
			// Just skip this data, we don't know how to handle it.
			reader.seek(reader.getOffset() + len);
		}
	}
}

void
PDBParser::resolveFunctionLines(const DBIModuleInfo* module, Functions& funcs, const UniqueSrcFiles& unique, const SrcFileIndex& fileIndex)
{
	readModule(module, Subsection::Lines,
		[&funcs, &unique, &fileIndex](StreamReader& reader, int32_t sig, uint32_t end)
		{
			auto ls = reader.read<CV_LineSection>();

			size_t min = 0;
			size_t max = funcs.size() - 1;
			while (min < max)
			{
				size_t mid = (min + max) >> 1;

				auto& func = funcs[mid];
				if (func.segment < ls->sec || (func.segment == ls->sec && func.offset < ls->off))
					min = mid + 1;
				else
					max = mid;
			}

			auto& function = funcs[min];
			if (function.lineOffset != 0 && (function.lineOffset - function.offset < ls->off - function.offset
				|| function.lineCount & 0xF0000000)) // This means the first function always wins, which seems to be the behavior of the original Breakpad implementation
				return;

			auto srcfile = reader.read<CV_SourceFile>();

			// First find the module specific file offset
			uint32_t fileChk = fileIndex.at(srcfile->index);

			// Next get the unique id that is paired with that particular file
			function.fileIndex = unique.at(fileChk).id;

			function.lineCount = srcfile->count;
			function.lineOffset = ls->off;

			if (function.lineCount)
				function.lines = reader.read<uint8_t>(srcfile->count * sizeof(CV_Line));

			// Mark that the function has been encountered
			function.lineCount |= 0xF0000000;
		});
}

void
PDBParser::printFunctions(Functions& funcs, const TypeMap& tm, FILE* of)
{
	std::string str;
	str.reserve(2048);

	std::string temp;
	str.reserve(1024);

	for (auto& func : funcs)
	{
		str.clear();

		if (func.segment == 0xffffffff)
			continue;

		if (func.typeIndex)
		{
			stringizeType(func.typeIndex, str, tm, IsTopLevel);

			temp.assign(func.name.data);
			std::string::size_type pos;
			while ((pos = temp.rfind(" __ptr64")) != std::string::npos)
			{
				temp.erase(pos, 8);
			}

			while ((pos = temp.rfind("__cdecl")) != std::string::npos)
			{
				temp.erase(pos, 7);
			}

			fprintf(of, "FUNC %x %x %x %s%s\n", func.offset, func.length, func.paramSize, temp.c_str(), str.c_str());

			uint32_t lineCount = func.lineCount & 0x0FFFFFFF;
			if (lineCount)
			{
				const CV_Line* lines = (const CV_Line*)func.lines.data;
				uint32_t fromNext = lineCount - 1;

				// Handle rare case where the last line offset exceeds the actual function length,
				// have only encountered this with '__security_check_cookie()'
				uint32_t modifier = 0;
				if (lines[fromNext].offset > func.length)
				{
					modifier = lines[fromNext].offset - func.length;
					if (uint32_t diff = modifier % 16)
						modifier = modifier + 16 - diff;
				}

				for (uint32_t i = 0; i < lineCount; ++i)
				{
					uint32_t size = i < fromNext ? lines[i + 1].offset - lines[i].offset : func.length + modifier - lines[i].offset;
					fprintf(of, "%x %x %u %u\n", lines[i].offset + func.offset - modifier, size, lines[i].flags & CV_Line_Flags::linenumStart, func.fileIndex);
				}
			}
		}
		else if (func.length)
		{
			fprintf(of, "FUNC %x %x %x %s\n", func.offset, func.length, func.paramSize, func.name.data);
		}
		else
		{
			fprintf(of, "PUBLIC %x %x %s\n", func.offset, func.paramSize, func.name.data);
		}
	}
}

template<typename T>
void
PDBParser::readFPO(uint32_t fpoStream, std::map<std::pair<uint32_t, uint32_t>, DataPtr<T>>& fpoData)
{
	auto& fs = getStream(fpoStream);

	StreamReader reader(fs, *this);

	T last = {};
	while (reader.getOffset() < fs.size)
	{
		auto fh = reader.read<T>();
		// PDB files contain lots of duplicated FPO records.
		if (fh.data->ulOffStart != last.ulOffStart || fh.data->cbProcSize != last.cbProcSize || fh.data->cbProlog != last.cbProlog)
		{
			last = *fh.data;
			fpoData.insert(std::make_pair(std::make_pair(fh.data->ulOffStart, fh.data->cbProcSize), std::move(fh)));
		}
	}
}

template<typename T>
bool
PDBParser::updateParamSize(FunctionRecord& func, std::map<std::pair<uint32_t, uint32_t>, DataPtr<T>>& fpoData)
{
	auto p = std::make_pair(func.offset, func.length);
	auto it = fpoData.find(p);
	if (it != fpoData.end())
	{
		updateParamSize(func, *it->second.data);
		return true;
	}
	return false;
}

bool
PDBParser::updateParamSize(FunctionRecord& func, Globals& globals)
{
	auto g = globals.find(func.offset);
	if (g != globals.end())
	{
		const char* name = g->second.data;
		// stdcall and fastcall functions have their param size embedded in the decorated name
		if (name[0] == '@' || name[0] == '_')
		{
			const char* p = strrchr(name, '@');
			if (p && p != name)
			{
				char* end;
				long val = strtol(p + 1, &end, 10);
				if (*end == '\0')
				{
					func.paramSize = val;
					// fastcall functions accept up to 8 bytes of parameters in registers
					if (name[0] == '@')
					{
						if (val > 8)
						{
							func.paramSize -= 8;
						}
						else
						{
							func.paramSize = 0;
						}
					}
					return true;
				}
			}
		}
	}
	return false;
}

void
PDBParser::updateParamSize(FunctionRecord& func, const FPO_DATA& fpoData)
{
	func.paramSize = fpoData.cdwParams * 4;
}

void
PDBParser::updateParamSize(FunctionRecord& func, const FPO_DATA_V2& fpoData)
{
	func.paramSize = fpoData.cbParams;
}

template<typename T>
void
PDBParser::printFPOs(std::map<std::pair<uint32_t, uint32_t>, DataPtr<T>>& fpoData, const NameStream& names, FILE* of)
{
	for (auto& f : fpoData)
	{
		printFPO(*f.second.data, names, of);
	}
}

void
PDBParser::printFPO(const FPO_DATA& data, const NameStream& names, FILE* of)
{
	(void)names;
	fprintf(of, "STACK WIN 0 %x %x %x %x %x %x %x %x 0 %d\n",
	data.ulOffStart, data.cbProcSize, data.cbProlog, 0, data.cdwParams, data.cbRegs, data.cdwLocals, 0, data.fUseBP);
}

void
PDBParser::printFPO(const FPO_DATA_V2& data, const NameStream& names, FILE* of)
{
	fprintf(of, "STACK WIN 4 %x %x %x %x %x %x %x %x 1 ",
		data.ulOffStart, data.cbProcSize, data.cbProlog, 0, data.cbParams, data.cbSavedRegs, data.cbLocals, data.maxStack);
	auto iter = names.map.find(data.ProgramStringOffset);
	if (iter != names.map.end())
	{
		fprintf(of, "%s", iter->second.data);
	}
	fprintf(of, "\n");
}

bool
PDBParser::stringizeType(uint32_t type, std::string& output, const TypeMap& tm, uint32_t flags)
{
	if (type == 0)
	{
		output.append("...", 3);
		return false;
	}

	auto ti = tm.find(type);
	if (ti == tm.end())
	{
		switch (type & 0xff)
		{
		case TYPE_ENUM::T_VOID:
			output.append("void", 4);
			break;
			// These ones don't follow the pattern
		case TYPE_ENUM::T_PVOID:
		case TYPE_ENUM::T_PFVOID:
		case TYPE_ENUM::T_PHVOID:
			output.append("void *", 5);
			break;
		case TYPE_ENUM::T_HRESULT: // Thanks Microsoft!
			output.append("long", 4);
			break;
		case TYPE_ENUM::T_INT1:
		case TYPE_ENUM::T_CHAR:
			output.append("signed char", 11);
			break;
		case TYPE_ENUM::T_RCHAR: // I have no idea what a "really char" is
			output.append("char", 4);
			break;
		case TYPE_ENUM::T_UINT1:
		case TYPE_ENUM::T_UCHAR:
			output.append("unsigned char", 13);
			break;
		case TYPE_ENUM::T_WCHAR:
			output.append("wchar_t", 7);
			break;
		case TYPE_ENUM::T_SHORT:
		case TYPE_ENUM::T_INT2:
			output.append("short", 5);
			break;
		case TYPE_ENUM::T_USHORT:
		case TYPE_ENUM::T_UINT2:
			output.append("unsigned short", 14);
			break;
		case TYPE_ENUM::T_LONG:
			output.append("long", 4);
			break;
		case TYPE_ENUM::T_INT4:
			output.append("int", 3);
			break;
		case TYPE_ENUM::T_ULONG:
			output.append("unsigned long", 13);
			break;
		case TYPE_ENUM::T_UINT4:
			output.append("unsigned int", 12);
			break;
		case TYPE_ENUM::T_QUAD:
		case TYPE_ENUM::T_INT8:
			output.append("__int64", 7);
			break;
		case TYPE_ENUM::T_UQUAD:
		case TYPE_ENUM::T_UINT8:
			output.append("unsigned __int64", 16);
			break;
		case TYPE_ENUM::T_OCT:
		case TYPE_ENUM::T_INT16:
			output.append("s128", 4);
			break;
		case TYPE_ENUM::T_UOCT:
		case TYPE_ENUM::T_UINT16:
			output.append("u128", 4);
			break;
		case TYPE_ENUM::T_REAL32:
			output.append("float", 5);
			break;
		case TYPE_ENUM::T_REAL64:
			output.append("double", 6);
			break;
		case TYPE_ENUM::T_REAL80:
			// I don't think this actually exists anymore.
			output.append("long double", 11);
			break;
		case TYPE_ENUM::T_REAL128:
			output.append("f128", 4);
			break;
		case TYPE_ENUM::T_BOOL08:
		case TYPE_ENUM::T_BOOL16:
		case TYPE_ENUM::T_BOOL32:
		case TYPE_ENUM::T_BOOL64:
			output.append("bool", 4);
			break;
		default:
			output.append("!Unknown!", 9);
			return false;
		}

		// Check to see if it is a pointer type, thankfully the enum values are consistent
		if (type & (0x0600 | 0x0400))
			output.append(" *", 2);

		return false;
	}

	const uint8_t* data = ti->second.data.data;

	switch (ti->second.type)
	{
		// const/volatile/unaligned
	case LEAF::LF_MODIFIER:
		{
			const LeafModifier* lm = (const LeafModifier*)data;
			stringizeType(lm->type, output, tm, 0);

			if (flags & (IsUnderlying | ~IsTopLevel))
				return false;

			switch (lm->attr)
			{
			case CV_modifier::MOD_const:
				output.append(" const", 6);
				break;
			case CV_modifier::MOD_volatile:
				output.append(" volatile", 9);
				break;
			case CV_modifier::MOD_unaligned:
				output.append(" unaligned", 10);
				break;
			}
		}
		break;
		// The argument list for a function definition
	case LEAF::LF_ARGLIST:
		{
			const LeafArgList* lal = (const LeafArgList*)data;
			output.append("(", 1);

			if (lal->count == 0 && (flags & ~IsTopLevel))
			{
				output.append(")", 1);
				return false;
			}

			const uint32_t* type = (const uint32_t*)lal;
			for (uint32_t i = 0; i < lal->count; ++i)
			{
				stringizeType(*++type, output, tm, flags);

				if (i != lal->count - 1)
					output.append(",", 1);
			}

			output.append(")", 1);
		}
		break;
		// A pointer, with an underlying type
	case LEAF::LF_POINTER:
		{
			const LeafPointer* lp = (const LeafPointer*)data;

			if (!stringizeType(lp->utype, output, tm, IsUnderlying & flags))
			{
				switch ((lp->attr & LeafPointerAttr::ptrmode) >> 5)
				{
				case CV_ptrmode::CV_PTR_MODE_REF:
					output.append(" &", 2);
					break;
				case CV_ptrmode::CV_PTR_MODE_PTR:
					output.append(" *", 2);
					break;
				case CV_ptrmode::CV_PTR_MODE_PMEM:
					output.append("::*", 3);
					break;
				case CV_ptrmode::CV_PTR_MODE_PMFUNC:
					output.append("::", 2);
					break;
				case CV_ptrmode::CV_PTR_MODE_RESERVED: // This is now being used for r-value references
					output.append("&&", 2);
					break;
				default:
					fprintf(stderr, "Unknown ptr type encountered\n");
					break;
				}
			}

			if (lp->attr & LeafPointerAttr::isconst)
				output.append(" const", 6);

			if (lp->attr & LeafPointerAttr::isvolatile)
				output.append(" volatile", 9);

			// restrict could be added, but not necessarily interesting?
		}
		break;
	case LEAF::LF_ARRAY:
		{
			const LeafArray* la = (const LeafArray*)data;

			stringizeType(la->elemtype, output, tm, 0);

			output.append("[", 1);
			// According to the comments, if this value is less than 0x8000 then the next 2 bytes are the actual value
			if (la->idxtype < 0x8000)
				output += std::to_string(*((uint16_t*)(data + sizeof(LeafArray))));
			else
				stringizeType(la->idxtype, output, tm, 0);
			output.append("]", 1);
		}
		break;
	case LEAF::LF_MFUNCTION:
		{
			const LeafMFunc* lmf = (const LeafMFunc*)data;

			if (flags & IsUnderlying)
			{
				stringizeType(lmf->rvtype, output, tm, 0);
				output.append(" (", 2);
				stringizeType(lmf->classtype, output, tm, 0);
				output.append("::*)", 4);
			}

			stringizeType(lmf->arglist, output, tm, 0);
		}
		return true;
	case LEAF::LF_PROCEDURE:
		{
			const LeafProc* proc = (const LeafProc*)data;

			if (flags & IsUnderlying)
			{
				stringizeType(proc->rvtype, output, tm, 0);
				output.append(" (*)", 4);
			}

			stringizeType(proc->arglist, output, tm, 0);
		}
		return true;
	case LEAF::LF_INDEX:
		{
			const LeafIndex* li = (const LeafIndex*)data;
			stringizeType(li->index, output, tm, flags);
		}
		break;
		// All types past this point are leaf types that terminate recursion
	case LEAF::LF_ENUM:
	case LEAF::LF_ALIAS:
	case LEAF::LF_UNION:
	case LEAF::LF_CLASS:
	case LEAF::LF_STRUCTURE:
		{
			output += ti->second.name.data;
		}
		break;
	case LEAF::LF_CHAR:
		{
			const LeafChar* ch = (const LeafChar*)data;
			output += ch->val;
		}
		break;
	case LEAF::LF_SHORT:
		{
			const LeafShort* sh = (const LeafShort*)data;
			output += std::to_string(sh->val);
		}
		break;
	case LEAF::LF_USHORT:
		{
			const LeafUShort* sh = (const LeafUShort*)data;
			output += std::to_string(sh->val);
		}
		break;
	case LEAF::LF_LONG:
		{
			const LeafLong* ll = (const LeafLong*)data;
			output += std::to_string(ll->val);
		}
		break;
	case LEAF::LF_ULONG:
		{
			const LeafULong* ll = (const LeafULong*)data;
			output += std::to_string(ll->val);
		}
		break;
	case LEAF::LF_REAL32:
		{
			const LeafReal32* ll = (const LeafReal32*)data;
			output += std::to_string(ll->val);
		}
		break;
	case LEAF::LF_REAL64:
		{
			const LeafReal64* ll = (const LeafReal64*)data;
			output += std::to_string(ll->val);
		}
		break;
	case LEAF::LF_REAL80:
		output.append("f80");
		break;
	case LEAF::LF_REAL128:
		output.append("f128");
		break;
	case LEAF::LF_QUADWORD:
		{
			const LeafQuad* ll = (const LeafQuad*)data;
			output += std::to_string(ll->val);
		}
		break;
	case LEAF::LF_UQUADWORD:
		{
			const LeafUQuad* ll = (const LeafUQuad*)data;
			output += std::to_string(ll->val);
		}
		break;
	default:
		// Unhandled...these are the only records I encountered, would need to be extended for managed code
		break;
	}

	return false;
}

}
