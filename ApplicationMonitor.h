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
#include "lsapi.h"
#include <strsafe.h>
#include <tlhelp32.h>

// Constants
const char g_rcsRevision[]	= "0.2";
const char g_szAppName[]	= "ApplicationMonitor";
const char g_szMsgHandler[]	= "LSApplicationMonitorManager";
const UINT g_lsMessages[]	= { LM_GETREVID, LM_REFRESH, 0 };

// Typedefs
typedef struct MonitorData
{
	char szName[64];
	char szApplication[MAX_PATH];
	UINT iUpdateFrequency;
	bool bIsRunning;
	char szOnEnd[MAX_LINE_LENGTH];
	char szOnStart[MAX_LINE_LENGTH];
} MonitorData;
typedef std::map<UINT, MonitorData> MonitorMap;

// Global Variables
HWND g_hParent, g_hwndMessageHandler;
HINSTANCE g_hInstance;
char g_szPath[MAX_PATH];
MonitorMap g_MonitorMap;

// Function declarations
extern "C" __declspec( dllexport ) int initModuleEx(HWND hwndParent, HINSTANCE hDllInstance, LPCSTR szPath);
extern "C" __declspec( dllexport ) void quitModule(HINSTANCE hDllInstance);
LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool CreateMessageHandler();
void LoadConfig();
void ParseConfigLine(LPCSTR szLine);
void Check(UINT id, bool bInitialize = false);
bool AppIsRunning(LPCSTR szApp);
void SetEvar(LPCSTR pszGroup, LPCSTR pszEvar, LPCSTR pszFormat, ...);
void GetPrefixedRCLine(char *szDest, LPCSTR szPrefix, LPCSTR szOption, LPCSTR szDefault);
void BangApplicationMonitor(HWND, LPCSTR pszArgs);
