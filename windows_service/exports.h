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
#ifndef __EXPORTS_H
#define __EXPORTS_H

#include <stdio.h>

#ifdef _MSC_VER

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <assert.h>
#include <stdint.h>

#if defined(DECLARE_STATIC)
#define DECLARE(type)			type __stdcall
#define DECLARE_NONSTD(type)	type __cdecl
#define DECLARE_DATA
#elif defined(EXPORTS)
#define DECLARE(type)			__declspec(dllexport) type __stdcall
#define DECLARE_NONSTD(type)	__declspec(dllexport) type __cdecl
#define DECLARE_DATA			__declspec(dllexport)
#else
#define DECLARE(type)			__declspec(dllimport) type __stdcall
#define DECLARE_NONSTD(type)	__declspec(dllimport) type __cdecl
#define DECLARE_DATA			__declspec(dllimport)
#endif

#endif

struct ftp_file 
{
  const char *filename;
  FILE *stream;
};


struct locale_struct
{
	char* locale;
};

enum curl_status
{
	CURL_STATUS_SUCCESS = 0,
	CURL_STATUS_FAILURE = -1
};

typedef struct ftp_file *ftp_file_t;
typedef struct locale_struct locale_struct;
typedef enum curl_status curl_status_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif