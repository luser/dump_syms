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

#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
std::string
getHResultString(long code)
{

	LPSTR msgBuff = nullptr;
	DWORD num = FormatMessageA(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					code,
					0,
					(LPSTR)&msgBuff,
					0,
					NULL);

	std::string ret;
	ret.assign(msgBuff, num);
	LocalFree(msgBuff);

	return ret;
}
#endif

char* strupper(char* str)
{
	if (!str)
		return nullptr;

	char* p = str;
	while (*p++ != 0)
	{
		*p = ::toupper(*p);
	}

	return str;
}
