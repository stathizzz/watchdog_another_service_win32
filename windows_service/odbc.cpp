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
#ifdef _MSC_VER

#define MAX 2000
extern GLOBALS gl;

void show_error(unsigned int handletype, const SQLHANDLE& handle)
{
	SQLCHAR sqlstate[1024];
	SQLCHAR message[1024];
	if (SQL_SUCCESS == SQLGetDiagRec((SQLSMALLINT)handletype, handle, 1, sqlstate, NULL, message, 1024, NULL))
	{				
		string msg = string((char *)message) + "\n";
		logToFile(DECL_ERROR, (char *)msg.c_str());
	}
}
bool test_connection_ok(SQLHANDLE sqlconnectionhandle) {

	SQLPERF*     pSQLPERF;
	SQLINTEGER   nValue;
	if (SQL_SUCCESS != SQLGetConnectAttr(sqlconnectionhandle, SQL_COPT_SS_PERF_DATA, &pSQLPERF, sizeof(SQLPERF*), &nValue)) {
		show_error(SQL_HANDLE_STMT, sqlconnectionhandle);
		return false;
	}
	return true;
}
int connect(string & connectionString, SQLHANDLE & sqlconnectionhandle, SQLHANDLE & sqlstatementhandle) {
	int status = SQL_SUCCESS;
	SQLHANDLE sqlenvhandle = SQL_NULL_HANDLE;
	SQLRETURN retcode = -1;

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlenvhandle)) {
		status = SQL_ERROR;
		goto end;
	}
	if (SQL_SUCCESS != SQLSetEnvAttr(sqlenvhandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)) {
		status = SQL_ERROR;
		goto end;
	}
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlenvhandle, &sqlconnectionhandle)) {
		status = SQL_ERROR;
		goto end;
	}

	SQLCHAR retconstring[1024];
#ifdef _UNICODE
	wchar_t dest[128] = { 0 };
	mbstowcs(dest, connectionString.c_str(), strlen(connectionString.c_str()));
#else
	const char *dest = connectionString.c_str();
#endif
	switch (SQLDriverConnect(sqlconnectionhandle, NULL, (SQLCHAR*)dest, SQL_NTS, retconstring, 1024, NULL, SQL_DRIVER_NOPROMPT))
	{
	case SQL_SUCCESS_WITH_INFO:
		retcode = SQL_SUCCESS_WITH_INFO;
		break;
	case SQL_INVALID_HANDLE:
	case SQL_ERROR:
		show_error(SQL_HANDLE_DBC, sqlconnectionhandle);
		retcode = -1;
		break;
	default:
		break;
	}

	if (retcode == -1) {
		status = SQL_ERROR;
		goto end;
	}
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle)) {
		status = SQL_ERROR;
		goto end;
	}
