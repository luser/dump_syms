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

#include <stdint.h>
#ifdef _WIN32
#include <Guiddef.h>
#endif

namespace google_breakpad
{
// All the structs here are read in-place from the PDB, so they need to be packed to match their figurative size
#pragma pack(push, 1)
#ifndef _WIN32
	struct GUID
	{
		uint32_t	Data1;
		uint16_t	Data2;
		uint16_t	Data3;
		uint8_t		Data4[8];
	};
#endif

	struct PDBHeader
	{
		char		signature[32];
		int32_t		pageSize;
		int32_t		freePageMap;
		int32_t		pagesUsed;
		int32_t		directorySize;
		uint32_t	reserved;
	};

	struct NameIndexHeader
	{
		uint32_t version;
		uint32_t timeDateStamp;
		uint32_t age;
		GUID	 guid;
		uint32_t names;
	};

	struct DBIHeader
	{
		uint32_t	signature;				// 0xFFFFFFFF
		uint32_t	version;
		uint32_t	age;
		int16_t		gssymStream;
		uint16_t	vers;
		int16_t		pssymStream;
		uint16_t	pdbVersion;
		int16_t		symRecordStream;	// Stream index containing global symbols
		uint16_t	pdbVersion2;
		uint32_t	moduleSize;
		uint32_t	secConSize;
		uint32_t	secMapSize;
		uint32_t	fileInfoSize;
		uint32_t	srcModuleSize;
		uint32_t	mfcIndex;
		uint32_t	dbgHeaderSize;
		uint32_t	ecInfoSize;
		uint16_t	flags;
		uint16_t	machine;			// ImageFileMachine
		uint32_t	reserved;
	};

	struct DBIDebugHeader
	{
		uint16_t FPO;
		uint16_t exception;
		uint16_t fixup;
		uint16_t omapToSource;
		uint16_t omapFromSource;
		uint16_t sectionHdr;
		uint16_t tokenRidMap;
		uint16_t XData;
		uint16_t PData;
		uint16_t newFPO;
		uint16_t sectionHdrOriginal;
	};

	struct DBISecCon
	{
		int16_t	 section;
		uint16_t padding1;
		int32_t  offset;
		uint32_t size;
		uint32_t flags;
		int16_t  module;
		uint16_t pad2;
		uint32_t dataCrc;
		uint32_t relocCrc;
	};

	struct DBIModuleInfo
	{
		int32_t		opened;
		DBISecCon	sector;
		uint16_t	lags;
		int16_t		stream;
		int32_t		cbSyms;
		int32_t		cbOldLines;
		int32_t		cbLines;
		int16_t		files;
		int16_t		padding;
		uint32_t	offsets;
		int32_t		niSource;
		int32_t		niCompiler;

		// const char* moduleName;
		// const char* objectName;
	};

	struct OffCb
	{
		int32_t	off;
		int32_t cb;
	};

	struct TypeInfoHeader
	{
		uint32_t	version;
		int32_t		headerSize;
		uint32_t	min;
		uint32_t	max;
		uint32_t	followSize;

		uint16_t	sn;
		uint16_t	padding;
		int32_t		hashKey;
		int32_t		buckets;
		OffCb		hashVals;
		OffCb		tiOff;
		OffCb		hashAdjust;
	};

	struct float10
	{
		int8_t val[10];
	};

	struct CV_SIGNATURE
	{
		enum Enum
		{
			C6=0,    // Actual signature is >64K
			C7=1,    // First explicit signature
			C11=2,    // C11 (vc5.x) 32-bit types
			C13=4,    // C13 (vc7.x) zero terminated names
			RESERVERD=5,    // All signatures from 5 to 64K are reserved
		};
	};


	//  CodeView Symbol and Type OMF type information is broken up into two
	//  ranges.  Type indices less than 0x1000 describe type information
	//  that is frequently used.  Type indices above 0x1000 are used to
	//  describe more complex features such as functions, arrays and
	//  structures.
	//

	//  Primitive types have predefined meaning that is encoded in the
	//  values of the various bit fields in the value.
	//
	//  A CodeView primitive type is defined as:
	//
	//  1 1
	//  1 089  7654  3  210
	//  r mode type  r  sub
	//
	//  Where
	//      mode is the pointer mode
	//      type is a type indicator
	//      sub  is a subtype enumeration
	//      r    is a reserved field
	//
	//  See Microsoft Symbol and Type OMF (Version 4.0) for more
	//  information.
	//

	//  pointer mode enumeration values

	struct CV_prmode
	{
		enum Enum
		{
			CV_TM_DIRECT=0,        // mode is not a pointer
			CV_TM_NPTR32=4,        // mode is a 32 bit near pointer
			CV_TM_NPTR64=6,        // mode is a 64 bit near pointer
			CV_TM_NPTR128=7,        // mode is a 128 bit near pointer
		};
	};

	//  type enumeration values

	struct CV_type
	{
		enum Enum
		{
			CV_SPECIAL=0x00,     // special type size values
			CV_SIGNED=0x01,     // signed integral size values
			CV_UNSIGNED=0x02,     // unsigned integral size values
			CV_BOOLEAN=0x03,     // Boolean size values
			CV_REAL=0x04,     // real number size values
			CV_COMPLEX=0x05,     // complex number size values
			CV_SPECIAL2=0x06,     // second set of special types
			CV_INT=0x07,     // integral (int) values
			CV_CVRESERVED=0x0f,
		};
	};

	//  subtype enumeration values for CV_SPECIAL

	struct CV_special
	{
		enum Enum
		{
			CV_SP_NOTYPE=0x00,
			CV_SP_ABS=0x01,
			CV_SP_SEGMENT=0x02,
			CV_SP_VOID=0x03,
			CV_SP_CURRENCY=0x04,
			CV_SP_NBASICSTR=0x05,
			CV_SP_FBASICSTR=0x06,
			CV_SP_NOTTRANS=0x07,
			CV_SP_HRESULT=0x08,
		};
	};

	//  subtype enumeration values for CV_SPECIAL2

	struct CV_special2
	{
		enum Enum
		{
			CV_S2_BIT=0x00,
			CV_S2_PASCHAR=0x01,     // Pascal CHAR
		};
	};

	//  subtype enumeration values for CV_SIGNED, CV_UNSIGNED and CV_BOOLEAN

	struct CV_integral
	{
		enum Enum
		{
			CV_IN_1BYTE=0x00,
			CV_IN_2BYTE=0x01,
			CV_IN_4BYTE=0x02,
			CV_IN_8BYTE=0x03,
			CV_IN_16BYTE=0x04,
		};
	};

	//  subtype enumeration values for CV_REAL and CV_COMPLEX

	struct CV_real
	{
		enum Enum
		{
			CV_RC_REAL32=0x00,
			CV_RC_REAL64=0x01,
			CV_RC_REAL80=0x02,
			CV_RC_REAL128=0x03,
		};
	};

	//  subtype enumeration values for CV_INT (really int)

	struct CV_int
	{
		enum Enum
		{
			CV_RI_CHAR=0x00,
			CV_RI_INT1=0x00,
			CV_RI_WCHAR=0x01,
			CV_RI_UINT1=0x01,
			CV_RI_INT2=0x02,
			CV_RI_UINT2=0x03,
			CV_RI_INT4=0x04,
			CV_RI_UINT4=0x05,
			CV_RI_INT8=0x06,
			CV_RI_UINT8=0x07,
			CV_RI_INT16=0x08,
			CV_RI_UINT16=0x09,
		};
	};

	struct CV_PRIMITIVE_TYPE
	{
		static const uint32_t CV_MMASK     = 0x700;       // mode mask
		static const uint32_t CV_TMASK     = 0x0f0;       // type mask
		static const uint32_t CV_SMASK     = 0x00f;       // subtype mask

