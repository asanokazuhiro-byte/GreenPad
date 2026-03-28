
#include "../kilib/stdafx.h"
#include "ip_doc.h"
using namespace ki;
using namespace editwing;
using namespace editwing::doc;



//=========================================================================
//---- ip_parse.cpp Keyword parsing
//
//		Set the string to be retained according to the keyword definition file.
//		Now is the time to properly separate them.
//
//---- ip_text.cpp String manipulation/etc.
//---- ip_wrap.cpp wrapping
//---- ip_scroll.cpp Scroll
//---- ip_draw.cpp Drawing/etc.
//---- ip_cursor.cpp Cursor control
//=========================================================================



//=========================================================================
//
// Analysis result data specifications
// It's true that they brought in so many palliative methods.
// I don't know if it's faster or not...(^^;
//
// -----------------------------------------------
//
// Line::isLineHeadCommented_
//    0: The beginning of the line is not inside the block comment
//    1: The beginning of the line is inside a block comment,
//
// -----------------------------------------------
//
// Line::commentTransition_
//   00: End of line always out of comment
//   01: Comment state is reversed at the beginning of a line and at the end of a line
//   10: Comment state is the same at the beginning of a line and at the end of a line
//   11: The end of the line is always in the comment
//
// -----------------------------------------------
//
// Based on the above two flags, the information of the current line is extracted from the information of the previous line.
//   this.head = (prev.trans >> prev.head)&1;
// You can calculate it sequentially.
// The state of the internal buffer is rewritten during this calculation.
// The cost is too high, so while looking at the flags shown below,
// Adjust as appropriate just before drawing.
//
// -----------------------------------------------
//
// Line::commentBitReady_
//   Is the comment bit adjusted?
//   Whether the comment bits have been adjusted or not
//
// -----------------------------------------------
//
// Line::str_[]
//   String data is stored as is in UCS-2 solid.
//   However, to speed up the parser, after the final character
//   0x007f is added.
//   UCS-2 solid, string data is stored as is.
//   However, to speed up the parser, the last character is followed by
//   0x007f is appended.
//
// -----------------------------------------------
//
// Line::flg_
//   Assign an 8-bit flag to each character as shown below.
//   Assign an 8-bit flag as shown below for each character
//   | aaabbbcd |
//
// -----------------------------------------------
//
// aaa == "PosInToken"
//     0: Token on the way
//   1-6: Token head. The next head is 1-6 characters ahead. , Token head. The next head is 1-6 characters ahead.
//     7: Token head. The next beginning is at least 7 characters ahead. , Token head. The next head is at least 7 characters ahead.
//
// -----------------------------------------------
//
// bbb == "TokenType"
//     0: TAB: tab character, tab cahar
//     1: WSP: white space, white space char
//     2: TXT: normal text, normal text
//     3: CE: comment start tag, comment-start tag
//     4: CB: end-of-comment tag, end-of-comment tag
//     5: LB: line comment start tag, line comment start tag
//     6: Q1: '' Quote 1, Sinlge Quote
//     7: Q2: "" Quote2, Double Quote
//
// -----------------------------------------------
//
//  c  == "isKeyword?"
//     0: Not a keyword.
//     1: Keyword
//
// -----------------------------------------------
//
//  d  == "inComment?"
//     0: not in a comment, not in a comment
//     1: in a comment, in a comment
//
// -----------------------------------------------



