// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont
//============================================================
//
//  sdlos_*.c - OS specific low level code
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <cstdio>
#include <memory>

// MAME headers
#include "osdlib.h"
#include "osdcomm.h"
#include "osdcore.h"
#include "strconv.h"

#ifdef OSD_WINDOWS
#include "winutf8.h"
#endif

//============================================================
//  MACROS
//============================================================

// presumed size of a page of memory
#define PAGE_SIZE           4096

// align allocations to start or end of the page?
#define GUARD_ALIGN_START   0

#if defined(__BIGGEST_ALIGNMENT__)
#define MAX_ALIGNMENT       __BIGGEST_ALIGNMENT__
#elif defined(__AVX__)
#define MAX_ALIGNMENT       32
#elif defined(__SSE__) || defined(__x86_64__) || defined(_M_X64)
#define MAX_ALIGNMENT       16
#else
#define MAX_ALIGNMENT       sizeof(INT64)
#endif


//============================================================
//  GLOBAL VARIABLES
//============================================================

#ifdef OSD_WINDOWS
void (*s_debugger_stack_crawler)() = nullptr;
#endif


//============================================================
//  osd_getenv
//============================================================

const char *osd_getenv(const char *name)
{
	return getenv(name);
}


//============================================================
//  osd_setenv
//============================================================

int osd_setenv(const char *name, const char *value, int overwrite)
{
	char *buf;
	int result;

	if (!overwrite)
	{
		if (osd_getenv(name) != nullptr)
			return 0;
	}
	buf = (char *) osd_malloc_array(strlen(name)+strlen(value)+2);
	sprintf(buf, "%s=%s", name, value);
	result = putenv(buf);

	/* will be referenced by environment
	 * Therefore it is not freed here
	 */

	return result;
}

//============================================================
//  osd_process_kill
//============================================================

void osd_process_kill()
{
	std::fflush(stdout);
	std::fflush(stderr);
	TerminateProcess(GetCurrentProcess(), -1);
}

//============================================================
//  osd_malloc
//============================================================

void *osd_malloc(size_t size)
{
#ifndef MALLOC_DEBUG
	return malloc(size);
#else
	// add in space for the size and offset
	size += MAX_ALIGNMENT + sizeof(size_t) + 2;
	size &= ~size_t(1);

	// basic objects just come from the heap
	UINT8 *const block = reinterpret_cast<UINT8 *>(HeapAlloc(GetProcessHeap(), 0, size));
	if (block == nullptr)
		return nullptr;
	UINT8 *const result = reinterpret_cast<UINT8 *>(reinterpret_cast<FPTR>(block + sizeof(size_t) + MAX_ALIGNMENT) & ~(FPTR(MAX_ALIGNMENT) - 1));

	// store the size and return and pointer to the data afterward
	*reinterpret_cast<size_t *>(block) = size;
	*(result - 1) = result - block;
	return result;
#endif
}


//============================================================
//  osd_malloc_array
//============================================================

