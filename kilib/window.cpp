#include "stdafx.h"
#include "app.h"
#include "window.h"
#include "../LangManager.h"
#ifndef SCS_CAP_SETRECONVERTSTRING
#define SCS_CAP_SETRECONVERTSTRING 0x00000004
#endif
using namespace ki;

HKL MyGetKeyboardLayout(DWORD dwLayout){ return GetKeyboardLayout(dwLayout); }
#define myImmSetCompositionFont ImmSetCompositionFont
//=========================================================================
// All about IME
//=========================================================================
static const GUID myIID_IActiveIMMMessagePumpOwner = {0xb5cf2cfa,0x8aeb,0x11d1,{0x93,0x64,0x00,0x60,0xb0,0x67,0xb8,0x6e}};
static const GUID myIID_IActiveIMMApp = {0x08c0e040, 0x62d1, 0x11d1,{0x93,0x26, 0x00,0x60,0xb0,0x67,0xb8,0x6e}};
static const GUID myCLSID_CActiveIMM = {0x4955dd33, 0xb159, 0x11d0, {0x8f,0xcf, 0x00,0xaa,0x00,0x6b,0xcc,0x59}};
IMEManager* IMEManager::pUniqueInstance_;
IMEManager::IMEManager()
{
	// the only instance is me
	pUniqueInstance_ = this;

	#ifdef USEGLOBALIME
		immApp_ = NULL;
		immMsg_ = NULL;
		// GlobalIME is not available on Win95 because it is a lot of trouble.
		// No global IME on Win95 because it is buggy...
		// RamonUnch: I found it is not so buggy so
		// I re-enabled it unless we are on a DBCS enabled system!
		{
			app().InitModule( App::OLE );
			if( S_OK == ::MyCoCreateInstance(
					myCLSID_CActiveIMM, NULL, CLSCTX_INPROC_SERVER,
					myIID_IActiveIMMApp, (void**)&immApp_ ) )
			{
				// MessageBox(NULL, TEXT("Global IME Loaded!"), NULL, 0);
				HRESULT ret = immApp_->QueryInterface(
					myIID_IActiveIMMMessagePumpOwner, (void**)&immMsg_ );
				if( ret == S_OK ) return;
			}
		}
	#endif //USEGLOBALIME
}

IMEManager::~IMEManager()
{
	#ifdef USEGLOBALIME
		if( immMsg_ != NULL )
		{
			immMsg_->Release();
			immMsg_ = NULL;
		}
		if( immApp_ != NULL )
		{
			immApp_->Deactivate();
			immApp_->Release();
			immApp_ = NULL;
		}
	#endif

}

void IMEManager::EnableGlobalIME( bool enable )
{
	#ifdef USEGLOBALIME
		if( immApp_ )
			if( enable ) immApp_->Activate( TRUE );
			else         immApp_->Deactivate();
	#endif
}

BOOL IMEManager::IsIME()
{
#ifndef NO_IME
	HKL hKL = MyGetKeyboardLayout(GetCurrentThreadId());
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			return immApp_->IsIME( hKL );
		}
		else
	#endif // USEGLOBALIME
		if( hIMM32_ )
		{
			return ::ImmIsIME( hKL );
		}
#endif // NO_IME
	return FALSE;
}

BOOL IMEManager::CanReconv()
{
#ifndef NO_IME
	HKL hKL = MyGetKeyboardLayout(GetCurrentThreadId());
	DWORD nImeProps = 0; //= ImmGetProperty( hKL, IGP_SETCOMPSTR );
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			immApp_->GetProperty( hKL, IGP_SETCOMPSTR, &nImeProps );
		}
		else
	#endif
		if( hIMM32_ )
		{
			nImeProps = ::ImmGetProperty( hKL, IGP_SETCOMPSTR );
		}
		return (nImeProps & SCS_CAP_SETRECONVERTSTRING) != 0;
#else
	return FALSE;
#endif // NO_IME
}