namespace {
//-------------------------------------------------------------------------
// Automaton for determining whether it is inside or outside a comment, etc.
//
// If /* appears, the area after it is a comment, and if */ appears, the area after it is a normal zone.
// A simple rule like... won't work. For example, str"/*"str
// You will be in trouble if it appears. So,
//   - Normal text
//   - Inside a block comment
//   - Inside a line comment
//   - Inside single quotes
//   - Inside double quotes
// Divided into 5 types of situations, and in each case, which symbol appears
// It is necessary to process which state to move to next. The rules for that state change
// It is given and managed as a 5x5 two-dimensional array.
//-------------------------------------------------------------------------

enum CommentDFASymbol{ sCE, sCB, sLB, sQ1, sQ2, sXXX };
struct CommentDFA
{
	// <Status>
	// The lowest bit is a flag indicating whether it is currently in a comment.
	// Use (state>>1)&(state) to check whether it is in a block comment.
	//   000: normal text        011: in BlockComment
	//   001: in LineComment     100: in Quote2
	//   010: in Quote1
	//
	// <symbol>
	// In C++, it is as follows
	// The value is now synchronized with the TokenType flag.
	//   000: CE */              011: Q1 '
	//   001: CB /*              100: Q2 "
	//   010: LB //

	// Specify the initial state. In comments or outside comments?
	CommentDFA( bool inComment )
		: state( inComment ? 3 : 0 ) {}

	// State transition by giving input sign
	void transit( uchar sym )
		{ state = tr_table[state][sym]; }

	static void SetCEequalCB(bool set)
	{
		// Is CE == CB then we must go from
		// iBc -> Ntx when we see CB
		tr_table[/*iBc*/3][/*QB*/1] = set? 0: 3;
	}

	// current status
	uchar state;

	// state transition table
	static uchar tr_table[5][5];
};

uchar CommentDFA::tr_table[5][5] = {
// state                  // CE,  CB,  LB,  Q1,  Q2
/* 000 Ntx */{0,3,1,2,4}, // Ntx, iBc, iLc, iQ1, iQ2
/* 001 iLc */{1,1,1,1,1}, // iLc, iLc, iLc, iLc, iLc
/* 010 iQ1 */{2,2,2,0,2}, // iQ1, iQ1, iQ1, Ntx, iQ1
/* 011 iBc */{0,3,3,3,3}, // Ntx,    , iBc, iBc, iBc
/* 100 iQ2 */{4,4,4,4,0}, // iQ2, iQ2, iQ2, iQ2, Ntx
};



//-------------------------------------------------------------------------
// A simple keyword storage structure.
// A next pointer is attached to make it an element of ChainHash.
// DO NOT USE new/delete for Keyword, stick to Keyword::New
//-------------------------------------------------------------------------
struct Keyword
{
	ushort      next;
	ushort      len;
	unicode     str[1];

	static Keyword *New(Arena *ar,  const unicode *s, size_t ll )
	{
		ushort l = static_cast<ushort>(ll);
		Keyword *x = reinterpret_cast<Keyword *>( ar->alloc( (sizeof(Keyword) + l * sizeof(unicode)) ) );
		if( x )
		{
			x->next = 0;
			x->len  = l;
			memmove(x->str, s, l*sizeof(unicode));
			x->str[l] = L'\0';
		}
		//else MessageBox(NULL, s, NULL, 0);
		//LOGGERF( TEXT("sizeof(Keyword) = %d, remaining = %d bytes"),  sizeof(Keyword), (UINT)(ar->end - ar->sta));
		return x;
	}
};



//-------------------------------------------------------------------------
// Support functions. Comparing Unicode text to each other
//-------------------------------------------------------------------------

static bool compare_s(const unicode* a,const unicode* b, size_t l)
{
	// Case sensitive, Case sensitive
	while( l-- )
		if( *a++ != *b++ )
			return false;
	return true;
}

static bool compare_i(const unicode* a,const unicode* b,size_t l)
{
	// Case insensitive (misc)
	while( l-- )
		if( ((*a++) ^ (*b++)) & 0xdf )
			return false;
	return true;
}



//-------------------------------------------------------------------------
// From the given symbol string, extract a meaningful token such as the start of a comment.
// Structure for cutting out.
// meaningful tokens from a given symbol string, such as the start of a comment.
// Structure to cut out.
//-------------------------------------------------------------------------

class TagMap
{
	Keyword* tag_[3]; // 0:CE 1:CB 2:LB
	bool esc_, q1_, q2_, map_[768]; // 128
	BYTE arbuf[64];
	Arena ar;

public:

	TagMap( const unicode* cb, size_t cblen,
		    const unicode* ce, size_t celen,
		    const unicode* lb, size_t lblen,
		    bool q1, bool q2, bool esc )
		: esc_( esc )
		, q1_ ( q1 )
		, q2_ ( q2 )
		, ar ( arbuf, sizeof(arbuf) )
	{
		// Are symbols starting with '/' used?
		// Create a table that is used to check only the first character, such as
		tag_[0] = tag_[1] = tag_[2] = NULL;
		mem00( map_, sizeof(map_) );
		map_[L'\''] = q1;
		map_[L'\"'] = q2;
		map_[L'\\'] = esc;
		if( celen!=0 && *ce < 0x80 ){ map_[*ce]=true; tag_[0]=Keyword::New(&ar, ce,celen); }
		if( cblen!=0 && *cb < 0x80 ){ map_[*cb]=true; tag_[1]=Keyword::New(&ar, cb,cblen); }
		if( lblen!=0 && *lb < 0x80 ){ map_[*lb]=true; tag_[2]=Keyword::New(&ar, lb,lblen); }
	}

//	~TagMap()
//	{
//		// keyword release,
//		Keyword::Delete( tag_[0] );
//		Keyword::Delete( tag_[1] );
//		Keyword::Delete( tag_[2] );
//	}

	bool does_esc()
	{
		// Whether to escape with \
		return esc_;
	}

	ulong SymbolLoop(
		const unicode* str, ulong len, ulong& mlen, uchar& sym )
	{
		// Loop until a meaningful symbol is matched
		// The return value is the number of characters skipped before matching,
		// Returns information about the matched symbol to mlen,sym
		// Loop until a meaningful symbol is matched.
		// Return value is the number of characters skipped before the match.
		// And the matched length/symbol are copied in mlen and sym.

		ulong ans=0;
		for( sym=sXXX, mlen=1; ans<len; ++ans )
		{
			if( map_[str[ans]] )
			{
				for( int i=2; i>=0; --i )
				{
					if( tag_[i]!=NULL
					 && tag_[i]->len <= len-ans
					 && compare_s(
						tag_[i]->str, str+ans, tag_[i]->len ) )
					{
						sym  = i;
						mlen = tag_[i]->len;
						goto symbolfound;
					}
				}

				if( str[ans] == L'\'' ) // single quote - single quote
				{
					if( q1_ )
					{
						sym  = sQ1;
						goto symbolfound;
					}
				}
				else if( str[ans] == L'\"' ) // double quote - double quote
				{
					if( q2_ )
					{
						sym  = sQ2;
						goto symbolfound;
					}
				}
				else if( str[ans] == L'\\' ) // Skip the character after \
				{
					if( esc_ && ans+1<len )
						++ans;
				}
			}
		}

	symbolfound:
		return ans;
	}
};



//-------------------------------------------------------------------------
// Hash table for quickly determining whether a given string is a keyword
// Hash table for fast determination of whether a given string is a keyword
//-------------------------------------------------------------------------
class KeywordMap
{
	// Should be a power of two!
	enum { HTABLE_SIZE = 2048 };
	ushort   backet_[HTABLE_SIZE];
	Arena ar;
	size_t elems_;
	bool (*compare_)(const unicode*,const unicode*,size_t);
	uint  (*hash)( const unicode* a, size_t al );
public:

	KeywordMap( bool bCaseSensitive )
		: ar ( NULL, 0 )
		, elems_ ( 0 )
		, compare_( bCaseSensitive ? compare_s : compare_i )
		, hash    ( bCaseSensitive ? hash_s : hash_i )
	{
		// Hash table initialization
		mem00( backet_, sizeof(backet_) );
	}

