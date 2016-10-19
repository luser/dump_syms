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

#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <unordered_map>
#include <vector>

namespace {

struct pipe_data {
	HANDLE pipe;
	char** buffer;
	size_t* buffer_size;
};

std::unordered_map<FILE*, HANDLE> threads;

const size_t kBufferSize = 1024 * 4;

unsigned WINAPI PipeReader(LPVOID lpArg)
{
	pipe_data* p = reinterpret_cast<pipe_data*>(lpArg);
	std::vector<uint8_t> data;

	bool done = false;
	while (!done)
	{
		DWORD bytes_read;
		uint8_t buffer[kBufferSize];
		if (!ReadFile(p->pipe,
			      buffer,
			      sizeof(buffer),
			      &bytes_read, nullptr)) {
			if (GetLastError() != ERROR_BROKEN_PIPE) {
				break;
			}
			done = true;
		}
		data.insert(data.end(), buffer, buffer + bytes_read);
	}

	*(p->buffer_size) = data.size();
	*(p->buffer) = reinterpret_cast<char*>(malloc(data.size()));
	memcpy(*(p->buffer), &data[0], data.size());
	delete p;
	return 0;
}

}  // namespace

FILE* open_memstream(char** buffer, size_t* buffer_size)
{
	HANDLE hread, hwrite;
	if (!CreatePipe(&hread, &hwrite, nullptr, 0))
	{
		return nullptr;
	}

	int fd = _open_osfhandle((intptr_t)hwrite, 0);
	FILE* f = nullptr;
	if (fd != -1)
	{
		f = _fdopen(fd, "wb");
		if (f != 0)
		{
			pipe_data* data = new pipe_data;
			data->pipe = hread;
			data->buffer = buffer;
			data->buffer_size = buffer_size;
			intptr_t ret = _beginthreadex(nullptr, 0, PipeReader, data, 0, nullptr);
			if (ret != -1)
			{
				threads[f] = reinterpret_cast<HANDLE>(ret);
			}
			else
			{
				fclose(f);
				f = nullptr;
			}
		}
		else
		{
			_close(fd);
		}
	}
	else
	{
		CloseHandle(hread);
		CloseHandle(hwrite);
	}
	return f;
}

bool close_memstream(FILE* f)
{
	auto thread = threads.find(f);
	if (thread == threads.end())
	{
		return false;
	}
	WaitForSingleObject(thread->second, INFINITE);
	CloseHandle(thread->second);
	threads.erase(thread);
	return true;
}
