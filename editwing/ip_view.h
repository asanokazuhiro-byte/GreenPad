#ifndef _EDITWING_IP_VIEW_H_
#define _EDITWING_IP_VIEW_H_
#include "ewView.h"
#include "ip_doc.h"
#ifndef __ccdoc__
namespace editwing {
namespace view {
#endif

#ifdef SHORT_TABLEWIDTH
#define CW_INTTYPE uchar
#else
#define CW_INTTYPE int
#endif

class View;

//=========================================================================
//@{ @pkg editwing.View.Impl //@}
//@{
//	Basic drawing routine
//
//	To use it, use getPainter from the Canvas object.
//	This is a layer of device context for screens. If you write it properly, you can print it out.
//	It may be easier when adding, but I don't have the planning ability to think about such things.
//	It shouldn't have happened, but it was very appropriate...
//@}
//=========================================================================
class Document;
class Painter
{
public:

	~Painter() {  Destroy(); }

	//@{ Output a single character at the specified position //@}
	void CharOut( unicode ch, int x, int y );

	//@{ Output a string at the specified position //@}
	void StringOut( const unicode* str, int len, int x, int y );
	void StringOutA( const char* str, int len, int x, int y );

	void DrawCTLs( const unicode* str, int len, int x, int y );

	//@{ text color switching, text color switching //@}
	void SetColor( int i );

	//@{ Fill rect with background color //@}
	void Fill( const RECT& rc );

	//@{ Invert, Invert color in the rectangle //@}
	void Invert( const RECT& rc );

	//@{ draw a line, Draw a line from point x to point y //@}
	void DrawLine( int x1, int y1, int x2, int y2 );

	//@{Clip region settings, (IntersectClipRect()) //@}
	void SetClip( const RECT& rc );

	//@{Remove clip region, SelectClipRgn( dc_, NULL)//@}
	void ClearClip();

	//@{ Symbol drawing for half-width space //@}
	void DrawHSP( int x, int y, int times );

	//@{ Symbol drawing for full-width spaces //@}
	void DrawZSP( int x, int y, int times );

	void SetupDC(HDC hdc);
	void RestoreDC();

public:

	//@{ height, height(pixel) //@}
	CW_INTTYPE H() const { return height_; }

	//@{ digit width, digit width (pixel) //@}
	CW_INTTYPE F() const { return figWidth_; }

	//@{ character width, character width (pixel) //@}
	CW_INTTYPE Wc( const unicode ch ) const
	{ // Directly return the character width!
	  // You must have initialized it before...
		return widthTable_[ ch ];
	}
	CW_INTTYPE W( const unicode* pch ) const // 1.08 Avoid Surrogate Pair
	{
		unicode ch = *pch;
		if( widthTable_[ ch ] == (CW_INTTYPE)-1 )
		{
			if( isHighSurrogate(ch) )
			{	// We cannot save the width of chars from the extended
				// Unicode plane inside the widthTable_[ ]
				// We could in the future increase the widthTable_[ ]
				// For each extended plane that will have to be mapped.
				SIZE sz;
				if( isLowSurrogate(pch[1])
				&&::GetTextExtentPoint32W( cdc_, pch, 2, &sz )
				){
					return (CW_INTTYPE)sz.cx; // Valid surrogate pair.
				}
				// Not a proper surrogate pair fallback to ?? (fast)
				return 2 * widthTable_[ L'?' ];
			}
			{ // Windows 9x/NT
				if( isInFontRange( ch ) )
				{
					#ifndef SHORT_TABLEWIDTH
					::GetCharWidthW( cdc_, ch, ch, widthTable_+ch );
					#else
					int width=0;
					::GetCharWidthW( cdc_, ch, ch, &width );
					widthTable_[ch] = (CW_INTTYPE)width;
					#endif
				}
				else
				{	// Use GetTextExtentPointW when cdc_ defaults to fallback font
					// TODO: Even beter, get the fallback font and use its char width.
					SIZE sz;
					::GetTextExtentPointW( cdc_, &ch, 1, &sz );
					widthTable_[ ch ] = (CW_INTTYPE)sz.cx;
				}
			}
		}
		return widthTable_[ ch ];
	}

	//@{ standard character width, standard character width (pixel) //@}
	CW_INTTYPE W() const { return widthTable_[ L'x' ]; }


