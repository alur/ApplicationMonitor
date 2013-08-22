/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  ApplicationMonitor.cpp
 *  ApplicationMonitor
 *
 *  Main code for ApplicationMonitor. A simple module which triggers events
 *  when programs start or stop.
 *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "ApplicationMonitor.h"


// Module entry point
BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID /* pvReserved */)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInst);

	return TRUE;
}


// Called on module load
int initModuleEx(HWND hwndParent, HINSTANCE hDllInstance, LPCSTR szPath)
{
	g_hParent = hwndParent;
	g_hInstance = hDllInstance;
	StringCchCopy(g_szPath, sizeof(g_szPath), szPath);

	if (!CreateMessageHandler())
		return 1;

	AddBangCommand("!ApplicationMonitor", BangApplicationMonitor);

	LoadConfig();

	return 0;
}


// Called on module unload
void quitModule(HINSTANCE hDllInstance)
{
	RemoveBangCommand("!ApplicationMonitor");

	if (g_hwndMessageHandler)
	{
		SendMessage(GetLitestepWnd(), LM_UNREGISTERMESSAGE, (WPARAM)g_hwndMessageHandler, (LPARAM)g_lsMessages);
		DestroyWindow(g_hwndMessageHandler);
	}

	UnregisterClass(g_szMsgHandler, hDllInstance);
}


// Loads all the .RC configurations
void LoadConfig()
{
	LPVOID f = LCOpen(NULL);
	char szLine[MAX_LINE_LENGTH];

	while (LCReadNextConfig(f, "*ApplicationMonitor", szLine, sizeof(szLine)))
	{
		ParseConfigLine(szLine);
	}

	LCClose(f);
}


//
void BangApplicationMonitor(HWND, LPCSTR pszArgs)
{
	ParseConfigLine(pszArgs);
}


// 
void ParseConfigLine(LPCSTR szLine)
{
	char szTimer[MAX_LINE_LENGTH];
	LPCSTR pszNext = szLine;
	MonitorData monData;
	monData.iUpdateFrequency = 100;

	// Retrive information from the line
	GetToken(pszNext, monData.szName, &pszNext, false);
	GetToken(pszNext, monData.szName, &pszNext, false);
	if (strcmp(monData.szName, "") == 0)
		return;
	GetToken(pszNext, monData.szApplication, &pszNext, false);
	if (strcmp(monData.szApplication, "") == 0)
		return;
	if (GetToken(pszNext, szTimer, &pszNext, false))
		if (strcmp(szTimer, "") != 0)
			monData.iUpdateFrequency = atoi(szTimer);

	// Keep the timer within resonable bounds
	if (monData.iUpdateFrequency > USER_TIMER_MAXIMUM) monData.iUpdateFrequency = USER_TIMER_MAXIMUM;
	if (monData.iUpdateFrequency < USER_TIMER_MINIMUM) monData.iUpdateFrequency = USER_TIMER_MINIMUM;

	// TODO::Avoid duplicates!

	// Find a free timer id
	UINT id = 0;
	MonitorMap::iterator iter;
	do
	{
		iter = g_MonitorMap.find(++id);
	}
	while (iter != g_MonitorMap.end());
	
	// Set the timer
	SetTimer(g_hwndMessageHandler, id, monData.iUpdateFrequency, NULL);

	// Load events from the RCs
	GetPrefixedRCLine(monData.szOnStart, monData.szName, "OnStart", "");
	GetPrefixedRCLine(monData.szOnEnd, monData.szName, "OnEnd", "");

	// Store all information in the MonitorMap
	g_MonitorMap.insert(MonitorMap::value_type(id, monData));
	
	// Initialize all values (avoids events fireing on startup/recycle)
	Check(id, true);
}


// Check if the application is running
void Check(UINT id, bool bInitialize)
{
	MonitorMap::iterator iter = g_MonitorMap.find(id);
	if (iter != g_MonitorMap.end())
	{
		bool bInitialState = iter->second.bIsRunning;

		iter->second.bIsRunning = AppIsRunning(iter->second.szApplication);

		if (bInitialize)
			SetEvar(iter->second.szName, "IsRunning", "%s", iter->second.bIsRunning ? "true" : "false");

		if (bInitialState != iter->second.bIsRunning )
		{
			SetEvar(iter->second.szName, "IsRunning", "%s", iter->second.bIsRunning ? "true" : "false");
			if (!bInitialize)
			{
				if (iter->second.bIsRunning)
				{
					LSExecute(NULL, iter->second.szOnStart, SW_SHOWNORMAL);
				}
				else
				{
					LSExecute(NULL, iter->second.szOnEnd, SW_SHOWNORMAL);
				}
			}
		}
	}
}


// Check whether or not an application is running
bool AppIsRunning(LPCSTR szApp)
{
	bool bReturn = false;
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32* processInfo = new PROCESSENTRY32;
	processInfo->dwSize = sizeof(PROCESSENTRY32);
	int index = 0;

	while(Process32Next(hSnapShot, processInfo) != FALSE)
	{
		if (!strcmp(processInfo->szExeFile, szApp))
		{
			bReturn = true;
			break;
		}
	}

	CloseHandle(hSnapShot);
	delete processInfo;

	return bReturn;
}


// Sets an evironment variable
void SetEvar(LPCSTR pszName, LPCSTR pszEvar, LPCSTR pszFormat, ...)
{
	char szValue[MAX_LINE_LENGTH] = { 0 };
	char szEvar[MAX_LINE_LENGTH] = { 0 };
	va_list argList;

	va_start(argList, pszFormat);
	StringCchVPrintfA(szValue, MAX_LINE_LENGTH, pszFormat, argList);
	va_end(argList);

	StringCchPrintfA(szEvar, sizeof(szEvar), "%s%s", pszName, pszEvar);
	LSSetVariable(szEvar, szValue);
}


// Retrive a prefixed line from the RCs
void GetPrefixedRCLine(char *szDest, LPCSTR szPrefix, LPCSTR szOption, LPCSTR szDefault)
{
	char szOptionName[MAX_LINE_LENGTH];
	StringCchPrintf(szOptionName, MAX_LINE_LENGTH, "%s%s", szPrefix, szOption);
	GetRCLine(szOptionName, szDest, MAX_LINE_LENGTH, szDefault);
}


// Creates the message handler
bool CreateMessageHandler()
{
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = MessageHandlerProc;
	wc.hInstance = g_hInstance;
	wc.lpszClassName = g_szMsgHandler;
	wc.hIconSm = 0;
	wc.style = CS_NOCLOSE;

	if (!RegisterClassEx(&wc))
		return false;

	g_hwndMessageHandler = CreateWindowEx(WS_EX_TOOLWINDOW, g_szMsgHandler, 0, WS_POPUP, 0, 0, 0, 0, g_hParent, 0, g_hInstance, 0);

	if (!g_hwndMessageHandler)
		return false;
	
	SendMessage(GetLitestepWnd(), LM_REGISTERMESSAGE, (WPARAM)g_hwndMessageHandler, (LPARAM)g_lsMessages);

	return true;
}


// Message Handler
LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case LM_REFRESH:
		{
			LoadConfig();
			return 0;
		}

	case LM_GETREVID:
		{
			StringCchPrintf((char*)lParam, 64, "%s: %s", g_szAppName, g_rcsRevision);
			size_t uLength;
			if (SUCCEEDED(StringCchLength((char*)lParam, 64, &uLength)))
				return uLength;
			lParam = NULL;
			return 0;
		}

	case WM_TIMER:
		{
			Check((UINT)wParam);
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
