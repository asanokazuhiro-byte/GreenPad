#ifndef _EDITWING_VIEW_H_
#define _EDITWING_VIEW_H_
#include "ewCommon.h"
#include "ewDoc.h"
#ifndef __ccdoc__
namespace editwing {
namespace view {
#endif

class Canvas;
class ViewImpl;
class Cursor;
class Caret;
class OleDnDTarget;


//=========================================================================
//@{
//	event handler interface
//
//	Various events that occur from the cursor
//@}
//=========================================================================

class CurEvHandler
{
	friend class Cursor;
	virtual void on_move( const DPos& c, const DPos& s ) {}
	virtual void on_char( Cursor& cur, unicode wch );
	virtual void on_key( Cursor& cur, int vk, bool sft, bool ctl );
	virtual void on_ime( Cursor& cur, unicode* str, size_t len );
};

//=========================================================================
//@{
// OLE Drag and Drop handler.
//@}
//=========================================================================
#ifndef NO_OLEDNDTAR
class OleDnDTarget A_FINAL: public IDropTarget
{
	friend class Cursor;
	OleDnDTarget( HWND hwnd, ViewImpl& vw );
	~OleDnDTarget();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

	ULONG STDMETHODCALLTYPE AddRef()  override { LOGGER("OleDnDTarget::AddRef()"); return ::InterlockedIncrement(&refcnt); }
	ULONG STDMETHODCALLTYPE Release() override
	{
		LOGGER("OleDnDTarget::Release()");
		ULONG ret = ::InterlockedDecrement(&refcnt);
//		if( ret == 0 )
//			delete this;
		return ret;
	}

	HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override
	{
		// Only accept DragEnter if we can get some text.
		comes_from_center_ = false;
		FORMATETC fmt = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		if( S_OK == pDataObj->QueryGetData(&fmt) )
		{
			LOGGER( "OleDnDTarget::DragEnter(CF_UNICODETEXT)" );
			return S_OK;
		}

		fmt.cfFormat = CF_TEXT;
		if( S_OK == pDataObj->QueryGetData(&fmt) )
		{
			LOGGER( "OleDnDTarget::DragEnter(CF_TEXT)" );
			return S_OK;
		}

		LOGGER( "OleDnDTarget::DragEnter(No supported IDataObject format)" );
		return E_UNEXPECTED;
	}

	HRESULT STDMETHODCALLTYPE DragLeave() override
		{ LOGGER( "OleDnDTarget::DragLeave()" ); return S_OK; }

	HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

	HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

private:
	void setDropEffect(DWORD grfKeyState, DWORD *pdwEffect) const;

private:
	LONG refcnt;
	HWND hwnd_;
	ViewImpl& view_;

	bool comes_from_center_;
};

#endif // NO_OLEDNDTAR

//=========================================================================
//@{
//	DPos including display location information
//@}
//=========================================================================

struct VPos : public DPos
{
	ulong vl; // VLine-Index
	ulong rl; // RLine-Index
	int   vx; // x coordinate (pixel) on the virtual screen without considering scrolling
	int   rx; // x coordinate (pixel) that is not affected by the arrangement of characters
		      //   == Move from the tail of the long line to the short line with [↑]
		      //   == After that, you can go back with [↓].
	void operator=( const DPos& dp ) { tl=dp.tl, ad=dp.ad; }

	VPos(bool) : DPos(0,0),vl(0),rl(0),vx(0),rx(0) {}
	VPos() {}
};


//-------------------------------------------------------------------------
// Caret control wrapper
//-------------------------------------------------------------------------
class Caret
{
public:
	Caret( HWND wnd )
		: hwnd_( wnd ), created_( false ) {}

	~Caret()
		{ Destroy(); }

	void Show()
		{ if( created_ ) ::ShowCaret( hwnd_ ); }

	void Hide()
		{ if( created_ ) ::HideCaret( hwnd_ ); }

	void Destroy()
		{ if( created_ ) ::DestroyCaret(), created_=false; }

	void SetPos( int x, int y )
		{ if( created_ ) ::SetCaretPos(x,y), ki::ime().SetPos(hwnd_,x,y); }

	void Create( int H, int W, const LOGFONT& lf )
		{
			if( created_ )
				::DestroyCaret();
			created_ = true;
			::CreateCaret( hwnd_, NULL, W, H );
			ki::ime().SetFont( hwnd_, lf );
			Show();
		}
	bool isAlive()
		{ return created_; }

	HWND hwnd()
		{ return hwnd_; }

private:
	const HWND hwnd_;
	bool    created_;
};


//=========================================================================
//@{
//	cursor
//@}
//=========================================================================
class Cursor
{
public:

	// Initialization etc.
	Cursor( HWND wnd, ViewImpl& vw, doc::Document& dc );
	~Cursor();
	void AddHandler( CurEvHandler* ev );
	void DelHandler( const CurEvHandler* ev );

	// Move cursor
	void MoveCur( const DPos& dp, bool select );

	// Cursor movement using keys
	void Left( bool wide, bool select );
	void Right( bool wide, bool select );
	void Up( bool wide, bool select );
	void Down( bool wide, bool select );
	void Home( bool wide, bool select );
	void End( bool wide, bool select );
	void PageUp( bool select );
	void PageDown( bool select );
	void GotoMatchingBrace( bool select );

