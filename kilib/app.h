#ifndef _KILIB_APP_H_
#define _KILIB_APP_H_
#include "types.h"
#include "log.h"
#include "memory.h"

HRESULT MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
HRESULT MyCoLockObjectExternal(IUnknown * pUnk, BOOL fLock, BOOL fLastUnlockReleases);

#if defined(__GNUC__) && defined(_M_IX86) && _M_IX86 == 300
	// In recent GCC versions this is the only way to link to the real
	// Win32 functions (i386 only).
	#undef InterlockedIncrement
	#undef InterlockedDecrement
	extern "C" WINBASEAPI LONG WINAPI InterlockedIncrement(LONG volatile *);
	extern "C" WINBASEAPI LONG WINAPI InterlockedDecrement(LONG volatile *);
#endif

// Use to make a ordered window version ie: MKVER(3,10,511) = 0x030A01FF
#define MKVER(M, m, b) ( (DWORD)( (BYTE)(M)<<24 | (BYTE)(m)<<16 | (WORD)(b) ) )

// roytam1's versioninfo style
typedef struct _MYVERINFO {
	union {
		DWORD dwVer;
		struct {
			WORD wBuild;
			union {
				WORD wVer;
				struct {
					BYTE cMinor;
					BYTE cMajor;
				} u;
			} ver;
		} vb;
	} v;
	WORD wPlatform;
	WORD wFromWhichAPI;
} MYVERINFO;

#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.Core //@}
//@{
//	Overall application management
//
//	Responsible for application startup/termination processing.
//	Unlike the old kilib, the application class on the user side
//	It cannot be derived from here. The user's code is
//	Execution always starts from the global function kmain().
//	This App class itself mainly manages HINSTANCE.
//@}
//=========================================================================

class App
{
public:

	enum imflag { CTL=1, COM=2, OLE=4, OLEDLL=8 };

	//@{
	//	Initialize various modules
	//
	//	If you initialize it with this, it will be automatically activated when the app closes.
	//	It is easy and convenient because the termination process is done.
	//	@param what CTL (common control), COM, OLE
	//@}
	void InitModule( imflag what );

	//@{ Force termination of process //@}
	void Exit( int code );

	//@{ resource //@}
	inline HACCEL LoadAccel( LPCTSTR name )
		{ return ::LoadAccelerators( hInst_, name ); }

	//@{ resource //@}
	inline HACCEL LoadAccel( UINT id )
		{ return ::LoadAccelerators( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ resource //@}
	inline HBITMAP LoadBitmap( LPCTSTR name )
		{ return ::LoadBitmap( hInst_, name ); }

	//@{ resource //@}
	inline HBITMAP LoadBitmap( UINT id )
		{ return ::LoadBitmap( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ Resource(OBM_XXXX) //@}
	inline HBITMAP LoadOemBitmap( LPCTSTR obm )
		{ return ::LoadBitmap( NULL, obm ); }

	//@{ resource //@}
	inline HCURSOR LoadCursor( LPCTSTR name )
		{ return ::LoadCursor( hInst_, name ); }

	//@{ resource //@}
	inline HCURSOR LoadCursor( UINT id )
		{ return ::LoadCursor( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ Resource(IDC_XXXX) //@}
	inline HCURSOR LoadOemCursor( LPCTSTR idc )
		{ return ::LoadCursor( NULL, idc ); }

	//@{ resource //@}
	inline HICON LoadIcon( LPCTSTR name )
		{ return ::LoadIcon( hInst_, name ); }

	//@{ resource //@}
	inline HICON LoadIcon( UINT id )
		{ return ::LoadIcon( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ Resource(IDI_XXXX) //@}
	inline HICON LoadOemIcon( LPCTSTR idi )
		{ return ::LoadIcon( NULL, idi ); }

	//@{ resource //@}
	inline HMENU LoadMenu( LPCTSTR name )
		{ return ::LoadMenu( hInst_, name ); }

	//@{ resource //@}
	inline HMENU LoadMenu( UINT id )
		{ return ::LoadMenu( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ resource //@}
	inline int LoadString( UINT id, LPTSTR buf, int siz )
		{ return ::LoadString( hInst_, id, buf, siz ); }

public:

	//@{ instance handle //@}
	HINSTANCE hinst() const { return hInst_; }
	HINSTANCE hOle32() const { return hOle32_; }
	bool hasSysDLL(const TCHAR *dllname) const;
	//@{ Windows version //@}

	DWORD getOOSVer() const A_PURE;
	WORD getOSVer() const A_PURE;
	WORD getOSBuild() const A_PURE;

private:

	App();
	~App();
	void SetExitCode( int code );
	MYVERINFO init_osver();

private:
	const MYVERINFO osver_;
	int             exitcode_;
	uint            loadedModule_;
	const HINSTANCE hInst_;
	HINSTANCE       hOle32_;
	HINSTANCE       hInstComCtl_;
	static App*     pUniqueInstance_;

private:

	friend void APIENTRY Startup();

	//@{ Return only one app information object //@}
	friend inline App& app();

	NOCOPY(App);
};

inline ki::App& app()
	{ return *App::pUniqueInstance_; }

//=========================================================================

}      // namespace ki
#endif // _KILIB_APP_H_
