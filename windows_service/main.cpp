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
#include <stdio.h>
#include <stdlib.h>
#include "exports.h"
#include "loader.h"
#include "odbc.h"
#include <iostream>

SERVICE_STATUS          ServiceStatus; 
SERVICE_STATUS_HANDLE   hStatus; 
using namespace std;

extern GLOBALS gl;

void  ServiceMain(int argc, char** argv); 
void  ControlHandler(DWORD request); 
int InitService();
void run(float *);

extern BOOL try_kill_process(int pid);
extern vector<string> get_all_jars_within_folder(string folder);
extern int compress_one_file(char *infilename, char *outfilename);
extern int decompress_one_file(const char *infilename, const char *outfilename);
extern unsigned long file_size(char *filename);
extern int zipppp(const char *file, const char *outfile);
extern bool checkIfUpdateRequired(const char *file, const char *newfile);
extern void readPcData();
extern void UpdaterLogic(SC_HANDLE schService, bool hasService);
extern void tryUpdateNoService(volatile BOOL *first_time, float *TIME_WAIT_UPDATE, SC_HANDLE *schService, bool overrideAll = true);
extern void tryUpdateWithService(volatile BOOL *first_time, float *TIME_WAIT_UPDATE, SC_HANDLE *schService);

// Service initialization
int InitService() 
{ 
	logToDBAndFile(DECL_INFO, "Monitoring started.\n");
	return 0;
}

void ServiceMain(int argc, char** argv) 
{ 
	int error; 
	MEMORYSTATUS memory;
	int result;	
	int status = -1;
	int counter = 0;
	float time_delay_update = get_time_ms();

	try	{
		readConfigToGlobals();
		readPcData();

		ServiceStatus.dwServiceType = SERVICE_WIN32;
		ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
		ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwServiceSpecificExitCode = 0;
		ServiceStatus.dwCheckPoint = 0;
		ServiceStatus.dwWaitHint = 0;

		hStatus = RegisterServiceCtrlHandler("ServiceWatchdog", (LPHANDLER_FUNCTION)ControlHandler);

		if (hStatus == (SERVICE_STATUS_HANDLE)0)
		{
			logToDBAndFile(DECL_INFO, "Watchdog Service could not get registered through RegisterServiceCtrlHandler.\n");
			// Registering Control Handler failed
			return;
		}
		// Initialize Service 
		error = InitService();
		if (error)
		{
			logToDBAndFile(DECL_INFO, "Watchdog Service could not get initialized.\n");
			// Initialization failed
			ServiceStatus.dwCurrentState = SERVICE_STOPPED;
			ServiceStatus.dwWin32ExitCode = -1;
			SetServiceStatus(hStatus, &ServiceStatus);
			return;
		}
		// We report the running status to SCM. 
		ServiceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus(hStatus, &ServiceStatus);
	}
	catch (exception &ex)
	{
		string fatal_log_file = ExePath() + "\\fatallog.txt";
		FILE * f = fopen(fatal_log_file.c_str(), "a");
		fprintf(f,"Fatal exception: %s", ex.what());
		fclose(f);
	}
	// The worker loop of a service
	while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		try {
			int mystatus;
			GlobalMemoryStatus(&memory);
			readConfigToGlobals();			
			run(&time_delay_update);					
			_sleep(gl.POLL_INTERVAL * 1000);
		} catch (...) {
			logToDBAndFile(DECL_ERROR, "Error!. Continuing watchdog service...\n");
		}
	}
}

void ControlHandler(DWORD request) 
{ 
   switch(request) 
   { 
      case SERVICE_CONTROL_STOP: 
		  logToDBAndFile(DECL_INFO, "Monitoring stopped.\n");

         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
         return; 
 
      case SERVICE_CONTROL_SHUTDOWN: 
		  logToDBAndFile(DECL_INFO, "Monitoring stopped.\n");

         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
         return; 
        
      default:
         break;
    } 
 
    // Report current status
    SetServiceStatus (hStatus, &ServiceStatus);
 
    return; 
}

volatile bool pending = false;
volatile bool pending_to_start = false;
volatile bool pending_on_stop = false;

