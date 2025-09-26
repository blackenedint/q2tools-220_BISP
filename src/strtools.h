#ifndef STRTOOLS_H
#define STRTOOLS_H
#pragma once

#include "cmdlib.h"

#ifdef _WIN32
	#define CORRECT_PATH_SEPARATOR '\\'
	#define CORRECT_PATH_SEPARATOR_S "\\"
	#define INCORRECT_PATH_SEPARATOR '/'
	#define INCORRECT_PATH_SEPARATOR_S "/"
	#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')
#else
	#define CORRECT_PATH_SEPARATOR '/'
	#define CORRECT_PATH_SEPARATOR_S "/"
	#define INCORRECT_PATH_SEPARATOR '\\'
	#define INCORRECT_PATH_SEPARATOR_S "\\"
	#define PATHSEPARATOR(c) ((c) == '/')
#endif

void Str_FixSlashes( char* str );
void Str_FixDoubleSlashes( char* str );
void Str_StripFilename( char* str );
void Str_StripExtension( const char* path, char* out, size_t outLen );
void Str_SetExtension(  char* path, size_t len, const char* ext );
qboolean Str_StripLastDir( char* str, size_t len );
qboolean Str_ExtractFilePath( const char* path, char* dest, size_t destlen );
const char* Str_ExtractFilename( const char* path );

#endif