BOOL IMEManager::GetState( HWND wnd )
{
	BOOL imeStatus = FALSE;
#ifndef NO_IME
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			HIMC ime = NULL;
			immApp_->GetContext( wnd, &ime );
			imeStatus = immApp_->GetOpenStatus( ime );
			immApp_->ReleaseContext( wnd, ime );
		}
		else
	#endif
		if( hIMM32_ )
		{
			HIMC ime = ::ImmGetContext( wnd );
			if( ime )
			{
				imeStatus = ::ImmGetOpenStatus(ime );
				::ImmReleaseContext( wnd, ime );
			}
		}
#endif // NO_IME
	return imeStatus;
}

void IMEManager::SetState( HWND wnd, bool enable )
{
#ifndef NO_IME
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			HIMC ime = NULL;
			immApp_->GetContext( wnd, &ime );
			immApp_->SetOpenStatus( ime, (enable ? TRUE : FALSE) );
			immApp_->ReleaseContext( wnd, ime );
		}
		else
	#endif // USEGLOBALIME
		if( hIMM32_ )
		{
			HIMC ime = ::ImmGetContext( wnd );
			::ImmSetOpenStatus(ime, (enable ? TRUE : FALSE) );
			::ImmReleaseContext( wnd, ime );
		}
#endif // NO_IME
}

void IMEManager::FilterWindows( ATOM* lst, UINT siz )
{
	#ifdef USEGLOBALIME
		if( immApp_ )
			immApp_->FilterClientWindows( lst, siz );
	#endif
}

inline void IMEManager::TranslateMsg( MSG* msg )
{
	#ifdef USEGLOBALIME
		if( immMsg_ )
			if( S_OK == immMsg_->OnTranslateMessage( msg ) )
				return;
	#endif
	::TranslateMessage( msg );
}

inline LRESULT IMEManager::DefProc( HWND h, UINT m, WPARAM w, LPARAM l )
{
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			LRESULT res;
			if( S_OK == immApp_->OnDefWindowProc( h,m,w,l,&res ) )
				return res;
		}
	#endif
	return ::DefWindowProc( h, m, w, l );
}

inline void IMEManager::MsgLoopBegin()
{
	#ifdef USEGLOBALIME
		if( immMsg_ )
			immMsg_->Start();
	#endif
}

inline void IMEManager::MsgLoopEnd()
{
	#ifdef USEGLOBALIME
		if( immMsg_ )
			immMsg_->End();
	#endif
}

void IMEManager::SetFont( HWND wnd, const LOGFONT& lf )
{
#ifndef NO_IME
	LOGFONT* plf = const_cast<LOGFONT*>(&lf);

	#ifdef USEGLOBALIME
	if( immApp_ )
	{
		HIMC ime = NULL;
		immApp_->GetContext( wnd, &ime );
		#ifdef _UNICODE
			immApp_->SetCompositionFontW( ime, plf );
		#else
			immApp_->SetCompositionFontA( ime, plf );
		#endif
		immApp_->ReleaseContext( wnd, ime );
	}
	else
	#endif //USEGLOBALIME
	if( hIMM32_ )
	{
		HIMC ime = ::ImmGetContext( wnd );
		if( ime )
		{
			// We must use Ansi version on ANSI build and
			// Unicode version on pure UNICODE build
			myImmSetCompositionFont( ime, plf ); // A/W
			::ImmReleaseContext( wnd, ime );
		}
	}
#endif // NO_IME
}

void IMEManager::SetPos( HWND wnd, int x, int y )
{
#ifndef NO_IME
	COMPOSITIONFORM cf;
	cf.dwStyle = CFS_POINT;
	cf.ptCurrentPos.x  = x;
	cf.ptCurrentPos.y  = y;

	#ifdef USEGLOBALIME
	if( immApp_ )
	{
		HIMC ime = NULL;
		immApp_->GetContext( wnd, &ime );
		immApp_->SetCompositionWindow( ime, &cf );
		immApp_->ReleaseContext( wnd, ime );
	}
	else
	#endif // USEGLOBALIME
	if( hIMM32_ )
	{
		HIMC ime = ::ImmGetContext( wnd );
		if( ime )
		{
			::ImmSetCompositionWindow( ime, &cf );
			::ImmReleaseContext( wnd, ime );
		}
	}
#endif // NO_IME
}