float time_pending = 0.0, time_pending_on_stop = 0.0, time_pending_to_start = 0.0;

volatile BOOL first_time = TRUE;

void run(float *TIME_WAIT_UPDATE) {
	
	SC_HANDLE schService = NULL;
	SC_HANDLE hSc = NULL;
	HANDLE hToken = NULL;
	
	//logToDBAndFile(DECL_INFO, "Reading variables from configuration file\n");
	readConfigToGlobals();
	
	hSc = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSc) {
		logToDBAndFile(DECL_ERROR, "OpenSCManager failed (%d)\n", GetLastError());
		tryUpdateNoService(&first_time, TIME_WAIT_UPDATE, &schService);
		return;
	}
	
	// Get a handle to the service.
	schService = OpenService(
		hSc,            
		gl.SERVICE_NAME.c_str(),
		SERVICE_START | SERVICE_STOP | SERVICE_INTERROGATE | SERVICE_QUERY_STATUS | SERVICE_PAUSE_CONTINUE);
		//SERVICE_ALL_ACCESS);

	if (schService == NULL) {		
		logToDBAndFile(DECL_ERROR, "OpenService failed (%d)\n", GetLastError());
		tryUpdateNoService(&first_time, TIME_WAIT_UPDATE, &schService);
		CloseServiceHandle(hSc);
		return;
	}
				
	SERVICE_STATUS_PROCESS status;
	DWORD bytesNeeded;
	QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded);
		
	if (status.dwCurrentState == SERVICE_RUNNING) {
		//get the proces id
		gl.SERVICE_PROCESS_ID = status.dwProcessId;		
				
		if (pending) pending = false;
		if (pending_to_start) pending_to_start = false;
		
		tryUpdateWithService(&first_time, TIME_WAIT_UPDATE, &schService);

	} else if (status.dwCurrentState == SERVICE_STOPPED) {	
		if (pending_on_stop) pending_on_stop = false;
		logToDBAndFile(DECL_ERROR, "Service is stopped.\n");
		tryUpdateNoService(&first_time, TIME_WAIT_UPDATE, &schService, true);
		int instance_id = -1;
		logToDBAndFile(DECL_ERROR, "Trying database logging\n");
		int ret = spAppInstancesSearchCreate(&instance_id);
		ret = spAppInstancesSetState(instance_id, NULL);
		ret = spAppEventsCreate(instance_id, NULL);
		if (ret != SQL_SUCCESS) {
			//Log it
			logToDBAndFile(DECL_ERROR, "Failed database event logging.\n");
		}				
		BOOL b = StartService(schService, NULL, NULL);
		if (b) {
			logToDBAndFile(DECL_INFO, "Service started.\n");
		} else {
			logToDBAndFile(DECL_INFO, "Service failed to start on SERVICE_STOP.\n");
			if (!pending_on_stop) {
				time_pending_on_stop = get_time_ms();
				pending_on_stop = true;
			}
			float tp = get_time_passed_ms(time_pending_on_stop) / 1000.0;
			logToDBAndFile(DECL_ERROR, "Time passed on SERVICE_STOP: %.03f sec(s).\n", tp);
			if (gl.WAIT_BEFORE_KILL_INTERVAL <= tp) {
				pending = false;
				pending_to_start = false;
				pending_on_stop = false;
				logToDBAndFile(DECL_INFO, "Service is pending on stop for more than %.03f secs.\n", gl.WAIT_BEFORE_KILL_INTERVAL);
				if (gl.SERVICE_PROCESS_ID <= 0) {
					/* try to revice it */
					gl.SERVICE_PROCESS_ID = status.dwProcessId;
				}
				BOOL f = try_kill_process(gl.SERVICE_PROCESS_ID);
				if (f) logToDBAndFile(DECL_INFO, "Service killed on SERVICE_STOP.\n");
				else logToDBAndFile(DECL_INFO, "Service could not get killed on SERVICE_STOP.Error: %d.\n", GetLastError());
			}
		}
	} else if (status.dwCurrentState == SERVICE_START_PENDING ||
			status.dwCurrentState == SERVICE_STOP_PENDING ||
			status.dwCurrentState == SERVICE_CONTINUE_PENDING ||
			status.dwCurrentState == SERVICE_PAUSE_PENDING ||
			status.dwCurrentState == SERVICE_PAUSED){
		
		logToDBAndFile(DECL_INFO, "Service is pending...\n");
		if (!pending) {
			time_pending = get_time_ms();
			pending = true;
		}
		float tp = get_time_passed_ms(time_pending) / 1000.0;
		logToDBAndFile(DECL_ERROR, "Time passed on general pending: %.03f sec(s).\n", tp);
		if (gl.WAIT_BEFORE_KILL_INTERVAL <= tp ) {
			pending = false;
			pending_to_start = false;
			pending_on_stop = false;
			logToDBAndFile(DECL_INFO, "Service is pending for more than %.03f secs.\n", gl.WAIT_BEFORE_KILL_INTERVAL);						
			/* try to kill it */
			BOOL b = ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&status);
			if (b) {
				logToDBAndFile(DECL_ERROR, "Service stopped.\n");
			} else {
				logToDBAndFile(DECL_ERROR, "Service failed to stop.\n");
				
				if (gl.SERVICE_PROCESS_ID <= 0) {
					/* try to revice it */
					gl.SERVICE_PROCESS_ID = status.dwProcessId;
				}
				BOOL f = try_kill_process(gl.SERVICE_PROCESS_ID);
				if (f) logToDBAndFile(DECL_INFO, "Service killed on SERVICE_STOP_PENDING.\n");		
				else logToDBAndFile(DECL_INFO, "Service could not get killed on SERVICE_STOP_PENDING.Error: %d.\n", GetLastError());
			}			
		}
		_sleep(1000);
		/* start service again */
		BOOL b = StartService(schService, NULL, NULL);
		if (b) {
			logToDBAndFile(DECL_INFO, "Service started.\n");			
		} else {
			logToDBAndFile(DECL_ERROR, "Service failed to start on SERVICE_PENDING.\n");	
			if (!pending_to_start) {
				time_pending_to_start = get_time_ms();
				pending_to_start = true;
			}
			float tp = get_time_passed_ms(time_pending_to_start) / 1000.0;
			logToDBAndFile(DECL_ERROR, "Time passed on SERVICE_PENDING: %.03f sec(s).\n", tp);
			if (gl.WAIT_BEFORE_KILL_INTERVAL <= tp) {
				pending = false;
				pending_to_start = false;
				pending_on_stop = false;
				logToDBAndFile(DECL_INFO, "Service is pending to start for more than %.03f secs.\n", gl.WAIT_BEFORE_KILL_INTERVAL);
				if (gl.SERVICE_PROCESS_ID <= 0) {
					/* try to revice it */
					gl.SERVICE_PROCESS_ID = status.dwProcessId;
				}
				BOOL f = try_kill_process(gl.SERVICE_PROCESS_ID);
				if (f) logToDBAndFile(DECL_INFO, "Service killed on SERVICE_PENDING.\n");
				else logToDBAndFile(DECL_INFO, "Service could not get killed on SERVICE_PENDING.Error: %d.\n", GetLastError());
			}
		}
	} 

	CloseServiceHandle(schService);
	CloseServiceHandle(hSc);

}
void main(int argc, char* argv[])
{
	bool debug = 
#ifdef _DEBUG	
	true;
#else
	false;
#endif	
	if (argc > 1 && strcmp(argv[1], "debug") == 0)
		debug = true;

	if (debug)
	{
		float time_delay_update = get_time_ms();
		readPcData();
		for (;;) {		
			run(&time_delay_update);
			_sleep(gl.POLL_INTERVAL * 1000);
		}
	}
	else
	{
		SERVICE_TABLE_ENTRY ServiceTable[2];
		ServiceTable[0].lpServiceName = "ServiceWatchdog";
		ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
		ServiceTable[1].lpServiceName = NULL;
		ServiceTable[1].lpServiceProc = NULL;
		// Start the control dispatcher thread for our service
		StartServiceCtrlDispatcher(ServiceTable);
	}

}