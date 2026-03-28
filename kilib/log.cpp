#include "stdafx.h"
#include "log.h"
#include "app.h"
#include "path.h"
#include "kstring.h"
using namespace ki;



//=========================================================================

void Logger::WriteLine( const TCHAR* str )
{
	int siz = my_lstrlen(str)*sizeof(TCHAR);
	WriteLine( str, siz );
}

void __cdecl Logger::WriteLineFmtErr(const TCHAR *fmt, ...)
{
#ifdef DO_LOGGING
	va_list arglist;
	TCHAR str[512];
	TCHAR lerrorstr[16];

	DWORD lerr = GetLastError();

	va_start( arglist, fmt );
	wvsprintf( str, fmt, arglist );
	va_end( arglist );

	wsprintf(lerrorstr, TEXT(" (LERR=%u)"), (UINT)lerr);
	lstrcat(str, lerrorstr);
	WriteLine( str, my_lstrlen(str)*sizeof(TCHAR) );

	SetLastError(0);
#endif
}

void Logger::WriteLine( const TCHAR* str, int siz )
{
#ifdef DO_LOGGING
	// It may be used for debugging the File class itself, so
	// File class cannot be used. API direct tap
	static bool st_firsttime = true;

	// file name
	TCHAR fname[MAX_PATH];
	DWORD dummy = Path::GetExeName( fname );
	if( dummy > 3 )
		my_lstrcpy( fname+dummy-3, TEXT("log") );

	// Open file for writing only
	HANDLE h = ::CreateFile( fname,
		FILE_APPEND_DATA
		, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( h == INVALID_HANDLE_VALUE )
		return;

	// First-time limited processing
	if( st_firsttime )
	{
		::SetEndOfFile( h );
		#ifdef _UNICODE
			::WriteFile( h, "\xff\xfe", 2, &dummy, NULL );
		#endif
		st_firsttime = false;
	}
	else
	{
		::SetFilePointer( h, 0, NULL, FILE_END );
	}

	// write
	::WriteFile( h, str, siz, &dummy, NULL );
	::WriteFile( h, TEXT("\r\n"), sizeof(TEXT("\r")), &dummy, NULL );

	// close
	::CloseHandle( h );

#endif
}