void IMEManager::GetString( HWND wnd, unicode** str, size_t* len )
{
#ifndef NO_IME
	#ifdef USEGLOBALIME
	if( immApp_ )
	{
		long s=0;
		HIMC ime = NULL;
		immApp_->GetContext( wnd, &ime );
		immApp_->GetCompositionStringW( ime, GCS_RESULTSTR, 0, &s, NULL );
		*str = (unicode *)malloc( sizeof(unicode) *((*len=s/2)+1) );
		if( *str )
			immApp_->GetCompositionStringW( ime, GCS_RESULTSTR, s, &s, *str );
		immApp_->ReleaseContext( wnd, ime );
	}
	else
	#endif //USEGLOBALIME
	if( hIMM32_ )
	{
		HIMC ime = ::ImmGetContext( wnd );
		if( !ime ) return;
		long s = ::ImmGetCompositionStringW( ime,GCS_RESULTSTR,NULL,0 );
		if( s > 0 )
		{
			*str = (unicode *)malloc( sizeof(unicode) * ((*len=s/2)+1) );
			if( *str )
				::ImmGetCompositionStringW( ime, GCS_RESULTSTR, *str, s );
		}
		::ImmReleaseContext( wnd, ime );
	} // end if( hIMM32_ )
#endif //NO_IME
}

void IMEManager::SetString( HWND wnd, unicode* str, size_t len )
{
#ifndef NO_IME

	#ifdef USEGLOBALIME
	if( immApp_ )
	{
		HIMC ime=NULL;
		immApp_->GetContext( wnd, &ime );
		immApp_->SetCompositionStringW( ime, SCS_SETSTR, str, len*sizeof(unicode), NULL, 0 );
		immApp_->NotifyIME( ime, NI_COMPOSITIONSTR, CPS_CONVERT, 0 );
		immApp_->NotifyIME( ime, NI_OPENCANDIDATE, 0, 0 );
		immApp_->ReleaseContext( wnd, ime );
	}
	else
	#endif //USEGLOBALIME
	if( hIMM32_ )
	{
		HIMC ime = ::ImmGetContext( wnd );
		if( !ime ) return;
		::ImmSetCompositionStringW( ime,SCS_SETSTR,str,len*sizeof(unicode),NULL,0 );
		::ImmNotifyIME( ime, NI_COMPOSITIONSTR, CPS_CONVERT, 0); // Conversion execution
		::ImmNotifyIME( ime, NI_OPENCANDIDATE, 0, 0 ); // Conversion candidate list display
		::ImmReleaseContext( wnd, ime );
	}// end if( hIMM32_ )
#endif //NO_IME
}


//=========================================================================
// All about Windows
//=========================================================================

Window::Window()
	: wnd_      (NULL)
	, isLooping_(false)
{
}

void Window::SetHwnd( HWND wnd )
{
	wnd_ = wnd;
}

void Window::MsgLoop()
{
	// With this as the main window,
	// Run main message loop
	isLooping_ = true;
	ime().MsgLoopBegin();
	for( MSG msg; ::GetMessage( &msg, NULL, 0, 0 ); )
	{
		if( TS.end != TS.sta )
			LOGGERF( TEXT("leak %d bytes!"), TS.end - TS.sta );
		TS.reset();
		if( !PreTranslateMessage( &msg ) )
		{
			ime().TranslateMsg( &msg );
			::DispatchMessage( &msg );
		}
	}
	ime().MsgLoopEnd();
	isLooping_ = false;
}

void Window::ProcessMsg()
{
	// This is a global function.
	// Clear unprocessed messages
	ime().MsgLoopBegin();
	for( MSG msg; ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ); )
	{
		ime().TranslateMsg( &msg );
		::DispatchMessage( &msg );
	}
	ime().MsgLoopEnd();
}