	void SetArenaBufSize( size_t count )
	{
		if (count > 65536)
			count = 65536; // LIMIT
		BYTE *buf = NULL;
		if(count != 0)
			buf = (BYTE*)malloc(count);

		BYTE *obuf = ar.sta;
		ar = Arena(buf, count);

		free( obuf );
	}

	~KeywordMap()
	{
		// release
		free( ar.sta );
//	#ifdef _DEBUG
//		if( elems_ )
//		{
//			LOGGER( "KEYWORD HASH MAP:" );
//			for( size_t i =0; i < countof(backet_); i++ )
//				LOGGERF( TEXT("%lu"), (DWORD)backet_[i] );
//		}
//	#endif
	}
	#define KW(a) ((Keyword*)(ar.sta + a))

	void AddKeyword( const unicode* str, size_t len )
	{
		// Data registration
		ushort x = (ushort)((BYTE*)Keyword::New(&ar, str,len) - ar.sta);
		int      h = hash(str,len);

		if( backet_[h] == 0 )
		{
			// If the hash table is empty, Hash table slot is free.
			backet_[h] = x;
		}
		else
		{
			// When connecting to the end of a chain, chain to the existing element
			//MessageBoxW(NULL, backet_[h]->str, x->str , MB_OK);
			ushort q=backet_[h], p=KW(q)->next;
			while( p!=0 )
				q=p, p=KW(q)->next;

			KW(q)->next = x;
		}

		// Also add it to the list for clearing data.
		//dustbox_.Add(x);
		++elems_;
	}

	uchar inline isKeyword( const unicode* str, size_t len ) const
	{
		// Does it match the registered keyword?
		if( elems_ ) // Nothing to do for empty keyword list.
			for( ushort p=backet_[hash(str,len)]; p!=0; p=KW(p)->next )
				if( KW(p)->len==len && compare_( KW(p)->str, str, len ) )
					return 2; // We must set the c bit of aaabbbcd
		return 0;
	}
	#undef KW
private:

	static uint hash_i( const unicode* a, size_t al )
	{
		// A very crude hash function that collapses to 12 bits
		// Because it is troublesome to separate routines, uppercase and lowercase letters are not always distinguished. (^^;
		// Very messy hash function that collapses to 12 bits.
		// case-insensitive.
		uint h=0,i=0;
		while( al-- )
		{
			h ^= ((*(a++)&0xdf)<<i);
			i = (i+5)&7;
		}
		return h&(HTABLE_SIZE-1);
	}

	static uint hash_s( const unicode* a, size_t al )
	{
		// case-sensitive
		uint h=0,i=0;
		while( al-- )
		{
			h ^= (*a++)<<i;
			i = (i+5)&7;
		}
		return h&(HTABLE_SIZE-1);
	}
};



//-------------------------------------------------------------------------
// Parser that analyzes text using the above tools
//-------------------------------------------------------------------------
}
class editwing::doc::Parser
{
public:
	KeywordMap kwd_;
	TagMap     tag_;

public:
	// Initialization 1
	Parser(
		const unicode* cb, size_t cblen,
		const unicode* ce, size_t celen,
		const unicode* lb, size_t lblen,
		bool q1, bool q2, bool esc,
		bool casesensitive
	)
		: kwd_( casesensitive )
		, tag_( cb, cblen, ce, celen, lb, lblen, q1, q2, esc )
	{
		if( cb && ce ) // In case begin and end comment strings are the same i.e.: Python
			CommentDFA::SetCEequalCB( cblen==celen && !my_lstrncmpW(ce, cb, cblen) );
	}

	//void Trim() { kwd_.Trim(); }
	void SetKeywordArenaSize(size_t count)
	{
		kwd_.SetArenaBufSize(count);
	}

	// Initialization 2: Add keywords
	void AddKeyword( const unicode* str, size_t len )
	{
		kwd_.AddKeyword( str, len );
	}

