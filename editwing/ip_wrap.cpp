
#include "../kilib/stdafx.h"
#include "ip_view.h"
using namespace editwing;
using namespace editwing::view;



//=========================================================================
//---- ip_wrap.cpp wrapping
//
//		As string data is updated in Document
//		In View, update the loopback position information. Here's the process.
//
//---- ip_text.cpp String manipulation/etc.
//---- ip_parse.cpp Keyword parsing
//---- ip_scroll.cpp Scroll
//---- ip_draw.cpp Drawing/etc.
//---- ip_cursor.cpp Cursor control
//=========================================================================



//-------------------------------------------------------------------------
// Initialization
//-------------------------------------------------------------------------

ViewImpl::ViewImpl( View& vw, doc::Document& dc )
	: doc_   ( dc )
	, cvs_   ( vw )
	, cur_   ( vw.hwnd(), *this, dc )
	, vlNum_ ( 0 )
	, textCx_( 0 )
	, accdelta_  ( 0 )
	, accdeltax_ ( 0 )
	, hwnd_  ( vw.hwnd() )
{
	// Initialize return information appropriately
	InsertMulti( 0, doc_.tln()-1 );

	// Initialize scroll information appropriately
	udScr_.cbSize = rlScr_.cbSize = sizeof(udScr_);
	udScr_.fMask  = rlScr_.fMask  = SIF_PAGE | SIF_POS | SIF_RANGE;
	udScr_.fMask |= SIF_DISABLENOSCROLL;
	udScr_.nMin   = rlScr_.nMin   = 0;
	udScr_.nMax   = rlScr_.nMax   = 1;
	udScr_.nPage  = rlScr_.nPage  = 1;
	udScr_.nPos   = rlScr_.nPos   = 0;
	udScr_.nTrackPos = rlScr_.nTrackPos = 0;
	udScr_tl_     = udScr_vrl_    = 0;
	//ReSetScrollInfo();
}

//-------------------------------------------------------------------------
// Responding to state changes
//-------------------------------------------------------------------------

void ViewImpl::DoResize( bool wrapWidthChanged )
{
	// Recalculation of wrap-around position
	if( wrapWidthChanged )
	{
		ReWrapAll();
		UpdateTextCx();
	}

	// Scroll information change
	ReSetScrollInfo();
	if( wrapWidthChanged )
		ForceScrollTo( udScr_tl_ );

	// repaint
	ReDraw( ALL );
	cur_.ResetPos();
}

void ViewImpl::DoConfigChange()
{
	// Recalculation of wrap-around position
	ReWrapAll();
	UpdateTextCx();

	// Scroll information change
	ReSetScrollInfo();
	ForceScrollTo( udScr_tl_ );

	// repaint
	ReDraw( ALL );
	cur_.ResetPos();
}

void ViewImpl::on_text_update
	( const DPos& s, const DPos& e, const DPos& e2, bool bAft, bool mCur )
{
	// First, recalculate the turning position

	// Adjust first row of replacement range
	int r3 = 0, r2 = 1, r1 = ReWrapSingle( s );

	// adjust the rest
	if( s.tl != e.tl )
		r2 = DeleteMulti( s.tl+1,  e.tl );
	if( s.tl != e2.tl )
		r3 = InsertMulti( s.tl+1, e2.tl );

	// With this change, the width...
	// if( "It's not longer" AND "It's possible it's shorter" )
	//     Recalculate width();
	if( !(r1==2 || r3==1) && (r1==0 || r2==0) )
		UpdateTextCx();

	// Scroll bar fix
	ReDrawType t = TextUpdate_ScrollBar( s, e, e2 );
	bool doResize = false;

	// When the number of lines changes and you need to change the width of the line number display area
	if( e.tl!=e2.tl && cvs_.on_tln_change( doc_.tln() ) )
	{
		doResize = true;
	}
	else if( bAft && t!=ALL )
	{
		t = AFTER;
	}

	// Move cursor
	cur_.on_text_update( s, e, e2, mCur );

	// repaint
	if( doResize )
		DoResize( true );
	else
	{
		if( e.tl != e2.tl ) // When it is necessary to redraw the line number area
			ReDraw( LNAREA, 0 );
		ReDraw( t, &s );
	}
}



//-------------------------------------------------------------------------
// Wrap-around position calculation assistance routine, Wrap-around position calculation assistance routine
//-------------------------------------------------------------------------