void *osd_malloc_array(size_t size)
{
#ifndef MALLOC_DEBUG
	return malloc(size);
#else
	// add in space for the size and offset
	size += MAX_ALIGNMENT + sizeof(size_t) + 2;
	size &= ~size_t(1);

	// round the size up to a page boundary
	size_t const rounded_size = ((size + sizeof(void *) + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

	// reserve that much memory, plus two guard pages
	void *page_base = VirtualAlloc(nullptr, rounded_size + 2 * PAGE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
	if (page_base == nullptr)
		return nullptr;

	// now allow access to everything but the first and last pages
	page_base = VirtualAlloc(reinterpret_cast<UINT8 *>(page_base) + PAGE_SIZE, rounded_size, MEM_COMMIT, PAGE_READWRITE);
	if (page_base == nullptr)
		return nullptr;

	// work backwards from the page base to get to the block base
	UINT8 *const block = GUARD_ALIGN_START ? reinterpret_cast<UINT8 *>(page_base) : (reinterpret_cast<UINT8 *>(page_base) + rounded_size - size);
	UINT8 *const result = reinterpret_cast<UINT8 *>(reinterpret_cast<FPTR>(block + sizeof(size_t) + MAX_ALIGNMENT) & ~(FPTR(MAX_ALIGNMENT) - 1));

	// store the size at the start with a flag indicating it has a guard page
	*reinterpret_cast<size_t *>(block) = size | 1;
	*(result - 1) = result - block;
	return result;
#endif
}


//============================================================
//  osd_free
//============================================================

void osd_free(void *ptr)
{
#ifndef MALLOC_DEBUG
	free(ptr);
#else
	UINT8 const offset = *(reinterpret_cast<UINT8 *>(ptr) - 1);
	UINT8 *const block = reinterpret_cast<UINT8 *>(ptr) - offset;
	size_t const size = *reinterpret_cast<size_t *>(block);

	if ((size & 0x1) == 0)
	{
		// if no guard page, just free the pointer
		HeapFree(GetProcessHeap(), 0, block);
	}
	else
	{
		// large items need more care
		ULONG_PTR const page_base = reinterpret_cast<ULONG_PTR>(block) & ~(PAGE_SIZE - 1);
		VirtualFree(reinterpret_cast<void *>(page_base - PAGE_SIZE), 0, MEM_RELEASE);
	}
#endif
}


//============================================================
//  osd_alloc_executable
//
//  allocates "size" bytes of executable memory.  this must take
//  things like NX support into account.
//============================================================

void *osd_alloc_executable(size_t size)
{
	return VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}


//============================================================
//  osd_free_executable
//
//  frees memory allocated with osd_alloc_executable
//============================================================

void osd_free_executable(void *ptr, size_t size)
{
	VirtualFree(ptr, 0, MEM_RELEASE);
}


//============================================================
//  osd_break_into_debugger
//============================================================

void osd_break_into_debugger(const char *message)
{
#ifdef OSD_WINDOWS
	if (IsDebuggerPresent())
	{
		win_output_debug_string_utf8(message);
		DebugBreak();
	}
	else if (s_debugger_stack_crawler != nullptr)
		(*s_debugger_stack_crawler)();
#else
	if (IsDebuggerPresent())
	{
		OutputDebugStringA(message);
		DebugBreak();
	}
#endif
}

//============================================================
//  get_clipboard_text_by_format
//============================================================

static char *get_clipboard_text_by_format(UINT format, std::string (*convert)(LPCVOID data))
{
	char *result = nullptr;
	HANDLE data_handle;
	LPVOID data;

	// check to see if this format is available
	if (IsClipboardFormatAvailable(format))
	{
		// open the clipboard
		if (OpenClipboard(nullptr))
		{
			// try to access clipboard data
			data_handle = GetClipboardData(format);
			if (data_handle != nullptr)
			{
				// lock the data
				data = GlobalLock(data_handle);
				if (data != nullptr)
				{
					// invoke the convert
					std::string s = (*convert)(data);

					// copy the string
					result = (char *) osd_malloc(s.size() + 1);
					if (result != nullptr)
						memcpy(result, s.data(), (s.size() + 1) * sizeof(*result));

					// unlock the data
					GlobalUnlock(data_handle);
				}
			}

			// close out the clipboard
			CloseClipboard();
		}
	}
	return result;
}

//============================================================
//  convert_wide
//============================================================

static std::string convert_wide(LPCVOID data)
{
	return utf8_from_wstring((LPCWSTR) data);
}

//============================================================
//  convert_ansi
//============================================================

static std::string convert_ansi(LPCVOID data)
{
	return utf8_from_astring((LPCSTR) data);
}

//============================================================
//  osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text(void)
{
	// try to access unicode text
	char *result = get_clipboard_text_by_format(CF_UNICODETEXT, convert_wide);

	// try to access ANSI text
	if (result == nullptr)
		result = get_clipboard_text_by_format(CF_TEXT, convert_ansi);

	return result;
}

//============================================================
//  osd_dynamic_bind
//============================================================

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
// for classic desktop applications
#define load_library(filename) LoadLibrary(filename)
#else
// for Windows Store universal applications
#define load_library(filename) LoadPackagedLibrary(filename, 0)
#endif

namespace osd {
class dynamic_module_win32_impl : public dynamic_module
{
public:
	dynamic_module_win32_impl(std::vector<std::string> &libraries)
		: m_module(nullptr)
	{
		m_libraries = libraries;
	}

	virtual ~dynamic_module_win32_impl() override
	{
		if (m_module != nullptr)
			FreeLibrary(m_module);
	};

protected:
	virtual generic_fptr_t get_symbol_address(char const *symbol) override
	{
		/*
		 * given a list of libraries, if a first symbol is successfully loaded from
		 * one of them, all additional symbols will be loaded from the same library
		 */
		if (m_module)
		{
			return reinterpret_cast<generic_fptr_t>(GetProcAddress(m_module, symbol));
		}

		for (auto const &library : m_libraries)
		{
			auto tempstr = tstring_from_utf8(library.c_str());
			HMODULE module = load_library(tempstr.c_str());

			if (module != nullptr)
			{
				generic_fptr_t function = reinterpret_cast<generic_fptr_t>(GetProcAddress(module, symbol));

				if (function != nullptr)
				{
					m_module = module;
					return function;
				}
				else
				{
					FreeLibrary(module);
				}
			}
		}

		return nullptr;
	}

private:
	std::vector<std::string> m_libraries;
	HMODULE                  m_module;
};

dynamic_module::ptr dynamic_module::open(std::vector<std::string> &&names)
{
	return std::make_unique<dynamic_module_win32_impl>(names);
}

} // namespace osd