		static const int CV_MSHIFT     = 8;           // primitive mode right shift count
		static const int CV_TSHIFT     = 4;           // primitive type right shift count
		static const int CV_SSHIFT     = 0;           // primitive subtype right shift count

		static const uint32_t CV_FIRST_NONPRIM = 0x1000;
	};

	// selected values for type_index - for a more complete definition, see
	// Microsoft Symbol and Type OMF document

	//  Special Types

	struct TYPE_ENUM
	{
		enum Enum
		{
			T_NOTYPE=0x0000,   // uncharacterized type (no type)
			T_ABS=0x0001,   // absolute symbol
			T_SEGMENT=0x0002,   // segment type
			T_VOID=0x0003,   // void
			T_HRESULT=0x0008,   // OLE/COM HRESULT
			T_32PHRESULT=0x0408,   // OLE/COM HRESULT __ptr32//
			T_64PHRESULT=0x0608,   // OLE/COM HRESULT __ptr64//
			T_PVOID=0x0103,   // near pointer to void
			T_PFVOID=0x0203,   // far pointer to void
			T_PHVOID=0x0303,   // huge pointer to void
			T_32PVOID=0x0403,   // 32 bit pointer to void
			T_64PVOID=0x0603,   // 64 bit pointer to void
			T_CURRENCY=0x0004,   // BASIC 8 byte currency value
			T_NOTTRANS=0x0007,   // type not translated by cvpack
			T_BIT=0x0060,   // bit
			T_PASCHAR=0x0061,   // Pascal CHAR

			//  Character types

			T_CHAR=0x0010,   // 8 bit signed
			T_32PCHAR=0x0410,   // 32 bit pointer to 8 bit signed
			T_64PCHAR=0x0610,   // 64 bit pointer to 8 bit signed

			T_UCHAR=0x0020,   // 8 bit unsigned
			T_32PUCHAR=0x0420,   // 32 bit pointer to 8 bit unsigned
			T_64PUCHAR=0x0620,   // 64 bit pointer to 8 bit unsigned

			//  really a character types

			T_RCHAR=0x0070,   // really a char
			T_32PRCHAR=0x0470,   // 32 bit pointer to a real char
			T_64PRCHAR=0x0670,   // 64 bit pointer to a real char

			//  really a wide character types

			T_WCHAR=0x0071,   // wide char
			T_32PWCHAR=0x0471,   // 32 bit pointer to a wide char
			T_64PWCHAR=0x0671,   // 64 bit pointer to a wide char

			//  8 bit int types

			T_INT1=0x0068,   // 8 bit signed int
			T_32PINT1=0x0468,   // 32 bit pointer to 8 bit signed int
			T_64PINT1=0x0668,   // 64 bit pointer to 8 bit signed int

			T_UINT1=0x0069,   // 8 bit unsigned int
			T_32PUINT1=0x0469,   // 32 bit pointer to 8 bit unsigned int
			T_64PUINT1=0x0669,   // 64 bit pointer to 8 bit unsigned int

			//  16 bit short types

			T_SHORT=0x0011,   // 16 bit signed
			T_32PSHORT=0x0411,   // 32 bit pointer to 16 bit signed
			T_64PSHORT=0x0611,   // 64 bit pointer to 16 bit signed

			T_USHORT=0x0021,   // 16 bit unsigned
			T_32PUSHORT=0x0421,   // 32 bit pointer to 16 bit unsigned
			T_64PUSHORT=0x0621,   // 64 bit pointer to 16 bit unsigned

			//  16 bit int types

			T_INT2=0x0072,   // 16 bit signed int
			T_32PINT2=0x0472,   // 32 bit pointer to 16 bit signed int
			T_64PINT2=0x0672,   // 64 bit pointer to 16 bit signed int

			T_UINT2=0x0073,   // 16 bit unsigned int
			T_32PUINT2=0x0473,   // 32 bit pointer to 16 bit unsigned int
			T_64PUINT2=0x0673,   // 64 bit pointer to 16 bit unsigned int

			//  32 bit long types

			T_LONG=0x0012,   // 32 bit signed
			T_ULONG=0x0022,   // 32 bit unsigned
			T_32PLONG=0x0412,   // 32 bit pointer to 32 bit signed
			T_32PULONG=0x0422,   // 32 bit pointer to 32 bit unsigned
			T_64PLONG=0x0612,   // 64 bit pointer to 32 bit signed
			T_64PULONG=0x0622,   // 64 bit pointer to 32 bit unsigned

			//  32 bit int types

			T_INT4=0x0074,   // 32 bit signed int
			T_32PINT4=0x0474,   // 32 bit pointer to 32 bit signed int
			T_64PINT4=0x0674,   // 64 bit pointer to 32 bit signed int

			T_UINT4=0x0075,   // 32 bit unsigned int
			T_32PUINT4=0x0475,   // 32 bit pointer to 32 bit unsigned int
			T_64PUINT4=0x0675,   // 64 bit pointer to 32 bit unsigned int

			//  64 bit quad types

			T_QUAD=0x0013,   // 64 bit signed
			T_32PQUAD=0x0413,   // 32 bit pointer to 64 bit signed
			T_64PQUAD=0x0613,   // 64 bit pointer to 64 bit signed

			T_UQUAD=0x0023,   // 64 bit unsigned
			T_32PUQUAD=0x0423,   // 32 bit pointer to 64 bit unsigned
			T_64PUQUAD=0x0623,   // 64 bit pointer to 64 bit unsigned

			//  64 bit int types

			T_INT8=0x0076,   // 64 bit signed int
			T_32PINT8=0x0476,   // 32 bit pointer to 64 bit signed int
			T_64PINT8=0x0676,   // 64 bit pointer to 64 bit signed int

			T_UINT8=0x0077,   // 64 bit unsigned int
			T_32PUINT8=0x0477,   // 32 bit pointer to 64 bit unsigned int
			T_64PUINT8=0x0677,   // 64 bit pointer to 64 bit unsigned int

			//  128 bit octet types

			T_OCT=0x0014,   // 128 bit signed
			T_32POCT=0x0414,   // 32 bit pointer to 128 bit signed
			T_64POCT=0x0614,   // 64 bit pointer to 128 bit signed

			T_UOCT=0x0024,   // 128 bit unsigned
			T_32PUOCT=0x0424,   // 32 bit pointer to 128 bit unsigned
			T_64PUOCT=0x0624,   // 64 bit pointer to 128 bit unsigned

			//  128 bit int types

			T_INT16=0x0078,   // 128 bit signed int
			T_32PINT16=0x0478,   // 32 bit pointer to 128 bit signed int
			T_64PINT16=0x0678,   // 64 bit pointer to 128 bit signed int

			T_UINT16=0x0079,   // 128 bit unsigned int
			T_32PUINT16=0x0479,   // 32 bit pointer to 128 bit unsigned int
			T_64PUINT16=0x0679,   // 64 bit pointer to 128 bit unsigned int

			//  32 bit real types

			T_REAL32=0x0040,   // 32 bit real
			T_32PREAL32=0x0440,   // 32 bit pointer to 32 bit real
			T_64PREAL32=0x0640,   // 64 bit pointer to 32 bit real

			//  64 bit real types

			T_REAL64=0x0041,   // 64 bit real
			T_32PREAL64=0x0441,   // 32 bit pointer to 64 bit real
			T_64PREAL64=0x0641,   // 64 bit pointer to 64 bit real

			//  80 bit real types

			T_REAL80=0x0042,   // 80 bit real
			T_32PREAL80=0x0442,   // 32 bit pointer to 80 bit real
			T_64PREAL80=0x0642,   // 64 bit pointer to 80 bit real

			//  128 bit real types

