
#include "../kilib/stdafx.h"
#include "ip_view.h"
using namespace ki;
using namespace editwing;
using namespace editwing::view;



//=========================================================================
//---- ip_scroll.cpp Scroll
//
//		Window size depends on scrollbar position
//		Here is the process to update the drawing position appropriately.
//
//---- ip_text.cpp String manipulation/etc.
//---- ip_parse.cpp Keyword parsing
//---- ip_wrap.cpp wrapping
//---- ip_draw.cpp Drawing/etc.
//---- ip_cursor.cpp Cursor control
//=========================================================================

#define MyGetScrollInfo GetScrollInfo
#define MySetScrollInfo SetScrollInfo

//-------------------------------------------------------------------------
// Drawing area size management
//-------------------------------------------------------------------------

namespace
{
	static int Log10( ulong n )
	{
		// return Max( 3, Log10(n) + 1 );
		ulong p = 1000;
		int i;
		for( i = 3; i < ULONG_DIGITS; i++ )
		{
			if( n < p ) return i;
			p *= 10;
		}
		return i;
	}
}

bool Canvas::CalcLNAreaWidth()
{
	const int prev = txtZone_.left;
	if( showLN_ )
	{
		txtZone_.left  = (1 + figNum_) * font_.F();
		if( txtZone_.left+4*font_.W() >= txtZone_.right )
			txtZone_.left = 0; // Do not display if the line number zone is too large
	}
	else
	{
		txtZone_.left = 0;
	}

	return (prev != txtZone_.left);
}

void Canvas::CalcWrapWidth()
{
	switch( wrapType_ )
	{
	case NOWRAP:
		wrapWidth_ = 0xffffffff;
		break;
	case RIGHTEDGE:
		wrapWidth_ = txtZone_.right - txtZone_.left - font_.W()/2 - 1;
		break; //Caret Min-3 Correction
	default:
		wrapWidth_ = wrapType_ * font_.W();
		break;
	}
}

Canvas::Canvas( const View& vw )
	: wrapType_ ( -1 )
	, warpSmart_( false )
	, showLN_   ( false )
	, font_     (  vw.hwnd(), VConfig(TEXT(""),0) )
	, wrapWidth_( 0xffffffff )
	, figNum_   ( 3 )
{
	vw.getClientRect( &txtZone_ );
}

Canvas::~Canvas()
{
	//delete font_;
}

// Return true if warp width has changed.
bool Canvas::on_view_resize( int cx, int cy )
{
	txtZone_.bottom = cy; // Update canva height
	if( txtZone_.right  == cx )
		return false; // Same canvas width, nothing more to do.

	txtZone_.right  = cx; // Canva width was changed.
	CalcLNAreaWidth(); // Is width is too small line number diseapear.
	if( wrapType_ == RIGHTEDGE )
	{
		CalcWrapWidth();
		return true;
	}
	return false; // No rewrapping to do
}

void Canvas::on_font_change( const VConfig& vc )
{
	//HWND hwnd = font_->getWHND();
	//delete font_; // Must call destructor first...
	              // Wow, that's so cool (T_T)
	//font_ = new Painter( hwnd, vc );
	font_.Destroy();
	font_.Init(vc);

	CalcLNAreaWidth();
	CalcWrapWidth();
}

void Canvas::on_config_change( short wrap, bool showln, bool wrapsmart )
{
	showLN_ = showln;
	wrapType_ = wrap;
	warpSmart_ = wrapsmart;

	CalcLNAreaWidth();
	CalcWrapWidth();
}

void Canvas::on_config_change_nocalc( short wrap, bool showln, bool wrapsmart )
{
	showLN_ = showln;
	wrapType_ = wrap;
	warpSmart_ = wrapsmart;
}

bool Canvas::on_tln_change( ulong tln )
{
	figNum_ = Log10( tln ); // Digit calculation

	if( CalcLNAreaWidth() )
	{
		if( wrapType_ == RIGHTEDGE )
			CalcWrapWidth();
		return true;
	}
	return false;
}



//-------------------------------------------------------------------------
// Scrollbar calculation routine
//-------------------------------------------------------------------------
// rl (horizontal scroll information)
// max:  view.txt.txtwidth()
// page: view.cx()
// pos:  0～max-page

// ud (vertical scroll information)
// max:   view.txt.vln() + page - 1
// page:  view.cy() / view.fnt.H()
// delta: 0～view.fnt.H()
// pos: 0~max-page (top line number)

