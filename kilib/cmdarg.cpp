#include "stdafx.h"
#include "cmdarg.h"
#include "kstring.h"
using namespace ki;



//=========================================================================

Argv::Argv( const TCHAR* cmd )
: argNum_ ( 0 )
{
	TCHAR *p, endc;

	buf_ = p = (TCHAR*)malloc( sizeof(TCHAR) * (my_lstrlen(cmd)+1) );
	if( !p ) return;
	my_lstrcpy( p, cmd );

	while( *p != TEXT('\0') )
	{
		// Skip spaces separating arguments
		while( *p == TEXT(' ') )
			++p;

		// " If so, record that and proceed one more step.
		if( *p == TEXT('\"') )
			endc=TEXT('\"'), ++p;
		else
			endc=TEXT(' ');

		// Terminate if the string ends
		if( *p == TEXT('\0') )
			break;

		// save to argument list
		if( argNum_ >= MAX_ARGS )
			break;
		
		arg_[ argNum_++ ] = p;


		// To the end of the argument...
		while( *p!=endc && *p!=TEXT('\0') )
			++p;

		// Separate arguments by ending with '\0'
		if( *p != TEXT('\0') )
			*p++ = TEXT('\0');
	}
}