			T_REAL128=0x0043,   // 128 bit real
			T_32PREAL128=0x0443,   // 32 bit pointer to 128 bit real
			T_64PREAL128=0x0643,   // 64 bit pointer to 128 bit real

			//  32 bit complex types

			T_CPLX32=0x0050,   // 32 bit complex
			T_32PCPLX32=0x0450,   // 32 bit pointer to 32 bit complex
			T_64PCPLX32=0x0650,   // 64 bit pointer to 32 bit complex

			//  64 bit complex types

			T_CPLX64=0x0051,   // 64 bit complex
			T_32PCPLX64=0x0451,   // 32 bit pointer to 64 bit complex
			T_64PCPLX64=0x0651,   // 64 bit pointer to 64 bit complex

			//  80 bit complex types

			T_CPLX80=0x0052,   // 80 bit complex
			T_32PCPLX80=0x0452,   // 32 bit pointer to 80 bit complex
			T_64PCPLX80=0x0652,   // 64 bit pointer to 80 bit complex

			//  128 bit complex types

			T_CPLX128=0x0053,   // 128 bit complex
			T_32PCPLX128=0x0453,   // 32 bit pointer to 128 bit complex
			T_64PCPLX128=0x0653,   // 64 bit pointer to 128 bit complex

			//  boolean types

			T_BOOL08=0x0030,   // 8 bit boolean
			T_32PBOOL08=0x0430,   // 32 bit pointer to 8 bit boolean
			T_64PBOOL08=0x0630,   // 64 bit pointer to 8 bit boolean

			T_BOOL16=0x0031,   // 16 bit boolean
			T_32PBOOL16=0x0431,   // 32 bit pointer to 18 bit boolean
			T_64PBOOL16=0x0631,   // 64 bit pointer to 18 bit boolean

			T_BOOL32=0x0032,   // 32 bit boolean
			T_32PBOOL32=0x0432,   // 32 bit pointer to 32 bit boolean
			T_64PBOOL32=0x0632,   // 64 bit pointer to 32 bit boolean

			T_BOOL64=0x0033,   // 64 bit boolean
			T_32PBOOL64=0x0433,   // 32 bit pointer to 64 bit boolean
			T_64PBOOL64=0x0633,   // 64 bit pointer to 64 bit boolean
		};
	};

	//  No leaf index can have a value of 0x0000.  The leaf indices are
	//  separated into ranges depending upon the use of the type record.
	//  The second range is for the type records that are directly referenced
	//  in symbols. The first range is for type records that are not
	//  referenced by symbols but instead are referenced by other type
	//  records.  All type records must have a starting leaf index in these
	//  first two ranges.  The third range of leaf indices are used to build
	//  up complex lists such as the field list of a class type record.  No
	//  type record can begin with one of the leaf indices. The fourth ranges
	//  of type indices are used to represent numeric data in a symbol or
	//  type record. These leaf indices are greater than 0x8000.  At the
	//  point that type or symbol processor is expecting a numeric field, the
	//  next two bytes in the type record are examined.  If the value is less
	//  than 0x8000, then the two bytes contain the numeric value.  If the
	//  value is greater than 0x8000, then the data follows the leaf index in
	//  a format specified by the leaf index. The final range of leaf indices
	//  are used to force alignment of subfields within a complex type record..
	//

	struct LEAF
	{
		enum Enum
		{
			// leaf indices starting records but referenced from symbol records

			LF_VTSHAPE=0x000a,
			LF_COBOL1=0x000c,
			LF_LABEL=0x000e,
			LF_NULL=0x000f,
			LF_NOTTRAN=0x0010,
			LF_ENDPRECOMP=0x0014,       // not referenced from symbol
			LF_TYPESERVER_ST=0x0016,       // not referenced from symbol

			// leaf indices starting records but referenced only from type records

			LF_LIST=0x0203,
			LF_REFSYM=0x020c,

			LF_ENUMERATE_ST=0x0403,

			// 32-bit type index versions of leaves, all have the 0x1000 bit set
			//
			LF_TI16_MAX=0x1000,

			LF_MODIFIER=0x1001,
			LF_POINTER=0x1002,
			LF_ARRAY_ST=0x1003,
			LF_CLASS_ST=0x1004,
			LF_STRUCTURE_ST=0x1005,
			LF_UNION_ST=0x1006,
			LF_ENUM_ST=0x1007,
			LF_PROCEDURE=0x1008,
			LF_MFUNCTION=0x1009,
			LF_COBOL0=0x100a,
			LF_BARRAY=0x100b,
			LF_DIMARRAY_ST=0x100c,
			LF_VFTPATH=0x100d,
			LF_PRECOMP_ST=0x100e,       // not referenced from symbol
			LF_OEM=0x100f,       // oem definable type string
			LF_ALIAS_ST=0x1010,       // alias (typedef) type
			LF_OEM2=0x1011,       // oem definable type string

			// leaf indices starting records but referenced only from type records

			LF_SKIP=0x1200,
			LF_ARGLIST=0x1201,
			LF_DEFARG_ST=0x1202,
			LF_FIELDLIST=0x1203,
			LF_DERIVED=0x1204,
			LF_BITFIELD=0x1205,
			LF_METHODLIST=0x1206,
			LF_DIMCONU=0x1207,
			LF_DIMCONLU=0x1208,
			LF_DIMVARU=0x1209,
			LF_DIMVARLU=0x120a,

			LF_BCLASS=0x1400,
			LF_VBCLASS=0x1401,
			LF_IVBCLASS=0x1402,
			LF_FRIENDFCN_ST=0x1403,
			LF_INDEX=0x1404,
			LF_MEMBER_ST=0x1405,
			LF_STMEMBER_ST=0x1406,
			LF_METHOD_ST=0x1407,
			LF_NESTTYPE_ST=0x1408,
			LF_VFUNCTAB=0x1409,
			LF_FRIENDCLS=0x140a,
			LF_ONEMETHOD_ST=0x140b,
			LF_VFUNCOFF=0x140c,
			LF_NESTTYPEEX_ST=0x140d,
			LF_MEMBERMODIFY_ST=0x140e,
			LF_MANAGED_ST=0x140f,

			// Types w/ SZ names

			LF_ST_MAX=0x1500,

			LF_TYPESERVER=0x1501,       // not referenced from symbol
			LF_ENUMERATE=0x1502,
			LF_ARRAY=0x1503,
			LF_CLASS=0x1504,
			LF_STRUCTURE=0x1505,
			LF_UNION=0x1506,
			LF_ENUM=0x1507,
			LF_DIMARRAY=0x1508,
			LF_PRECOMP=0x1509,       // not referenced from symbol
			LF_ALIAS=0x150a,       // alias (typedef) type
			LF_DEFARG=0x150b,
			LF_FRIENDFCN=0x150c,
			LF_MEMBER=0x150d,
			LF_STMEMBER=0x150e,
			LF_METHOD=0x150f,
			LF_NESTTYPE=0x1510,
			LF_ONEMETHOD=0x1511,
			LF_NESTTYPEEX=0x1512,
			LF_MEMBERMODIFY=0x1513,
			LF_MANAGED=0x1514,
			LF_TYPESERVER2=0x1515,

			LF_NUMERIC=0x8000,
			LF_CHAR=0x8000,
			LF_SHORT=0x8001,
			LF_USHORT=0x8002,
			LF_LONG=0x8003,
			LF_ULONG=0x8004,
			LF_REAL32=0x8005,
			LF_REAL64=0x8006,
			LF_REAL80=0x8007,
			LF_REAL128=0x8008,
			LF_QUADWORD=0x8009,
			LF_UQUADWORD=0x800a,
			LF_COMPLEX32=0x800c,
			LF_COMPLEX64=0x800d,
			LF_COMPLEX80=0x800e,
			LF_COMPLEX128=0x800f,
			LF_VARSTRING=0x8010,

