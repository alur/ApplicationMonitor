/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  ApplicationMonitor.h
 *  ApplicationMonitor
 *
 *  Constants and function declarations for ApplicationMonitor.
 *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <map>
#include <string>
#include <windows.h>
#include <tchar.h>
#include "lsapi.h"
#include <strsafe.h>
#include <tlhelp32.h>

// Constants
LPCTSTR g_rcsRevision	    = _T("0.2");
LPCTSTR g_szAppName	        = _T("ApplicationMonitor");
LPCTSTR g_szMsgHandler	    = _T("LSApplicationMonitorManager");
const UINT g_lsMessages[]	= { LM_GETREVID, LM_REFRESH, 0 };

// Typedefs
typedef struct MonitorData
{
    TCHAR szName[MAX_RCCOMMAND];
    TCHAR szApplication[MAX_PATH];
    UINT iUpdateFrequency;
    bool bIsRunning;
    TCHAR szOnEnd[MAX_LINE_LENGTH];
    TCHAR szOnStart[MAX_LINE_LENGTH];
} MonitorData;
typedef std::map<UINT, MonitorData> MonitorMap;

// Global Variables
HWND g_hParent, g_hwndMessageHandler;
HINSTANCE g_hInstance;
TCHAR g_szPath[MAX_PATH];
MonitorMap g_MonitorMap;

// Function declarations
extern "C" __declspec( dllexport ) int initModuleW(HWND hwndParent, HINSTANCE hDllInstance, LPCWSTR pszPath);
extern "C" __declspec( dllexport ) void quitModule(HINSTANCE hDllInstance);
LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool CreateMessageHandler();
void LoadConfig();
void ParseConfigLine(LPCTSTR pszLine);
void Check(UINT id, bool bInitialize = false);
bool AppIsRunning(LPCTSTR pszApp);
void SetEvar(LPCTSTR pszGroup, LPCTSTR pszEvar, LPCTSTR pszFormat, ...);
void GetPrefixedRCLine(LPTSTR pszDest, LPCTSTR pszPrefix, LPCTSTR pszOption, LPCTSTR pszDefault);
void BangApplicationMonitor(HWND, LPCTSTR pszArgs);