bool ViewImpl::ReSetScrollInfo()
{
	const int prevRlPos = rlScr_.nPos;
	const ulong cx = cvs_.zone().right - cvs_.zone().left;
	const ulong cy = cvs_.zone().bottom;

	// You only need to correct the horizontal values ​​so that they do not become strange values.
//	rlScr_.nPage = cx + 1;
//	rlScr_.nMax  = Max( textCx_, cx );
//	rlScr_.nPos  = Min<int>( rlScr_.nPos, rlScr_.nMax-rlScr_.nPage+1 );
	rlScr_.nPage = cx + 1;
	rlScr_.nMax  = Max( textCx_+cvs_.font_.W()/2+1, cx );
	rlScr_.nPos  = Min( rlScr_.nPos, (int)(rlScr_.nMax-rlScr_.nPage+1) );

	// For the vertical, nPage and nMax are corrected for now.
	// The way to fix nPos differs depending on the case, so use a separate routine.
	udScr_.nPage = cy / NZero(cvs_.font_.H()) + 1;
	//udScr_.nMax  = vln() + udScr_.nPage - 2; // Old code (not so nice)

	// WIP: Adjust so that scroll bar appears only when we are shy oneline from the page.
	// PB: Drawing problem when adding/removing line and scroll bar did not kick in.
	// We need to do a proper InvalidateRect in the TextUpdate_ScrollBar()?
	// udScr_.nMax  = vln() + udScr_.nPage - Max( 2, Min<int>( udScr_.nPage-1, vln()+1 ) );

	// Limit more scrolling when there is more tha a single page
	if( vln()*cvs_.font_.H() < cy )
		udScr_.nMax  = vln() + udScr_.nPage - 2;
	else
		udScr_.nMax  = vln() + udScr_.nPage - Max( 2, (int)Min( udScr_.nPage-1, (uint)(vln()+1) ) );

	// true if horizontal scrolling occurs
	return (prevRlPos != rlScr_.nPos);
}

void ViewImpl::ForceScrollTo( ulong tl )
{
	udScr_.nPos = tl2vl(tl);
	udScr_tl_   = tl;
	udScr_vrl_  = 0;
}

ulong ViewImpl::tl2vl( ulong tl ) const
{
	if( vln() == doc_.tln() )
		return tl;

	ulong vl=0;
	for( ulong i=0; i<tl; ++i )
		vl += rln(i);
	return vl;
}

void ViewImpl::UpdateScrollBar()
{
	::MySetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
	::MySetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
}

ReDrawType ViewImpl::TextUpdate_ScrollBar
	( const DPos& s, const DPos& e, const DPos& e2 )
{
	const ulong prevUdMax = udScr_.nMax;
	const bool rlScrolled = ReSetScrollInfo();
	const long vl_dif = (udScr_.nMax - prevUdMax);
	ReDrawType ans =
		(vl_dif!=0 || s.tl!=e2.tl ? AFTER : LINE);

	if( udScr_tl_ < s.tl )
	{
		// Pattern 1: When updated below the top of the current screen
		// don't scroll
	}
	else if( udScr_tl_ == s.tl )
	{
		// Pattern 2: When updated on the same line as the top of the current screen
		// Try to keep displaying the same location as much as possible.

		if( static_cast<ulong>(udScr_.nPos) >= vln() )
		{
			// Pattern 2-1: But it's already below EOF!
			// I can't help it, so display the bottom line
			udScr_.nPos = vln()-1;
			udScr_tl_   = doc_.tln()-1;
			udScr_vrl_  = rln(udScr_tl_)-1;
			ans = ALL;
		}
		else
		{

			// Pattern 2-2:
			// No scrolling
			while( udScr_vrl_ >= rln(udScr_tl_) )
			{
				udScr_vrl_ -= rln(udScr_tl_);
				udScr_tl_++;
			}
		}
	}
	else
	{
		// Pattern 3: When updated above the top of the current screen
		// Try not to change the displayed content

		if( e.tl < udScr_tl_ )
		{
			// Pattern 3-1: When the end of the change range is also above the current line
			// The line number will change, but the displayed content will remain the same.
			udScr_.nPos += vl_dif;
			udScr_tl_   += (e2.tl - e.tl);
			ans = LNAREA;
		}
		else
		{
			// Pattern 3-2:
			// There's nothing I can do, so I scroll to an appropriate position.
			ForceScrollTo( e2.tl );
			ans = ALL;
		}
	}

	// Returns what kind of redrawing should be done
	return (rlScrolled ? ALL : ans);
}

void ViewImpl::ScrollTo( const VPos& vp )
{
	// horizontal focus
	int dx=0;
	if( vp.vx < (signed)rlScr_.nPos )
	{
		dx = vp.vx - rlScr_.nPos;
	}
	else
	{
		const int W = cvs_.font_.W();
		if( rlScr_.nPos + (signed)(rlScr_.nPage-W) <= vp.vx )
			dx = vp.vx - (rlScr_.nPos + rlScr_.nPage) + W;
	}

	// vertical focus
	int dy=0;
	if( vp.vl < (unsigned)udScr_.nPos )
		dy = vp.vl - udScr_.nPos;
	else if( udScr_.nPos + (udScr_.nPage-1) <= vp.vl )
		dy = vp.vl - (udScr_.nPos + udScr_.nPage) + 2;

	// scroll
	if( dy!=0 )	UpDown( dy, dx==0 );
	if( dx!=0 )	ScrollView( dx, 0, true );
}

void ViewImpl::GetDrawPosInfo( VDrawInfo& v ) const
{
	const int H = cvs_.font_.H();

	long most_under = (vln()-udScr_.nPos)*H;
	if( most_under <= v.rc.top )
	{
		v.YMIN  = v.rc.top;
		v.YMAX  = most_under;
	}
	else
	{
		int    y = -(signed)udScr_vrl_;
		ulong tl = udScr_tl_;
		int  top = v.rc.top / NZero(H);
		while( y + (signed)rln(tl) <= top )
			y += rln( tl++ );

		// vertical coordinates
		v.YMIN  = y * H;
		v.YMAX  = Min( v.rc.bottom, most_under );
		v.TLMIN = tl;

		// horizontal coordinates
		v.XBASE = left() - rlScr_.nPos;
		v.XMIN  = v.rc.left  - v.XBASE;
		v.XMAX  = v.rc.right - v.XBASE;

		// selection range, selection range
		v.SXB = v.SXE = v.SYB = v.SYE = 0x7fffffff;

		const VPos *bg, *ed;
		if( cur_.getCurPos( &bg, &ed ) )
		{
			v.SXB = bg->vx - rlScr_.nPos + left();
			v.SXE = ed->vx - rlScr_.nPos + left();
			v.SYB = (bg->vl - udScr_.nPos) * H;
			v.SYE = (ed->vl - udScr_.nPos) * H;
		}
	}
}

void ViewImpl::ScrollView( int dx, int dy, bool update )
{
	// Scroll notification start, Scroll notification start
	cur_.on_scroll_begin();

	const RECT* clip = (dy==0 ? &cvs_.zone() : NULL);
	const int H = cvs_.font_.H();

	// Scrollbar updated, Scrollbar updated
	if( dx != 0 )
	{
		// range check, range check
		if( rlScr_.nPos+dx < 0 )
			dx = -rlScr_.nPos;
		else if( rlScr_.nMax-(signed)rlScr_.nPage < rlScr_.nPos+dx )
			dx = rlScr_.nMax-rlScr_.nPage-rlScr_.nPos+1;

		rlScr_.nPos += dx;
		::MySetScrollInfo( hwnd_, SB_HORZ, &rlScr_, TRUE );
		dx = -dx;
	}
	if( dy != 0 )
	{
		// Range check... is finished with preprocessing.
		udScr_.nPos += dy;
		::MySetScrollInfo( hwnd_, SB_VERT, &udScr_, TRUE );
		dy *= -H;
	}
	if( dx!=0 || dy!=0 )
	{
		if( -dx>=right() || dx>=right()
		 || -dy>=bottom() || dy>=bottom() )
		{
			// Full screen redraw
			// When I scrolled exactly a multiple of 65536,
			::InvalidateRect( hwnd_, NULL, FALSE );
		}
		else
		{
			// Scroll areas that do not need to be redrawn
			// Scroll through areas that do not need redrawing
			::ScrollWindowEx( hwnd_, dx, dy, NULL,
					clip, NULL, NULL, SW_INVALIDATE );

			// Instant redraw? , Immediate redraw?
			if( update )
			{
				// Vertical scrolling is one way to speed up the process.
				if( dy != 0 )
				{
					// Calculate the areas that need redrawing yourself
					// Calculate the area that needs to be redrawn
					RECT rc = { 0, 0, right(), bottom() };
					if( dy < 0 ) rc.top  = rc.bottom + dy;
					else         rc.bottom = dy;

					// by IntelliMouse middle button click
					// Draw the part below the autoscroll cursor first
					// By dividing it into two parts, only two small rectangular parts are required, making it faster.
					// By clicking the middle button of the IntelliMouse
					// Draw the area under the cursor for auto scrolling first.
					// By splitting it into two parts,
					// only two small rectangles are needed, which is faster.
					::ValidateRect( hwnd_, &rc );
					//::UpdateWindow( hwnd_ );
					::InvalidateRect( hwnd_, &rc, FALSE );
				}
				// RAMON: I much prefer to have a slightly sloppy drawing
				// on slow computers rather than have my scroll messages pile up
				// and keep scrolling the window when I stopped scrolling.
				// Reactivity is more important than perfect drawing.
				// This is why I commented out the ::UpdateWindow() calls
				//::UpdateWindow( hwnd_ );
			}
		}
	}

	// Scroll end notification
	cur_.on_scroll_end();
}