	// Row data analysis
	uchar Parse( Line& line, uchar cmst )
	{
		line.TransitCmt( cmst );

		// ASCII sorting table.
		// In order to be able to use it for TokenType without shifting,
		// The value jumps by 4
		// We want to separate the tabs from other non printable characters.
		enum { T=1, W=4, A=8, S=12, O=0 };
		static const uchar letter_type[768] = {
			O,O,O,O,O,O,O,O,O,T,O,O,O,O,O,O, // NULL-SHI_IN
			O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O, // DLE-US
			W,S,S,S,S,S,S,S,S,S,S,S,S,S,S,S, //  !"#$%&'()*+,-./
			A,A,A,A,A,A,A,A,A,A,S,S,S,S,S,S, // 0123456789:;<=>?
			S,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A, // @ABCDEFGHIJKLMNO
			A,A,A,A,A,A,A,A,A,A,A,S,S,S,S,A, // PQRSTUVWXYZ[\]^_
			S,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A, // `abcdefghijklmno
			A,A,A,A,A,A,A,A,A,A,A,S,S,S,S,O, // pqrstuvwxyz{|}~
			// Latin-1 Supplement (0x0080-0x00FF)
			O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O, // C1 Controls
			O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O, // C1 Controls
			S,A,A,A,A,A,S,A,A,A,A,S,A,A,A,S, //  !￠￡?\|§¨ca≪￢[SHY-]R￣
			A,S,A,A,A,A,S,S,A,S,A,S,A,A,A,S, // 0x00B0-0x00BF symbol range
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A, // AAAAAAACEEEEIIII
			A,A,A,A,A,A,A,S,A,A,A,A,A,A,A,A, // DNOOOOO×OUUUUYTs
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A, // aaaaaaaceeeeiiii
			A,A,A,A,A,A,A,S,A,A,A,A,A,A,A,A, // dnooooo÷ouuuuyty
			// Latin Extended-A (0x0100-0x017f)
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			// Latin Extended-B (0x0180-0x024f)
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			// IPA Extensions (0x0250-0x02af)
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			// Spacing Modifier Letters (0x02b0-0x02ff)
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
		};

		// Distance encoder for PosInToken calculation (5bit shifted)
		// Distance encoder for PosInToken calculation ( 5bit shifted )
		//  ( _d>7 ? 7<<5 : _d<<5 )
		#define tkenc(_d) ( (_d)>7 ? 0xe0 : (_d)<<5 )

		// Comment state transition tracking automaton
		CommentDFA dfa[2] = {CommentDFA(false), CommentDFA(true)};
		const uchar& cmtState  = dfa[line.isLineHeadCmt()].state;
		uchar commentbit = cmtState&1;

		// work area, workspace
		uchar sym;
		ulong j, k, um, m;
		uchar t, f;

		// Loop~
		const unicode* str = line.str();
		uchar*         flg = line.flg();
		ulong           ie = line.size();
		for( ulong i=0; i<ie; i=j )
		{
			j = i;

			// Not ASCII char
			// was 0x007f (ASCII) I replaced by 0x02ff
			// that corresponds to all extended latin charatcter.
			// This is needed if yu want to ctrl+select words that
			// have a mix of ASCII and other LATIN characters.
			if( str[i] > 0x02ff ) // Non latin
			{
				f = (ALP | commentbit);
				if( str[i] == 0x3000 )//L'　' )
					while( str[++j] == 0x3000 )
						flg[j] = f;
				else
					while( str[++j] >= 0x02ff && str[j]!=0x3000 )
						flg[j] = f;
				flg[i] = static_cast<uchar>(tkenc(j-i) | f);
			}
			// For ASCII characters?? (ASCII char?)
			// All latin chars up to IPA Extensions 0x0000-0x02ff
			else
			{
				t = letter_type[str[i]];
				if( t==S && tag_.does_esc() )
					do
						if( j+1<ie && str[j]==L'\\' )
							j++;
					while( str[++j]<0x7f && S==letter_type[str[j]] );
				else
					while( ++j<ie && str[j]<0x02ff && t==letter_type[str[j]] );

				f = (t | commentbit);

				switch( t ) // letter type:
				{
				// Alphabets & Numbers (a-Z, 0-9)
				case A:
					if( str[i] < 0x007f ) // ASCII only
						f |= kwd_.isKeyword( str+i, j-i );
					// fall...

				// CTL characters (0-32 + 127-160)
				case O:
				// Tabs/control characters, Tabs
				case T:
					// fall...

				// Half-width space (space, 32)
				case W:
					for( k=i+1; k<j; ++k )
						flg[k] = f;
					flg[i] = (uchar)(tkenc(j-i)|f);
					break;

				// Symbol (Symbol, 33-47, 58-64, 91-94, 96, 123-126)
				case S:
					k = i;
					while( k < j )
					{
						// The part that did not match
						um = tag_.SymbolLoop( str+k, j-k, m, sym );
						f = (0x20 | ALP | commentbit);
						while( um-- )
							flg[k++] = f;
						if( k >= j )
							break;

						// matched part, matched part
						f = (CE | commentbit);
						dfa[0].transit( sym );
						dfa[1].transit( sym );
						commentbit = cmtState&1;
						if( sym != 0 ) // 0:comment end
							f = (((sym+3)<<2) | commentbit);
						flg[k++] = (uchar)(tkenc(m)|f);
						while( --m )
							flg[k++] = f;
					}
					break;
				}
			}
		}

		// Update transit flag
		line.SetTransitFlag(
			(dfa[1].state & (dfa[1].state<<1)) |
			((dfa[0].state>>1) & dfa[0].state)
		);
		line.CommentBitUpdated();
		return line.TransitCmt( cmst );
	}

