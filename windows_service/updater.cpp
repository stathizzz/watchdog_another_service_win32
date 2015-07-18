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
#include "dirent.h"

#ifdef ZLIB_USED
#include "zlib.h"
unsigned long file_size(char *filename)
{
	FILE *pFile = fopen(filename, "rb");
	fseek(pFile, 0, SEEK_END);
	unsigned long size = ftell(pFile);
	fclose(pFile);
	return size;
}

int decompress_one_file(const char *infilename, const char *outfilename)
{
	gzFile infile = gzopen(infilename, "rb");
	FILE *outfile = fopen(outfilename, "wb");
	if (!infile || !outfile) return -1;

	char buffer[128];
	int num_read = 0;
	while ((num_read = gzread(infile, buffer, sizeof(buffer))) > 0) {
		fwrite(buffer, 1, num_read, outfile);
	}

	gzclose(infile);
	fclose(outfile);
}

int compress_one_file(char *infilename, char *outfilename)
{
	FILE *infile = fopen(infilename, "rb");
	gzFile outfile = gzopen(outfilename, "wb");
	if (!infile || !outfile) return -1;

	char inbuffer[128];
	int num_read = 0;
	unsigned long total_read = 0, total_wrote = 0;
	while ((num_read = fread(inbuffer, 1, sizeof(inbuffer), infile)) > 0) {
		total_read += num_read;
		gzwrite(outfile, inbuffer, num_read);

	}
	fclose(infile);
	gzclose(outfile);

	logToFile(DECL_INFO, "Read %ld bytes, Wrote %ld bytes,Compression factor %4.2f%%\n",
		total_read, file_size(outfilename),
		(1.0 - file_size(outfilename)*1.0 / total_read)*100.0);
}
#endif

extern GLOBALS gl;

bool find_specified_file_within_folder(string folder, string filename)
{
	char search_path[200];
	if (!folder.empty() && (folder.back() != '\\' && folder.back() != '/'))
		folder += '\\';
		
	sprintf(search_path, "%s*", folder.c_str());
	WIN32_FIND_DATA fd;			
	HANDLE hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				if (caseInsensitiveStringCompare(string(fd.cFileName), filename) == true) {
					::FindClose(hFind);
					return true;
				}
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return false;
}
vector<string> get_all_jars_within_folder(string folder)
{
	vector<string> names;
	char search_path[200];
	if (!folder.empty() && (folder.back() != '\\' && folder.back() != '/'))
		folder += '\\';
	sprintf(search_path, "%s*.jar", folder.c_str());
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}

	return names;
}
vector<string> get_all_files_within_folder(string folder)
{
	vector<string> names;
	char search_path[200];
	if (!folder.empty() && (folder.back() != '\\' && folder.back() != '/'))
		folder += '\\';
	sprintf(search_path, "%s*.*", folder.c_str());
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}

	return names;
}
char *search_buffer(char *haystack, size_t haystacklen, char *needle, size_t needlelen)
{   /* warning: O(n^2) */
	int searchlen = haystacklen - needlelen + 1;
	for (; searchlen-- > 0; haystack++)
	if (!memcmp(haystack, needle, needlelen))
		return haystack;
	return NULL;
}
bool checkIfUpdateRequired(const char *oldfile, const char *newfile)
{
	WIN32_FILE_ATTRIBUTE_DATA  oldwfad, newwfad;
	BOOL a, b;
	a = GetFileAttributesEx(oldfile, GetFileExInfoStandard, &oldwfad);
	if (!a) throw std::runtime_error("couldn't read old file attributes");
	b = GetFileAttributesEx(newfile, GetFileExInfoStandard, &newwfad);
	if (!b) throw std::runtime_error("couldn't read new file attributes");
	
	if (CompareFileTime(&oldwfad.ftLastWriteTime, &newwfad.ftLastWriteTime)) {
		return true;
	}
	
	return false;
}
int updateDirsAndFiles(string & oldfolder, string & newfolder) {

	int result;
	SHFILEOPSTRUCTA sf;	
	sf.wFunc = FO_COPY;
	sf.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_SILENT;
	
	DIR *dp = opendir(newfolder.c_str());
	struct dirent *dirp = NULL;
	struct stat filestat;

	logToDBAndFile(DECL_INFO, "Updating dirs...\n");

	if (dp == NULL) {
		logToDBAndFile(DECL_ERROR, "Updater error: recreating the new directory structure failed: Error %d.\n", GetLastError());
		return -1;
	}

	while ((dirp = readdir(dp))) {
		
		if (!strncmp(dirp->d_name, "..", 2)) {
			continue;
		} else if (!strncmp(dirp->d_name, ".", 1)) {			
			continue;
		}

		string path = newfolder + dirp->d_name;
		path.append(1, '\0');

		if (stat(path.c_str(), &filestat)) {
			continue;
		}
		if (S_ISREG(filestat.st_mode))  {
			sf.fFlags = FOF_FILESONLY | FOF_NO_UI;
			sf.pFrom = path.c_str();
			string l = string(oldfolder + dirp->d_name);
			l.append(1, '\0');
			sf.pTo = l.c_str();
			logToDBAndFile(DECL_INFO, "Copying from file %s to file %s...\n", path.c_str(), l.c_str());
			if (SHFileOperationA(&sf)) {
				logToDBAndFile(DECL_ERROR, "Updater error: couldn't recreate file from %s to %s. Error %d.\n", path.c_str(), l.c_str(), GetLastError());
				closedir(dp);
				return -1;
			}
			logToDBAndFile(DECL_INFO, "Copying from file %s to file %s completed successfully.\n", path.c_str(), l.c_str());
			continue;
		} else if (S_ISDIR(filestat.st_mode))  {
			sf.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_SILENT;
			sf.pFrom = path.c_str();
			string l = string(oldfolder);
			l.append(1, '\0');			
			sf.pTo = l.c_str();
			logToDBAndFile(DECL_INFO, "Copying from directory %s to directory %s...\n", path.c_str(), l.c_str());
			if (SHFileOperationA(&sf)) {
				logToDBAndFile(DECL_ERROR, "Updater error: recreating the new directory structure failed. Error %d.\n", GetLastError());
				closedir(dp);
				return -1;
			}
			logToDBAndFile(DECL_INFO, "Copying from directory %s to directory %s completed successfully.\n", path.c_str(), l.c_str());
			continue;
		}		
	}

	closedir(dp);
	logToDBAndFile(DECL_INFO, "Updating dirs completed.\n");
	return 0;
}
void putforth(string & oldfolder) {
	int copy_status;
	copy_status = CopyFile((oldfolder + gl.JAR_TO_CHECK).c_str(), (oldfolder + gl.JAR_TO_CHECK + ".temp").c_str(), 0);
	if (!copy_status) {
		logToDBAndFile(DECL_ERROR, "Updater error: copy file %s to %s failed. Error %d.\n", (oldfolder + gl.JAR_TO_CHECK).c_str(), (oldfolder + gl.JAR_TO_CHECK + ".temp").c_str(), GetLastError());
	}
}
void rollback(string & oldfolder) {
	int copy_status;
	copy_status = CopyFile((oldfolder + gl.JAR_TO_CHECK + ".temp").c_str(), (oldfolder + gl.JAR_TO_CHECK).c_str(), 0);
	if (!copy_status) {
		logToDBAndFile(DECL_ERROR, "Updater error: copy file %s to %s location failed. Error %d.\n", (oldfolder + gl.JAR_TO_CHECK + ".temp").c_str(), (oldfolder + gl.JAR_TO_CHECK).c_str(), GetLastError());
	}
	DeleteFile((oldfolder + gl.JAR_TO_CHECK + ".temp").c_str());
}
void deleteTempCheckerFile(string & oldfolder) {
	DeleteFile((oldfolder + gl.JAR_TO_CHECK + ".temp").c_str());
}
int updateRootFiles(string & oldfolder, string & newfolder) {
	int delete_status, copy_status;
	WIN32_FIND_DATA fd;
	HANDLE hFind;

	logToDBAndFile(DECL_INFO, "Updating root files...\n");
	
	char search_path[200];	
	sprintf(search_path, "%s*", newfolder.c_str());
	
	putforth(oldfolder);
	hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {			
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {								
				string tmp = string(fd.cFileName);
				copy_status = CopyFile((newfolder + tmp).c_str(), (oldfolder + tmp).c_str(), 0);
				if (!copy_status) {					
					logToDBAndFile(DECL_ERROR, "Updater error: copy file %s to %s failed:Error %d\n", (newfolder + tmp).c_str(), (oldfolder + tmp).c_str(),  GetLastError());
					rollback(oldfolder);
					FindClose(hFind);
					return -1;
				}								
			}
		} while (::FindNextFile(hFind, &fd));
		FindClose(hFind);
	}

	deleteTempCheckerFile(oldfolder);
	logToDBAndFile(DECL_INFO, "Updating root files completed.\n");
	return 0;
}
void updateFiles(string & oldfolder, string & newfolder) {

	int copy_status;
	putforth(oldfolder);
	if (updateDirsAndFiles(oldfolder, newfolder) == -1) {
		logToDBAndFile(DECL_ERROR, "Updater error: unable to recreate new dir structure.\n");
		rollback(oldfolder);
		return;
	}
	deleteTempCheckerFile(oldfolder);
	
	/*if (updateRootFiles(oldfolder, newfolder) == -1) {
		logToDBAndFile(DECL_ERROR, "Updater error: unable to update files on root folder.\n");
		return;
	}*/
}
void UpdaterLogicCheckAllJarsAndUpdate(SC_HANDLE schService) {
		
	vector<string> oldjars = get_all_jars_within_folder(gl.PATH_TO_OLD_JARS);
	vector<string> newjars = get_all_jars_within_folder(gl.PATH_TO_NEW_JARS);
	string oldfolder = gl.PATH_TO_OLD_JARS;
	if (!oldfolder.empty() && (oldfolder.back() != '/' && oldfolder.back() != '\\'))
		oldfolder += '\\';
	string newfolder = gl.PATH_TO_NEW_JARS;
	if (!newfolder.empty() && (newfolder.back() != '/' && newfolder.back() != '\\'))
		newfolder += '\\';
	bool ch = false;
	for (string oldjar : oldjars) {
		bool pp = false;
		for (string newjar : newjars) {
			if (newjar == oldjar) {
				ch = checkIfUpdateRequired((oldfolder + oldjar).c_str(), (newfolder + newjar).c_str());
				if (ch) {
					pp = true;
					break;
				}
			}
		}
		if (pp) break;
	}
	
	if (ch) {		
		SERVICE_STATUS_PROCESS status;
		/* Stopping service */
		BOOL b = ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&status);
		if (b) {
			logToDBAndFile(DECL_INFO, "Service stopped.\n");
		} else {
			logToDBAndFile(DECL_ERROR, "Service failed to stop.\n");
			throw std::runtime_error("Service failed to stop.\n");
		}
		
		updateFiles(oldfolder, newfolder);

		/* Restarting service */
		b = StartService(schService, NULL, NULL);
		if (b) {
			logToDBAndFile(DECL_INFO, "Service started.\n");
		} else {
			logToDBAndFile(DECL_ERROR, "Service failed to start on update 2.\n");
			throw std::runtime_error("Service failed to start on update 2.\n");
		}
	}
}
void UpdaterLogic(SC_HANDLE schService, bool hasService) {

	logToDBAndFile(DECL_INFO, (char *)"Checking for new version.\n");
	bool found = find_specified_file_within_folder(gl.PATH_TO_OLD_JARS, gl.JAR_TO_CHECK);
	if (!found) {
		throw std::runtime_error("Required jar file for update not found on destination.\n");
	}
	found = find_specified_file_within_folder(gl.PATH_TO_NEW_JARS, gl.JAR_TO_CHECK);
	if (!found) {
		throw std::runtime_error("Required jar file for update not found on update folder.\n");
	}
	string oldfolder = gl.PATH_TO_OLD_JARS;
	if (!oldfolder.empty() && (oldfolder.back() != '/' && oldfolder.back() != '\\'))
		oldfolder += '\\';
	string newfolder = gl.PATH_TO_NEW_JARS;
	if (!newfolder.empty() && (newfolder.back() != '/' && newfolder.back() != '\\'))
		newfolder += '\\';
	bool ch = checkIfUpdateRequired((oldfolder + gl.JAR_TO_CHECK).c_str(), (newfolder + gl.JAR_TO_CHECK).c_str());
	if (!ch) {
		/* No update required*/
		throw std::runtime_error("No new version found.\n");
	}
	
	logToDBAndFile(DECL_INFO, (char *)"New version found.\n");
	
	if (hasService) {		
		SERVICE_STATUS_PROCESS status;
		/* Stopping service */
		BOOL b = ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&status);
		if (b) {
			logToDBAndFile(DECL_INFO, "Service stopped so as to update.\n");
		} else {
			throw std::runtime_error("Service failed to stop before update.\n");
		}
	}
	/* Updating program jars */
	updateFiles(oldfolder, newfolder);

	logToDBAndFile(DECL_INFO, "Done update.\n");	
	/* Restarting service */
	if (hasService) {
		BOOL b = StartService(schService, NULL, NULL);
		if (b) {
			logToDBAndFile(DECL_INFO, "Service started after update.\n");
		} else {
			throw std::runtime_error("Service failed to start after update.");
		}
	}
}
void tryUpdateNoService(volatile BOOL *first_time, float *TIME_WAIT_UPDATE, SC_HANDLE *schService, bool overrideAll = true)
{
	if (overrideAll) {
		try {
			UpdaterLogic(*schService, false);
		} catch (std::runtime_error ex) {
			logToDBAndFile(DECL_WARNING, (char *)ex.what());
		}
	} else {
		float wbubs = get_time_passed_ms(*TIME_WAIT_UPDATE) / 1000.0;
		if (*first_time || gl.WAIT_BEFORE_UPDATE <= wbubs) {
			*first_time = FALSE;
			*TIME_WAIT_UPDATE = get_time_ms();
			try {
				UpdaterLogic(*schService, false);
			} catch (std::runtime_error ex) {
				logToDBAndFile(DECL_WARNING, (char *)ex.what());
			}
		}
	}
}
void tryUpdateWithService(volatile BOOL *first_time, float *TIME_WAIT_UPDATE, SC_HANDLE *schService) 
{
	float wbu = get_time_passed_ms(*TIME_WAIT_UPDATE) / 1000.0;
	if (*first_time || gl.WAIT_BEFORE_UPDATE <= wbu) {
		*first_time = FALSE;
		*TIME_WAIT_UPDATE = get_time_ms();
		try {
			UpdaterLogic(*schService, true);
		} catch (std::runtime_error ex) {
			logToDBAndFile(DECL_WARNING, (char *)ex.what());
		}
	}
}