//-------------------------------------------------------------------------

int __cdecl Window::MsgBoxf( HWND hwnd,  LPCTSTR title, LPCTSTR fmt, ... )
{
	va_list arglist;
	TCHAR str[512];
	TCHAR lerrorstr[16];
	LPVOID lpMsgBuf = NULL;

	DWORD lerr = GetLastError();

	va_start( arglist, fmt );
	wvsprintf( str, fmt, arglist );
	va_end( arglist );

	DWORD ok = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		lerr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
		reinterpret_cast<TCHAR*>(&lpMsgBuf), /* &, because FORMAT_MESSAGE_ALLOCATE_BUFFER */
		0, NULL
	);

	wsprintf( lerrorstr, TEXT("\nLast Eror #%u:\n"), (UINT)lerr );
	lstrcat( str, lerrorstr );
	lstrcat( str, ok? lpMsgBuf? (LPCTSTR)lpMsgBuf: TEXT("(null)"): TEXT("FormatMessage() failed!") );

	/* Free the buffer using LocalFree. */
	LocalFree( lpMsgBuf );
	return ::MessageBox( hwnd, str, title, 0 );
}

void Window::SetCenter( HWND hwnd, HWND rel )
{
	// get your size
	RECT rc,pr;
	::GetWindowRect( hwnd, &rc );

	// Get parent position or full screen position
	if( rel != NULL )
		::GetWindowRect( rel, &pr );
	else
		::SystemParametersInfo( SPI_GETWORKAREA, 0, &pr, 0 );

	long Xdiff = (pr.right-pr.left)-(rc.right-rc.left);
	long Ydiff = (pr.bottom-pr.top)-(rc.bottom-rc.top);
	// calculate center
	if( Xdiff >= 0 && Ydiff >= 0 )
	{	// Only center if rc is smaller than pr
		// Otherwise the window can move out of reach.
		::SetWindowPos( hwnd, 0,
			pr.left + Xdiff/2,
			pr.top  + Ydiff/2,
			0, 0, SWP_NOSIZE|SWP_NOZORDER );
	}
}

void Window::SetFront( HWND hwnd )
{
	// I referred to Mr. kazubon's TClock source. Thank you!

	{
		DWORD pid;
		HWND  fore= ::GetForegroundWindow();
		DWORD th1 = ::GetWindowThreadProcessId( fore, &pid );
		DWORD th2 = ::GetCurrentThreadId();
		::AttachThreadInput( th2, th1, TRUE );
		::SetForegroundWindow( hwnd );
		::AttachThreadInput( th2, th1, FALSE );
		::BringWindowToTop( hwnd );
	}
}

//=========================================================================
// Static Thunk allocator, we use a single 4K memory page for all thunks
#ifndef NO_ASMTHUNK
namespace
{
	// THUNK allocator variables
	enum {
		TOT_THUNK_SIZE=4096,
		#if defined(_M_AMD64) || defined(WIN64)
		THUNK_SIZE=32, // x86_64 mode
		#elif defined(_M_IX86)
		THUNK_SIZE=16, // i386 mode
		#else
		#error Unsupported processor type, determine THUNK_SIZE here...
		#endif
		MAX_THUNKS=TOT_THUNK_SIZE/THUNK_SIZE,
	};

	static BYTE *thunks_array = NULL;
	static WORD free_idx = 0;
	static WORD numthunks = 0;

