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
#ifndef __LOADER_H
#define __LOADER_H

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdio.h>

#ifdef _MSC_VER

#if defined(LOADER_DECLARE_STATIC)
#define LOADER_DECLARE(type)			type __stdcall
#define LOADER_DECLARE_NONSTD(type)	type __cdecl
#define LOADER_DECLARE_DATA
#elif defined(LOADER_EXPORTS)
#define LOADER_DECLARE(type)			__declspec(dllexport) type __stdcall
#define LOADER_DECLARE_NONSTD(type)	__declspec(dllexport) type __cdecl
#define LOADER_DECLARE_DATA			__declspec(dllexport)
#else
#define LOADER_DECLARE(type)			__declspec(dllimport) type __stdcall
#define LOADER_DECLARE_NONSTD(type)	__declspec(dllimport) type __cdecl
#define LOADER_DECLARE_DATA			__declspec(dllimport)
#endif

#endif



typedef int (*dso_func_t) (void);

#ifdef _MSC_VER
#include <windows.h>
typedef HINSTANCE dso_lib_t;
#else
typedef void *dso_lib_t;
#endif

typedef void *dso_data_t;

LOADER_DECLARE(void) destroy_dll(dso_lib_t *lib);
LOADER_DECLARE(dso_lib_t) open_dll(const char *path, int global, char **err);
LOADER_DECLARE(dso_func_t) func_sym_dll(dso_lib_t lib, const char *sym, char **err);
LOADER_DECLARE(void *) data_sym_dll(dso_lib_t lib, const char *sym, char **err);

#endif

#ifdef __cplusplus
	}
#endif
