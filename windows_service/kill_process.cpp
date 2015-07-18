/*
Copyright (c) 2015, Efstathios D. Sfecas  <stathizzz@gmail.com>
All rights reserved.

Version: MPL 2.0

The contents of this file are subject to the Mozilla Public License Version
2.0 (the "License"); you may not use this file except in compliance with
the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/2.0/

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "odbc.h"
#include <tlhelp32.h>
#include <stdio.h>
#include <aclapi.h>
#include <tchar.h>
#include <stdlib.h>

#if defined (_DEBUG)
#define TRACE(str)    OutputDebugString(str)
#else
#define TRACE(str)
#endif

BOOL adjust_dacl(HANDLE h, DWORD dwDesiredAccess)
{
	SID world = { SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, 0 };

	EXPLICIT_ACCESS ea =
	{
		0,
		SET_ACCESS,
		NO_INHERITANCE,
		{
			0, NO_MULTIPLE_TRUSTEE,
			TRUSTEE_IS_SID,
			TRUSTEE_IS_USER,
			0
		}
	};

	ACL* pdacl = 0;
	DWORD err = SetEntriesInAcl(1, &ea, 0, &pdacl);

	ea.grfAccessPermissions = dwDesiredAccess;
	ea.Trustee.ptstrName = (LPTSTR)(&world);

	if (err == ERROR_SUCCESS)
	{
		err = SetSecurityInfo(h, SE_KERNEL_OBJECT,
			DACL_SECURITY_INFORMATION, 0, 0, pdacl, 0);
		LocalFree(pdacl);

		return (err == ERROR_SUCCESS);
	} else
	{
		TRACE("adjust_dacl");

		return(FALSE);
	}
}
BOOL enable_token_privilege(HANDLE htok, LPCTSTR szPrivilege, TOKEN_PRIVILEGES *tpOld)
{
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (LookupPrivilegeValue(0, szPrivilege, &tp.Privileges[0].Luid))
	{
		DWORD cbOld = sizeof (*tpOld);

		if (AdjustTokenPrivileges(htok, FALSE, &tp, cbOld, tpOld, &cbOld))
		{
			return (ERROR_NOT_ALL_ASSIGNED != GetLastError());
		} else
		{
			TRACE("enable_token_privilege");

			return (FALSE);
		}
	} else
	{
		TRACE("enable_token_privilege");

		return (FALSE);
	}
}
BOOL enable_privilege(LPCTSTR szPrivilege)
{
	BOOL bReturn = FALSE;
	HANDLE hToken;
	TOKEN_PRIVILEGES tpOld;

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TRACE("enable_privilege");

		return(FALSE);
	}

	bReturn = (enable_token_privilege(hToken, szPrivilege, &tpOld));
	CloseHandle(hToken);

	return (bReturn);
}
HANDLE adv_open_process(DWORD pid, DWORD dwAccessRights)
{
	HANDLE hProcess = OpenProcess(dwAccessRights, FALSE, pid);

	if (hProcess == NULL)
	{
		HANDLE hpWriteDAC = OpenProcess(WRITE_DAC, FALSE, pid);

		if (hpWriteDAC == NULL)
		{
			HANDLE htok;
			TOKEN_PRIVILEGES tpOld;

			if (!OpenProcessToken(GetCurrentProcess(),
				TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &htok))
			{
				return(FALSE);
			}

			if (enable_token_privilege(htok, SE_TAKE_OWNERSHIP_NAME, &tpOld))
			{
				HANDLE hpWriteOwner = OpenProcess(WRITE_OWNER, FALSE, pid);

				if (hpWriteOwner != NULL)
				{
					BYTE buf[512];
					DWORD cb = sizeof buf;

					if (GetTokenInformation(htok, TokenUser, buf, cb, &cb))
					{
						DWORD err = SetSecurityInfo(hpWriteOwner, SE_KERNEL_OBJECT,
							OWNER_SECURITY_INFORMATION,
							((TOKEN_USER *)(buf))->User.Sid, 0, 0, 0);

						if (err == ERROR_SUCCESS)
						{
							if (!DuplicateHandle(GetCurrentProcess(), hpWriteOwner,
								GetCurrentProcess(), &hpWriteDAC,
								WRITE_DAC, FALSE, 0))
							{
								hpWriteDAC = NULL;
							}
						}
					}

					CloseHandle(hpWriteOwner);
				}

				AdjustTokenPrivileges(htok, FALSE, &tpOld, 0, 0, 0);
			}

			CloseHandle(htok);
		}

		if (hpWriteDAC)
		{
			adjust_dacl(hpWriteDAC, dwAccessRights);

			if (!DuplicateHandle(GetCurrentProcess(), hpWriteDAC,
				GetCurrentProcess(), &hProcess, dwAccessRights, FALSE, 0))
			{
				hProcess = NULL;
			}

			CloseHandle(hpWriteDAC);
		}
	}

	return (hProcess);
}
BOOL kill_process(DWORD pid)
{
	HANDLE hp = adv_open_process(pid, PROCESS_TERMINATE);

	if (hp != NULL) {
		BOOL bRet = TerminateProcess(hp, 1);
		CloseHandle(hp);
		return (bRet);
	}

	return (FALSE);
}
BOOL list_process_modules(DWORD dwPID,	LPCTSTR szProcessExeFile, LPCTSTR szLibrary)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	enable_privilege(SE_DEBUG_NAME);
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);

	if (hModuleSnap == (HANDLE)-1)
	{
		TRACE("CreateToolhelp32Snapshot() failed!");
		TRACE(szProcessExeFile);
		return (FALSE);
	}

	me32.dwSize = sizeof(MODULEENTRY32);

	if (!Module32First(hModuleSnap, &me32))
	{
		TRACE("Module32First() failed!");
		CloseHandle(hModuleSnap);
		return (FALSE);
	}

	do
	{
		if (!lstrcmpi(szLibrary, me32.szModule))
		{
			if (kill_process(dwPID)) {
				logToDBAndFile(DECL_INFO, "PROCESS: [%d] %s \t MODULE: %s ->", dwPID, szProcessExeFile, me32.szModule);
				CloseHandle(hModuleSnap);
				return (TRUE);
			}			
		}

	} while (Module32Next(hModuleSnap, &me32));

	CloseHandle(hModuleSnap);

	return (FALSE);
}
BOOL try_kill_process(int pid)
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	if (kill_process(pid)) {
		logToDBAndFile(DECL_INFO, "PROCESS: [%d] -> ", pid);
		return (TRUE);
	};

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		logToDBAndFile(DECL_ERROR, "CreateToolhelp32Snapshot (of processes) failed -> ");
		return (FALSE);
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32)) {
		logToDBAndFile(DECL_ERROR, "Process32First -> ");
		CloseHandle(hProcessSnap);
		return (FALSE);
	}

	do {		
		if (pid == pe32.th32ProcessID) {
			dwPriorityClass = 0;
			hProcess = adv_open_process(pe32.th32ProcessID, PROCESS_ALL_ACCESS);

			if (hProcess == NULL){
				logToDBAndFile(DECL_ERROR, "OpenProcess NULL -> ");
			} else {
				dwPriorityClass = GetPriorityClass(hProcess);
				if (!dwPriorityClass) {
					logToDBAndFile(DECL_ERROR, "GetPriorityClass 0 -> ");
				}
				CloseHandle(hProcess);
			}
				
			logToDBAndFile(DECL_INFO, "PROCESS: [%d] %s \t MODULE: %d -> ", pid, pe32.szExeFile, pe32.th32ModuleID);
			if (kill_process(pid)) {				
				CloseHandle(hProcessSnap);
				return (TRUE);			
			}
			CloseHandle(hProcessSnap);
			return (FALSE);
		}		
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return (FALSE);
}
