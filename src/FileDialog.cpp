/**
  * IceSL-vrprinter, a tool to help simulate and visualize Gcode for 3D printers
  * Copyright (C) 2021  Sylvain Lefebvre    sylvain.lefebvre@inria.fr
  *                     Pierre Bedell       pierre.bedell@gmail.com
  *                     Salim Perchy        salim.perchy@gmail.com
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU Affero General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU Affero General Public License for more details.
  *
  * You should have received a copy of the GNU Affero General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

#include <LibSL/LibSL.h>
#include "FileDialog.h"

#ifndef EMSCRIPTEN

#ifndef WIN32

#include <tinyfiledialogs.h>

/// ==============================  Linux  =================================

std::string openFileDialog(std::vector<const char*> filter)
{
  const char* file = tinyfd_openFileDialog("Open File", NULL, filter.size(), filter.data(), NULL, 0);
  if(file == NULL) {
    return "";
  } else {
    return std::string(file);
  }
}

std::string openFileDialog(const char* directory, std::vector<const char*> filter)
{
  const char* file = tinyfd_openFileDialog("Open File", directory, filter.size(), filter.data(), NULL, 0);
  if (file == NULL) {
    return "";
  }
  else {
    return std::string(file);
  }
}

std::string saveFileDialog(const char* proposedFileNameFullPath, std::vector<const char*> filter)
{
  const char* file = tinyfd_saveFileDialog("Save File", proposedFileNameFullPath, filter.size(), filter.data(), NULL);
  if(file == NULL) {
    return "";
  } else {
    return std::string(file);
  }
}


std::string openFolderDialog(const char* proposedFolderNameFullPath)
{
  const char* folder = tinyfd_selectFolderDialog("Open File", proposedFolderNameFullPath);
  if(folder == NULL) {
    return "";
  } else {
    return std::string(folder);
  }
}

#else

#include <Windows.h>
#include <Shlobj.h>

using namespace std;

/// ============================== Windows =================================

std::string openFileDialog(const char *filter, bool changedir)
{
	////// DEBUG
	{
		char str[1024];
		GetCurrentDirectoryA(1024, str);
	}
	//////

  char szFile[512];
  memset(szFile,0x00,512);
  OPENFILENAMEA of;
  ZeroMemory(&of, sizeof(of));
  of.lStructSize = sizeof(of);
  of.hwndOwner = NULL; // SimpleUI::getHWND();
  of.lpstrFile = szFile;
  // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
  // use the contents of szFile to initialize itself.
  of.lpstrFile[0] = '\0';
  of.nMaxFile = sizeof(szFile);
  of.lpstrFilter = filter;
  of.nFilterIndex = 1;
  of.lpstrFileTitle = NULL;
  of.nMaxFileTitle = 0;
  of.lpstrInitialDir = NULL;
  of.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (changedir ? 0 : OFN_NOCHANGEDIR);
  if (GetOpenFileNameA(&of)) {
    string fname = string(of.lpstrFile);
		////// DEBUG
		{
			char str[1024];
			GetCurrentDirectoryA(1024, str);
		}
		//////
    return fname;
  }
	return "";
}

std::string openFileDialog(const char* directory, const char* filter, bool changedir)
{
  ////// DEBUG
  {
    char str[1024];
    GetCurrentDirectoryA(1024, str);
  }
  //////

  char szFile[512];
  memset(szFile, 0x00, 512);
  OPENFILENAMEA of;
  ZeroMemory(&of, sizeof(of));
  of.lStructSize = sizeof(of);
  of.hwndOwner = NULL; // SimpleUI::getHWND();
  of.lpstrFile = szFile;
  // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
  // use the contents of szFile to initialize itself.
  of.lpstrFile[0] = '\0';
  of.nMaxFile = sizeof(szFile);
  of.lpstrFilter = filter;
  of.nFilterIndex = 1;
  of.lpstrFileTitle = NULL;
  of.nMaxFileTitle = 0;
  of.lpstrInitialDir = directory;
  of.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (changedir ? 0 : OFN_NOCHANGEDIR);
  if (GetOpenFileNameA(&of)) {
    string fname = string(of.lpstrFile);
    ////// DEBUG
    {
      char str[1024];
      GetCurrentDirectoryA(1024, str);
    }
    //////
    return fname;
  }
  return "";
}

std::string saveFileDialog(const char* proposedFileNameFullPath, const char* filter)
{
	std::string proposedFileName(proposedFileNameFullPath);
  char szFile[MAX_PATH];
  std::replace(proposedFileName.begin(), proposedFileName.end(), '/', '\\');
#ifdef _MSC_VER
	strcpy_s(szFile, MAX_PATH, proposedFileName.c_str());
#else
	strcpy(szFile, proposedFileNameFullPath.c_str());
#endif
  OPENFILENAMEA of;
  ZeroMemory(&of, sizeof(of));
  of.lStructSize = sizeof(of);
  of.hwndOwner = NULL;
  of.lpstrFile = szFile;
  of.nMaxFile = sizeof(szFile);
  of.lpstrFilter = filter;
  of.nFilterIndex = 1;
  of.lpstrFileTitle = NULL;
  of.nMaxFileTitle = 0;
  of.lpstrInitialDir = NULL;
  of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
  if (GetSaveFileNameA(&of)) {
    string fname = string(of.lpstrFile);
    return fname;
  }
  return "";
}

std::string openFolderDialog(const char* title)
{
	TCHAR szDir[MAX_PATH];
	BROWSEINFO bInfo;
	bInfo.hwndOwner = NULL;
	bInfo.pidlRoot = NULL;
	bInfo.pszDisplayName = szDir;
	bInfo.lpszTitle = title;
	bInfo.ulFlags = 0;
	bInfo.lpfn = NULL;
	bInfo.lParam = 0;
	bInfo.iImage = -1;

	LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
	if (lpItem != NULL)
	{
		SHGetPathFromIDList(lpItem, szDir);
		return string(szDir);
	}
	return "";
}

#endif

#else

/// =========================================================================
/// Emscripten no-op
/// =========================================================================

std::string openFileDialog(const char* filter) { return ""; }
std::string openFileDialog(const char* directory, const char* filter) { return ""; }
std::string saveFileDialog(const char* proposedFileNameFullPath, const char* filter) { return ""; }
std::string openFolderDialog(const char* proposedFolderNameFullPath) { return ""; }

std::string openFileDialog(std::vector<const char *> filter) { return ""; }
std::string openFileDialog(const char *directory, std::vector<const char *> filter) { return ""; }
std::string saveFileDialog(const char *proposedFileNameFullPath, std::vector<const char *> filter) { return ""; }

#endif

/// =========================================================================
