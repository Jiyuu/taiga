/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include "file.h"
#include "string.h"
#include "win/win_main.h"
#include "win/win_registry.h"

#define MAKEQWORD(a, b)	((QWORD)( ((QWORD) ((DWORD) (a))) << 32 | ((DWORD) (b))))

////////////////////////////////////////////////////////////////////////////////

unsigned long GetFileAge(const wstring& path) {
  FILETIME ft_file, ft_now;
  
  // Get the time the file was last modified
  HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, 
    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) return 0;
  BOOL result = GetFileTime(hFile, NULL, NULL, &ft_file);
  CloseHandle(hFile);
  if (!result) return 0;
  
  // Get current time
  SYSTEMTIME st_now;
  GetSystemTime(&st_now);
  SystemTimeToFileTime(&st_now, &ft_now);

  // Convert to ULARGE_INTEGER
  ULARGE_INTEGER ul_file, ul_now;
  ul_file.LowPart = ft_file.dwLowDateTime;
  ul_file.HighPart = ft_file.dwHighDateTime;
  ul_now.LowPart = ft_now.dwLowDateTime;
  ul_now.HighPart = ft_now.dwHighDateTime;

  // Return difference in seconds
  return static_cast<unsigned long>((ul_now.QuadPart - ul_file.QuadPart) / 10000000);
}

QWORD GetFileSize(const wstring& path) {
  QWORD file_size = 0;
  HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, 
    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile != INVALID_HANDLE_VALUE) {
    DWORD size_low = 0, size_high = 0;
    size_low = ::GetFileSize(hFile, &size_high);
    file_size = (size_low == INVALID_FILE_SIZE) ? 0 : MAKEQWORD(size_high, size_low);
    CloseHandle(hFile);
  }

  return file_size;
}

QWORD GetFolderSize(const wstring& path, bool recursive) {
  QWORD folder_size = 0;
  WIN32_FIND_DATA wfd;
  wstring folder = path + L"*.*";
  
  HANDLE hFind = FindFirstFile(folder.c_str(), &wfd);
  if (hFind == INVALID_HANDLE_VALUE) return 0;

  do {
    if (recursive && wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (wcscmp(wfd.cFileName, L".") != 0 && wcscmp(wfd.cFileName, L"..") != 0) {
        folder = path + wfd.cFileName + L"\\";
        folder_size += GetFolderSize(folder, recursive);
      }
    }
    folder_size += (wfd.nFileSizeHigh * (MAXDWORD + 1)) + wfd.nFileSizeLow;
  } while (FindNextFile(hFind, &wfd));
	
  FindClose(hFind);
  return folder_size;
}

////////////////////////////////////////////////////////////////////////////////

bool Execute(const wstring& path, const wstring& parameters) {
  if (path.empty()) return false;
  HINSTANCE value = ShellExecute(NULL, L"open", path.c_str(), parameters.c_str(), NULL, SW_SHOWNORMAL);
  return reinterpret_cast<int>(value) > 32;
}

BOOL ExecuteEx(const wstring& path, const wstring& parameters) {
  SHELLEXECUTEINFO si = {0};
  si.cbSize = sizeof(SHELLEXECUTEINFO);
  si.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE;
  si.lpVerb = L"open";
  si.lpFile = path.c_str();
  si.lpParameters = parameters.c_str();
  si.nShow = SW_SHOWNORMAL;
  return ShellExecuteEx(&si);
}

