#ifndef _EDITWING_COMMON_H_
#define _EDITWING_COMMON_H_
#include "../kilib/kilib.h"
#ifndef __ccdoc__

namespace editwing {
#endif


//=========================================================================
// Unicode related
//=========================================================================

inline bool isHighSurrogate(unicode ch)
{
	return (0xD800 <= ch && ch <= 0xDBFF);
}

inline bool isLowSurrogate(unicode ch)
{
	return (0xDC00 <= ch && ch <= 0xDFFF);
}

//=========================================================================
//@{ @pkg editwing.Common //@}
//@{
//	Location information in text
//@}
//=========================================================================

struct DPos
{
	//@{Address in buffer (0~) //@}
	ulong ad;

	//@{Logical line number (0~) //@}
	ulong tl;

	bool operator == ( const DPos& r ) const
		{ return (tl==r.tl && ad==r.ad); }
	bool operator != ( const DPos& r ) const
		{ return (tl!=r.tl || ad!=r.ad); }
	bool operator <  ( const DPos& r ) const
		{ return (tl<r.tl || (tl==r.tl && ad<r.ad)); }
	bool operator >  ( const DPos& r ) const
		{ return (tl>r.tl || (tl==r.tl && ad>r.ad)); }
	bool operator <= ( const DPos& r ) const
		{ return (tl<r.tl || (tl==r.tl && ad<=r.ad)); }
	bool operator >= ( const DPos& r ) const
		{ return (tl>r.tl || (tl==r.tl && ad>=r.ad)); }

	DPos( ulong t, ulong a ) : ad(a), tl(t) {}
	DPos() {}
};



//=========================================================================
//@{
//	Constant values ​​representing special characters
//@}
//=========================================================================

enum SpecialChars
{
	scEOF = 0, // EOF
	scEOL = 1, // line break
	scTAB = 2, // tab
	scHSP = 3, // half-width space
	scZSP = 4  // full-width space
};



//=========================================================================
//@{
//	Constant value representing the type of word
//@}
//=========================================================================

enum TokenType
{
	TAB = 0x00, // Tab
	WSP = 0x04, // half-width space
	ALP = 0x08, // normal character
	 CE = 0x0c, // comment end tag
	 CB = 0x10, // comment start tag
	 LB = 0x14, // line comment start tag
	 Q1 = 0x18, // single quote
	 Q2 = 0x1c  // double quote
};



//=========================================================================
//@{
//	Constant value representing the color specified location
//@}
//=========================================================================

enum ColorType
{
	TXT = 0, // font color
	CMT = 1, // Comment text color
	KWD = 2, // Keyword text color
	//  = 3, // (commented keyword text color)
	CTL = 4, // special text color
	BG  = 5, // background color
	LN  = 6  // line number
};



//=========================================================================
//@{
//	Constant value indicating wrap position
//@}
//=========================================================================

enum WrapType
{
	NOWRAP    = -1, // No wrapping
	RIGHTEDGE =  0  // right end
};



//=========================================================================
//@{
//	Display settings
//
//	Retains information about fonts, colors, tab widths, and special character display.
//	However, the emphasized word is specified for the Document.
//@}
//=========================================================================

struct VConfig
{
	//@{ font //@}
	LOGFONT font;

	//@{ color //@}
	COLORREF color[7];

	short fontsize;
	short fontwidth;

	//@{ tab width character count, tab width character count //@}
	uchar tabstep;

	//@{Special character display //@}
	byte sc;

	//@{ Dangerous default constructor //@}
	VConfig() {}

	//@{ Initialize font relationship //@}
	VConfig( const TCHAR* fnam, short fsiz )
	{
		SetFont( fnam,fsiz );
		tabstep    = 4;
		color[TXT] =
		color[CMT] =
		color[KWD] =
		color[CTL] = RGB(0,0,0);
		color[ BG] = RGB(255,255,255);
		color[ LN] = RGB(0,0,0);//255,255,0);
		sc = 0;
	}

	//@{ Font related settings //@}
	void SetFont(
		const TCHAR* fnam, short fsiz
		, uchar fontCS=DEFAULT_CHARSET
		, LONG fw=FW_DONTCARE, BYTE ff=0, short fx=0
		, int qual=DEFAULT_QUALITY )
	{
		// Actual font size in points at 72 DPI.
		fontsize              = fsiz;
		fontwidth             = fx;

		ki::mem00( &font, sizeof(font) );
	//	font.lfHeight         = 0; // We do not set lfHeight
	//	font.lfWidth          = 0; // because it must be set for
	//	font.lfEscapement     = 0; // each DC
	//	font.lfOrientation    = 0;
		font.lfWeight         = fw; // FW_DONTCARE;
		font.lfItalic         = ff&1; // FALSE
		font.lfUnderline      = ff&2; // FALSE
		font.lfStrikeOut      = ff&4; // FALSE
		font.lfOutPrecision   = OUT_DEFAULT_PRECIS;
		font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
		font.lfQuality        = qual;
		font.lfPitchAndFamily = VARIABLE_PITCH|FF_DONTCARE;
		font.lfCharSet        = fontCS;

		my_lstrcpys( font.lfFaceName, LF_FACESIZE, fnam );

		// ki::LOGGER( "VConfig::SetFont() end" );
	}

	//@{Tab width setting //@}
	void SetTabStep( int tab )
	{
		tabstep = Max( 1, tab );
	}
};



//=========================================================================

}      // namespace editwing
#endif // _EDITWING_COMMON_H_
