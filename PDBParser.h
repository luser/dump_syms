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

#pragma once

#include <functional>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <unordered_map>
#include <stdlib.h>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "PDBHeaders.h"
#ifndef _WIN32
#include "WinStructs.h"
#endif

typedef void* HANDLE;

namespace google_breakpad
{

typedef IMAGE_SECTION_HEADER SectionHeader;
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

class MMapWrapper
{
public:
	MMapWrapper() :
#ifdef _WIN32
		m_mapFile(0),
#else
		m_length(0),
#endif
		m_base(nullptr)
	{}

	bool Map(const char* filename);
	bool Unmap();
	bool Valid()
	{
#ifdef _WIN32
		return m_mapFile != nullptr && m_base != nullptr;
#else
		return m_base != nullptr;
#endif
	}
	const uint8_t* base() const { return m_base; }
private:
#ifdef _WIN32
	HANDLE			m_mapFile;
#else
	size_t			m_length;
#endif
	const uint8_t*	m_base;
};

class PDBParser
{
public:

	PDBParser()
		: m_base(nullptr)
		, m_foundPE(false)
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
		uint32_t			paramSize;

		FunctionRecord(uint32_t offset, uint32_t segment)
			: segment(segment)
			, offset(offset)

		{}

		FunctionRecord(DataPtr<char>&& iname)
			: name(std::move(iname))
			, lineCount(0)
			, segment(0)
			, offset(0)
			, fileIndex(0)
			, length(0)
			, lineOffset(0)
			, typeIndex(0)
			, paramSize(0)
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
	typedef std::unordered_map<uint32_t, DataPtr<char>> Globals;
	// If we decide to only support VC2013 we can use this.
	//template<typename T>
	//using FPODataMap = std::map<std::pair<uint32_t, uint32_t>, DataPtr<T>>;

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
	void getGlobalFunctions(uint16_t symRecStream, const SectionHeaders& headers, Globals& globals);
	void resolveFunctionLines(const DBIModuleInfo* module, Functions& funcs, const UniqueSrcFiles& unique, const SrcFileIndex& fileIndex);
	void printFunctions(Functions& funcs, const TypeMap& tm, FILE* of);
	template<typename T>
	void readFPO(uint32_t fpoStream, std::map<std::pair<uint32_t, uint32_t>, DataPtr<T>>& fpoData);
	template<typename T>
	bool updateParamSize(FunctionRecord& func, std::map<std::pair<uint32_t, uint32_t>, DataPtr<T>>& fpoData);
	bool updateParamSize(FunctionRecord& func, Globals& globals);
	void updateParamSize(FunctionRecord& func, const FPO_DATA& fpoData);
	void updateParamSize(FunctionRecord& func, const FPO_DATA_V2& fpoData);
	template<typename T>
	void printFPOs(std::map<std::pair<uint32_t, uint32_t>, DataPtr<T>>& fpoData, const NameStream& names, FILE* of);
	void printFPO(const FPO_DATA& data, const NameStream& names, FILE* of);
	void printFPO(const FPO_DATA_V2& data, const NameStream& names, FILE* of);

	std::vector<StreamPair>			m_streams;
	std::map<std::string, int32_t>	m_nameIndices;

	GUID			m_guid;		//!< Unique GUID for the PDB, found in the NameIndexHeader in the root stream, matches the guid returned by IDiaSession::get_globalScope()->get_guid()

	const uint8_t*	m_base;
	MMapWrapper		m_mapping;
	std::string		m_filename;

	bool m_foundPE;
	uint32_t m_PETimeStamp;	//!< Timestamp for the executable
	uint32_t m_PESize;		//!< Size of the executable

	uint32_t	m_pageSize;
	uint32_t	m_numPages;
	bool		m_isExe;
}; // PDBParser

} // google_breakpad
