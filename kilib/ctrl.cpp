#include "stdafx.h"
#include "app.h"
#include "ctrl.h"
using namespace ki;



//=========================================================================

StatusBar::StatusBar()
	: width_   (0)
	, visible_ (false)
	, parent_  (NULL)
{
	//app().InitModule( App::CTL );
}

bool StatusBar::Create( )
{
	HWND h = NULL;
	WNDCLASS wc;
	app().InitModule( App::CTL );
	// Avoid using CreateStatusWindow that is not present on NT3.1.
	h = ::CreateWindowEx(
		0, // ExStyle
		GetClassInfo(NULL, STATUSCLASSNAME, &wc)?
		STATUSCLASSNAME: // TEXT("msctls_statusbar32")...
		TEXT("msctls_statusbar"), // TEXT("msctls_statusbar") for NT3.1 and 3.5 build <711

		NULL, // pointer to window name
		WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP , // window style
		0, 0, 0, 0, //x, y, w, h
		parent_, // handle to parent or owner window
		reinterpret_cast<HMENU>(1787), // handle to menu or child-window identifier
		app().hinst(), // handle to application instance
		NULL // pointer to window-creation data
	);

	if( h == NULL )
	{
		LOGGERF(TEXT("StatusBar::Create() failed"));
		return false;
	}

	SetHwnd( h );
	visible_ = true;
	AutoResize( false );
	return true;
}

void StatusBar::SetStatusBarVisible(bool b)
{
	if (b && !hwnd() && parent_ )
		Create(); // Create if needed
	if( hwnd() )
		::ShowWindow( hwnd(), b? SW_SHOW: SW_HIDE );
	visible_= b && hwnd();
}

int StatusBar::AutoResize( bool maximized )
{
	// automatic resize
	SendMsg( WM_SIZE );

	// Get the changed size
	RECT rc;
	getPos( &rc );
	width_ = rc.right - rc.left;
	return (isStatusBarVisible() ? rc.bottom - rc.top : 0);
}

bool StatusBar::PreTranslateMessage( MSG* )
{
	// do nothing
	return false;
}

void StatusBar::SetText( const TCHAR* str, int part )
{
	SendMsg( SB_SETTEXT, part, reinterpret_cast<LPARAM>(str) );
}

int StatusBar::GetTextLen( int part )
{
	return SendMsg( SB_GETTEXTLENGTH, part, 0 );
}
int StatusBar::GetText( TCHAR* str, int part )
{
	int len = GetTextLen(part);
	if( len >= 255 )
	{
		str[0] = TEXT('\0');
		return 0;
	}
	SendMsg( SB_GETTEXT, part, reinterpret_cast<LPARAM>(str) );
	return len;
}



//=========================================================================

void ComboBox::Select( const TCHAR* str )
{
	// SELECTSTRING cannot be used because it matches everything with the same beginning.
	// Perhaps it should be used for incremental search or something.
	LRESULT i = SendMsg( CB_FINDSTRINGEXACT, ~0, reinterpret_cast<LPARAM>(str) );
	if( i != CB_ERR )
		SendMsg( CB_SETCURSEL, i );
//	else
//		SendMsg( WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str) );
}

bool ComboBox::PreTranslateMessage( MSG* )
{
	return false;
}
