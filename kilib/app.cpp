#include "stdafx.h"
#include "app.h"
#include "log.h"
#include "memory.h"
#include "thread.h"
#include "window.h"
#include "kstring.h"
using namespace ki;

#ifndef NO_OLE32
static HRESULT MyOleInitialize(LPVOID r)
{
	typedef HRESULT (WINAPI * Initialize_funk)(LPVOID r);
	Initialize_funk func = (Initialize_funk)GetProcAddress(app().hOle32(), "OleInitialize");

	if (func) { // We got the function!
		return func(r);
	}
	return 666; // Fail with 666 error.
}
//static HRESULT MyCoInitialize(LPVOID r)
//{
//	Initialize_funk func = (Initialize_funk)GetProcAddress(app().hOle32(), "CoInitialize");
//
//	if (func) { // We got the function!
//		return func(r);
//	}
//	return 666; // Fail with 666 error.
//}

static void MyOleUninitialize( )
{
	typedef void (WINAPI * UnInitialize_funk)( );
	UnInitialize_funk func = (UnInitialize_funk)GetProcAddress(app().hOle32(), "OleUninitialize");

	if (func) { // We got the function!
		func();
	}
}
HRESULT MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
	typedef HRESULT (WINAPI * CoCreateInstance_funk)(REFCLSID , LPUNKNOWN , DWORD , REFIID , LPVOID *);
	static CoCreateInstance_funk func = (CoCreateInstance_funk)(-1);
	if (func == (CoCreateInstance_funk)(-1)) // First time!
		func = (CoCreateInstance_funk)GetProcAddress(app().hOle32(), "CoCreateInstance");

	if (func)
		return func(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	return 666; // Fail with 666 error
}
HRESULT MyCoLockObjectExternal(IUnknown * pUnk, BOOL fLock, BOOL fLastUnlockReleases)
{
	typedef HRESULT (WINAPI *funk_t)(IUnknown *, BOOL, BOOL);
	static funk_t funk = (funk_t)1;

	if( funk == (funk_t)1)
		funk = (funk_t)GetProcAddress(app().hOle32(), "CoLockObjectExternal");

	if( funk )
		return funk(pUnk, fLock, fLastUnlockReleases);

	return E_NOTIMPL;
}
#endif // NO_OLE32


//=========================================================================

App* App::pUniqueInstance_;

inline App::App()
	: osver_       (init_osver())
	, exitcode_    (-1)
	, loadedModule_(0)
	, hInst_       (::GetModuleHandle(NULL))
	, hOle32_      ((HINSTANCE)(-1))
	, hInstComCtl_ (NULL)
{
	// 唯一のインスタンスは私です。
	pUniqueInstance_ = this;
}

#pragma warning( disable : 4722 ) // 警告：デストラクタに値が戻りません
App::~App()
{
	// ロード済みモジュールがあれば閉じておく
#ifndef NO_OLE32
	if( hOle32_ && hOle32_ != (HINSTANCE)(-1) )
	{
	//	if( loadedModule_ & COM )
	//		::MyCoUninitialize();
		if( loadedModule_ & OLE )
			::MyOleUninitialize();

		::FreeLibrary( hOle32_ );
	}

	if( hInstComCtl_ )
		::FreeLibrary( hInstComCtl_ );
#endif

	// 終～了～
	::ExitProcess( exitcode_ );
}

inline void App::SetExitCode( int code )
{
	// 終了コードを設定
	exitcode_ = code;
}