end:
	SQLFreeHandle(SQL_HANDLE_ENV, sqlenvhandle);
	return status;
}
void disconnect(SQLHANDLE & sqlconnectionhandle, SQLHANDLE & sqlstatementhandle) {

	SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle);
	SQLDisconnect(sqlconnectionhandle);
	SQLFreeHandle(SQL_HANDLE_DBC, sqlconnectionhandle);
}
int spAppEventsCreate(int instanceId, int *out) {
	bool error = false;
	int status = SQL_SUCCESS;
	SQLHANDLE sqlconnectionhandle = SQL_NULL_HANDLE;
	SQLHANDLE sqlstatementhandle = SQL_NULL_HANDLE;

	char *strProcName = "AppEventsCreate";

	if (gl.CONNECTION_STRING.empty()) {
		logToFile(DECL_ERROR, "Connection string should be set on the configuration file.\n");
		return SQL_ERROR;
	}

	if ((status = connect(gl.CONNECTION_STRING, sqlconnectionhandle, sqlstatementhandle)) != SQL_SUCCESS) {
		show_error(SQL_HANDLE_STMT, sqlstatementhandle);
		status = SQL_ERROR;
		goto end;
	}
	SQLINTEGER val;
	SWORD  RetParam = 1, OutParam = 1;

	status = ListStoredProcedure(sqlconnectionhandle, strProcName);

	if (SQL_SUCCESS != status) {
		status = SQL_ERROR;
		goto end;		
	}
	SQLINTEGER arg1 = instanceId;
	SQLINTEGER arg2[] = {1, 2, 3};
	SQLINTEGER arg3 = 2;
	SQLINTEGER arg4 = 0;
	

	status = SQLBindParameter(sqlstatementhandle, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_BIGINT, 0, 0, (SQLPOINTER)&arg1, 0, NULL);	
	status = SQLBindParameter(sqlstatementhandle, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&arg3, 0, NULL);
	status = SQLBindParameter(sqlstatementhandle, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&arg4, 0, NULL);
	
	for (int i = 0; i < (sizeof(arg2) / sizeof(arg2[0])); i++) {
		status = SQLBindParameter(sqlstatementhandle, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&arg2[i], 0, NULL);
		status = SQLPrepare(sqlstatementhandle, (SQLCHAR*)"{CALL AppEventsCreate (?, ?, ?, ?)}", SQL_NTS);

		if (SQL_SUCCESS != SQLExecute(sqlstatementhandle)) {
			show_error(SQL_HANDLE_STMT, sqlstatementhandle);
			error = true;
			continue;
		}

		/*int ins;
		for (int i = 0; SQLFetch(sqlstatementhandle) == SQL_SUCCESS; i++) {

			SQLGetData(sqlstatementhandle, 1, SQL_C_ULONG, &ins, 0, NULL);
		}
		if (out) *out = ins;*/
		out;
	}
			
end:
	disconnect(sqlconnectionhandle, sqlstatementhandle);
	if (error) return SQL_ERROR;
	return status;
}
int spAppInstancesSetState(int instanceId, int *out) {
	int status = SQL_SUCCESS;
	SQLHANDLE sqlconnectionhandle = SQL_NULL_HANDLE;
	SQLHANDLE sqlstatementhandle = SQL_NULL_HANDLE;

	char  *strProcName = "AppInstancesSetState";

	if (gl.CONNECTION_STRING.empty()) {
		logToFile(DECL_ERROR, "Connection string should be set on the configuration file.\n");
		return SQL_ERROR;
	}

	if ((status = connect(gl.CONNECTION_STRING, sqlconnectionhandle, sqlstatementhandle)) != SQL_SUCCESS) {
		show_error(SQL_HANDLE_STMT, sqlstatementhandle);
		status = SQL_ERROR;
		goto end;
	}
	SQLINTEGER val;
	SWORD  RetParam = 1, OutParam = 1;

	status = ListStoredProcedure(sqlconnectionhandle, strProcName);

	if (SQL_SUCCESS != status) {
		status = SQL_ERROR;
		goto end;		
	}
	SQLINTEGER arg1 = instanceId;
	SQLINTEGER arg2 = 0;

	SQLLEN cbRetParam = SQL_NTS, cbOutParam = SQL_NTS;
	status = SQLBindParameter(sqlstatementhandle, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_BIGINT, 0, 0, (SQLPOINTER)&arg1, 0, NULL);
	status = SQLBindParameter(sqlstatementhandle, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&arg2, 0, NULL);
	status = SQLPrepare(sqlstatementhandle, (SQLCHAR*)"{CALL AppInstancesSetState (?, ?)}", SQL_NTS);

	if (SQL_SUCCESS != SQLExecute(sqlstatementhandle)) {
		show_error(SQL_HANDLE_STMT, sqlstatementhandle);
		status = SQL_ERROR;
		goto end;
	}

	/*int ins;
	for (int i = 0; SQLFetch(sqlstatementhandle) == SQL_SUCCESS; i++) {
		SQLGetData(sqlstatementhandle, 1, SQL_C_ULONG, &ins, 0, NULL);
	}
	if (out) *out = ins;*/
	out;

end:
	disconnect(sqlconnectionhandle, sqlstatementhandle);
	return status;
}
int spAppInstancesSearchCreate(int *instance_id) 
{
	int status = SQL_SUCCESS;
	SQLHANDLE sqlconnectionhandle = SQL_NULL_HANDLE;
	SQLHANDLE sqlstatementhandle = SQL_NULL_HANDLE;

	char    * strProcName = "AppInstancesSearchCreate";

	if (gl.CONNECTION_STRING.empty()) {
		logToFile(DECL_ERROR, "Connection string should be set on the configuration file.\n");
		return SQL_ERROR;
	}

	if ((status = connect(gl.CONNECTION_STRING, sqlconnectionhandle, sqlstatementhandle)) != SQL_SUCCESS) {
		show_error(SQL_HANDLE_STMT, sqlstatementhandle);
		status = SQL_ERROR;
		goto end;
	}
	SQLINTEGER val;
	SWORD  RetParam = 1, OutParam = 1;

	status = ListStoredProcedure(sqlconnectionhandle, strProcName);

	if (SQL_SUCCESS != status) {
		status = SQL_ERROR;
		goto end;		
	}
	SQLCHAR *arg1 = (SQLCHAR *)gl.COMPUTERNAME.c_str();
	SQLCHAR *arg2 = (SQLCHAR *)gl.USERNAME.c_str();

	SQLLEN cbRetParam = SQL_NTS, cbOutParam = SQL_NTS;
	status = SQLBindParameter(sqlstatementhandle, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 200, 0, (SQLPOINTER)arg1, (SQLINTEGER)strlen((char *)arg1), NULL);
	status = SQLBindParameter(sqlstatementhandle, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 200, 0, (SQLPOINTER)arg2, (SQLINTEGER)strlen((char *)arg2), NULL);
	status = SQLPrepare(sqlstatementhandle, (SQLCHAR*)"{CALL AppInstancesSearchCreate (?, ?)}", SQL_NTS);
		
	if (SQL_SUCCESS != SQLExecute(sqlstatementhandle)) {
		show_error(SQL_HANDLE_STMT, sqlstatementhandle);
		status = SQL_ERROR;
		goto end;
	}
	
	int ins;

	for (int i = 0; SQLFetch(sqlstatementhandle) == SQL_SUCCESS; i++) {

		SQLGetData(sqlstatementhandle, 1, SQL_C_ULONG, &ins, 0, NULL);
	}
	
	if (instance_id) *instance_id = ins;
end:
	disconnect(sqlconnectionhandle, sqlstatementhandle);
	return status;
}
int spLogNewCreate(int instance_id, SEVERITY severity, char *msg)
{
	int status = SQL_SUCCESS;
	SQLHANDLE sqlconnectionhandle = SQL_NULL_HANDLE;
	SQLHANDLE sqlstatementhandle = SQL_NULL_HANDLE;

	char *strProcName = "LogNewCreate";

	if (gl.CONNECTION_STRING.empty()) {
		logToFile(DECL_ERROR, "Connection string should be set on the configuration file.\n");
		return SQL_ERROR;
	}

	if ((status = connect(gl.CONNECTION_STRING, sqlconnectionhandle, sqlstatementhandle)) != SQL_SUCCESS) {
		show_error(SQL_HANDLE_STMT, sqlstatementhandle);
		status = SQL_ERROR;
		goto end;
	}
	SQLINTEGER val;
	SWORD  RetParam = 1, OutParam = 1;

	status = ListStoredProcedure(sqlconnectionhandle, strProcName);

	if (SQL_SUCCESS != status) {
		status = SQL_ERROR;
		goto end;
	}
		
	status = SQLBindParameter(sqlstatementhandle, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_BIGINT, 0, 0, (SQLPOINTER)&instance_id, 0, NULL);
	status = SQLBindParameter(sqlstatementhandle, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&severity, 0, NULL);
	status = SQLBindParameter(sqlstatementhandle, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 100, 0, (SQLPOINTER)msg, (SQLINTEGER)strlen((char *)msg), NULL);
	status = SQLPrepare(sqlstatementhandle, (SQLCHAR*)"{CALL LogNewCreate (?, ?, ?)}", SQL_NTS);

	if (SQL_SUCCESS != SQLExecute(sqlstatementhandle)) {
		show_error(SQL_HANDLE_STMT, sqlstatementhandle);
		status = SQL_ERROR;
		goto end;
	}

	/*int ins;
	for (int i = 0; SQLFetch(sqlstatementhandle) == SQL_SUCCESS; i++) {

		SQLGetData(sqlstatementhandle, 1, SQL_C_ULONG, &ins, 0, NULL);
	}*/
	
end:
	disconnect(sqlconnectionhandle, sqlstatementhandle);
	return status;
}
SQLRETURN CreateStoredProcedure(SQLHDBC hdbc, char *ProcBody) {

	SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
	SQLRETURN retcode = SQL_SUCCESS;	// Return status

	SQLCHAR strCreateSP[1024];
			
	strcpy((char *)strCreateSP, ProcBody);
	
	// Allocate a statement handle
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	
	// Execute Create Procedure
	retcode = SQLExecDirect(hstmt, strCreateSP, SQL_NTS);
	
	// Free statement handle
	if (hstmt != SQL_NULL_HSTMT)
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

	return retcode;
}
SQLRETURN DropStoredProcedure(SQLHDBC  hdbc, char *ProcName) {

	SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
	SQLRETURN retcode = SQL_SUCCESS;	// Return status

	SQLCHAR   strDropSP[1024];

	sprintf((char *)strDropSP, "IF EXISTS (SELECT * FROM sys.objects WHERE "
		"type='P' AND name='%s') DROP PROCEDURE %s", ProcName, ProcName);

	// Allocate a statement handle
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	
	// Execute Drop Procedure
	retcode = SQLExecDirect(hstmt, strDropSP, SQL_NTS);
	
	// Free statement handle
	if (hstmt != SQL_NULL_HSTMT)
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

	return retcode;
}
SQLRETURN ListStoredProcedure(SQLHDBC hdbc, char *ProcName) {
#define BUFF_SIZE 255
	// Columns for binding to SQLProcedures() results set
	SQLCHAR strProcedureCat[BUFF_SIZE];
	SQLCHAR strProcedureSchema[BUFF_SIZE];
	SQLCHAR strProcedureName[BUFF_SIZE];
	SQLSMALLINT ProcedureType;

	SQLLEN  lenProcedureCat, lenProcedureSchema;
	SQLLEN  lenProcedureName, lenProcedureType;

	int header = 0;
	SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
	SQLRETURN retcode = SQL_SUCCESS;	// Return status

	// Allocate a statement handle
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	
	retcode = SQLProcedures(hstmt,
		NULL, 0,
		NULL, 0,
		(SQLCHAR *)ProcName, strlen(ProcName));

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		// Bind columns in result set to buffers
		// column 1 is the catalogue
		SQLBindCol(hstmt, 1, SQL_C_CHAR, strProcedureCat, sizeof(strProcedureCat), &lenProcedureCat);
		// column 2 is the schema
		SQLBindCol(hstmt, 2, SQL_C_CHAR, strProcedureSchema, sizeof(strProcedureSchema), &lenProcedureSchema);
		// column 3 is the procedure name
		SQLBindCol(hstmt, 3, SQL_C_CHAR, strProcedureName, sizeof(strProcedureName), &lenProcedureName);
		// columns 4 to 7 are skipped
		// column 8 is the procedure type
		SQLBindCol(hstmt, 8, SQL_C_SHORT, &ProcedureType,
			sizeof(ProcedureType), &lenProcedureType);

		// fetch results (only expecting one row)
		while (SQL_SUCCESS == retcode) {
			retcode = SQLFetch(hstmt);
#ifdef _DEBUG
			if (header++ == 0) {
				logToFile(DECL_INFO, "\nDSN : SQLSRV");
			}

			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				logToFile(DECL_INFO, "\nProcedure Cat    : %s\n", strProcedureCat);
				logToFile(DECL_INFO, "Procedure Schema : %s\n", strProcedureSchema);
				logToFile(DECL_INFO, "Procedure Name   : %s\n", strProcedureName);
				logToFile(DECL_INFO, "Procedure Type   : ");
				switch (ProcedureType) {
				case SQL_PT_PROCEDURE:
					logToFile(DECL_INFO, "%s\n", "SQL_PT_PROCEDURE");
					break;
				case SQL_PT_FUNCTION:
					logToFile(DECL_INFO, "%s\n", "SQL_PT_FUNCTION");
					break;
				case SQL_PT_UNKNOWN:
				default:
					logToFile(DECL_INFO, "%s\n", "SQL_PT_UNKNOWN");
					break;
				}
			}

			if (retcode == SQL_NO_DATA && header == 1) {
				logToFile(DECL_INFO, "(NO DATA)\n");
			}
#endif
		}
	}

exit:
	// Free statement handle
	if (hstmt != SQL_NULL_HSTMT)
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

	// if last status is no data it is ok
	if (retcode == SQL_NO_DATA) {
		retcode = SQL_SUCCESS;
	}

	return retcode;
}
#endif