			LF_OCTWORD=0x8017,
			LF_UOCTWORD=0x8018,

			LF_DECIMAL=0x8019,
			LF_DATE=0x801a,
			LF_UTF8STRING=0x801b,

			LF_PAD0=0xf0,
			LF_PAD1=0xf1,
			LF_PAD2=0xf2,
			LF_PAD3=0xf3,
			LF_PAD4=0xf4,
			LF_PAD5=0xf5,
			LF_PAD6=0xf6,
			LF_PAD7=0xf7,
			LF_PAD8=0xf8,
			LF_PAD9=0xf9,
			LF_PAD10=0xfa,
			LF_PAD11=0xfb,
			LF_PAD12=0xfc,
			LF_PAD13=0xfd,
			LF_PAD14=0xfe,
			LF_PAD15=0xff,
		};
	};

	// end of leaf indices

	//  Type enum for pointer records
	//  Pointers can be one of the following types

	struct CV_ptrtype
	{
		enum Enum
		{
			CV_PTR_BASE_SEG=0x03, // based on segment
			CV_PTR_BASE_VAL=0x04, // based on value of base
			CV_PTR_BASE_SEGVAL=0x05, // based on segment value of base
			CV_PTR_BASE_ADDR=0x06, // based on address of base
			CV_PTR_BASE_SEGADDR=0x07, // based on segment address of base
			CV_PTR_BASE_TYPE=0x08, // based on type
			CV_PTR_BASE_SELF=0x09, // based on self
			CV_PTR_NEAR32=0x0a, // 32 bit pointer
			CV_PTR_64=0x0c, // 64 bit pointer
			CV_PTR_UNUSEDPTR=0x0d  // first unused pointer type
		};
	};

	//  Mode enum for pointers
	//  Pointers can have one of the following modes

	struct CV_ptrmode
	{
		enum Enum
		{
			CV_PTR_MODE_PTR=0x00, // "normal" pointer
			CV_PTR_MODE_REF=0x01, // reference
			CV_PTR_MODE_PMEM=0x02, // pointer to data member
			CV_PTR_MODE_PMFUNC=0x03, // pointer to member function
			CV_PTR_MODE_RESERVED=0x04  // first unused pointer mode
		};
	};

	//  enumeration for pointer-to-member types

	struct CV_pmtype
	{
		enum Enum
		{
			CV_PMTYPE_Undef=0x00, // not specified (pre VC8)
			CV_PMTYPE_D_Single=0x01, // member data, single inheritance
			CV_PMTYPE_D_Multiple=0x02, // member data, multiple inheritance
			CV_PMTYPE_D_Virtual=0x03, // member data, virtual inheritance
			CV_PMTYPE_D_General=0x04, // member data, most general
			CV_PMTYPE_F_Single=0x05, // member function, single inheritance
			CV_PMTYPE_F_Multiple=0x06, // member function, multiple inheritance
			CV_PMTYPE_F_Virtual=0x07, // member function, virtual inheritance
			CV_PMTYPE_F_General=0x08, // member function, most general
		};
	};

	//  enumeration for method properties

	struct CV_methodprop
	{
		enum Enum
		{
			CV_MTvanilla=0x00,
			CV_MTvirtual=0x01,
			CV_MTstatic=0x02,
			CV_MTfriend=0x03,
			CV_MTintro=0x04,
			CV_MTpurevirt=0x05,
			CV_MTpureintro=0x06
		};
	};

	//  enumeration for virtual shape table entries

	struct CV_VTS_desc
	{
		enum Enum
		{
			CV_VTS_near=0x00,
			CV_VTS_far=0x01,
			CV_VTS_thin=0x02,
			CV_VTS_outer=0x03,
			CV_VTS_meta=0x04,
			CV_VTS_near32=0x05,
			CV_VTS_far32=0x06,
			CV_VTS_unused=0x07
		};
	};

	//  enumeration for LF_LABEL address modes

	struct CV_LABEL_TYPE
	{
		enum Enum
		{
			CV_LABEL_NEAR=0,       // near return
			CV_LABEL_FAR=4        // far return
		};
	};

	//  enumeration for LF_MODIFIER values

	struct CV_modifier
	{
		enum Enum
		{
			MOD_const=0x0001,
			MOD_volatile=0x0002,
			MOD_unaligned=0x0004,
		};
	};

	//  bit field structure describing class/struct/union/enum properties

	struct CV_prop
	{
		enum Enum
		{
			packed=0x0001,   // true if structure is packed
			ctor=0x0002,   // true if constructors or destructors present
			ovlops=0x0004,   // true if overloaded operators present
			isnested=0x0008,   // true if this is a nested class
			cnested=0x0010,   // true if this class contains nested types
			opassign=0x0020,   // true if overloaded assignment (=)
			opcast=0x0040,   // true if casting methods
			fwdref=0x0080,   // true if forward reference (incomplete defn)
			scoped=0x0100,   // scoped definition
		};
	};

	//  class field attribute

	struct CV_fldattr
	{
		enum Enum
		{
			access=0x0003,   // access protection CV_access_t
			mprop=0x001c,   // method properties CV_methodprop_t
			pseudo=0x0020,   // compiler generated fcn and does not exist
			noinherit=0x0040,   // true if class cannot be inherited
			noconstruct=0x0080,   // true if class cannot be constructed
			compgenx=0x0100,   // compiler generated fcn and does exist
		};
	};

	//  Structures to access to the type records

	struct TYPTYPE
	{
		uint16_t len;
		uint16_t leaf;
		// byte data[];

		//  char *NextType (char * pType) {
		//  return (pType + ((TYPTYPE *)pType)->len + sizeof(ushort));
		//  }
	};          // general types record

	//  memory representation of pointer to member.  These representations are
	//  indexed by the enumeration above in the LF_POINTER record

	//  representation of a 32 bit pointer to data for a class with
	//  or without virtual functions and no virtual bases

	struct CV_PDMR32_NVVFCN
	{
		int32_t	mdisp;      // displacement to data (NULL = 0x80000000)
	};

	//  representation of a 32 bit pointer to data for a class
	//  with virtual bases

	struct CV_PDMR32_VBASE
	{
		int32_t mdisp;      // displacement to data
		int32_t pdisp;      // this pointer displacement
		int32_t vdisp;      // vbase table displacement
		// NULL = (,,0xffffffff)
	};

	//  representation of a 32 bit pointer to member function for a
	//  class with no virtual bases and a single address point

	struct CV_PMFR32_NVSA
	{
		uint32_t off;        // near address of function (NULL = 0L)
	};

	//  representation of a 32 bit pointer to member function for a
	//  class with no virtual bases and multiple address points

	struct CV_PMFR32_NVMA
	{
		uint32_t off;        // near address of function (NULL = 0L,x)
		int32_t	 disp;
	};

	//  representation of a 32 bit pointer to member function for a
	//  class with virtual bases

	struct CV_PMFR32_VBASE
	{
		uint32_t off;        // near address of function (NULL = 0L,x,x,x)
		int32_t  mdisp;      // displacement to data
		int32_t  pdisp;      // this pointer displacement
		int32_t  vdisp;      // vbase table displacement
	};

	//////////////////////////////////////////////////////////////////////////////
	//
	//  The following type records are basically variant records of the
	//  above structure.  The "ushort leaf" of the above structure and
	//  the "ushort leaf" of the following type definitions are the same
	//  symbol.
	//

	//  Notes on alignment
	//  Alignment of the fields in most of the type records is done on the
	//  basis of the TYPTYPE record base.  That is why in most of the lf*
	//  records that the type is located on what appears to
	//  be a offset mod 4 == 2 boundary.  The exception to this rule are those
	//  records that are in a list (lfFieldList, lfMethodList), which are
	//  aligned to their own bases since they don't have the length field
	//

