#include "strtools.h"
#include "mathlib.h"

void Str_FixSlashes( char* str ) {
	while( *str ) {
		if ( *str == INCORRECT_PATH_SEPARATOR || *str == CORRECT_PATH_SEPARATOR )
			*str = CORRECT_PATH_SEPARATOR;
		str++;
	}	
}
void Str_FixDoubleSlashes( char* str ) {
	size_t len = strlen(str);
	for ( int i = 1; i < len - 1; i++ ) {
		if ( PATHSEPARATOR(str[i]) && PATHSEPARATOR(str[i+1] ))	{
			memmove( &str[i], &str[i+1], len - i );
			len--;
		}
	}
}

// walk backwards
void Str_StripFilename( char* path ) {
	size_t len = strlen(path) - 1;
	if ( len <= 0 ) return;
	while( len > 0 && !PATHSEPARATOR(path[len]))
		len--;
	path[len] = 0;
}

void Str_StripExtension( const char* path, char* out, size_t outLen ) {
	size_t end = strlen(path) - 1;
	while( end > 0 && path[end] != '.' && !PATHSEPARATOR((char)path[end])) 
		end--;
	
	if ( end > 0 && !PATHSEPARATOR(path[end] ) && end < outLen ) {
		size_t chars = MIN(end, outLen-1 );
		if ( out != path )
			memcpy( out, path, chars );
		out[chars] = '\0';
	} else {
		if ( out != path )
			strncpy( out, path, outLen );
	}
}

void Str_SetExtension( char* path, size_t len, const char* ext ) {
	Str_StripExtension( path, path, len );
	char temp[_MAX_PATH];
	if ( ext[0] != '.' ) {
		temp[0] = '.';
		strncpy(&temp[1], ext, sizeof(temp)-1);
		ext = temp;
	}
	strncat( path, ext, len-1);
}
qboolean Str_StripLastDir( char* path, size_t maxLen ) {
	if ( path[0] == 0 || !stricmp( path, "./") || !stricmp( path, ".\\"))
		return false;
	size_t pathLen = strlen(path);
	
	if ( PATHSEPARATOR(path[pathLen-1]))
		pathLen--;

	qboolean colon = false;
	while( pathLen > 0 ) {
		if ( PATHSEPARATOR( path[pathLen-1] ) ) {
			path[pathLen] = 0;
			Str_FixSlashes(path);
			return true;
		}
		else if (path[pathLen-1] == ':' ) 
			colon = true;
		pathLen--;
	}

	if ( colon ) { 
		path[0] = '\0'; 
		return false;
	}

	if ( pathLen == 0 ) {
		snprintf( path, maxLen, ".%c", CORRECT_PATH_SEPARATOR );
		return true;
	}

	return true;
}
qboolean Str_ExtractFilePath( const char* path, char* dest, size_t destlen ) {
	if ( destlen < 1 )
		return false;
	size_t pathLen = strlen(path);
	const char* pstr = path + (pathLen ? pathLen-1 : 0);
	while(pstr != path && !PATHSEPARATOR(*(pstr-1)))
		pstr--;
	
	size_t copysize = MIN( pstr - path, destlen - 1 );
	memcpy( dest, path, copysize );
	dest[copysize] = '\0';
	return copysize != 0;
}

const char* Str_ExtractFilename( const char* path ) {
	if ( !path || !path[0] )
		return path;

	const char* filename = path + strlen(path) - 1;
	while (( filename > path ) && !PATHSEPARATOR(*(filename-1) ) ) {
		filename--;
	}
	return filename;
}