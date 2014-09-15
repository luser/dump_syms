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

#pragma once

#include <functional>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <unordered_map>
#include <windows.h>

#include "PDBHeaders.h"

typedef void* HANDLE;
typedef IMAGE_SECTION_HEADER SectionHeader;

namespace google_breakpad
{

class StreamReader;

template<typename T>
struct DataPtr
{
	const T*	data;
	bool		isAllocated;

	DataPtr(const void* data, bool isAllocated = false)
		: data((T*)data)
		, isAllocated(isAllocated)
	{}

	DataPtr(DataPtr&& other)
		: data(nullptr)
		, isAllocated(false)
	{
		*this = std::move(other);
	}

	DataPtr& operator =(DataPtr&& other)
	{
		std::swap(data, other.data);
		std::swap(isAllocated, other.isAllocated);

		return *this;
	}

	~DataPtr()
	{
		if (isAllocated)
			free((void*)data);
	}

	const T* operator ->() const
	{
		return data;
	}

	DataPtr()
		: data(nullptr)
		, isAllocated(false)
	{}

private:

	DataPtr& operator =(const DataPtr&) { return *this; }
};

class PDBParser
{
public:

	PDBParser()
		: m_base(nullptr)
		, m_mapFile(0)
	{}

	~PDBParser() { close(); }
	
	void load(const char* path);
	
	void close();

	enum KnownStreams
	{
		Root,
		Streams,
		TypeInfoStream,
		DebugInfo
	};

	typedef std::function<const char*(const char*, size_t)> FileMod;

	void printBreakpadSymbols(FILE* of, const char* platform = nullptr, FileMod* file = nullptr);

	const uint32_t pageSize() const { return m_pageSize; }
	const uint8_t* data() const { return m_base; }

	struct StreamPair
	{
		uint32_t				size;
		std::vector<uint32_t>	pageIndices;

		StreamPair(uint32_t size)
			: size(size)
		{}
	};

	const StreamPair& getStream(int index) const { return m_streams[index]; }

private:

	struct FunctionRecord
	{
		DataPtr<char>		name;
		DataPtr<uint8_t>	lines;
		uint32_t			lineCount;
		uint32_t			segment;
		uint32_t			offset;
		uint32_t			fileIndex;
		uint32_t			length;
		uint32_t			lineOffset;
		uint32_t			typeIndex;	// If this is non-zero, the function is a procedure, not a thunk (I don't know how to read thunk type info...)

		FunctionRecord(uint32_t offset, uint32_t segment)
			: offset(offset)
			, segment(segment)
		{}

		FunctionRecord(DataPtr<char>&& iname)
			: name(std::move(iname))
			, segment(0)
			, offset(0)
			, typeIndex(0)
			, lineCount(0)
			, lineOffset(0)
			, fileIndex(0)
		{}

		bool operator <(const FunctionRecord& other) const
		{
			if (segment != other.segment)
				return segment < other.segment;
			else if (offset != other.offset)
				return offset < other.offset;
			else
			{
				// This is merely to make the sort algorithm not throw a shit fit
				// at this point we have no way to differentiate between two functions
				// so we will have to let one of them 'win' after the sort is finished
				return typeIndex < other.typeIndex;
			}
		}

		bool operator ==(const FunctionRecord& other) const
		{
			return segment == other.segment && offset == other.offset;
		}

		FunctionRecord(FunctionRecord&& other)
		{
			*this = std::move(other);
		}

		FunctionRecord& operator =(FunctionRecord&& other);

	private:

		FunctionRecord(){}
		FunctionRecord(const FunctionRecord&) {}
		FunctionRecord& operator =(const FunctionRecord&) { return *this; }
	};

	struct TypeInfo
	{
		DataPtr<uint8_t>	data;
		DataPtr<char>		name;
		LEAF::Enum			type;

		TypeInfo() {}

		TypeInfo(TypeInfo&& ti)
		{
			*this = std::move(ti);
		}

		TypeInfo& operator =(TypeInfo&& other)
		{
			std::swap(data, other.data);
			std::swap(name, other.name);
			std::swap(type, other.type);

			return *this;
		}

	private:

		TypeInfo(const TypeInfo&) {}
		TypeInfo& operator =(const TypeInfo&) { return *this; }
	};

	bool readRootStream();

	struct UniqueSrc
	{
		uint32_t id;
		uint32_t visited;

		UniqueSrc()
			: id(0)
			, visited(0)
		{}
	};

	typedef std::map<uint32_t, DataPtr<char>> NameMap;
	typedef std::map<uint32_t, uint32_t> SrcFileIndex;
	typedef std::unordered_map<uint32_t, UniqueSrc> UniqueSrcFiles;
	typedef std::vector<FunctionRecord> Functions;
	typedef std::unordered_map<uint32_t, TypeInfo> TypeMap;
	typedef std::vector<SectionHeader> SectionHeaders;

	struct NameStream
	{
		NameMap map;
		char*	buffer;

		NameStream() : buffer(nullptr){}
		~NameStream() { delete [] buffer; }
	};

	// The name stream maps file indices with the path of the source file
	void loadNameStream(NameStream& ns);
	// The type stream maps a type id to a description of that type
	TypeMap loadTypeStream();

	enum StringizeFlags
	{
		IsUnderlying = 0x1,
		IsTopLevel = 0x2
	};

	static bool stringizeType(uint32_t type, std::string& output, const TypeMap& tm, uint32_t flags);

	typedef std::function<void(StreamReader&, int32_t, uint32_t)> ModuleReadCB;
	void readModule(const DBIModuleInfo* module, int32_t section, ModuleReadCB cb);

	void printHeader(const DBIHeader* header, FILE* of, const char* platform = nullptr);
	void readSectionHeaders(uint32_t headerStream, SectionHeaders& headers);
	void getModuleFiles(const DBIModuleInfo* module, uint32_t& id, UniqueSrcFiles& unique, SrcFileIndex& fileIndex);
	void printFiles(const SrcFileIndex& fileIndex, FILE* of);
	void getModuleFunctions(const DBIModuleInfo* module, Functions& funcs);
	void getGlobalFunctions(uint16_t symRecStream, Functions& funcs);
	void resolveFunctionLines(const DBIModuleInfo* module, Functions& funcs, const UniqueSrcFiles& unique, const SrcFileIndex& fileIndex);
	void printFunctions(Functions& funcs, const SectionHeaders& headers, const TypeMap& tm, FILE* of);
	void readAndPrintFPOv2(uint32_t fpoStream, const NameStream& names, FILE* of);
	void printFPOv2(const FPO_DATA_V2& data, const NameStream& names, FILE* of);

	std::vector<StreamPair>			m_streams;
	std::map<std::string, int32_t>	m_nameIndices;

	GUID			m_guid;		//!< Unique GUID for the PDB, found in the NameIndexHeader in the root stream, matches the guid returned by IDiaSession::get_globalScope()->get_guid()

	const uint8_t*	m_base;
	HANDLE			m_mapFile;
	std::string		m_filename;

	uint32_t m_PETimeStamp;	//!< Timestamp for the executable
	uint32_t m_PESize;		//!< Size of the executable

	uint32_t	m_pageSize;
	uint32_t	m_numPages;
	bool		m_isExe;
}; // PDBParser

} // google_breakpad
