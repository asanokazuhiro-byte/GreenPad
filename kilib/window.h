#ifndef _KILIB_WINDOW_H_
#define _KILIB_WINDOW_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__
namespace ki {
#endif



// MsgBox return value with timeout
// #define IDTIMEOUT 0



//=========================================================================
//@{ @pkg ki.Window //@}
//@{
//	window operation class
//
//	From the outside, dialogs, controls, property sheets, etc.
//	An interface for handling all normal windows in common.
//	The actual implementation is done in the descendant class XxxImpl.
//@}
//=========================================================================

class A_NOVTABLE Window
{
public:

	//@{ Main message loop //@}
	void MsgLoop();

	//@{ Send message and wait until processed //@}
	inline LRESULT SendMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 ) const
		{ return ::SendMessage( wnd_, msg, wp, lp ); }

	//@{ Send a message and return immediately //@}
	inline BOOL PostMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 ) const
		{ return ::PostMessage( wnd_, msg, wp, lp ); }

	//@{
	//	Message box with auto-destruct function
	//	@param msg String to display
	//	@param caption Dialog title
	//	@param type See the Win32SDK explanation
	//@}
	inline int MsgBox( LPCTSTR msg, LPCTSTR caption=NULL, UINT type=MB_OK ) const
		{ return ::MessageBox( wnd_?wnd_:GetActiveWindow(), msg, caption, type ); }

	static int __cdecl MsgBoxf( HWND hwnd,  LPCTSTR title, LPCTSTR fmt, ... );

	//@{Text settings //@}
	inline void SetText( const TCHAR* str )
		{ ::SetWindowText( wnd_, str ); }

	//@{ display //@}
	inline void ShowUp( int sw=SW_SHOW )
		{ ::ShowWindow( wnd_, sw ), ::UpdateWindow( wnd_ ); }

	//@{ move //@}
	inline void MoveTo( int l, int t, int r, int b )
		{ ::MoveWindow( wnd_, l, t, r-l, b-t, TRUE ); }

	//@{ focus //@}
	inline void SetFocus() { ::SetFocus( wnd_ ); }

	//@{ Go to the front! //@}
	inline void SetFront() { SetFront( wnd_ ); }

	inline void SetActive() { ::SetActiveWindow( wnd_ ); }

	//@{ Go to the center of the screen! //@}
	inline void SetCenter() { SetCenter( wnd_ ); }

public:

	//@{ window handle //@}
	inline HWND hwnd() const { return wnd_; }

	//@{Position/Size //@}
	inline void getPos( RECT* rc ) const { ::GetWindowRect( wnd_, rc ); }

	//@{ size //@}
	inline void getClientRect( RECT* rc ) const { ::GetClientRect( wnd_, rc ); }

	//@{ Whether the window is running the main loop //@}
	inline bool isMainWnd() const { return isLooping_; }

	//@{Are you alive? //@}
	inline bool isAlive() const { return FALSE != ::IsWindow( wnd_ ); }

	//@{ Is the window visible? //@}
	inline bool isVisible() const { return FALSE != ::IsWindowVisible( wnd_ ); }

public:

	//@{ Handle unprocessed messages appropriately //@}
	static void ProcessMsg();

	//@{ Go to the front! //@}
	static void SetFront( HWND hwnd );

	//@{
	//	Go to the center of the screen!
	//	@param hwnd Window to move
	//	@param rel Base window
	//@}
	static void SetCenter( HWND hwnd, HWND rel=NULL );

protected:

	// Constructor that does nothing
	Window();

	// Set Hwnd
	void SetHwnd( HWND wnd );

	// Passing through accelerators, processing dialog messages, etc.
	virtual bool PreTranslateMessage( MSG* ) = 0;

private:

	HWND wnd_;
	bool isLooping_;

private:

	NOCOPY(Window);
};

//=========================================================================
//@{
//	IME control manager
//
//	To support Global IME, window message handling must be
//	It needs to be fundamentally replaced. Therefore, the processing is summarized in this class.
//	By performing cooperative processing with the Window class, nothing can be done from outside the library.
//	Make sure you can handle it without worrying. In addition, it supports Global IME.
//	Because you need a newer version of the Platform SDK to
//	If the macro USEGLOBALIME is not defined, it will not be processed.
//@}
//=========================================================================
class IMEManager
{
public:

	//@{ Font specification //@}
	void SetFont( HWND wnd, const LOGFONT& lf ) A_COLD;

	//@{ position specification //@}
	void SetPos( HWND wnd, int x, int y )  A_COLD;

	//@{ Get fixed string. FreeString it when you receive it. //@}
	void GetString( HWND wnd, unicode** str, size_t* len ) A_COLD;

	static void FreeString( unicode *str ) { free( str ); }

	//@{ Reconversion //@}
	void SetString( HWND wnd, unicode* str, size_t len ) A_COLD;

	//@{ Make GlobalIME available //@}
	void EnableGlobalIME( bool enable ) A_COLD;

	//@{ Turn IME ON/OFF //@}
	void SetState( HWND wnd, bool enable ) A_COLD;

	//@{ IME ON/OFF judgment //@}
	BOOL GetState( HWND wnd ) A_COLD;

	//@{ Check if it has an IME //@}
	BOOL IsIME() A_COLD;

	//@{ Check support for inverse transformation //@}
	BOOL CanReconv() A_COLD;

	//@{ Register the list of Windows that can use GlobalIME //@}
	void FilterWindows( ATOM* lst, UINT siz ) A_COLD;

private:

	IMEManager() A_COLD;
	~IMEManager() A_COLD;
	void    TranslateMsg( MSG* msg );
	LRESULT DefProc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp );
	void    MsgLoopBegin();
	void    MsgLoopEnd();

private:

	#ifdef USEGLOBALIME
		IActiveIMMApp*              immApp_;
		IActiveIMMMessagePumpOwner* immMsg_;
	#endif
	#ifdef NO_IME
		#define hIMM32_ 0
	#else
		#define hIMM32_ 1
	#endif
	static IMEManager* pUniqueInstance_;

private:

	friend class Window;
	friend class WndImpl;
	friend void APIENTRY  Startup();

	//@{ Return only one IME managed object //@}
	friend inline IMEManager& ime();
	NOCOPY(IMEManager);
};

inline IMEManager& ime()
	{ return *IMEManager::pUniqueInstance_; }


//=========================================================================
//@{
//	normal window implementation
//
//	Define a derived class and call it WndImpl in the constructor argument.
//	Pass the WNDCLASS name and style, and register WNDCLASS if it is the first time
//	Then, we assume that you can use Create() at an appropriate time.
//
//	Conversion from HWND to WndImpl* is performed using a thunk dedicated to x86/x86-64.
//	Therefore, it will not work as is on other architectures. transplant
//	If so, use GWL_USERDATA or put it in clsExtra.
//	Please change to a more versatile method as appropriate.
//@}
//=========================================================================

class A_NOVTABLE WndImpl : public Window
{

public:

	//@{Create window //@}
	bool Create( LPCTSTR wndName=NULL,      HWND parent=NULL,
	             int  x     =CW_USEDEFAULT, int y      =CW_USEDEFAULT,
	             int  width =CW_USEDEFAULT, int height =CW_USEDEFAULT,
	             void* param=NULL );

	//@{ destroy window //@}
	void Destroy();

protected:

	//@{
	//	constructor
	//	@param className Window class name
	//	@param style standard style
	//	@param styleEx standard extended style
	//@}
	WndImpl( LPCTSTR className, DWORD style, DWORD styleEx=0 );
	~WndImpl();

	//@{ Type for class name //@}
	typedef const TCHAR* const ClsName;

	//@{ Window class registration //@}
	static ATOM Register( WNDCLASS* cls );

	// Please implement it and respond accordingly.
	// on_command should return false if not processed.
	// on_message should call WndImpl::on_message if not processed.
	// PreTranslateMessage should be called inside whether it is processed or not.
	virtual void    on_create( CREATESTRUCT* cs );
	virtual void    on_destroy();
	virtual bool    on_command( UINT id, HWND ctrl );
	virtual LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp );
	bool    PreTranslateMessage( MSG* msg ) override;