	// Adjust comment bit correctly
	void SetCommentBit( Line& line )
	{
		CommentDFA dfa( line.isLineHeadCmt()==1 );
		uchar commentbit = dfa.state&1;

		// Loop~
		// const unicode* str = line.str();
		uchar*         flg = line.flg();
		ulong         j,ie = line.size();
		for( ulong i=0; i<ie; i=j )
		{
			// Get the end of the Token
			uchar k = (flg[i]>>5);
			j = i + k;
			if( j >= ie )
				j = ie;
			else if( k==7 ) // || k==0 )
				while( j<ie && (flg[j]>>5)==0  ) // check bound BEFORE [] !!!
					++j;

			k = (flg[i] & 0x1c);
			if( k <= CE )
			{
				for( ; i<j; ++i )
					flg[i] = (uchar)((flg[i] & 0xfe) | commentbit);
			}
			if( k >= CE )
			{
				dfa.transit( (k>>2)-3 );
				commentbit = dfa.state&1;
				if( k != CE )
					for( ; i<j; ++i )
						flg[i] = (uchar)((flg[i] & 0xfe) | commentbit);
			}
		}

		line.CommentBitUpdated();
	}
};


//-------------------------------------------------------------------------
// Definition file reading process
//-------------------------------------------------------------------------

Document::Document( )
	: evHanNum_ ( 0 )
	, busy_   ( false )
	, acc_textupdate_mode_ ( false )
	, acc_reparsed_ ( false )
	, acc_nmlcmd_ (false )
{
	text_.Add( Line(L"",0) ); // Only one line at first
	SetKeyword( NULL, 0 );        // No keyword
}

Document::~Document()
{
	// If you don't put a destructor in this file,
	// delete parser_ is no longer possible. ^^;
}

