// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  strconv.h - String conversion
//
//============================================================

#ifndef MAME_OSD_STRCONV_H
#define MAME_OSD_STRCONV_H

#include "osdcore.h"



//============================================================
//  FUNCTION PROTOTYPES
//============================================================

#if defined(WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

// the result of these functions has to be released with osd_free()

std::string astring_from_utf8(const char *s);
std::string &astring_from_utf8(std::string &dst, const char *s);
std::string utf8_from_astring(const CHAR *s);
std::string &utf8_from_astring(std::string &dst, const CHAR *s);

std::wstring wstring_from_utf8(const char *s);
std::wstring &wstring_from_utf8(std::wstring &dst, const char *s);
std::string utf8_from_wstring(const WCHAR *s);
std::string &utf8_from_wstring(std::string &dst, const WCHAR *s);

#ifdef UNICODE
#define tstring_from_utf8   wstring_from_utf8
#define utf8_from_tstring   utf8_from_wstring
#else // !UNICODE
#define tstring_from_utf8   astring_from_utf8
#define utf8_from_tstring   utf8_from_astring
#endif // UNICODE

#endif // defined(WIN32)


#endif // MAME_OSD_STRCONV_H