void ViewImpl::UpdateTextCx()
{
	if( cvs_.wrapType() == NOWRAP )
	{
		// If there is no wrapping, you won't know the width unless you count it.
		ulong cx=0;
		for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
			if( cx < wrap_[i].width() )
				cx = wrap_[i].width();
		textCx_ = cx;
	}
	else
	{
		// If there is wrapping, width:=wrapping width.
		textCx_ = cvs_.wrapWidth();
	}
}

#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC optimize ("-O3")
#endif
ulong A_HOT ViewImpl::CalcLineWidth( const unicode* txt, ulong len ) const
{
	// Calculate the width when writing without wrapping lines
	// For text where most lines are displayed without wrapping,
	// By calculating this value, processing speed can be increased.
	// Calculate the width of the text when lines are written without wrapping
	// For text where most of the lines are displayed without wrapping.
	// Calculating this value can speed up the process.
	const Painter& p = cvs_.font_;

	ulong w=0;
	for( ulong i=0; i<len ; ++i )
	{
		if( txt[i] < 128 )
			if( txt[i] != L'\t' )
				w += p.Wc( txt[i] );
			else
				w = p.nextTab(w);
		else
			w += p.W( &txt[i] );
	}
	return w;
}
#ifdef __GNUC__
#pragma GCC pop_options
#endif

void ViewImpl::CalcEveryLineWidth()
{
	// Run CalcLineWidth on all lines
	// …Just do it.
	for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
		wrap_[i].width() = CalcLineWidth( doc_.tl(i), doc_.len(i) );
}

bool ViewImpl::isSpaceLike(unicode ch) const
{
	static const bool spacelike[128] = {
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0-15
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 16-31
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //  !"#$%&'()* 32-47
		0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1, // 0123456789:;<=>?
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // @ABCDEFGHIJKLMNO
		0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1, // PQRSTUVWXYZ[\]^_
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // `abcdefghijklmno
		0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1, // pqrstuvwxyz{|}~
	};
	// Use Table for ASCII character and also inlcude all C1 control
	// And also all characters beyond 0x2500 (Box Drawing and above)
	// This includes all ideographic characters and all extended planes
	return ch < 128? spacelike[ch]: ch < 0x00a0 || ch >= 0x2500 ;
}

void ViewImpl::ModifyWrapInfo(
		const unicode* txt, ulong len, WLine& wl, ulong stt )
{
	// Executes wrapping at the set width.
	// If the change is from the middle of the line, stt points to the starting address
	const Painter& p = cvs_.font_;
	const bool smart = cvs_.wrapSmart();
	const ulong   ww = smart? cvs_.wrapWidth()-p.W()/2: cvs_.wrapWidth();

	while( stt < len )
	{
		ulong i, w;
		for( w=0,i=stt; i<len; ++i )
		{
			if( txt[i] == L'\t' )
				w = p.nextTab(w);
			else
				w += p.W( &txt[i] );
			if( w > ww )
			{
				if( smart )
				{	// Prefer wrapping on words
					// Back-track to previous spacelike char.
					ulong oi = i;
					ulong ow = w;
					// If we meet a surrogate we will stop
					// and because low surrogate have a zero width
					// they will never be separated from high surrogate.
					while( !isSpaceLike( txt[i] ) && stt < i )
						w -= p.W( &txt[i--] );
					if( stt == i )
						w = ow, i = oi;
					w += p.Wc( txt[i] );
					i++;
				}
				break;
			}
		}
		wl.Add( stt = (i==stt?i+1:i) );
	}
}

int ViewImpl::GetLastWidth( ulong tl ) const
{
	if( rln(tl)==1 )
		return wrap_[tl][0];

	ulong beg = rlend(tl,rln(tl)-2);
	return CalcLineWidth( doc_.tl(tl)+beg, doc_.len(tl)-beg );
}

void ViewImpl::ReWrapAll()
{
	// If there is a change in the wrapping width, all lines
	// Change callback location information.
	// If there is a change in the wrapping width,
	// the wrapping position information for all lines is changed.
	const ulong ww = cvs_.wrapWidth();

	ulong vln=0;
	for( ulong i=0, ie=doc_.tln(); i<ie; ++i )
	{
		WLine& wl = wrap_[i];
		wl.ForceSize(1);

		if( wl.width() < ww )
		{
			// If it is shorter than the set wrapping width, one line is sufficient.
			wl.Add( doc_.len(i) );
			++vln;
		}
		else
		{
			// If there are multiple lines
			ModifyWrapInfo( doc_.tl(i), doc_.len(i), wl, 0 );
			vln += wl.rln();
		}
	}
	vlNum_ = vln;
}