	static byte *AllocThunk(LONG_PTR thunkProc, void *vParam)
	{
		if( !thunks_array )
		{
			thunks_array = (BYTE*)::VirtualAlloc( NULL, TOT_THUNK_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
			if ( !thunks_array )
				return NULL;

			free_idx = 0;
			for (WORD i=0; i < MAX_THUNKS; i++)
				*(WORD*)&thunks_array[i*THUNK_SIZE] = i+1;
		}

		if (numthunks >= MAX_THUNKS) {
			return NULL;
		}
		++numthunks;

		// get the free thunk
		BYTE *thunk = &thunks_array[free_idx * THUNK_SIZE];
		free_idx = *(WORD*)thunk; // Pick up the free index

		DWORD oldprotect;
		::VirtualProtect(thunks_array, TOT_THUNK_SIZE, PAGE_EXECUTE_READWRITE, &oldprotect);

		// Here, the x86 instruction sequence is dynamically
		//   | mov dword ptr [esp+4] this
		//   | jmp MainProc
		// Or AMD64 instruction sequence
		//   | mov rcx this
		//   | mov rax MainProc
		//   | jmp rax
		// Generate and replace it as a message procedure.
		//
		// Now from next time, the first argument will be hwnd instead of
		// MainProc is called with this pointer
		// I can write a program that considers...
		//
		// Reference material: ATL source

		#if defined(_M_AMD64) || defined(WIN64)
		*reinterpret_cast<dbyte*>   (thunk+ 0) = 0xb948;
		*reinterpret_cast<void**>   (thunk+ 2) = vParam;
		*reinterpret_cast<dbyte*>   (thunk+10) = 0xb848;
		*reinterpret_cast<void**>   (thunk+12) = (LONG_PTR*)thunkProc;
		*reinterpret_cast<dbyte*>   (thunk+20) = 0xe0ff;
		#elif defined(_M_IX86)
		*reinterpret_cast<qbyte*>   (thunk+0) = 0x042444C7;
		*reinterpret_cast<void**>   (thunk+4) = vParam;
		*reinterpret_cast< byte*>   (thunk+8) = 0xE9;
		*reinterpret_cast<qbyte*>   (thunk+9) = reinterpret_cast<byte*>((void*)thunkProc)-(thunk+13);
		#else
		#error Unsupported processor type, please implement assembly code or consider defining NO_ASMTHUNK
		#endif

		// Make thunk read+execute only, for safety.
		::VirtualProtect(thunks_array, TOT_THUNK_SIZE, PAGE_EXECUTE, &oldprotect);
		::FlushInstructionCache( GetCurrentProcess(), thunks_array, TOT_THUNK_SIZE );

		LOGGERF( TEXT("THUNK ALLOC at %lX"), (UINT)(UINT_PTR)thunk );
		return thunk;
	}
	static void ReleaseThunk(byte *thunk)
	{
		if ( !thunk || !thunks_array )
			return;

		DWORD oldprotect;
		::VirtualProtect(thunks_array, TOT_THUNK_SIZE, PAGE_EXECUTE_READWRITE, &oldprotect);

		*(WORD*)thunk = free_idx; // Set free index in the current thunk location
		free_idx = (WORD)( (thunk - thunks_array) / THUNK_SIZE );
		--numthunks;

		::VirtualProtect(thunks_array, TOT_THUNK_SIZE, PAGE_EXECUTE, &oldprotect);

		// Everything is free!
		if (numthunks == 0) {
			::VirtualFree( thunks_array, 0, MEM_RELEASE );
			thunks_array = NULL;
		}
		LOGGERF( TEXT("THUNK FREE at %lX"), (UINT)(UINT_PTR)thunk );
	}
}
#endif //NO_ASMTHUNK

WndImpl::WndImpl( LPCTSTR className, DWORD style, DWORD styleEx )
	: className_( className )
	, style_    ( style )
	, styleEx_  ( styleEx )
#ifndef NO_ASMTHUNK
	, thunk_    ( NULL )
#endif
{
}

//=========================================================================
WndImpl::~WndImpl()
{
	// If you forget to destroy the window, close it.
	// ...but at this point the vtable is already being destroyed, so
	// There is no guarantee that the correct on_destroy will be called. Just to the last
	// Think of it as an emergency escape (^^;).
	Destroy();
#ifndef NO_ASMTHUNK
	ReleaseThunk( thunk_ );
#endif
}

void WndImpl::Destroy()
{
	if( hwnd() != NULL )
		::DestroyWindow( hwnd() );
}

ATOM WndImpl::Register( WNDCLASS* cls )
{
	// Register WndClass to be used in WndImpl derived class.
	// The procedure will be rewritten to one made by kilib.
	cls->hInstance   = app().hinst();
	cls->lpfnWndProc = StartProc;
#ifdef NO_ASMTHUNK
	cls->cbWndExtra  = sizeof(void*);
#endif // NO_ASMTHUNK
	return ::RegisterClass( cls );
}

struct ThisAndParam
{
	// There are a few things I want to pass other than user-specified parameters...
	WndImpl* pThis;
	void*   pParam;
};

bool WndImpl::Create(
	LPCTSTR wndName, HWND parent, int x, int y, int w, int h, void* param )
{
	// Let's sneak in the this pointer here
	ThisAndParam z = { this, param };

	LOGGER("WndImpl::Create before CreateWindowEx API call");

	return (NULL != ::CreateWindowEx(
		styleEx_, className_, wndName, style_,
		x, y, w, h, parent, NULL, app().hinst(), &z
	));
}



//-------------------------------------------------------------------------

LRESULT CALLBACK WndImpl::StartProc(
	HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
	// The policy is to ignore everything except WM_CREATE.
	if( msg != WM_CREATE )
		return ::DefWindowProc( wnd, msg, wp, lp );

	LOGGER("WndImpl::StartProc WM_CREATE kitaaaaa!!");

	// Retrieve the hidden this pointer
	CREATESTRUCT* cs   = reinterpret_cast<CREATESTRUCT*>(lp);
	ThisAndParam* pz   = static_cast<ThisAndParam*>(cs->lpCreateParams);
	WndImpl*   pThis   = pz->pThis;
	cs->lpCreateParams = pz->pParam;
	#ifdef NO_ASMTHUNK
	// Store the this pointer in the cbWndExtra bytes when not using ASM thunking
	::SetWindowLongPtr(wnd, 0, reinterpret_cast<LONG_PTR>(pThis));
	#endif
	// Thunk
	pThis->SetUpThunk( wnd );
	LOGGER("WndImpl::StartProc SetUpThunk( wnd ) OK");

	// Call message for WM_CREATE
	pThis->on_create( cs );
	return 0;
}

void WndImpl::SetUpThunk( HWND wnd )
{
	SetHwnd( wnd );
#ifdef NO_ASMTHUNK
	// Use TrunkMainProc() that does not depends on any asmembly language.
	::SetWindowLongPtr( wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(TrunkMainProc) );
#else // USE ASM
	// Create a thunk to use  the this pointer as first param instead of hwnd
	thunk_ = AllocThunk( reinterpret_cast<LONG_PTR>(MainProc), this );
	::SetWindowLongPtr( wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&thunk_[0]) );
#endif // NO_ASMTHUNK
}

