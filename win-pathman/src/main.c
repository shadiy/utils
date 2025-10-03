#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include <shellapi.h>
#include <synchapi.h>
#include <windef.h>
#include <winuser.h>

#include <psapi.h>

#include "../libs/argparse/argparse.h"

BOOL add = FALSE;
BOOL rem = FALSE;
BOOL temp = FALSE;

wchar_t *GetCurrentPath()
{
  // Get current PATH (wide)
  wchar_t *path_w = NULL;
  size_t path_w_len = 0;
  _wdupenv_s(&path_w, &path_w_len, L"PATH");
  return path_w;
}

void RemoveFromPath(wchar_t *buffer)
{
  wchar_t *old_path_w = GetCurrentPath();
  if (!old_path_w)
  {
    wprintf(L"ERROR: Failed to get current PATH.\n");
    return;
  }

  // Remove buffer from PATH (temporary)
  wchar_t *start = old_path_w;
  wchar_t *found;
  size_t buffer_len = wcslen(buffer);
  wchar_t new_path_w[4096] = {0};
  size_t pos = 0;
  while ((found = wcsstr(start, buffer)) != NULL)
  {
    // Copy up to found
    size_t len = found - start;
    if (len > 0)
    {
      wcsncpy(new_path_w + pos, start, len);
      pos += len;
    }
    // Skip the buffer and any trailing semicolon
    start = found + buffer_len;
    if (*start == L';')
      start++;
  }
  // Copy the rest
  wcscpy(new_path_w + pos, start);

  // Remove leading/trailing semicolons
  wchar_t *np = new_path_w;
  while (*np == L';')
    np++;
  wchar_t *end = new_path_w + wcslen(new_path_w) - 1;
  while (end > np && *end == L';')
    *end-- = L'\0';

  // Set PATH for current process (temporary, wide)
  _wputenv_s(L"PATH", np);
  wprintf(L"Temporary PATH set: %ls\n", np);

  if (!temp)
  {
    // Set system PATH (permanent, wide)
    HKEY hKey;
    LONG res = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                             L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                             0, KEY_READ | KEY_WRITE, &hKey);
    if (res == ERROR_SUCCESS)
    {
      wchar_t sys_path[4096];
      DWORD sys_path_len = sizeof(sys_path);
      DWORD type = REG_SZ;
      if (RegQueryValueExW(hKey, L"Path", NULL, &type, (LPBYTE)sys_path, &sys_path_len) == ERROR_SUCCESS)
      {
        // Remove buffer from system PATH
        wchar_t *sys_start = sys_path;
        wchar_t *sys_found;
        wchar_t new_sys_path[4096] = {0};
        size_t sys_pos = 0;
        while ((sys_found = wcsstr(sys_start, buffer)) != NULL)
        {
          size_t sys_len = sys_found - sys_start;
          if (sys_len > 0)
          {
            wcsncpy(new_sys_path + sys_pos, sys_start, sys_len);
            sys_pos += sys_len;
          }
          sys_start = sys_found + buffer_len;
          if (*sys_start == L';')
            sys_start++;
        }
        wcscpy(new_sys_path + sys_pos, sys_start);
        // Remove leading/trailing semicolons
        wchar_t *nsp = new_sys_path;
        while (*nsp == L';')
          nsp++;
        wchar_t *sys_end = new_sys_path + wcslen(new_sys_path) - 1;
        while (sys_end > nsp && *sys_end == L';')
          *sys_end-- = L'\0';
        RegSetValueExW(hKey, L"Path", 0, REG_SZ, (const BYTE *)nsp, (DWORD)(wcslen(nsp) + 1) * sizeof(wchar_t));
        wprintf(L"Permanent system PATH updated.\n");
      }
      else
      {
        wprintf(L"Failed to query system PATH.\n");
      }
      RegCloseKey(hKey);
    }
    else
    {
      wprintf(L"Failed to open registry for system PATH update.\n");
    }
  }

  wprintf(L"Removed %ls\n from PATH", buffer);
}

void AddToPath(wchar_t *buffer)
{
  wchar_t *old_path_w = GetCurrentPath();
  if (!old_path_w)
  {
    wprintf(L"ERROR: Failed to get current PATH.\n");
    return;
  }

  // Combine paths
  size_t new_path_w_len = wcslen(old_path_w) + wcslen(buffer) + 2;
  wchar_t *new_path_w = malloc(new_path_w_len * sizeof(wchar_t));
  swprintf(new_path_w, new_path_w_len, L"%s;%s", old_path_w, buffer);

  // Set PATH for current process (temporary, wide)
  _wputenv_s(L"PATH", new_path_w);
  wprintf(L"Temporary PATH set: %ls\n", new_path_w);

  if (!temp)
  {
    // Set system PATH (permanent, wide)
    HKEY hKey;
    LONG res = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                             L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                             0, KEY_READ | KEY_WRITE, &hKey);
    if (res == ERROR_SUCCESS)
    {
      wchar_t sys_path[4096];
      DWORD sys_path_len = sizeof(sys_path);
      DWORD type = REG_SZ;
      if (RegQueryValueExW(hKey, L"Path", NULL, &type, (LPBYTE)sys_path, &sys_path_len) == ERROR_SUCCESS)
      {
        // Append buffer to system PATH if not already present
        if (!wcsstr(sys_path, buffer))
        {
          wchar_t new_sys_path[4096];
          swprintf(new_sys_path, sizeof(new_sys_path) / sizeof(wchar_t), L"%s;%s", sys_path, buffer);
          RegSetValueExW(hKey, L"Path", 0, REG_SZ, (const BYTE *)new_sys_path, (DWORD)(wcslen(new_sys_path) + 1) * sizeof(wchar_t));
          wprintf(L"Permanent system PATH updated.\n");
        }
        else
        {
          wprintf(L"Directory already in system PATH.\n");
        }
      }
      RegCloseKey(hKey);
    }
    else
    {
      wprintf(L"Failed to open registry for system PATH update.\n");
    }
  }

  wprintf(L"Added %ls\n to PATH", buffer);
}

static const char *const usages[] = {
    "pathman [OPTIONS] {summary} [body]",
    NULL,
};

int main(int argc, const char *argv[])
{
  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_GROUP("Basic options"),
      OPT_STRING('a', "add", &add, "Add the directory", NULL, 0, 0),
      OPT_STRING('r', "remove", &rem, "Remove the directory", NULL, 0, 0),
      OPT_STRING('t', "temp", &temp, "Add the directory temporarily (for the current process)", NULL, 0, 0),
      OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);
  argparse_describe(&argparse, "\npathman - a program to add the current working directory to PATH", "\n");
  argc = argparse_parse(&argparse, argc, argv);

  if (!add && !rem)
  {
    printf("Please specify if you would like to add(-a) or remove(-r) directory");
    return 1;
  }

  // Get Current Directory
  wchar_t buffer[MAX_PATH] = {0};
  GetCurrentDirectoryW(MAX_PATH, buffer);

  if (add)
    AddToPath(buffer);

  if (rem)
    RemoveFromPath(buffer);

  return 0;
}
