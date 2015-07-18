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
/* Currently odbc is implemented for wiidows only machines */
#ifdef _MSC_VER

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <tuple>
#include <numeric>
#include <functional> 
#include <cctype>
#include <locale>
#include <Lmcons.h>

#ifdef _MSC_VER
#include <windows.h>
#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcss.h>
#endif


#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <conio.h>
#include <vector>
#include <chrono>

using namespace std;

typedef enum SEVERITY {
	DECL_DEBUG = 10,
	DECL_INFO = 20,
	DECL_WARNING = 30,
	DECL_ERROR = 40
} SEVERITY;

//GLOBAL VARIABLES
typedef struct GLOBALS {
	string CONNECTION_STRING;
	string COMPUTERNAME;
	string USERNAME;
	string LOGFILE;
	int POLL_INTERVAL;
	float WAIT_BEFORE_KILL_INTERVAL;
	float WAIT_BEFORE_UPDATE;
	int SERVICE_PROCESS_ID;
	string SERVICE_NAME;
	string PATH_TO_OLD_JARS;
	string PATH_TO_NEW_JARS;
	string JAR_TO_CHECK;
} GLOBALS;

typedef struct cardsseenonshoes_db {
	int id;
	int table_id;
	char time_started[64], time_finished[64];
	int card;
	char occurance[64];
} cardsseenonshoes_db;

typedef struct cardsseen_db {
	int id, shoe_id, card, bjvalue;
	char occurance[64];
} cardsseen_db;

typedef struct shoes_db {
	int id, playing_session_id, table_id;
	char time_started[64], time_finished[64];
} shoes_db;

typedef struct shoeswithcards {
	int id;
} shoeswithcards;

int connect(string & connectionString, SQLHANDLE & sqlconnectionhandle, SQLHANDLE & sqlstatementhandle);
void disconnect(SQLHANDLE & sqlconnectionhandle, SQLHANDLE & sqlstatementhandle);

bool test_connection_ok(SQLHANDLE sqlconnectionhandle);
#ifdef __cplusplus
extern "C" {
#endif
	void saveCardsSeenOnShoes(const SQLHANDLE & sqlstatementhandle, void * (__stdcall *callback)(const shoeswithcards &, const shoeswithcards &));
	int connnectToDbManipulateCardsSeen(void *(__stdcall *callback)(const shoeswithcards &, const shoeswithcards &, const string &, const string &));
	SQLRETURN CreateStoredProcedure(SQLHDBC hdbc, char *ProcBody);
	SQLRETURN DropStoredProcedure(SQLHDBC  hdbc, char *ProcName);
	SQLRETURN ListStoredProcedure(SQLHDBC hdbc, char *ProcName);
	int spAppInstancesSearchCreate(int *instance_id);	
	int spAppInstancesSetState(int instanceId, int *out);
	int spAppEventsCreate(int instanceId, int *out);
	int spLogNewCreate(int instance_id, SEVERITY severity, char *msg);

	void logToDBAndFile(SEVERITY sev, char *str, ...);
	void logToFile(SEVERITY sev, char *str, ...);
	string ExePath();
	float get_time_passed_ms(float from_time);
	float get_time_ms();
#ifdef __cplusplus
}
#endif
bool caseInsensitiveStringCompare(const string& str1, const string& str2);
void readConfigToGlobals();

#endif