	// text rewriting
	void Input( const unicode* str, size_t len );
	void Input( const char* str, size_t len );
	void InputAt( const unicode *str, size_t len, int x, int y );
	void InputAt( const char *str, size_t len, int x, int y );
	void InputChar( unicode ch );
	void InputUTF32( qbyte utf32 );
	void Del(bool wide);
	void DelBack(bool wide);
	void DelToEndline( bool wide );
	void DelToStartline( bool wide );
	void Return();
	void Tabulation(bool shift);
	void QuoteSelectionW(const unicode *qs, bool shift);
	void QuoteSelection(bool unquote);

	// clipboard
	void Cut();
	void Copy();
	void Paste();

	// Get selected text
	ki::aarr<unicode> getSelectedStr() const;

	// Mode switching
	void SetInsMode( bool bIns );
	void SetROMode( bool bRO );

	// More Edit
	typedef unicode *(WINAPI *ModProc)(unicode* str, size_t *len, LPARAM param);
	static unicode* WINAPI InvertCaseW(unicode *, size_t *len, LPARAM param);
	static unicode* WINAPI UpperCaseW(unicode *, size_t *len, LPARAM param);
	static unicode* WINAPI LowerCaseW(unicode *, size_t *len, LPARAM param);
	static unicode* WINAPI TrimTrailingSpacesW(unicode *, size_t *len, LPARAM param);
	static unicode* WINAPI StripLastCharsW(unicode *, size_t *len, LPARAM param);
	static unicode* WINAPI ASCIIOnlyW(unicode *, size_t *len, LPARAM param);
	static unicode* WINAPI UnicodeNormalizeW(unicode *str, size_t *lenp, LPARAM param);
	void ModSelection(ModProc funk, LPARAM lp=0);
	void UpperCaseSel();
	void LowerCaseSel();
	void InvertCaseSel();
	void TitleCaseSel();
	void TTSpacesSel();
	void StripFirstChar();
	void StripLastChar();
	void ASCIIFy();
	void UnicodeNormalize(int mode);
	// IME
	void Reconv();
	void ToggleIME();

public:

	bool isInsMode() const;
	bool isROMode() const;
	bool isSelected() const;
	bool isInSelection(const VPos &vp) const;
	bool getCurPos( const VPos** start, const VPos** end ) const;
	bool getCurPosUnordered( const VPos** cur, const VPos** sel ) const;
	void ResetPos();
	void on_scroll_begin();
	void on_scroll_end();
	void on_text_update( const DPos& s, const DPos& e, const DPos& e2, bool mCur );
	void on_setfocus();
	void on_killfocus();
	void on_keydown( int vk, LPARAM flag );
	void on_char( TCHAR ch );
	void on_ime_composition( LPARAM lp );
	void on_lbutton_down( short x, short y, bool shift );
	bool on_drag_start( short x, short y );
	void on_mouse_move( short x, short y, WPARAM fwKeys );
	void on_lbutton_up( short x, short y );
	void on_lbutton_dbl( short x, short y );
	bool on_contextmenu( short x, short y );
	void on_timer();
	int  on_ime_reconvertstring( RECONVERTSTRING* rs );
	bool on_ime_confirmreconvertstring( RECONVERTSTRING* rs );
	void on_inputlangchange( HKL hkl );

private:

	ViewImpl&       view_;
	doc::Document&   doc_;
	CurEvHandler*   pEvHan_;
	Caret           caret_;
#ifndef NO_OLEDNDTAR
	OleDnDTarget    dndtg_;
#endif
	VPos cur_;  // cursor position
	VPos sel_;  // Pivot position when selected
	bool bIns_; // Insert mode? , Insertion mode?
	bool bRO_;  // Read only? , Read Only?
	bool lineSelectMode_; // Row selection mode?
//	bool squareSelect_; // Select in square AA mode

	UINT_PTR timerID_;// for mouse drag control
	int  keyRepTime_; // Timer related
	int  dragX_;      // position
	int  dragY_;      // position
	DWORD lastDblClkTick_;
	ulong lastDblClkLine_;
	short lastDblClkX_;
	short lastDblClkY_;
	bool hasLastDblClk_;

	UINT inputCP_;    // Current keyboard codepage

	CurEvHandler defaultHandler_;

private:

	void MoveTo( const VPos& vp, bool sel );
	void MoveByMouse( int x, int y );
	void Ud( int dy, bool select );
	void UpdateCaretPos();
	void Redraw( const VPos& s, const VPos& e );
};



//-------------------------------------------------------------------------

inline bool Cursor::isSelected() const
	{ return cur_!=sel_; }

inline bool Cursor::isInsMode() const
	{ return bIns_; }

inline bool Cursor::isROMode() const
	{ return bRO_; }

inline bool Cursor::isInSelection(const VPos &vp) const
	{ return Min(cur_, sel_) <= vp && vp < Max(cur_,sel_); }


//inline void Cursor::on_input_lang_change(int cp, HKL hkl)
//	{ kbcp_ = cp; hkl_ = hkl; }

//=========================================================================

}}    // namespace editwing::view
#endif // _EDITWING_VIEW_H_