void App::InitModule( imflag what )
{
#ifndef NO_OLE32
	if (hOle32_ == (HINSTANCE)(-1) && what&(OLE|COM|OLEDLL))
		hOle32_ = hasSysDLL(TEXT("OLE32.DLL"))? ::LoadLibrary(TEXT("OLE32.DLL")): NULL;
#endif
	// 初期化済みでなければ初期化する
	bool ret = true;
	if( !(loadedModule_ & what) )
		switch( what )
		{
		case CTL: {
			// ::InitCommonControls();
			if( !hInstComCtl_ )
				hInstComCtl_ = hasSysDLL(TEXT("COMCTL32.DLL"))? ::LoadLibrary(TEXT("COMCTL32.DLL")): NULL;
			if( hInstComCtl_ )
			{
				void (WINAPI *dyn_InitCommonControls)(void) = ( void (WINAPI *)(void) )
					GetProcAddress( hInstComCtl_, "InitCommonControls" );
				if (dyn_InitCommonControls)
					dyn_InitCommonControls();
			}
			} break;
		//case COM:
			// Actually we only ever use OLE, that calls COM, so it can
			// be Ignored safely...
			//ret = hOle32_ && S_OK == ::MyCoInitialize( NULL );
			//MessageBoxA(NULL, "CoInitialize", ret?"Sucess": "Failed", MB_OK);
			//break;
		case OLE:
			#ifndef NO_OLE32
			ret = hOle32_ && S_OK == ::MyOleInitialize( NULL );
			#endif
			// MessageBoxA(NULL, "OleInitialize", ret?"Sucess": "Failed", MB_OK);
			break;
		default: break;
		}

	// 今回初期化したモノを記憶
	if (ret) loadedModule_ |= what;
}
bool App::hasSysDLL(const TCHAR *dllname) const
{
	return true;
}
void App::Exit( int code )
{
	// 終了コードを設定して
	SetExitCode( code );

	// 自殺
	this->~App();
}



//-------------------------------------------------------------------------

MYVERINFO App::init_osver()
{
	OSVERSIONINFOW v = {};
	v.dwOSVersionInfoSize = sizeof(v);
	typedef LONG (WINAPI* pRtlGetVersion)(OSVERSIONINFOW*);
	auto RtlGetVersion = (pRtlGetVersion)GetProcAddress(
		GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
	if (RtlGetVersion)
		RtlGetVersion(&v);

	LOGGERF( TEXT("Windows NT %u.%u build %u")
		, v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber );

	MYVERINFO mv;
	mv.wFromWhichAPI = 1;
	mv.wPlatform = VER_PLATFORM_WIN32_NT;
	mv.v.dwVer = MKVER(v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber);
	return mv;
}

DWORD App::getOOSVer() const
{
	return osver_.v.dwVer;
}

WORD App::getOSVer() const
{
	return osver_.v.vb.ver.wVer;
}

WORD App::getOSBuild() const
{
	return osver_.v.vb.wBuild;
}


//=========================================================================

extern int kmain();
#ifdef WIN64
#define ARENA_RES (1024*1024*8)
#else
#define ARENA_RES (1024*1024)
#endif
namespace ki
{
	DArena TS;
	void APIENTRY Startup()
	{
		// Startup :
		// プログラム開始すると、真っ先にここに来ます。

		// C++のローカルオブジェクトの破棄順序の仕様に
		// 自信がないので(^^;、スコープを利用して順番を強制
		// たぶん宣言の逆順だとは思うんだけど…
		TS.Init( ARENA_RES );

		LOGGER( "StartUp" );
		App myApp;
		{
			LOGGER( "StartUp app ok" );
			#ifdef USE_THREADS
			ThreadManager myThr;
			{
				LOGGER( "StartUp thr ok" );
			#endif
				MemoryManager myMem;
				{
					LOGGER( "StartUp mem ok" );
					IMEManager myIME;
					{
						LOGGER( "StartUp ime ok" );
						String::LibInit();
						{
							LOGGER( "StartUp strings ok" );
							const int r = kmain();
							myApp.SetExitCode( r );
						}
					} //~IMEManager
				} //~MemoryManager
			#ifdef USE_THREADS
			} //~ThreadManager
			#endif
		} //~App
		TS.FreeAll();
	}
}


#ifdef __GNUC__
	// GCC/Clang: custom entry point (nostdlib build)
	extern "C" void WINAPI entryp() { ki::Startup(); }
	extern "C" void __cdecl __cxa_pure_virtual(void) { ExitProcess(1); }
	extern "C" void __deregister_frame_info() {}
	extern "C" void __register_frame_info() {}
	int __stack_chk_guard = 696115047;
	extern "C" int __stack_chk_fail() { MessageBoxA(NULL, "__stack_chk_fail", NULL, MB_OK|MB_TOPMOST); ExitProcess(1); }
#else
	// MSVC: standard WinMain entry
	extern "C" int __cdecl _purecall() { return 0; }
	int APIENTRY WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
	{
		ki::Startup();
		return 0;
	}
#endif