void ExecuteLink(const wstring& link) {
  ShellExecute(NULL, NULL, link.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

////////////////////////////////////////////////////////////////////////////////

wstring BrowseForFile(HWND hwndOwner, LPCWSTR lpstrTitle, LPCWSTR lpstrFilter) {
  WCHAR szFile[MAX_PATH] = {'\0'};

  if (!lpstrFilter) {
    lpstrFilter = L"All files (*.*)\0*.*\0";
  }

  OPENFILENAME ofn = {0};
  ofn.lStructSize  = sizeof(OPENFILENAME);
  ofn.hwndOwner    = hwndOwner;
  ofn.lpstrFile    = szFile;
  ofn.lpstrFilter  = lpstrFilter;
  ofn.lpstrTitle   = lpstrTitle;
  ofn.nMaxFile     = sizeof(szFile);
  ofn.Flags        = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
  
  if (GetOpenFileName(&ofn)) {
    return szFile;
  } else {
    return L"";
  }
}

bool BrowseForFolderVista(HWND hwnd, const wstring& title, const wstring& default_folder, wstring& output) {
  IFileDialog* pFileDialog;
  bool result = false;

  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&pFileDialog));

  if (SUCCEEDED(hr)) {
    FILEOPENDIALOGOPTIONS fos;
    pFileDialog->GetOptions(&fos);
    fos |= FOS_PICKFOLDERS;
    pFileDialog->SetOptions(fos);

    if (!title.empty())
      pFileDialog->SetTitle(title.c_str());

    if (!default_folder.empty()) {
      IShellItem* pShellItem;
      HRESULT hr = NULL;

      typedef HRESULT (WINAPI *_SHCreateItemFromParsingName)(
        PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);
      HMODULE hShell32 = LoadLibrary(L"shell32.dll");
      if (hShell32 != NULL) {
        _SHCreateItemFromParsingName proc = 
          (_SHCreateItemFromParsingName)GetProcAddress(hShell32, "SHCreateItemFromParsingName");
        if (proc != NULL) {
          hr = (proc)(default_folder.c_str(), nullptr, IID_IShellItem,
                      reinterpret_cast<void**>(&pShellItem));
        }
        FreeLibrary(hShell32);
      }

      if (SUCCEEDED(hr)) {
        pFileDialog->SetDefaultFolder(pShellItem);
        pShellItem->Release();
      }
    }

    hr = pFileDialog->Show(hwnd);
    if (SUCCEEDED(hr)) {
      IShellItem* pShellItem;
      hr = pFileDialog->GetFolder(&pShellItem);
      if (SUCCEEDED(hr)) {
        LPWSTR path = nullptr;
        hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &path);
        if (SUCCEEDED(hr)) {
          output.assign(path);
          CoTaskMemFree(path);
          result = true;
        }
        pShellItem->Release();
      }
    }
  
    pFileDialog->Release();
  }

  return result;
}

static int CALLBACK BrowseForFolderXPProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
  switch (uMsg) {
    case BFFM_INITIALIZED:
      if (lpData != 0)
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
      break;
  }
  return 0;
}

bool BrowseForFolderXP(HWND hwnd, const wstring& title, const wstring& default_path, wstring& output) {
  BROWSEINFO bi = {0};
  bi.hwndOwner = hwnd;
  bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;
  
  if (!title.empty())
    bi.lpszTitle = title.c_str();
  
  if (!default_path.empty()) {
    WCHAR lpszDefault[MAX_PATH] = {'\0'};
    default_path.copy(lpszDefault, MAX_PATH);
    bi.lParam = reinterpret_cast<LPARAM>(lpszDefault);
    bi.lpfn = BrowseForFolderXPProc;
  }
  
  PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
  if (pidl == nullptr)
    return false;
  
  WCHAR path[MAX_PATH];
  SHGetPathFromIDList(pidl, path);
  output = path;
  if (output.empty())
    return false;

  return true;
}

BOOL BrowseForFolder(HWND hwnd, const wstring& title, const wstring& default_path, wstring& output) {
  if (win::GetWinVersion() >= win::VERSION_VISTA) {
    return BrowseForFolderVista(hwnd, title, default_path, output);
  } else {
    return BrowseForFolderXP(hwnd, title, default_path, output);
  }
}

////////////////////////////////////////////////////////////////////////////////

bool CreateFolder(const wstring& path) {
  return SHCreateDirectoryEx(NULL, path.c_str(), NULL) == ERROR_SUCCESS;
}

int DeleteFolder(wstring path) {
  if (path.back() == '\\') path.pop_back();
  path.push_back('\0');
  SHFILEOPSTRUCT fos = {0};
  fos.wFunc = FO_DELETE;
  fos.pFrom = path.c_str();
  fos.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
  return SHFileOperation(&fos);
}

////////////////////////////////////////////////////////////////////////////////

bool FileExists(const wstring& file) {
  if (file.empty()) return false;
  HANDLE hFile = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE) {
    CloseHandle(hFile);
    return true;
  }
  return false;
}