void ViewImpl::on_hscroll( int code, int pos )
{
	// Calculate the amount of change
	int dx;
	switch( code )
	{
	default:           return;
	case SB_LINELEFT:  dx= -cvs_.font_.W(); break;
	case SB_LINERIGHT: dx= +cvs_.font_.W(); break;
	case SB_PAGELEFT:  dx= -(cx()>>1); break;
	case SB_PAGERIGHT: dx= +(cx()>>1); break;
	case SB_THUMBTRACK:
	{
		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
		si.nTrackPos = pos;
		::MyGetScrollInfo( hwnd_, SB_HORZ, &si );
		dx = si.nTrackPos - rlScr_.nPos;
		break;
	}
	case SB_LEFT:    dx = -rlScr_.nPos; break;
	case SB_RIGHT:   dx = rlScr_.nMax+1-(signed)rlScr_.nPage-rlScr_.nPos; break;
	}

	// scroll
	ScrollView( dx, 0, code!=SB_THUMBTRACK );
}

void ViewImpl::on_vscroll( int code, int pos )
{
	// Calculate the amount of change
	int dy;
	switch( code )
	{
	default:          return;
	case SB_LINEUP:   dy= -1; break;
	case SB_LINEDOWN: dy= +1; break;
	case SB_PAGEUP:   dy= -( cy() / NZero(cvs_.font_.H()) ); break;
	case SB_PAGEDOWN: dy= +( cy() / NZero(cvs_.font_.H()) ); break;
	case SB_THUMBTRACK:
	{
		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
		si.nTrackPos = pos; // in case
		::MyGetScrollInfo( hwnd_, SB_VERT, &si );
		dy = si.nTrackPos - udScr_.nPos;
		break;
	}
	case SB_TOP:      dy = -udScr_.nPos; break;
	case SB_BOTTOM:   dy = udScr_.nMax+1-(signed)udScr_.nPage-udScr_.nPos; break;
	}

	// scroll
	UpDown( dy, code==SB_THUMBTRACK );
}

int ViewImpl::getNumScrollLines( void )
{
	uint scrolllines = 3; // Number of lines to scroll (default 3).
	{   // Read the system value for the wheel scroll lines.
		UINT numlines;
		if( ::SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &numlines, 0 ) )
			scrolllines = numlines; // Sucess!
	}
	// If the number of lines is larger than a single page then we only scroll a page.
	// This automatically takes into account the page scroll mode where
	// SPI_GETWHEELSCROLLLINES value if 0xFFFFFFFF.
	uint nlpage;
	nlpage = Max( (uint)cy() / (uint)NZero(cvs_.font_.H()), 1U );
	// scrolllines can be zero, in this case no scroll should occur.
	return (int)Min( scrolllines, nlpage );
}

void ViewImpl::on_wheel( short delta )
{
	// scroll
	int nl = getNumScrollLines();
	int step = (-(int)delta * nl) / WHEEL_DELTA;

	if( step == 0 )
	{ // step is too small, we need to accumulate delta
		accdelta_ += delta;
		// Recalculate the step.
		step = (-(int)accdelta_ * nl) / WHEEL_DELTA;
		if( step )
		{
			UpDown( step, false );
			// set accumulator to the remainder.
			accdelta_ -= (-step * WHEEL_DELTA) / nl;
		}
	}
	else
	{
		UpDown( step, false );
	}
}
int ViewImpl::getNumScrollRaws( void )
{
	uint scrollnum = 3; // Number of lines to scroll (default 3).
	// TODO: Get more accurate version numbers for scroll wheel support?
	{   // Read system value for horizontal scroll chars (Vista+)
		UINT num;
		if( ::SystemParametersInfo( SPI_GETWHEELSCROLLCHARS, 0, &num, 0 ) )
			scrollnum = num; // Sucess!
	}
	// Avoid scrolling more that halfa screen horizontally.
	uint nlpage;
	nlpage = Max( (uint)cx() / (uint)NZero(cvs_.font_.W()/2), 1U );
	// scrollnum can be zero, in this case no scroll should occur.
	return (int)Min( scrollnum, nlpage );
}