	//  Type record for LF_MODIFIER

	struct LeafModifier
	{
		// internal ushort leaf;      // LF_MODIFIER [TYPTYPE]
		uint32_t	type;	// (type index) modified type
		uint16_t	 attr;	// modifier attribute modifier_t CV_modifier
	};

	//  type record for LF_POINTER

	struct LeafPointerAttr
	{
		enum Enum
		{
			ptrtype=0x0000001f,   // ordinal specifying pointer type (CV_ptrtype)
			ptrmode=0x000000e0,   // ordinal specifying pointer mode (CV_ptrmode)
			isflat32=0x00000100,   // true if 0:32 pointer
			isvolatile=0x00000200,   // TRUE if volatile pointer
			isconst=0x00000400,   // TRUE if const pointer
			isunaligned=0x00000800,   // TRUE if unaligned pointer
			isrestrict=0x00001000,   // TRUE if restricted pointer (allow agressive opts)
		};
	};

	struct LeafPointer
	{
		uint32_t utype;	// (type index) type index of the underlying type
		uint32_t attr; // LeafPointerAttr::Enum
	};

	//  type record for LF_ARRAY

	struct LeafArray
	{
		// internal ushort leaf;      // LF_ARRAY [TYPTYPE]
		uint32_t elemtype;   // (type index) type index of element type
		uint32_t idxtype;    // (type index) type index of indexing type
		//internal byte[] data;       // variable length data specifying size in bytes
		//internal string name;
	};

	//  type record for LF_CLASS, LF_STRUCTURE

	struct LeafClass
	{
		// internal ushort leaf;      // LF_CLASS, LF_STRUCT [TYPTYPE]
		uint16_t	count;      // count of number of elements in class
		uint16_t	prop;   // (CV_prop_t) property attribute field (prop_t)
		uint32_t	field;      // (type index) type index of LF_FIELD descriptor list
		uint32_t	derived;    // (type index) type index of derived from list if not zero
		uint32_t	vshape;     // (type index) type index of vshape table for this class
		//internal byte[] data;       // data describing length of structure in bytes
		//internal string name;
	};

	//  type record for LF_UNION

	struct LeafUnion
	{
		// internal ushort leaf;      // LF_UNION [TYPTYPE]
		uint16_t	count;      // count of number of elements in class
		uint16_t	prop;   // (CV_prop_t) property attribute field
		uint32_t	field;      // (type index) type index of LF_FIELD descriptor list
		//internal byte[] data;       // variable length data describing length of
		//internal string name;
	};

	//  type record for LF_ALIAS

	struct LeafAlias
	{
		// internal ushort leaf;      // LF_ALIAS [TYPTYPE]
		uint32_t utype;      // (type index) underlying type
		//internal string name;       // alias name
	};

	//  type record for LF_MANAGED

	struct LeafManaged {
		// internal ushort leaf;      // LF_MANAGED [TYPTYPE]
		//internal string name;       // utf8, zero terminated managed type name
	};

	//  type record for LF_ENUM

	struct LeafEnum
	{
		// internal ushort leaf;      // LF_ENUM [TYPTYPE]
		uint16_t count;      // count of number of elements in class
		uint16_t prop;   // (CV_propt_t) property attribute field
		uint32_t utype;      // (type index) underlying type of the enum
		uint32_t field;      // (type index) type index of LF_FIELD descriptor list
		//internal string name;       // length prefixed name of enum
	};

	//  Type record for LF_PROCEDURE

	struct LeafProc
	{
		// internal ushort leaf;      // LF_PROCEDURE [TYPTYPE]
		uint32_t rvtype;     // (type index) type index of return value
		uint8_t  calltype;   // calling convention (CV_call_t)
		uint8_t  reserved;   // reserved for future use
		uint16_t parmcount;  // number of parameters
		uint32_t arglist;    // (type index) type index of argument list
	};

	//  Type record for member function

	struct LeafMFunc
	{
		// internal ushort leaf;      // LF_MFUNCTION [TYPTYPE]
		uint32_t rvtype;     // (type index) type index of return value
		uint32_t classtype;  // (type index) type index of containing class
		uint32_t thistype;   // (type index) type index of this pointer (model specific)
		uint8_t  calltype;   // calling convention (call_t)
		uint8_t  reserved;   // reserved for future use
		uint16_t parmcount;  // number of parameters
		uint32_t arglist;    // (type index) type index of argument list
		int32_t  thisadjust; // this adjuster (long because pad required anyway)
	};

	//  type record for virtual function table shape

	struct LeafVTShape
	{
		// internal ushort leaf;      // LF_VTSHAPE [TYPTYPE]
		uint16_t count;      // number of entries in vfunctable
		//internal byte[] desc;       // 4 bit (CV_VTS_desc) descriptors
	};

	//  type record for cobol0

	struct LeafCobol0
	{
		// internal ushort leaf;      // LF_COBOL0 [TYPTYPE]
		//internal uint type;       // (type index) parent type record index
		//internal byte[] data;
	};

	//  type record for cobol1

	struct LeafCobol1
	{
		// internal ushort leaf;      // LF_COBOL1 [TYPTYPE]
		//internal byte[] data;
	};

	//  type record for basic array

	struct LeafBArray
	{
		// internal ushort leaf;      // LF_BARRAY [TYPTYPE]
		uint32_t utype;      // (type index) type index of underlying type
	};

	//  type record for assembler labels

	struct LeafLabel
	{
		// internal ushort leaf;      // LF_LABEL [TYPTYPE]
		uint16_t mode;       // addressing mode of label
	};

	//  type record for dimensioned arrays

	struct LeafDimArray
	{
		// internal ushort leaf;      // LF_DIMARRAY [TYPTYPE]
		uint32_t utype;      // (type index) underlying type of the array
		uint32_t diminfo;    // (type index) dimension information
		//internal string name;       // length prefixed name
	};

	//  type record describing path to virtual function table

	struct LeafVFTPath
	{
		// internal ushort leaf;      // LF_VFTPATH [TYPTYPE]
		uint32_t count;      // count of number of bases in path
		//internal uint[] bases;      // (type index) bases from root to leaf
	};

	//  type record describing inclusion of precompiled types

	struct LeafPreComp
	{
		// internal ushort leaf;      // LF_PRECOMP [TYPTYPE]
		uint32_t start;      // starting type index included
		uint32_t count;      // number of types in inclusion
		uint32_t signature;  // signature
		//internal string name;       // length prefixed name of included type file
	};

	//  type record describing end of precompiled types that can be
	//  included by another file

	struct LeafEndPreComp
	{
		// internal ushort leaf;      // LF_ENDPRECOMP [TYPTYPE]
		uint32_t signature;  // signature
	};

	//  type record describing using of a type server

	//  description of type records that can be referenced from
	//  type records referenced by symbols

	//  type record for skip record

	struct LeafSkip
	{
		// internal ushort leaf;      // LF_SKIP [TYPTYPE]
		uint32_t type;       // (type index) next valid index
		//internal byte[] data;       // pad data
	};

	//  argument list leaf

	struct LeafArgList
	{
		// internal ushort leaf;      // LF_ARGLIST [TYPTYPE]
		uint32_t count;      // number of arguments
		//internal uint[] arg;        // (type index) number of arguments
	};

	//  derived class list leaf

	struct LeafDerived
	{
		// internal ushort leaf;      // LF_DERIVED [TYPTYPE]
		uint32_t count;      // number of arguments
		//internal uint[] drvdcls;    // (type index) type indices of derived classes
	};

	//  leaf for default arguments

