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

GLOBALS gl;
int instance_id = -1;

string ExePath() {
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	return string(buffer).substr(0, pos);
}

bool caseInsensitiveStringCompare(const string& str1, const string& str2) {
	if (str1.size() != str2.size()) {
		return false;
	}
	for (string::const_iterator c1 = str1.begin(), c2 = str2.begin(); c1 != str1.end(); ++c1, ++c2) {
		if (tolower(*c1) != tolower(*c2)) {
			return false;
		}
	}
	return true;
}

void ltrim(std::string &n) {	
	n.erase(n.begin(), std::find_if(n.begin(), n.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));	
}

void readConfigToGlobals() {
		
	std::ifstream ss;
	ss.open(ExePath() + "/configuration.conf", std::ios::in);
	if (ss)
	{
		string word;
		string exe_path = ExePath();
		while (ss >> word)
		{				
			if (word == "LOGFILE:") {
				string logfile, timestr;
				std::getline(ss, logfile);
				time_t rawtime;
				struct tm * timeinfo;
				char buffer[50];
				memset(buffer, 0, sizeof(buffer));
				rawtime = time(NULL);
				timeinfo = localtime(&rawtime);
				strftime(buffer, sizeof(buffer), "%Y_%m_%d", timeinfo);
				timestr = string(buffer);

				if (logfile.empty())  {
					gl.LOGFILE = "watchdog_event_log_" + timestr + ".txt";
					continue;
				}
				ltrim(logfile);
				string extension, filename;
				try {
					extension = logfile.substr(logfile.find_last_of("."));
					filename = logfile.substr(0, logfile.find_last_of("."));
				} catch (out_of_range ofr) {
					extension = "log";
					filename = logfile;
				}														
				gl.LOGFILE = exe_path + "\\" + filename + "_" + timestr + extension;				
			}
			if (word == "CONNECTION_STRING:") {
				std::getline(ss, gl.CONNECTION_STRING);		
				ltrim(gl.CONNECTION_STRING);
			}			
			if (word == "POLL_INTERVAL:") {
				ss >> gl.POLL_INTERVAL;				
			}
			if (word == "WAIT_BEFORE_KILL_INTERVAL:") {
				ss >> gl.WAIT_BEFORE_KILL_INTERVAL;
			}	
			if (word == "WAIT_BEFORE_UPDATE:") {
				ss >> gl.WAIT_BEFORE_UPDATE;
			}			
			if (word == "SERVICE_NAME:") {
				std::getline(ss, gl.SERVICE_NAME);
				ltrim(gl.SERVICE_NAME);
			}
			if (word == "PATH_TO_OLD_JARS:") {
				std::getline(ss, gl.PATH_TO_OLD_JARS);
				ltrim(gl.PATH_TO_OLD_JARS);
				string prefix = "\\\\";
				if (gl.PATH_TO_OLD_JARS.compare(0, prefix.size(), prefix)) {
					/* relative path to absolute */
					gl.PATH_TO_OLD_JARS = exe_path + "\\" + gl.PATH_TO_OLD_JARS;
				}				
			}
			if (word == "PATH_TO_NEW_JARS:") {
				std::getline(ss, gl.PATH_TO_NEW_JARS);
				ltrim(gl.PATH_TO_NEW_JARS);
				string prefix = "\\\\";
				if (gl.PATH_TO_NEW_JARS.compare(0, prefix.size(), prefix)) {
					/* relative path to absolute*/
					gl.PATH_TO_NEW_JARS = exe_path + "\\" + gl.PATH_TO_NEW_JARS;
				}				
			}
			if (word == "JAR_TO_CHECK:") {
				std::getline(ss, gl.JAR_TO_CHECK);
				ltrim(gl.JAR_TO_CHECK);
			}
	
		}
		//ss.close();
	}
}

void logToFile(SEVERITY severity, char *str, ...) {	
	
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[512];
	memset(buffer, 0, 512);
	rawtime = time(NULL);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 512, "%d-%m-%Y %I:%M:%S  ", timeinfo);

	strncat(buffer, str, strlen(str));
	const char * pa = gl.LOGFILE.c_str();

	va_list arglist;
	va_start(arglist, str);
	FILE * fp = fopen(pa, "a");
	vfprintf(fp, buffer, arglist);
	va_end(arglist);
	fclose(fp);
}

void logToDBAndFile(SEVERITY severity, char *str, ...) {
	int status = SQL_ERROR;
	
	if (instance_id == -1)
		status = spAppInstancesSearchCreate(&instance_id);

	if (status == SQL_SUCCESS) {
		va_list arglist;
		va_start(arglist, str);
		char dstBuf[1000];
		vsnprintf(dstBuf, 1000, str, arglist);
		status = spLogNewCreate(instance_id, severity, str);
	} 
	
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	rawtime = time(NULL);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%d-%m-%Y %I:%M:%S  ", timeinfo);

	strncat(buffer, str, strlen(str));
	const char * pa = gl.LOGFILE.c_str();

	va_list arglist;
	va_start(arglist, str);
	FILE * fp = fopen(pa, "a");
	vfprintf(fp, buffer, arglist);
	va_end(arglist);
	fclose(fp);
}

string getWindowsUsername() {
	char username[UNLEN + 1];
	DWORD username_len = UNLEN + 1;
	GetUserName(username, &username_len);

	return string(username, username_len);
}

string getComputerName()
{
	char buffer[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerName(buffer, &len))
		return std::string(buffer, len);
	return "UNKNOWN";
}

void readPcData() 
{
	gl.COMPUTERNAME = getComputerName();
	gl.USERNAME = getWindowsUsername();
}

float get_time_ms()
{
	return (float)GetTickCount();
}
float get_time_passed_ms(float from_time)
{
	return ((float)GetTickCount() - from_time);
}