	//@{ Is the character is in the selected font? //@}
	bool isInFontRange( const unicode ch ) const
	{
		if( fontranges_ )
		{
			const WCRANGE *range = fontranges_->ranges;
			for(uint i=0; i < fontranges_->cRanges; i++)
			{
				if( range[i].wcLow <= ch && ch <= range[i].wcLow + range[i].cGlyphs)
					return true;
			}
			return false;
		}
		// If we have no font range then we always use GetCharWidthW.
		return true;
	}

	//@{ Calculate next tab alignment position, //@}
	//int nextTab(int x) const { int t=T(); return (x/t+1)*t; }
	inline int nextTab(int x) const { int t=(int)T(); return ((x+4)/t+1)*t; }
	private: CW_INTTYPE T() const { return widthTable_[ L'\t' ]; } public:

	//@{Current font information, Current font information //@}
	const LOGFONT& LogFont() const { return logfont_; }

	//@{ Whether to draw special characters or not //@}
	bool sc( int i ) const { return 0 != (scDraw_ & (1u << i)); }

private:

	const HWND   hwnd_;// Window in which we paint
	HDC          dc_;  // Device context used for Painting (non const)
	HDC          cdc_; // Compatible DC used for W() (const)
	HFONT        font_;
	HPEN         pen_;
	HBRUSH       brush_;
	HFONT  oldfont_;   // Old objects to be released before
	HPEN   oldpen_;    // the EndPaint() call.
	HBRUSH oldbrush_;  //
	CW_INTTYPE*  const widthTable_; // int or short [65535] values
	CW_INTTYPE   height_;
	CW_INTTYPE   figWidth_;
	LOGFONT      logfont_;
	GLYPHSET     *fontranges_;
	COLORREF     colorTable_[7];
	byte         scDraw_;

private:

	Painter( HWND hwnd, const VConfig& vc );
	void Init( const VConfig& vc );
	void Destroy();
	HFONT init_font( const VConfig& vc );
	HWND getWHND() { return hwnd_; }
	friend class Canvas;
	NOCOPY(Painter);
};



//=========================================================================
//@{
//	drawable area
//
//	Change window size, wrap or not, font settings, etc.
//	Correspondingly, manage the size of the drawing area appropriately. What to do
//	That's it for now.
//@}
//=========================================================================

class Canvas
{
public:

	Canvas( const View& vw );
	~Canvas();
	//@{ View resize event handling
	//	 @return true if wrapping width changes //@}
	bool on_view_resize( int cx, int cy );

	//@{ Row count change event processing
	//	 @return true if the width of the text area changes //@}
	bool on_tln_change( ulong tln );

	//@{ Font change event handling //@}
	void on_font_change( const VConfig& vc );

	//@{ Setting change event processing //@}
	void on_config_change( short wrap, bool showln, bool warpSmart );

	void on_config_change_nocalc( short wrap, bool showln, bool warpSmart );

public:

	//@{ [Whether to display line numbers] //@}
	bool showLN() const { return showLN_; }

	//@{ [-1: No wrapping 0: Right edge of window else: Specified number of characters] //@}
	int wrapType() const { return wrapType_; }

	bool wrapSmart() const { return warpSmart_; }

	//@{ Wrapping width (pixel) //@}
	ulong wrapWidth() const { return wrapWidth_; }

	//@{ Display area position (pixel) //@}
	const RECT& zone() const { return txtZone_; }

private:

	short wrapType_;  // [ -1: No wrapping 0: Right edge of window else: Specified number of characters ]
	bool  warpSmart_; // [ Enable wrapping at word boundaries ]
	bool  showLN_;    // [Whether to display line numbers]

public:
	Painter       font_; // drawing object
private:
	ulong    wrapWidth_; // Folding width (pixel)
	RECT       txtZone_; // Position of text display area (pixel)
	int         figNum_; // Number of digits in line number

private:

	bool CalcLNAreaWidth();
	void CalcWrapWidth();

private:

	NOCOPY(Canvas);
};



//=========================================================================
//@{
//	Wrapping information per line
//@}
//=========================================================================

struct WLine: public ki::sstorage<ulong, false>
{
	// [0]: Stores the width of the line without wrapping
	// [1-n]: Stores the index of the end of the nth line.
	//
	//   For example, if you fold the logical line "aaabbb" into "aaab" "bb"
	//   This results in an array of length 3, such as {48, 4, 6}.