void ViewImpl::on_hwheel( short delta )
{
	int nl = getNumScrollRaws();
	int dx = nl * cvs_.font_.W() * (int)delta / WHEEL_DELTA;
	if( dx == 0 )
	{ // step is too small, we need to accumulate delta
		accdeltax_ += delta;
		// Recalculate the step.
		dx = nl * cvs_.font_.W() * (int)accdeltax_ / WHEEL_DELTA;
		if( dx )
		{
			ScrollView( dx, 0, 0 );
			accdeltax_ = 0;
		}
	}
	else
	{
		ScrollView( dx, 0, 0 );
	}
}

void ViewImpl::UpDown( int dy, bool thumb )
{
  // 1. Corrected so that udScr_.nPos + dy falls within the normal range
	if( udScr_.nPos+dy < 0 )
		dy = -udScr_.nPos;
	else if( udScr_.nMax+1-(signed)udScr_.nPage < udScr_.nPos+dy )
		dy = udScr_.nMax+1-udScr_.nPage-udScr_.nPos;

	if( dy==0 )
		return;

  // 2-1. If there is no turnaround, you can jump at once.
	if( !wrapexists() )
	{
		udScr_tl_ = udScr_.nPos + dy;
	}

  // 2-2. Otherwise, search relative to current position
  // If you jump at once without dragging the ScrollBar continuously,
  // A relative search from the first or last line may be more effective, but
  // In that case, even if the speed becomes a little slower, the drawing will not get stuck, so it is OK.
	else
	{
		int   rl = dy + udScr_vrl_;
		ulong tl = udScr_tl_;

		if( dy<0 ) // Return to top
		{
			// Dash to the beginning of the jump destination logical line!
			while( rl < 0 )
				rl += rln(--tl);
		}
		else/*if( dy>0 )*/ // If you go down
		{
			// Dash to the beginning of the jump destination logical line!
			while( rl > 0 )
				rl -= rln(tl++);
			if( rl < 0 )
				rl += rln(--tl); //Excessive correction
		}
		udScr_tl_ = tl;
		udScr_vrl_= static_cast<ulong>(rl);
	}

  // 4. scroll the screen
	ScrollView( 0, dy, !thumb );
}

void ViewImpl::InvalidateView( const DPos& dp, bool afterall ) const
{
	const int H = cvs_.font_.H();

	// Update above the display area
	if( dp.tl < udScr_tl_ )
	{
		if( afterall )
			::InvalidateRect( hwnd_, NULL, FALSE );
		return;
	}

	// Start y-coordinate calculation, Start y-coordinate calculation
	int r=0, yb=-(signed)udScr_vrl_;
	for( int t=udScr_tl_, ybe=cy() / NZero(H); (unsigned)t<dp.tl; yb+=rln(t++) )
		if( yb >= ybe )
			return;
	for( ; dp.ad>rlend(dp.tl,r); ++r,++yb );
	yb = H * Max( yb, -100 ); // Adjustment to avoid overhang on top
	if( yb >= cy() )
		return;

	// Redraw the first line
	int rb = (r==0 ? 0 : rlend(dp.tl,r-1));
 	int xb = left() + Max( (ulong)0,
		CalcLineWidth(doc_.tl(dp.tl)+rb, (ulong) (dp.ad-rb)) - rlScr_.nPos );
	if( xb < right() )
	{
		RECT rc={xb,yb,right(),yb+H};
		::InvalidateRect( hwnd_, &rc, FALSE );
	}

	// remaining
	int ye;
	yb += H;
	if( afterall )
		xb=0, ye=cy();
	else
		xb=left(), ye=Min(cy(),yb+(int)(H*(rln(dp.tl)-r-1)));
	if( yb < ye )
	{
		RECT rc={xb,yb,right(),ye};
		::InvalidateRect( hwnd_, &rc, FALSE );
	}
}