#ifdef NO_ASMTHUNK
// To avoid ASM thunking we can use cbWndExtra bytes in the window structure
LRESULT CALLBACK WndImpl::TrunkMainProc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
	WndImpl*   pThis  = reinterpret_cast<WndImpl*>(GetWindowLongPtr(wnd, 0));
	if(pThis) {
		return WndImpl::MainProc(pThis, msg, wp, lp);
	} else {
		return DefWindowProc(wnd, msg, wp, lp);
	}
}
#endif // NO_ASMTHUNK

LRESULT CALLBACK WndImpl::MainProc(
	WndImpl* ptr, UINT msg, WPARAM wp, LPARAM lp )
{
	if( msg == WM_COMMAND )
	{
		if( !ptr->on_command( LOWORD(wp), (HWND)lp ) )
			return ::DefWindowProc( ptr->hwnd(), msg, wp, lp );
	}
	else if( msg == WM_DESTROY )
	{
		ptr->on_destroy();
		::SetWindowLongPtr( ptr->hwnd(), GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(StartProc) );
		if( ptr->isMainWnd() )
			::PostQuitMessage( 0 );
		ptr->SetHwnd(NULL);
	}
	else
	{
		return ptr->on_message( msg, wp, lp );
	}

	return 0;
}



//-------------------------------------------------------------------------