	ulong& width()      { return (*this)[0]; }
	ulong width() const { return (*this)[0]; }
	ulong rln() const   { return (*this).size()-1; }
};



//=========================================================================
//@{
//	Flag for specifying redraw range
//@}
//=========================================================================

enum ReDrawType
{
	LNAREA, // Line number zone only
	LINE,   // Only one line changed
	AFTER,  // Everything below the changed line
	ALL     // full screen
};



//=========================================================================
//@{
//	Structure that specifies drawing processing in detail
//@}
//=========================================================================

struct VDrawInfo
{
	const RECT rc;  // redraw range, redraw range
	int XBASE;      // x-coordinate of the leftmost character
	int XMIN;       // left edge of text redraw range
	int XMAX;       // Right edge of text redraw range
	int YMIN;       // Top edge of text redraw range
	int YMAX;       // Bottom edge of text redraw range
	ulong TLMIN;    // Logical line number at the top of the text redraw range
	int SXB, SXE;   // x-coordinate of selection
	int SYB, SYE;   // y-coordinate of selection, y-coordinate of selection

	explicit VDrawInfo( const RECT& r )
		: rc(r), XBASE(0), XMIN(0),XMAX(0), YMIN(0),YMAX(0)
		, TLMIN(0), SXB(0),SXE(0), SYB(0),SYE(0) {}
};



//=========================================================================
//@{
//	Management and display of wrapped ed text, etc.
//
//	The text area size is calculated by the Canvas class as a reference.
//	Execute wrap-around processing. Here, scroll control, drawing processing, etc.
//	All major processing will be executed.
//@}
//=========================================================================

class ViewImpl
{
public:

	ViewImpl( View& vw, doc::Document &dc );

	//@{ Switch loopback method //@}
	inline void SetWrapType( short wt )
		{ cvs_.on_config_change( wt, cvs_.showLN(), cvs_.wrapSmart() );
		  DoConfigChange(); }

	inline void SetWrapSmart( bool ws )
		{ cvs_.on_config_change( cvs_.wrapType(), cvs_.showLN(), ws );
		  DoConfigChange(); }


	//@{Show/hide line number //@}
	inline void ShowLineNo( bool show )
		{ cvs_.on_config_change( cvs_.wrapType(), show, cvs_.wrapSmart() ); DoConfigChange(); }

	//@{ Display color/font switching //@}
	inline void SetFont( const VConfig& vcc, short zoom )
	{
		VConfig vc = vcc;
		vc.fontsize = vc.fontsize * zoom / 100;
		vc.fontsize = vc.fontsize < 1 ? 1 : vc.fontsize;
		cvs_.on_font_change( vc );
		cur_.on_setfocus();
		CalcEveryLineWidth(); // Recalculate line width
		DoConfigChange();
	}

	//@{ All of the above in one go //@}
	inline void SetWrapLNandFont( short wt, bool ws, bool showLN, const VConfig& vc, short zoom )
		{ cvs_.on_config_change_nocalc( wt, showLN, ws );
		  SetFont( vc, zoom ); }

	//@{Text area resize event //@}
	inline void on_view_resize( int cx, int cy ) { DoResize( cvs_.on_view_resize( cx, cy ) ); }

	void DoResize( bool wrapWidthChanged );
	void DoConfigChange();

	//@{Text data update event //@}
	void on_text_update( const DPos& s,
		const DPos& e, const DPos& e2, bool bAft, bool mCur );

	//@{Drawing process //@}
	void on_paint( const PAINTSTRUCT& ps );

public:

	//@{ Total number of displayed lines //@}
	ulong vln() const { return vlNum_; }

	//@{ Number of displayed lines per line //@}
	ulong rln( ulong tl ) const { return wrap_[tl].rln(); }

	//@{ wrap position //@}
	ulong rlend( ulong tl, ulong rl ) const { return wrap_[tl][rl+1]; }

	//@{ Whether there is at least one wrapping //@}
	bool wrapexists() const { return doc_.tln() != vln(); }

	//@{ cursor //@}
	Cursor& cur() { return cur_; }

	//@{ font //@}
	const Painter& fnt() const { return cvs_.font_; }


	void on_hscroll( int code, int pos );
	void on_vscroll( int code, int pos );
	void on_wheel( short delta );
	void on_hwheel( short delta );

