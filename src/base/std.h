/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STD_H
#define STD_H

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <psapi.h>
#include <shlobj.h>
#include <winhttp.h>

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::wstring;

extern HINSTANCE g_hInstance;
extern HWND g_hMain;

#endif // STD_H