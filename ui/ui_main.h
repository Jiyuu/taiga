/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#ifndef UI_MAIN_H
#define UI_MAIN_H

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include <map>
#include <string>
#include <vector>

#include "ui_window.h"

using std::vector;
using std::wstring;

// =============================================================================

/* Window map */

typedef std::map<HWND, CWindow*> WndMap;

class CWindowMap {
public:  
  CWindow* GetWindow(HWND hWnd);
  void     Add(HWND hWnd, CWindow* pWindow);
  void     Remove(CWindow* pWindow);
  void     Remove(HWND hWnd);
  void     Clear();

private:
  WndMap   m_WindowMap;
};

extern CWindowMap WindowMap;

// =============================================================================

/* Application */

class CApp {
public:
  CApp();
  virtual ~CApp();

  virtual BOOL InitInstance();
  virtual int  MessageLoop();
  virtual void PostQuitMessage(int nExitCode = 0);
  virtual int  Run();

  BOOL      InitCommonControls(DWORD dwFlags);
  HINSTANCE GetInstanceHandle() const { return m_hInstance; }
  wstring   GetCurrentDirectory();
  wstring   GetModulePath();
  BOOL      SetCurrentDirectory(const wstring& directory);

private:
  HINSTANCE m_hInstance;
};

// =============================================================================

enum WinVersion {
  WINVERSION_PRE_2000    = 0,
  WINVERSION_2000        = 1,
  WINVERSION_XP          = 2,
  WINVERSION_SERVER_2003 = 3,
  WINVERSION_VISTA       = 4,
  WINVERSION_2008        = 5,
  WINVERSION_WIN7        = 6
};

WinVersion GetWinVersion();

void DEBUG_PRINT(wstring text);

#endif // UI_MAIN_H