	void GetVPos( int x, int y, VPos* vp, bool linemode=false ) const;
	inline void GetOrigin( int* x, int* y ) const
		{ *x = left()-rlScr_.nPos, *y = -udScr_.nPos*cvs_.font_.H(); }
	void ConvDPosToVPos( DPos dp, VPos* vp, const VPos* base=NULL ) const;
	void ScrollTo( const VPos& vp );
	int  GetLastWidth( ulong tl ) const;
	int  getNumScrollLines( void );
	int  getNumScrollRaws( void );

public:

	inline const RECT& zone() const { return cvs_.zone(); }
	inline int left()  const { return cvs_.zone().left; }
	inline int right() const { return cvs_.zone().right; }
	inline int bottom()const { return cvs_.zone().bottom; }
	inline int lna()   const { return cvs_.zone().left; }
	inline int cx()    const { return cvs_.zone().right - cvs_.zone().left; }
	inline int cxAll() const { return cvs_.zone().right; }
	inline int cy()    const { return cvs_.zone().bottom; }

private:

	const doc::Document&   doc_;
	Canvas           cvs_;
	Cursor           cur_;
	ki::gapbufobjnoref<WLine> wrap_;
	ulong            vlNum_;
	ulong            textCx_;
	short            accdelta_;
	short            accdeltax_;

private:

	void DrawLNA( const VDrawInfo& v, Painter& p );
	void DrawTXT( const VDrawInfo& v, Painter& p );
	void Inv( int y, int xb, int xe, Painter& p );

	void CalcEveryLineWidth();
	ulong CalcLineWidth( const unicode* txt, ulong len ) const;
	bool isSpaceLike(unicode ch) const A_XPURE;
	void ModifyWrapInfo( const unicode* txt, ulong len, WLine& wl, ulong stt );
	void ReWrapAll();
	int ReWrapSingle( const DPos& s );
	int InsertMulti( ulong ti_s, ulong ti_e );
	int DeleteMulti( ulong ti_s, ulong ti_e );
	void UpdateTextCx();
	void ReDraw( ReDrawType r, const DPos* s=NULL );

private:

	HWND hwnd_;
	SCROLLINFO rlScr_; // Horizontal scroll information (in pixels)
	SCROLLINFO udScr_; // Vertical scroll information (line by line)
	ulong udScr_tl_;   // TLine_Index of the logical line displayed at the top
	ulong udScr_vrl_;  // VRLine_Index of the top visible line

private:

	bool ReSetScrollInfo();
	void ForceScrollTo( ulong tl );
	void UpdateScrollBar();
	ReDrawType TextUpdate_ScrollBar( const DPos& s, const DPos& e, const DPos& e2 );

	ulong tl2vl( ulong tl ) const;
	void GetDrawPosInfo( VDrawInfo& v ) const;
	void InvalidateView( const DPos& dp, bool afterall ) const;
	void ScrollView( int dx, int dy, bool update );
	void UpDown( int dy, bool thumb );
};



//=========================================================================

//=========================================================================
//@{ @pkg editwing.View //@}
//@{
//	drawing processing etc.
//
//	This class only performs message distribution and the implementation is
//	This is done using Canvas/ViewImpl etc. So please refer there for details.
//@}
//=========================================================================

class View A_FINAL: public ki::WndImpl, public doc::DocEvHandler
{
public:

	//@{ Constructor that does nothing //@}
	View( doc::Document& d, HWND wnd );
	~View();

	//@{ Switch loopback method //@}
	void SetWrapType( short wt );

	void SetWrapSmart( bool ws);

	//@{Show/hide line number //@}
	void ShowLineNo( bool show );

	//@{ Display color/font switching //@}
	void SetFont( const VConfig& vc, short zoom );

	//@{ Set all canva stuff at once (faster) //@}
	void SetWrapLNandFont( short wt, bool ws, bool showLN, const VConfig& vc, short zoom );

	//@{ cursor //@}
	Cursor& cur();

private:

	doc::Document&      doc_;
	ViewImpl*           impl_;
	static ClsName     className_;

private:

	void    on_create( CREATESTRUCT* cs ) override;
	void    on_destroy() override;
	LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp ) override;
	void    on_text_update( const DPos& s, const DPos& e, const DPos& e2, bool bAft, bool mCur ) override;
	void    on_keyword_change() override;
};


}}     // namespace editwing::view
#endif // _EDITWING_IP_VIEW_H_