	struct LeafDefArg
	{
		// internal ushort leaf;      // LF_DEFARG [TYPTYPE]
		uint32_t type;       // (type index) type of resulting expression
		//internal byte[] expr;       // length prefixed expression string
	};

	//  list leaf
	//      This list should no longer be used because the utilities cannot
	//      verify the contents of the list without knowing what type of list
	//      it is.  New specific leaf indices should be used instead.

	struct LeafList
	{
		// internal ushort leaf;      // LF_LIST [TYPTYPE]
		//internal byte[] data;       // data format specified by indexing type
	};

	//  field list leaf
	//  This is the header leaf for a complex list of class and structure
	//  subfields.

	struct LeafFieldList
	{
		// internal ushort leaf;      // LF_FIELDLIST [TYPTYPE]
		//internal char[] data;       // field list sub lists
	};

	//  type record for non-static methods and friends in overloaded method list

	struct mlMethod
	{
		uint16_t attr;       // (CV_fldattr_t) method attribute
		uint16_t pad0;       // internal padding, must be 0
		uint32_t index;      // (type index) index to type record for procedure
		//internal uint[] vbaseoff;   // offset in vfunctable if intro virtual
	};

	struct LeafMethodList
	{
		// internal ushort leaf;      // LF_METHODLIST [TYPTYPE]
		//internal byte[] mList;      // really a mlMethod type
	};

	//  type record for LF_BITFIELD

	struct LeafBitfield
	{
		// internal ushort leaf;      // LF_BITFIELD [TYPTYPE]
		uint32_t type;       // (type index) type of bitfield
		uint8_t  length;
		uint8_t	 position;
	};

	//  type record for dimensioned array with constant bounds

	struct LeafDimCon
	{
		// internal ushort leaf;      // LF_DIMCONU or LF_DIMCONLU [TYPTYPE]
		uint32_t typ;        // (type index) type of index
		uint16_t rank;       // number of dimensions
		//internal byte[] dim;        // array of dimension information with
		// either upper bounds or lower/upper bound
	};

	//  type record for dimensioned array with variable bounds

	struct LeafDimVar
	{
		// internal ushort leaf;      // LF_DIMVARU or LF_DIMVARLU [TYPTYPE]
		uint32_t rank;       // number of dimensions
		uint32_t typ;        // (type index) type of index
		//internal uint[] dim;        // (type index) array of type indices for either
		// variable upper bound or variable
		// lower/upper bound.  The count of type
		// indices is rank or rank*2 depending on
		// whether it is LFDIMVARU or LF_DIMVARLU.
		// The referenced types must be
		// LF_REFSYM or T_VOID
	};

	//  type record for referenced symbol

	struct LeafRefSym
	{
		// internal ushort leaf;      // LF_REFSYM [TYPTYPE]
		//internal byte[] Sym;        // copy of referenced symbol record
		// (including length)
	};

	//  the following are numeric leaves.  They are used to indicate the
	//  size of the following variable length data.  When the numeric
	//  data is a single byte less than 0x8000, then the data is output
	//  directly.  If the data is more the 0x8000 or is a negative value,
	//  then the data is preceded by the proper index.
	//

	//  signed character leaf

	struct LeafChar
	{
		// internal ushort leaf;      // LF_CHAR [TYPTYPE]
		int8_t val;        // signed 8-bit value
	};

	//  signed short leaf

	struct LeafShort
	{
		// internal ushort leaf;      // LF_SHORT [TYPTYPE]
		int16_t val;        // signed 16-bit value
	};

	//  ushort leaf

	struct LeafUShort
	{
		// internal ushort leaf;      // LF_ushort [TYPTYPE]
		uint16_t val;        // unsigned 16-bit value
	};

	//  signed (32-bit) long leaf

	struct LeafLong
	{
		// internal ushort leaf;      // LF_LONG [TYPTYPE]
		int32_t val;        // signed 32-bit value
	};

	//  uint    leaf

	struct LeafULong
	{
		// internal ushort leaf;      // LF_ULONG [TYPTYPE]
		uint32_t val;        // unsigned 32-bit value
	};

	//  signed quad leaf

	struct LeafQuad
	{
		// internal ushort leaf;      // LF_QUAD [TYPTYPE]
		int64_t val;        // signed 64-bit value
	};

	//  unsigned quad leaf

	struct LeafUQuad
	{
		// internal ushort leaf;      // LF_UQUAD [TYPTYPE]
		uint64_t val;        // unsigned 64-bit value
	};

	//  signed int128 leaf

	struct LeafOct
	{
		// internal ushort leaf;      // LF_OCT [TYPTYPE]
		uint64_t val0;
		uint64_t val1;       // signed 128-bit value
	};

	//  unsigned int128 leaf

	struct LeafUOct
	{
		// internal ushort leaf;      // LF_UOCT [TYPTYPE]
		uint64_t val0;
		uint64_t val1;       // unsigned 128-bit value
	};

	//  real 32-bit leaf

	struct LeafReal32
	{
		// internal ushort leaf;      // LF_REAL32 [TYPTYPE]
		float val;        // 32-bit real value
	};

	//  real 64-bit leaf

	struct LeafReal64
	{
		// internal ushort leaf;      // LF_REAL64 [TYPTYPE]
		double val;        // 64-bit real value
	};

	//  real 80-bit leaf

	struct LeafReal80
	{
		// internal ushort leaf;      // LF_REAL80 [TYPTYPE]
		float10 val;        // real 80-bit value
	};

	//  real 128-bit leaf

	struct LeafReal128
	{
		// internal ushort leaf;      // LF_REAL128 [TYPTYPE]
		uint64_t val0;
		uint64_t val1;       // real 128-bit value
	};

	//  complex 32-bit leaf

	struct LeafCmplx32
	{
		// internal ushort leaf;      // LF_COMPLEX32 [TYPTYPE]
		float val_real;   // real component
		float val_imag;   // imaginary component
	};

	//  complex 64-bit leaf

	struct LeafCmplx64
	{
		// internal ushort leaf;      // LF_COMPLEX64 [TYPTYPE]
		double val_real;   // real component
		double val_imag;   // imaginary component
	};

	//  complex 80-bit leaf

	struct LeafCmplx80
	{
		// internal ushort leaf;      // LF_COMPLEX80 [TYPTYPE]
		float10 val_real;   // real component
		float10 val_imag;   // imaginary component
	};

	//  complex 128-bit leaf

	struct LeafCmplx128
	{
		// internal ushort leaf;      // LF_COMPLEX128 [TYPTYPE]
		uint64_t val0_real;
		uint64_t val1_real;  // real component
		uint64_t val0_imag;
		uint64_t val1_imag;  // imaginary component
	};

	//  variable length numeric field

	struct LeafVarString
	{
		// internal ushort leaf;      // LF_VARSTRING [TYPTYPE]
		uint16_t len;        // length of value in bytes
		//internal byte[] value;      // value
	};

	//  index leaf - contains type index of another leaf
	//  a major use of this leaf is to allow the compilers to emit a
	//  long complex list (LF_FIELD) in smaller pieces.

	struct LeafIndex
	{
		// internal ushort leaf;      // LF_INDEX [TYPTYPE]
		uint16_t pad0;       // internal padding, must be 0
		uint32_t index;      // (type index) type index of referenced leaf
	};

	//  subfield record for base class field

	struct LeafBClass
	{
		// internal ushort leaf;      // LF_BCLASS [TYPTYPE]
		uint16_t attr;       // (CV_fldattr_t) attribute
		uint32_t index;      // (type index) type index of base class
		//internal byte[] offset;     // variable length offset of base within class
	};

	//  subfield record for direct and indirect virtual base class field