void Document::SetKeyword( const unicode* defbuf, size_t siz )
{
	// Skip if there is a BOM
	if( siz!=0 && *defbuf==0xfeff )
		++defbuf, --siz;

	// Ready to load
	const unicode* str=NULL;
	size_t       len=0;
	UniReader r( defbuf, siz, &str, &len );
	bool          flags[] = {false,false,false,false};
	const unicode* tags[] = {NULL,NULL,NULL};
	size_t       taglen[] = {0,0,0};
	if( siz != 0 )
	{
		if( *defbuf != L'0' && *defbuf != L'1')
		{
			LOGGER( "Invalid kwd file, (should start with 0 or 1)" );
			return;
		}

		// 1st line: Flag
		//   case? q1? q2? esc?
		r.getLine();
		for( size_t i=0; i<len; ++i )
			flags[i] = (str[i]==L'1');

		// 2nd to 4th lines
		//   Brocome start symbol, Brocome end symbol, Line comedy symbol
		// comment start symbol, end symbol line comment symbol
		for( int j=0; j<3; ++j )
		{
			r.getLine();
			  tags[j] = str;
			taglen[j] = len;
		}
	}

	if( taglen[2] )
	{// Copy single line comment string (LB) in a convenient buffer.
		size_t cstrlen=Min(taglen[2], countof(CommentStr_));
		memmove(CommentStr_, tags[2], cstrlen*sizeof(*CommentStr_));
		CommentStr_[cstrlen]='\0'; // be sure to NULL terminate
	}
	else
	{// Default comment string is > when there is no .kwd files.
		CommentStr_[0] = L'>'; CommentStr_[1] = L'\0';
	}


	// Parser creation
	void *pp = parser_.get();
	if( pp )
	{
		new ( pp ) Parser(
			tags[0], taglen[0], tags[1], taglen[1], tags[2], taglen[2],
			flags[1], flags[2], flags[3], flags[0] );
	}
	else
	{
		Parser *prs = new Parser(
			tags[0], taglen[0], tags[1], taglen[1], tags[2], taglen[2],
			flags[1], flags[2], flags[3], flags[0] );
		if( prs )
			parser_.reset( prs );
	}

	if( siz > 0 && !r.isEmpty() )
	{
		// calculate the necessary memory for the keyword list.
		const unicode *b = str;
		size_t n = siz - (str - defbuf), totsz = 0;
		while( n-- )
		{
			enum { align = sizeof(void*) -1 };
			++b;
			if( *b == '\n' )
			{
				if( n && b[1] == '\n' || b[1] == '\r' )
					continue; // Skip extra empty lines
				totsz = (totsz + align + sizeof(Keyword)) & ~(size_t)align;
			}
			else
				totsz += 2;
		}

		parser_->SetKeywordArenaSize( totsz );
		// 5th line onwards: Keyword list
		while( !r.isEmpty() )
		{
			r.getLine();
			if( len != 0 )
				parser_->AddKeyword( str, len );
		}
	}

	// Parse all lines again
	//DWORD otime = GetTickCount();
	ReParse( 0, tln()-1 );

	//TCHAR buf[128];
	//wsprintf(buf, TEXT("%lu ms"), GetTickCount() - otime);
	//MessageBox(NULL, NULL, buf, 0);

	// Change notification
	Fire_KEYWORDCHANGE();
}

bool A_HOT Document::ReParse( ulong s, ulong e )
{
	ulong i;
	uchar cmt = text_[s].isLineHeadCmt();

	// First, reanalyze the scope of the change
	for( i=s; i<=e; ++i )
		cmt = parser_->Parse( text_[i], cmt );

	// If there is no change in the commented out status, end here.
	// If there is no change in the commented-out status, we are done here.
	if( i==tln() || text_[i].isLineHeadCmt()==cmt )
		return false;

	// For example, if /* is entered, the line below
	// It is necessary to communicate changes in the commented out state.
	// For example, if a /* is entered, then down to the bottom line.
	// need to communicate the change in commented-out status.
	do
		cmt = text_[i++].TransitCmt( cmt );
	while( i<tln() && text_[i].isLineHeadCmt()!=cmt );
	return true;
}

void Document::SetCommentBit( const Line& x ) const
{
	parser_->SetCommentBit( const_cast<Line&>(x) );
}