void WndImpl::on_create( CREATESTRUCT* cs )
{
	// do nothing
}

void WndImpl::on_destroy()
{
	// do nothing
}

bool WndImpl::on_command( UINT, HWND )
{
	// do nothing
	return false;
}

LRESULT WndImpl::on_message( UINT msg, WPARAM wp, LPARAM lp )
{
	// do nothing
	return ime().DefProc( hwnd(), msg, wp, lp );
}

bool WndImpl::PreTranslateMessage( MSG* )
{
	// do nothing
	return false;
}



//=========================================================================


DlgImpl::~DlgImpl()
{
	// If you forget to destroy the window, close it.
	if( hwnd() != NULL )
		End( IDCANCEL );
}

void DlgImpl::End( UINT code )
{
	endCode_ = code;

	if( type() == MODAL )
		::EndDialog( hwnd(), code );
	else
		::DestroyWindow( hwnd() );
}

void DlgImpl::GoModal( HWND parent )
{
	type_ = MODAL;
	::DialogBoxParam( app().hinst(), MAKEINTRESOURCE(rsrcID_), parent,
		(DLGPROC)MainProc, reinterpret_cast<LPARAM>(this) );
}

void DlgImpl::GoModeless( HWND parent )
{
	type_ = MODELESS;
	::CreateDialogParam( app().hinst(), MAKEINTRESOURCE(rsrcID_), parent,
		(DLGPROC)MainProc, reinterpret_cast<LPARAM>(this) );
}



//-------------------------------------------------------------------------

INT_PTR CALLBACK DlgImpl::MainProc(
	HWND dlg, UINT msg, WPARAM wp, LPARAM lp )
{
	if( msg == WM_INITDIALOG )
	{
		::SetWindowLongPtr( dlg, GWLP_USERDATA, lp );

		DlgImpl* ptr = reinterpret_cast<DlgImpl*>(lp);
		ptr->SetHwnd( dlg );
		LangManager::Get().ApplyToDialog( dlg, ptr->rsrcID_ );
		ptr->on_init();
		return FALSE;
	}

	DlgImpl* ptr =
		reinterpret_cast<DlgImpl*>(::GetWindowLongPtr(dlg,GWLP_USERDATA));

	if( ptr != NULL )
		switch( msg )
		{
		case WM_COMMAND:
			switch( LOWORD(wp) )
			{
			case IDOK:
				if( ptr->on_ok() )
					ptr->End( IDOK );
				return TRUE;

			case IDCANCEL:
				if( ptr->on_cancel() )
					ptr->End( IDCANCEL );
				return TRUE;

			default:
				return ptr->on_command( HIWORD(wp), LOWORD(wp),
					reinterpret_cast<HWND>(lp) ) ? TRUE : FALSE;
			}

		case WM_DESTROY:
			ptr->on_destroy();
			if( ptr->isMainWnd() )
				::PostQuitMessage( 0 );
			ptr->SetHwnd(NULL);
			break;

		default:
			return ptr->on_message( msg, wp, lp ) ? TRUE : FALSE;
		}

	return FALSE;
}



//-------------------------------------------------------------------------

void DlgImpl::on_init()
{
	// do nothing
}

void DlgImpl::on_destroy()
{
	// do nothing
}

bool DlgImpl::on_ok()
{
	// do nothing
	return true;
}

bool DlgImpl::on_cancel()
{
	// do nothing
	return true;
}

bool DlgImpl::on_command( UINT, UINT, HWND )
{
	// do nothing
	return false;
}

bool DlgImpl::on_message( UINT, WPARAM, LPARAM )
{
	// do nothing
	return false;
}

bool DlgImpl::PreTranslateMessage( MSG* msg )
{
	// For modeless use. Dialog message processing.
	return (FALSE != ::IsDialogMessage( hwnd(), msg ));
}
