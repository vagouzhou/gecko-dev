/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "webrtc/modules/desktop_capture/win/desktop_device_info_win.h"
typedef BOOL (WINAPI * QueryFullProcessImageNameProc)( HANDLE hProcess,DWORD dwFlags,LPTSTR lpExeName,PDWORD lpdwSize);
typedef DWORD (WINAPI *GetProcessImageFileNameProc)(HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);

namespace webrtc{

std::string Utf16ToUtf8(const WCHAR* str) {
	int len_utf8 = WideCharToMultiByte(CP_UTF8, 0, str, 1,
		NULL, 0, NULL, NULL);
	if (len_utf8 <= 0)
		return std::string();
	std::string result(len_utf8, '\0');
	int rv = WideCharToMultiByte(CP_UTF8, 0, str, 1,
		&*(result.begin()), len_utf8, NULL, NULL);
	if (rv != len_utf8)
		assert(false);

	return result;
}

DesktopDeviceInfo * DesktopDeviceInfoImpl::Create() {
	DesktopDeviceInfoWin * pDesktopDeviceInfo = new DesktopDeviceInfoWin();
	if(pDesktopDeviceInfo && pDesktopDeviceInfo->Init() != 0){
		delete pDesktopDeviceInfo;
		pDesktopDeviceInfo = NULL;
	}
	return pDesktopDeviceInfo;
}

DesktopDeviceInfoWin::DesktopDeviceInfoWin() {
}

DesktopDeviceInfoWin::~DesktopDeviceInfoWin() {
}

int32_t DesktopDeviceInfoWin::RefreshApplicationList() {
	CleanUpApplicationList();
	//List all running applications exclude background process.
	HWND hWnd = ::GetWindow(::GetDesktopWindow(), GW_CHILD );
	if(hWnd){
		do
		{
			if (!IsWindowVisible(hWnd))
				continue;

			DWORD dwProcessId = 0;
			::GetWindowThreadProcessId(hWnd, &dwProcessId);

			if(desktop_application_list_.find(dwProcessId)!=desktop_application_list_.end())
				continue;

			//Add one application
			DesktopApplication *pDesktopApplication = new DesktopApplication;
			if(pDesktopApplication){
				//process id
				pDesktopApplication->setProcessId(dwProcessId);

				//process path name
				WCHAR szFilePathName[MAX_PATH]={0};
				QueryFullProcessImageNameProc lpfnQueryFullProcessImageNameProc = (QueryFullProcessImageNameProc) ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")), "QueryFullProcessImageNameW");
				if(lpfnQueryFullProcessImageNameProc)//After Vista
				{
					DWORD dwMaxSize = _MAX_PATH;
					HANDLE hWndPro = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION  , FALSE, dwProcessId);
					if(hWndPro)
					{
						if(lpfnQueryFullProcessImageNameProc(hWndPro, 0, szFilePathName, &dwMaxSize))
							::CloseHandle(hWndPro);
					}
				}
				else{
					HMODULE hModPSAPI = LoadLibrary(TEXT("PSAPI.dll"));
					if(hModPSAPI)
					{
						GetProcessImageFileNameProc pfnGetProcessImageFileName =
							(GetProcessImageFileNameProc)GetProcAddress(hModPSAPI, "GetProcessImageFileNameW");

						if (pfnGetProcessImageFileName)
						{
							HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, 0, dwProcessId);
							if (hProcess)
							{
								DWORD dwMaxSize = _MAX_PATH;
								pfnGetProcessImageFileName(hProcess, szFilePathName, dwMaxSize);
								CloseHandle(hProcess);
							}
						}
						FreeLibrary(hModPSAPI);
					}
				}
				pDesktopApplication->setProcessPathName(Utf16ToUtf8(szFilePathName).c_str());

				//application name
				WCHAR szWndTitle[_MAX_PATH]={0};
				::GetWindowText(hWnd,szWndTitle,MAX_PATH);
				if(lstrlen(szWndTitle)<=0)
					pDesktopApplication->setProcessAppName(Utf16ToUtf8(szFilePathName).c_str());
				else
					pDesktopApplication->setProcessAppName(Utf16ToUtf8(szWndTitle).c_str());

				//setUniqueIdName
				std::ostringstream s;
				s<<dwProcessId;
				pDesktopApplication->setUniqueIdName(s.str().c_str());

				desktop_application_list_[pDesktopApplication->getProcessId()] = pDesktopApplication;
			}
		} while ((hWnd = ::GetWindow(hWnd, GW_HWNDNEXT)));
	}


	return 0;
}

} //namespace webrtc