	struct LeafVBClass
	{
		// internal ushort leaf;      // LF_VBCLASS | LV_IVBCLASS [TYPTYPE]
		uint16_t attr;       // (CV_fldattr_t) attribute
		uint32_t index;      // (type index) type index of direct virtual base class
		uint32_t vbptr;      // (type index) type index of virtual base pointer
		//internal byte[] vbpoff;     // virtual base pointer offset from address point
		// followed by virtual base offset from vbtable
	};

	//  subfield record for friend class

	struct LeafFriendCls
	{
		// internal ushort leaf;      // LF_FRIENDCLS [TYPTYPE]
		uint16_t pad0;       // internal padding, must be 0
		uint32_t index;      // (type index) index to type record of friend class
	};

	//  subfield record for friend function

	struct LeafFriendFcn
	{
		// internal ushort leaf;      // LF_FRIENDFCN [TYPTYPE]
		uint16_t pad0;       // internal padding, must be 0
		uint32_t index;      // (type index) index to type record of friend function
		//internal string name;       // name of friend function
	};

	//  subfield record for non-static data members

	struct LeafMember
	{
		// internal ushort leaf;      // LF_MEMBER [TYPTYPE]
		uint16_t attr;       // (CV_fldattr_t)attribute mask
		uint32_t index;      // (type index) index of type record for field
		//internal byte[] offset;     // variable length offset of field
		//internal string name;       // length prefixed name of field
	};

	//  type record for static data members

	struct LeafSTMember
	{
		// internal ushort leaf;      // LF_STMEMBER [TYPTYPE]
		uint16_t attr;       // (CV_fldattr_t) attribute mask
		uint32_t index;      // (type index) index of type record for field
		//internal string name;       // length prefixed name of field
	};

	//  subfield record for virtual function table pointer

	struct LeafVFuncTab
	{
		// internal ushort leaf;      // LF_VFUNCTAB [TYPTYPE]
		uint16_t pad0;       // internal padding, must be 0
		uint32_t type;       // (type index) type index of pointer
	};

	//  subfield record for virtual function table pointer with offset

	struct LeafVFuncOff
	{
		// internal ushort leaf;      // LF_VFUNCOFF [TYPTYPE]
		uint16_t pad0;       // internal padding, must be 0.
		uint32_t type;       // (type index) type index of pointer
		int32_t	 offset;     // offset of virtual function table pointer
	};

	//  subfield record for overloaded method list

	struct LeafMethod
	{
		// internal ushort leaf;      // LF_METHOD [TYPTYPE]
		uint16_t count;      // number of occurrences of function
		uint32_t mList;      // (type index) index to LF_METHODLIST record
		//internal string name;       // length prefixed name of method
	};

	//  subfield record for nonoverloaded method

	struct LeafOneMethod
	{
		// internal ushort leaf;      // LF_ONEMETHOD [TYPTYPE]
		uint16_t attr;       // (CV_fldattr_t) method attribute
		uint32_t index;      // (type index) index to type record for procedure
		//internal uint[] vbaseoff;   // offset in vfunctable if intro virtual
		//internal string name;
	};

	//  subfield record for enumerate

	struct LeafEnumerate
	{
		// internal ushort leaf;      // LF_ENUMERATE [TYPTYPE]
		uint16_t attr;       // (CV_fldattr_t) access
		//internal byte[] value;      // variable length value field
		//internal string name;
	};

	//  type record for nested (scoped) type definition

	struct LeafNestType
	{
		// internal ushort leaf;      // LF_NESTTYPE [TYPTYPE]
		uint16_t pad0;       // internal padding, must be 0
		uint32_t index;      // (type index) index of nested type definition
		//internal string name;       // length prefixed type name
	};

	//  type record for nested (scoped) type definition, with attributes
	//  new records for vC v5.0, no need to have 16-bit ti versions.

	struct LeafNestTypeEx
	{
		// internal ushort leaf;      // LF_NESTTYPEEX [TYPTYPE]
		uint16_t attr;       // (CV_fldattr_t) member access
		uint32_t index;      // (type index) index of nested type definition
		//internal string name;       // length prefixed type name
	};

	//  type record for modifications to members

	struct LeafMemberModify
	{
		// internal ushort leaf;      // LF_MEMBERMODIFY [TYPTYPE]
		uint16_t attr;       // (CV_fldattr_t) the new attributes
		uint32_t index;      // (type index) index of base class type definition
		//internal string name;       // length prefixed member name
	};

	//  type record for pad leaf

	struct LeafPad
	{
		uint8_t leaf;
	};

	struct TypeRecord
	{
		uint16_t	length;
		uint16_t	leafType;
	};

	struct Subsection
	{
		enum Enum
		{
			Ignore			= 0x80000000,   // if this bit is set in a subsection type then ignore the subsection contents)
			Symbols			= 0xF1,
			Lines			= 0xF2,
			StringTable		= 0xF3,
			FileChecksums	= 0xF4,
			FrameData		= 0xF5,
			_Unknown1		= 0xF6,			// Encountered, don't know what it is though
		};
	};

	struct CVFileChecksum
	{
		uint32_t	name;           // Index of name in name table.
		uint8_t		len;            // Hash length
		uint8_t		type;           // Hash type
	};

	struct SymbolDefs
	{
		enum Enum
		{
			S_END=0x0006,  // Block, procedure, "with" or thunk end
			S_OEM=0x0404,  // OEM defined symbol

			S_REGISTER_ST=0x1001,  // Register variable
			S_CONSTANT_ST=0x1002,  // constant symbol
			S_UDT_ST=0x1003,  // User defined type
			S_COBOLUDT_ST=0x1004,  // special UDT for cobol that does not symbol pack
			S_MANYREG_ST=0x1005,  // multiple register variable
			S_BPREL32_ST=0x1006,  // BP-relative
			S_LDATA32_ST=0x1007,  // Module-local symbol
			S_GDATA32_ST=0x1008,  // Global data symbol
			S_PUB32_ST=0x1009,  // a internal symbol (CV internal reserved)
			S_LPROC32_ST=0x100a,  // Local procedure start
			S_GPROC32_ST=0x100b,  // Global procedure start
			S_VFTABLE32=0x100c,  // address of virtual function table
			S_REGREL32_ST=0x100d,  // register relative address
			S_LTHREAD32_ST=0x100e,  // local thread storage
			S_GTHREAD32_ST=0x100f,  // global thread storage

			S_LPROCMIPS_ST=0x1010,  // Local procedure start
			S_GPROCMIPS_ST=0x1011,  // Global procedure start

			// new symbol records for edit and continue information

			S_FRAMEPROC=0x1012,  // extra frame and proc information
			S_COMPILE2_ST=0x1013,  // extended compile flags and info

			// new symbols necessary for 16-bit enumerates of IA64 registers
			// and IA64 specific symbols

			S_MANYREG2_ST=0x1014,  // multiple register variable
			S_LPROCIA64_ST=0x1015,  // Local procedure start (IA64)
			S_GPROCIA64_ST=0x1016,  // Global procedure start (IA64)

			// Local symbols for IL
			S_LOCALSLOT_ST=0x1017,  // local IL sym with field for local slot index
			S_PARAMSLOT_ST=0x1018,  // local IL sym with field for parameter slot index

			S_ANNOTATION=0x1019,  // Annotation string literals

			// symbols to support managed code debugging
			S_GMANPROC_ST=0x101a,  // Global proc
			S_LMANPROC_ST=0x101b,  // Local proc
			S_RESERVED1=0x101c,  // reserved
			S_RESERVED2=0x101d,  // reserved
			S_RESERVED3=0x101e,  // reserved
			S_RESERVED4=0x101f,  // reserved
			S_LMANDATA_ST=0x1020,
			S_GMANDATA_ST=0x1021,
			S_MANFRAMEREL_ST=0x1022,
			S_MANREGISTER_ST=0x1023,
			S_MANSLOT_ST=0x1024,
			S_MANMANYREG_ST=0x1025,
			S_MANREGREL_ST=0x1026,
			S_MANMANYREG2_ST=0x1027,
			S_MANTYPREF=0x1028,  // Index for type referenced by name from metadata
			S_UNAMESPACE_ST=0x1029,  // Using namespace