//-------------------------------------------------------------------------
// Folding Position Calculation Main Routine
//-------------------------------------------------------------------------

int ViewImpl::ReWrapSingle( const DPos& s )
{
	// Correct wrapping for only one specified line.
	//
	// The return value is
	//   2: "With wrapping" or "This line is the longest horizontally"
	//   1: "Somewhere other than this line is the longest"
	//   0: "This line was the longest until now, but now it's shorter."
	// Then, inform the upper level routine of the need to modify m_TextCx.
	//
	// In the past, the change in the number of displayed lines was returned to calculate the redraw range, but
	// This can be done by comparing vln() on the upper routine side,
	// In fact, it was abolished because it was more efficient.


	// Save old info
	WLine& wl            = wrap_[s.tl];
	const ulong oldVRNum = wl.rln();
	const ulong oldWidth = wl.width();

	// Update width, Update width
	wl.width() = CalcLineWidth( doc_.tl(s.tl), doc_.len(s.tl) );

	if( wl.width() < cvs_.wrapWidth() )
	{
		// If it is shorter than the set wrapping width, one line is sufficient.
		// Only one line is needed
		wl[1] = doc_.len(s.tl);
		wl.ForceSize( 2 );
	}
	else
	{
		// If there are multiple lines, There are multiple lines
		ulong vr=1, stt=0;
		while( wl[vr] < s.ad ) // while (vr line is before the changed part)
			stt = wl[ vr++ ];  // stt = address of the start of the next line, next line stt

		// Corrected only after the point of change
		wl.ForceSize( vr );
		ModifyWrapInfo( doc_.tl(s.tl), doc_.len(s.tl), wl, stt );
	}

	// Fix total number of display rows, Fix total number of display rows
	vlNum_ += ( wl.rln() - oldVRNum );

	// If there is no wrapping, the total width needs to be updated
	// Without wrapping, total width needs to be updated
	if( cvs_.wrapType() == NOWRAP )
	{
		if( textCx_ <= wl.width() )
		{
			textCx_ = wl.width();
			return 2;
		}
		else if( textCx_ == oldWidth )
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	return 2;
}

int ViewImpl::InsertMulti( ulong ti_s, ulong ti_e )
{
	// Adds new line information for the specified amount.
	// & Calculate return information properly
	//
	// The return value is
	//   1: "There is wrapping" or "This line is the longest horizontally"
	//   0: "Somewhere other than this line is the longest"
	// See ReWrapSingle() for details.

	ulong dy=0, cx=0;
	for( ulong i=ti_s; i<=ti_e; ++i )
	{
		WLine pwl;
		pwl.Add( CalcLineWidth( doc_.tl(i), doc_.len(i) ) );

		if( pwl.width() < cvs_.wrapWidth() )
		{
			// If it is shorter than the set wrapping width, one line is sufficient.
			pwl.Add( doc_.len(i) );
			dy++;
			if( cx < pwl.width() )
				cx = pwl.width();
		}
		else
		{
			// If there are multiple lines
			ModifyWrapInfo( doc_.tl(i), doc_.len(i), pwl, 0 );
			dy += pwl.rln();
		}

		wrap_.InsertAt( i, pwl );
	}

	// Correct total number of displayed rows
	vlNum_ += dy;

	// If there is no wrapping, the total width needs to be updated
	if( cvs_.wrapType() == NOWRAP )
	{
		if( textCx_ <= cx )
		{
			textCx_ = cx;
			return 1;
		}
		return 0;
	}
	return 1;
}

int ViewImpl::DeleteMulti( ulong ti_s, ulong ti_e )
{
	// Delete line information in specified range
	//
	// The return value is
	//   1: "Wraps" or "Longest line is somewhere other than this line"
	//   0: "This line was the longest until now, but now it's shorter."
	// See ReWrapSingle() for details.

	bool  widthChanged = false;
	ulong dy = 0;

	// Delete while collecting information
	for( ulong cx=textCx_, i=ti_s; i<=ti_e; ++i )
	{
		WLine& wl = wrap_[i];
		dy += wl.rln();
		if( cx == wl.width() )
			widthChanged = true;
	}
	wrap_.RemoveAt( ti_s, (ti_e-ti_s+1) );

	// Correct total number of displayed rows
	vlNum_ -= dy;

	// If there is no wrapping, the total width needs to be updated
	return ( cvs_.wrapType()==NOWRAP && widthChanged ) ? 0 : 1;
}



//-------------------------------------------------------------------------
// Coordinate value conversion
//-------------------------------------------------------------------------

void ViewImpl::ConvDPosToVPos( DPos dp, VPos* vp, const VPos* base ) const
{
	// Corrections
	dp.tl = Min( dp.tl, doc_.tln()-1 );
	dp.ad = Min( dp.ad, doc_.len(dp.tl) );

	// If no reference point is specified for transformation, the origin is used as the reference point.
	// If no reference point is specified for the conversion,
	// the origin is used as the reference point
	VPos topPos(false); // 0 clear
	if( base == NULL )
		base = &topPos;

	// For now, enter the value at the beginning of the base line.
	// Put the value at the beginning of the BASE line for now
	ulong vl = base->vl - base->rl;
	ulong rl = 0;
	int vx;

	// If they were the same line
	//if( dp.tl == base->tl )
	//{
	//	For example, when you press [→], you can change the width of the character to the right.
	//	The next position can be calculated just by adding. Use this to create a normal
	//	Cursor movement should be much faster,
	//	It's a hassle, so I'll omit it for now.
	//}

	// If they were a different line
	//else
	{
		// match vl
		ulong tl = base->tl;
		if( tl > dp.tl )      // If the destination is above the ref, dest above ref
			do
				vl -= rln(--tl);
			while( tl > dp.tl );
		else if( tl < dp.tl ) // If the destination is below the ref, dest below ref
			do
				vl += rln(tl++);
			while( tl < dp.tl );

		// match rl
		ulong stt=0;
		while( wrap_[tl][rl+1] < dp.ad )
			stt = wrap_[tl][++rl];
		vl += rl;

		// x coordinate calculation, calculate x coordinate
		vx = CalcLineWidth( doc_.tl(tl)+stt, dp.ad-stt );
	}

	vp->tl = dp.tl;
	vp->ad = dp.ad;
	vp->vl = vl;
	vp->rl = rl;
	vp->rx = vp->vx = vx;
}

void ViewImpl::GetVPos( int x, int y, VPos* vp, bool linemode ) const
{
// x coordinate correction

	x = x - lna() + rlScr_.nPos;

// First, calculate the line number

	int tl = udScr_tl_;
	int vl = udScr_.nPos - udScr_vrl_;
	int rl = y / NZero(fnt().H()) + udScr_vrl_;
	if( rl >= 0 ) // If it is below the top edge of the View, check the downward direction
		while( tl < (int)doc_.tln() && (int)rln(tl) <= rl )
		{
			vl += rln(tl);
			rl -= rln(tl);
			++tl;
		}
	else           // If it is above the top of the View, check the upward direction
		while( 0<=tl && rl<0 )
		{
			vl -= rln(tl);
			rl += rln(tl);
			--tl;
		}

	if( tl == (int)doc_.tln() ) // Correction when going below EOF
	{
		--tl, vl-=rln(tl), rl=rln(tl)-1;
		if( linemode )
			x = 0x4fffffff;
	}
	else if( tl == -1 ) // Correction when the file goes above the beginning
	{
		tl = vl = rl = 0;
		if( linemode )
			x = 0;
	}
	else
	{
		if( linemode ) // In row selection mode
		{
			if( tl == (int)doc_.tln()-1 )
				rl=rln(tl)-1, x=0x4fffffff;
			else
				vl+=rln(tl), rl=0, ++tl, x=0;
		}
	}

	vp->tl = tl;
	vp->vl = vl + rl;
	vp->rl = rl;

// Next, calculate the horizontal position

	if( rl < static_cast<int>(wrap_[tl].rln()) )
	{
		const unicode* str = doc_.tl(tl);
		const ulong adend = rlend(tl,rl);
		ulong ad = (rl==0 ? 0 : rlend(tl,rl-1));
		int   vx = (rl==0 ? 0 : fnt().W(&str[ad++]));

		while( ad<adend )
		{
			int nvx = (str[ad]==L'\t'
				? fnt().nextTab(vx)
				:  vx + fnt().W(&str[ad])
			);
			if( x+2 < nvx )
				break;
			vx = nvx;
			++ad;
		}

		vp->ad          = ad;
		vp->rx = vp->vx = vx;
	}
	else
	{
		vp->ad = vp->rx = vp->vx = 0;
	}
}
