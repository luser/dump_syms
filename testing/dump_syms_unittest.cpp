/* -*- Mode: C++; indent-tabs-mode: t; c-file-style: "linux" -*- */
// Copyright (C) 2015 Jake Shadle
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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "PDBParser.h"

#include <string>

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include "memstream_win.h"
#endif

#ifdef __APPLE__
extern "C" {
#include "memstream_mac.h"
}
#endif

using std::string;
namespace {
#ifdef _WIN32
const char PATHSEP = '\\';
#else
const char PATHSEP = '/';
#endif

void join(string& path, const string& rest)
{
	if (path.empty())
	{
		path = rest;
		return;
	}

	if (path[path.length() - 1] != PATHSEP)
	{
		path += PATHSEP;
	}

	path += rest;
}

bool read_file(const string& filename, string& buffer)
{
	FILE* f = fopen(filename.c_str(), "r");
	if (!f)
	{
		return false;
	}
	fseek(f, 0, SEEK_END);
	size_t fsize = (size_t)ftell(f);
	fseek(f, 0, SEEK_SET);
	buffer.resize(fsize, '\0');
	size_t read = fread(&buffer[0], 1, fsize, f);
	// On Windows with text mode fseek gives you bytes not characters.
	buffer.resize(read);
	fclose(f);
#ifdef _WIN32
	return true;
#else
	return read == fsize;
#endif
}

#if 0
// For debugging...
void write_file(const string& filename, const char* buffer, size_t size)
{
	FILE* f = fopen(filename.c_str(), "wb");
	fwrite(buffer, size, 1, f);
	fclose(f);
}
#endif

} // namespace

TEST(DumpSyms, SimpleTest)
{
	char* testdata_dir = getenv("TESTDATA_DIR");
	ASSERT_NE(testdata_dir, nullptr)
		<< "TESTDATA_DIR must be set in the environment!";

	string test_pdb(testdata_dir);
	join(test_pdb, "TestApp.pdb");

	google_breakpad::PDBParser parser;
	parser.load(test_pdb.c_str());

	char* buffer = nullptr;
	size_t buffer_size;
	FILE* out_file = open_memstream(&buffer, &buffer_size);
	ASSERT_TRUE(out_file);
	parser.printBreakpadSymbols(out_file);
	fclose(out_file);
#ifdef _WIN32
	ASSERT_TRUE(close_memstream(out_file));
#endif
	string actual(buffer, buffer_size);

	string test_sym(testdata_dir);
	join(test_sym, "TestApp.sym");
	string expected;
	ASSERT_TRUE(read_file(test_sym, expected));

	ASSERT_EQ(expected.size(), actual.size());
	ASSERT_EQ(expected, actual);
	free(buffer);
}