bool FolderExists(const wstring& path) {
  DWORD file_attr = GetFileAttributes(path.c_str());
  return (file_attr != INVALID_FILE_ATTRIBUTES) && (file_attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool PathExists(const wstring& path) {
  return GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void ValidateFileName(wstring& file) {
  EraseChars(file, L"\\/:*?<>|");
}

////////////////////////////////////////////////////////////////////////////////

wstring ExpandEnvironmentStrings(const wstring& path) {
  WCHAR buff[MAX_PATH];
  if (::ExpandEnvironmentStrings(path.c_str(), buff, MAX_PATH)) {
    return buff;
  } else {
    return path;
  }
}

wstring GetDefaultAppPath(const wstring& extension, const wstring& default_value) {
  win::Registry reg;
  reg.OpenKey(HKEY_CLASSES_ROOT, extension, 0, KEY_QUERY_VALUE);
  wstring path = reg.QueryValue(L"");
  
  if (!path.empty()) {
    path += L"\\shell\\open\\command";
    reg.OpenKey(HKEY_CLASSES_ROOT, path, 0, KEY_QUERY_VALUE);
    path = reg.QueryValue(L"");
    Replace(path, L"\"", L"");
    Trim(path, L" %1");
  }

  reg.CloseKey();
  return path.empty() ? default_value : path;
}

////////////////////////////////////////////////////////////////////////////////

int PopulateFiles(vector<wstring>& file_list, wstring path, wstring extension, bool recursive, bool trim_extension) {
  if (path.empty()) return 0;
  wstring folder = path + L"*.*";
  int found = 0;

  WIN32_FIND_DATA wfd;
  HANDLE hFind = FindFirstFile(folder.c_str(), &wfd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (recursive && wcscmp(wfd.cFileName, L".") != 0 && wcscmp(wfd.cFileName, L"..") != 0) {
          folder = path + wfd.cFileName + L"\\";
          found += PopulateFiles(file_list, folder, extension, recursive, trim_extension);
        }
      } else if (wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
        if (extension.empty() || IsEqual(GetFileExtension(wfd.cFileName), extension)) {
          if (trim_extension) {
            file_list.push_back(GetFileWithoutExtension(wfd.cFileName));
          } else {
            file_list.push_back(wfd.cFileName);
          }
          found++;
        }
      }
    } while (FindNextFile(hFind, &wfd));
    FindClose(hFind);
  }

  return found;
}

int PopulateFolders(vector<wstring>& folder_list, wstring path) {
  if (path.empty()) return 0;
  path += L"*.*";
  int found = 0;

  WIN32_FIND_DATA wfd;
  HANDLE hFind = FindFirstFile(path.c_str(), &wfd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (wcscmp(wfd.cFileName, L".") != 0 && wcscmp(wfd.cFileName, L"..") != 0) {
          found++;
          folder_list.push_back(wfd.cFileName);
        }
      }
    } while (FindNextFile(hFind, &wfd));
    FindClose(hFind);
  }

  return found;
}

////////////////////////////////////////////////////////////////////////////////

bool SaveToFile(LPCVOID data, DWORD length, const string_t& path, bool take_backup) {
  // Make sure the path is available
  CreateFolder(GetPathOnly(path));

  // Take a backup if needed
  if (take_backup) {
    wstring new_path = path + L".bak";
    MoveFileEx(path.c_str(), new_path.c_str(),
               MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
  }

  // Save the data
  BOOL result = FALSE;
  HANDLE file_Handle = ::CreateFile(path.c_str(), GENERIC_WRITE, 0, NULL,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_Handle != INVALID_HANDLE_VALUE) {
    DWORD bytes_written = 0;
    result = ::WriteFile(file_Handle, (LPCVOID)data, length, &bytes_written, NULL);
    ::CloseHandle(file_Handle);
  }

  return result != FALSE;
}

////////////////////////////////////////////////////////////////////////////////

wstring ToSizeString(QWORD qwSize) {
  wstring size, unit;

  if (qwSize > 1073741824) {      // 2^30
    size = ToWstr(static_cast<double>(qwSize) / 1073741824, 2);
    unit = L" GB";
  } else if (qwSize > 1048576) {  // 2^20
    size = ToWstr(static_cast<double>(qwSize) / 1048576, 2);
    unit = L" MB";
  } else if (qwSize > 1024) {     // 2^10
    size = ToWstr(static_cast<double>(qwSize) / 1024, 2);
    unit = L" KB";
  } else {
    size = ToWstr(qwSize);
    unit = L" bytes";
  }

  return size + unit;
}