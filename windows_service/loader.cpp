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
#include "loader.h"

#if defined _MSC_VER
/*use 
* extern "C" { 
* on functions loaded by dll
*/

LOADER_DECLARE(void) destroy_dll(dso_lib_t *lib)
{
	if (lib && *lib) 
	{
		FreeLibrary(*lib);
		*lib = NULL;
	}
}

LOADER_DECLARE(dso_lib_t) open_dll(const char *path, int global, char **err)
{
	HINSTANCE lib;

	lib = LoadLibraryEx(path, NULL, 0);

	if (!lib) 
	{
		LoadLibraryEx(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	}

	if (!lib) 
	{
		DWORD error = GetLastError();
		fprintf(stdout, "dll open error [%ul]\n", error);
		sprintf(*err, "dll open error [%ul]\n", error);
	}
	return lib;
}

LOADER_DECLARE(dso_func_t) func_sym_dll(dso_lib_t lib, const char *sym, char **err)
{
	FARPROC func = GetProcAddress(lib, sym);
	if (!func) 
	{
		DWORD error = GetLastError();
		fprintf(stdout, "dll sym error [%ul]\n", error);
		sprintf(*err, "dll sym error [%ul]\n", error);
	}
	return (dso_func_t) func;
}

LOADER_DECLARE(void *) data_sym_dll(dso_lib_t lib, const char *sym, char **err)
{
	FARPROC addr = GetProcAddress(lib, sym);
	if (!addr) 
	{
		DWORD error = GetLastError();
		fprintf(stdout, "dll sym error [%ul]\n", error);
		sprintf(*err, "dll sym error [%ul]\n", error);
	}
	return (void *) (intptr_t) addr;
}

#endif