			// Symbols w/ SZ name fields. All name fields contain utf8 encoded strings.
			S_ST_MAX=0x1100,  // starting point for SZ name symbols

			S_OBJNAME=0x1101,  // path to object file name
			S_THUNK32=0x1102,  // Thunk Start
			S_BLOCK32=0x1103,  // block start
			S_WITH32=0x1104,  // with start
			S_LABEL32=0x1105,  // code label
			S_REGISTER=0x1106,  // Register variable
			S_CONSTANT=0x1107,  // constant symbol
			S_UDT=0x1108,  // User defined type
			S_COBOLUDT=0x1109,  // special UDT for cobol that does not symbol pack
			S_MANYREG=0x110a,  // multiple register variable
			S_BPREL32=0x110b,  // BP-relative
			S_LDATA32=0x110c,  // Module-local symbol
			S_GDATA32=0x110d,  // Global data symbol
			S_PUB32=0x110e,  // a internal symbol (CV internal reserved)
			S_LPROC32=0x110f,  // Local procedure start
			S_GPROC32=0x1110,  // Global procedure start
			S_REGREL32=0x1111,  // register relative address
			S_LTHREAD32=0x1112,  // local thread storage
			S_GTHREAD32=0x1113,  // global thread storage

			S_LPROCMIPS=0x1114,  // Local procedure start
			S_GPROCMIPS=0x1115,  // Global procedure start
			S_COMPILE2=0x1116,  // extended compile flags and info
			S_MANYREG2=0x1117,  // multiple register variable
			S_LPROCIA64=0x1118,  // Local procedure start (IA64)
			S_GPROCIA64=0x1119,  // Global procedure start (IA64)
			S_LOCALSLOT=0x111a,  // local IL sym with field for local slot index
			S_SLOT=S_LOCALSLOT,  // alias for LOCALSLOT
			S_PARAMSLOT=0x111b,  // local IL sym with field for parameter slot index

			// symbols to support managed code debugging
			S_LMANDATA=0x111c,
			S_GMANDATA=0x111d,
			S_MANFRAMEREL=0x111e,
			S_MANREGISTER=0x111f,
			S_MANSLOT=0x1120,
			S_MANMANYREG=0x1121,
			S_MANREGREL=0x1122,
			S_MANMANYREG2=0x1123,
			S_UNAMESPACE=0x1124,  // Using namespace

			// ref symbols with name fields
			S_PROCREF=0x1125,  // Reference to a procedure
			S_DATAREF=0x1126,  // Reference to data
			S_LPROCREF=0x1127,  // Local Reference to a procedure
			S_ANNOTATIONREF=0x1128,  // Reference to an S_ANNOTATION symbol
			S_TOKENREF=0x1129,  // Reference to one of the many MANPROCSYM's

			// continuation of managed symbols
			S_GMANPROC=0x112a,  // Global proc
			S_LMANPROC=0x112b,  // Local proc

			// short, light-weight thunks
			S_TRAMPOLINE=0x112c,  // trampoline thunks
			S_MANCONSTANT=0x112d,  // constants with metadata type info

			// native attributed local/parms
			S_ATTR_FRAMEREL=0x112e,  // relative to virtual frame ptr
			S_ATTR_REGISTER=0x112f,  // stored in a register
			S_ATTR_REGREL=0x1130,  // relative to register (alternate frame ptr)
			S_ATTR_MANYREG=0x1131,  // stored in >1 register

			// Separated code (from the compiler) support
			S_SEPCODE=0x1132,

			S_LOCAL=0x1133,  // defines a local symbol in optimized code
			S_DEFRANGE=0x1134,  // defines a single range of addresses in which symbol can be evaluated
			S_DEFRANGE2=0x1135,  // defines ranges of addresses in which symbol can be evaluated

			S_SECTION=0x1136,  // A COFF section in a PE executable
			S_COFFGROUP=0x1137,  // A COFF group
			S_EXPORT=0x1138,  // A export

			S_CALLSITEINFO=0x1139,  // Indirect call site information
			S_FRAMECOOKIE=0x113a,  // Security cookie information

			S_DISCARDED=0x113b,  // Discarded by LINK /OPT:REF (experimental, see richards)

			S_RECTYPE_MAX,              // one greater than last
			S_RECTYPE_LAST=S_RECTYPE_MAX - 1,
		};
	};

	// Struct for S_LPROC32 and S_GPROC32
	struct ProcSym32
	{
		// internal ushort reclen;    // Record length [SYMTYPE]
		// internal ushort rectyp;    // S_GPROC32 or S_LPROC32
		uint32_t			parent;     // pointer to the parent
		uint32_t			end;        // pointer to this blocks end
		uint32_t			next;       // pointer to next symbol
		uint32_t			len;        // Proc length
		uint32_t			dbgStart;   // Debug start offset
		uint32_t			dbgEnd;     // Debug end offset
		uint32_t			typind;     // (type index) Type index
		uint32_t			off;
		uint16_t			seg;
		uint8_t				flags;      // (CV_PROCFLAGS) Proc flags
		//internal string name;       // Length-prefixed name
	};

	// Struct for S_THUNK32
	struct ThunkSym32
	{
		// internal ushort reclen;    // Record length [SYMTYPE]
		// internal ushort rectyp;    // S_THUNK32
		uint32_t		parent;     // pointer to the parent
		uint32_t		end;        // pointer to this blocks end
		uint32_t		next;       // pointer to next symbol
		uint32_t		off;
		uint16_t		seg;
		uint16_t		len;        // length of thunk
		uint8_t			ord;        // THUNK_ORDINAL specifying type of thunk
		//internal string name;       // Length-prefixed name
		//internal byte[] variant;    // variant portion of thunk
	};

	struct BlockSym32
	{
		uint32_t	parent;
		uint32_t	end;
		uint32_t	len;
		uint32_t	off;
		uint16_t	seg;
	};

	struct CV_LineSection
	{
		uint32_t		off;
		uint16_t		sec;
		uint16_t		flags;
		uint32_t		cod;
	};

	struct CV_SourceFile
	{
		uint32_t	index;          // Index to file in checksum section.
		uint32_t	count;          // Number of CV_Line records.
		uint32_t	linsiz;         // Size of CV_Line records.
	};

	struct CV_Line_Flags
	{
		enum Enum
		{
			linenumStart	= 0x00ffffff,   // line where statement/expression starts
			deltaLineEnd	= 0x7f000000,   // delta to line where statement ends (optional)
			fStatement		= 0x80000000,   // true if a statement linenumber, else an expression line num
		};
	};

	struct CV_Line
	{
		uint32_t	offset;         // Offset to start of code bytes for line number
		uint32_t	flags;          // (CV_Line_Flags)
	};

	struct CV_Column
	{
		uint16_t start;
		uint16_t end;
	};

	struct GlobalRecord
	{
		uint16_t leafType;
		uint32_t symType;
		uint32_t offset;
		uint16_t segment;
	};

	enum class FPOFlags : uint32_t
	{
		SEH = 1,
		CPPEH = 2,
		fnStart = 4
	};

	struct FPO_DATA_V2
	{
		uint32_t ulOffStart;
		uint32_t cbProcSize;
		uint32_t cbLocals;
		uint32_t cbParams;
		uint32_t maxStack;
		uint32_t ProgramStringOffset;
		uint16_t cbProlog;
		uint16_t cbSavedRegs;
		FPOFlags flags;
	};
#pragma pack(pop)
}
