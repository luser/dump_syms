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

// Original author: Ted Mielczarek <ted@mielczarek.org>

#pragma once

#include <stdint.h>

namespace google_breakpad {
#pragma pack(push)

	struct IMAGE_DOS_HEADER
	{
		uint16_t e_magic;
		uint16_t e_cblp;
		uint16_t e_cp;
		uint16_t e_crlc;
		uint16_t e_cparhdr;
		uint16_t e_minalloc;
		uint16_t e_maxalloc;
		uint16_t e_ss;
		uint16_t e_sp;
		uint16_t e_csum;
		uint16_t e_ip;
		uint16_t e_cs;
		uint16_t e_lfarlc;
		uint16_t e_ovno;
		uint16_t e_res[4];
		uint16_t e_oemid;
		uint16_t e_oeminfo;
		uint16_t e_res2[10];
		uint32_t e_lfanew;
	};

	struct IMAGE_FILE_HEADER
	{
		uint16_t Machine;
		uint16_t NumberOfSections;
		uint32_t TimeDateStamp;
		uint32_t PointerToSymbolTable;
		uint32_t NumberOfSymbols;
		uint16_t SizeOfOptionalHeader;
		uint16_t Characteristics;
	};

	const int IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR = 14;

	struct IMAGE_DATA_DIRECTORY
	{
		uint32_t VirtualAddress;
		uint32_t Size;
	};

	const int IMAGE_NUMBEROF_DIRECTORY_ENTRIES = 16;

	struct IMAGE_OPTIONAL_HEADER
	{
		uint16_t Magic;
		uint8_t MajorLinkerVersion;
		uint8_t MinorLinkerVersion;
		uint32_t SizeOfCode;
		uint32_t SizeOfInitializedData;
		uint32_t SizeOfUninitializedData;
		uint32_t AddressOfEntryPoint;
		uint32_t BaseOfCode;
		uint32_t BaseOfData;
		uint32_t ImageBase;
		uint32_t SectionAlignment;
		uint32_t FileAlignment;
		uint16_t MajorOperatingSystemVersion;
		uint16_t MinorOperatingSystemVersion;
		uint16_t MajorImageVersion;
		uint16_t MinorImageVersion;
		uint16_t MajorSubsystemVersion;
		uint16_t MinorSubsystemVersion;
		uint32_t Win32VersionValue;
		uint32_t SizeOfImage;
		uint32_t SizeOfHeaders;
		uint32_t CheckSum;
		uint16_t Subsystem;
		uint16_t DllCharacteristics;
		uint32_t SizeOfStackReserve;
		uint32_t SizeOfStackCommit;
		uint32_t SizeOfHeapReserve;
		uint32_t SizeOfHeapCommit;
		uint32_t LoaderFlags;
		uint32_t NumberOfRvaAndSizes;
		IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
	};

	const uint16_t IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x10b;
	const uint16_t IMAGE_NT_OPTIONAL_HDR64_MAGIC = 0x20b;

	struct IMAGE_OPTIONAL_HEADER64
	{
		uint16_t Magic;
		uint8_t MajorLinkerVersion;
		uint8_t MinorLinkerVersion;
		uint32_t SizeOfCode;
		uint32_t SizeOfInitializedData;
		uint32_t SizeOfUninitializedData;
		uint32_t AddressOfEntryPoint;
		uint32_t BaseOfCode;
		uint64_t ImageBase;
		uint32_t SectionAlignment;
		uint32_t FileAlignment;
		uint16_t MajorOperatingSystemVersion;
		uint16_t MinorOperatingSystemVersion;
		uint16_t MajorImageVersion;
		uint16_t MinorImageVersion;
		uint16_t MajorSubsystemVersion;
		uint16_t MinorSubsystemVersion;
		uint32_t Win32VersionValue;
		uint32_t SizeOfImage;
		uint32_t SizeOfHeaders;
		uint32_t CheckSum;
		uint16_t Subsystem;
		uint16_t DllCharacteristics;
		uint64_t SizeOfStackReserve;
		uint64_t SizeOfStackCommit;
		uint64_t SizeOfHeapReserve;
		uint64_t SizeOfHeapCommit;
		uint32_t LoaderFlags;
		uint32_t NumberOfRvaAndSizes;
		IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
	};

	struct IMAGE_NT_HEADERS64
	{
		uint32_t Signature;
		IMAGE_FILE_HEADER FileHeader;
		IMAGE_OPTIONAL_HEADER64 OptionalHeader;
	};

	struct IMAGE_NT_HEADERS32
	{
		uint32_t Signature;
		IMAGE_FILE_HEADER FileHeader;
		IMAGE_OPTIONAL_HEADER OptionalHeader;
	};

	const int IMAGE_SIZEOF_SHORT_NAME = 8;

	struct IMAGE_SECTION_HEADER
	{
		uint8_t Name[IMAGE_SIZEOF_SHORT_NAME];
		union {
			uint32_t PhysicalAddress;
			uint32_t VirtualSize;
		};
		uint32_t VirtualAddress;
		uint32_t SizeOfRawData;
		uint32_t PointerToRawData;
		uint32_t PointerToRelocations;
		uint32_t PointerToLinenumbers;
		uint16_t NumberOfRelocations;
		uint16_t NumberOfLinenumbers;
		uint32_t Characteristics;
	};

	struct FPO_DATA
	{
		uint32_t ulOffStart;
		uint32_t cbProcSize;
		uint32_t cdwLocals;
		uint16_t  cdwParams;
		uint16_t  cbProlog  :8;
		uint16_t  cbRegs  :3;
		uint16_t  fHasSEH  :1;
		uint16_t  fUseBP  :1;
		uint16_t  reserved  :1;
		uint16_t  cbFrame  :2;
	};

	const uint16_t IMAGE_FILE_MACHINE_I386 = 0x014c;
	const uint16_t IMAGE_FILE_MACHINE_AMD64 = 0x8664;
	const uint16_t IMAGE_FILE_MACHINE_ARM = 0x01c4;

#pragma pack(pop)
}