private:

	static LRESULT CALLBACK StartProc( HWND, UINT, WPARAM, LPARAM ) A_COLD;
	static LRESULT CALLBACK MainProc( WndImpl*, UINT, WPARAM, LPARAM );
	#ifdef NO_ASMTHUNK
	static LRESULT CALLBACK TrunkMainProc( HWND, UINT, WPARAM, LPARAM );
	#endif
	void SetUpThunk( HWND wnd );

private:

	LPCTSTR     className_;
	const DWORD style_, styleEx_;
#ifndef NO_ASMTHUNK
	byte*       thunk_;
#endif
};


//=========================================================================
//@{
//	Dialog implementation
//
//	Define a derived class and call it DlgImpl in the constructor argument.
//	Pass the resource ID and use GoModal at an appropriate time.
//	Assumed to be used as Modeless.
//
//	Conversion from HWND to DlgImpl* is performed using GWL_USERDATA.
//	So let's avoid using that.
//@}
//=========================================================================

class A_NOVTABLE DlgImpl : public Window
{
public:

	enum dlgtype { MODAL, MODELESS, UNDEFINED };

	//@{ Run modally //@}
	void GoModal( HWND parent=NULL );

	//@{ Create modeless //@}
	void GoModeless( HWND parent=NULL );

	//@{ Force termination //@}
	void End( UINT code );

public:

	//@{ Modal or modeless //@}
	inline dlgtype type() const { return type_; }

	//@{ Get exit code //@}
	inline UINT endcode() const { return endCode_; }

protected:

	//@{ constructor //@}
	explicit DlgImpl( UINT id )
		: type_(UNDEFINED), rsrcID_( id ) { }
	~DlgImpl();

	//@{ Child item ID → HWND conversion //@}
	inline HWND item( UINT id ) const { return ::GetDlgItem( hwnd(), id ); }

	//@{ Send message for item //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, WPARAM wp=0, LPARAM lp=0 )
		{ return ::SendDlgItemMessage( hwnd(), id, msg, wp, lp ); }

	//@{ Send message to item (pointer sending version) //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, void* lp )
		{ return ::SendDlgItemMessage( hwnd(), id, msg, 0, reinterpret_cast<LPARAM>(lp) ); }


	//@{ Send message to item (version to send string) //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, const TCHAR* lp )
		{ return ::SendDlgItemMessage( hwnd(), id, msg, 0, reinterpret_cast<LPARAM>(lp) ); }

	bool isItemChecked( UINT id ) const
		{ return BST_CHECKED == ::SendDlgItemMessage(hwnd(), id, BM_GETCHECK, 0, 0); }
	void setItemCheck( UINT id, WPARAM state)
		{ ::SendDlgItemMessage(hwnd(), id, BM_SETCHECK, state, 0); }
	inline void CheckItem( UINT id)     { setItemCheck(id, BST_CHECKED); }
	inline void UncheckItem( UINT id)   { setItemCheck(id, BST_UNCHECKED); }
	inline void GrayCheckItem( UINT id) { setItemCheck(id, BST_INDETERMINATE); }

	LRESULT SetItemText( UINT id, const TCHAR *str )
		{ return ::SendDlgItemMessage(hwnd(), id, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str) ); }
	LRESULT GetItemText( UINT id, size_t len, TCHAR *str ) const
		{ return ::SendDlgItemMessage(hwnd(), id, WM_GETTEXT, static_cast<WPARAM>(len), reinterpret_cast<LPARAM>(str) ); }

	// Please implement it and respond accordingly.
	// on_ok/on_cancel should return true if it is OK to terminate.
	// on_cmd/on_msg should return true if it has been processed.
	virtual void on_init();
	virtual void on_destroy();
	virtual bool on_ok();
	virtual bool on_cancel();
	virtual bool on_command( UINT cmd, UINT id, HWND ctrl );
	virtual bool on_message( UINT msg, WPARAM wp, LPARAM lp );
	bool PreTranslateMessage( MSG* msg ) override;

private:

	static INT_PTR CALLBACK MainProc( HWND, UINT, WPARAM, LPARAM );

private:

	dlgtype    type_;
	UINT       endCode_;
	const UINT rsrcID_;
};


//=========================================================================

}      // namespace ki
#endif // _KILIB_WINDOW_H_
