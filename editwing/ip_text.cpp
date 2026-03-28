
#include "../kilib/stdafx.h"
#include "ip_doc.h"
using namespace ki;
using namespace editwing;
using namespace editwing::doc;



//=========================================================================
//---- ip_text.cpp String manipulation/etc.
//
//		Processing such as inserting and deleting character strings...
//		They are summarized in this file. external interface
//		The implementation is also here.
//
//---- ip_parse.cpp Keyword parsing
//---- ip_wrap.cpp wrapping
//---- ip_scroll.cpp Scroll
//---- ip_draw.cpp Drawing/etc.
//---- ip_cursor.cpp Cursor control
//=========================================================================


//-------------------------------------------------------------------------
// Event handler processing
//-------------------------------------------------------------------------

void Document::AddHandler( DocEvHandler* eh )
{
	// Add handler
	if( evHanNum_ < MAX_EVHAN )
		pEvHan_[ evHanNum_++ ] = eh;
	
	LOGGERF( TEXT("Add Document Event Handler (%u / %u)"), (uint)evHanNum_, (uint)MAX_EVHAN );
}

void Document::DelHandler( const DocEvHandler* eh )
{
	// Look from behind...
	const int last = evHanNum_ - 1;

	// ...delete if found
	for( int i=last; i>=0; --i )
		if( pEvHan_[i] == eh )
		{
			pEvHan_[i] = pEvHan_[last];
			evHanNum_ = last;
			break;
		}
}

// Start accumulating the coordinates to send to Fire_TEXTUPDATE() function
// because it is slow. It is used for the MacroCommand, and it can speeds up
// find/replace by more than a hundred in a big file.
void Document::acc_Fire_TEXTUPDATE_begin()
{
	acc_textupdate_mode_ = true;
	acc_s_ = DPos(-1, -1);
	acc_e2_ = DPos(0,0);
	acc_reparsed_ = acc_nmlcmd_ = false;
}
// End the above and send Fire_TEXTUPDATE() for real.
void Document::acc_Fire_TEXTUPDATE_end()
{
	acc_textupdate_mode_ = false;
	Fire_TEXTUPDATE( acc_s_, acc_e2_, acc_e2_, acc_reparsed_, acc_nmlcmd_ );
}

void Document::Fire_TEXTUPDATE
	( const DPos& s, const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd )
{
	AutoLock lk(this);

	if( acc_textupdate_mode_ )
	{	// Accumulate positions.
		acc_s_  = Min(acc_s_, s);
		acc_e2_ = Max(acc_e2_, e2);
		acc_reparsed_ = acc_reparsed_ || reparsed;
		acc_nmlcmd_   = acc_nmlcmd_ || nmlcmd;
	}
	else
	{
		// Event notification for all
		for( size_t i=0, ie=evHanNum_; i<ie; ++i )
			pEvHan_[i]->on_text_update( s, e, e2, reparsed, nmlcmd );
	}
}

void Document::Fire_KEYWORDCHANGE()
{
	AutoLock lk(this);

	// Event notification for all
	for( size_t i=0, ie=evHanNum_; i<ie; ++i )
		pEvHan_[i]->on_keyword_change();
}

void Document::Fire_MODIFYFLAGCHANGE()
{
	AutoLock lk(this);

	// Event notification for all
	bool b = urdo_.isModified();
	for( size_t i=0, ie=evHanNum_; i<ie; ++i )
		pEvHan_[i]->on_dirtyflag_change( b );
}



//-------------------------------------------------------------------------
// UnDo,ReDo processing
//-------------------------------------------------------------------------

UnReDoChain::Node::Node( Command* c, Node* p, Node* n )
	: next_( n )
	, prev_( p )
	, cmd_ ( c )
{
}

UnReDoChain::Node::Node()
{
	next_ = prev_ = this;
	cmd_  = NULL;
}

UnReDoChain::Node::~Node()
{
	delete cmd_;
}

void UnReDoChain::Node::ResetCommand( Command* cmd )
{
	delete cmd_;
	cmd_ = cmd;
}

UnReDoChain::UnReDoChain()
	: savedPos_( &headTail_ )
	, lastOp_  ( &headTail_ )
	, num_     ( 0 )
	, limit_   ( static_cast<size_t>(-1) )
{
}

UnReDoChain::~UnReDoChain()
{
	Clear();
}

ulong UnReDoChain::Node::ChainDelete(Node*& savedPos_ref)
{
	if( cmd_ == NULL )
		return 0;
	if( savedPos_ref == this )
		savedPos_ref = NULL;
	ulong ret = 1 + next_->ChainDelete(savedPos_ref);
	delete this; // Delete this node.
	return ret;
}

void UnReDoChain::Clear()
{
	headTail_.next_->ChainDelete(savedPos_);
	headTail_.next_ = headTail_.prev_ = lastOp_  = savedPos_ = &headTail_;
	num_ = 0;
}

void UnReDoChain::SetLimit( long lim )
{
	limit_ = Max( (size_t)1, (size_t)lim );
}

inline void UnReDoChain::Undo( Document& doc )
{
	lastOp_->ResetCommand( (*lastOp_->cmd_)(doc) );
	lastOp_ = lastOp_->prev_;
}

inline void UnReDoChain::Redo( Document& doc )
{
	lastOp_ = lastOp_->next_;
	lastOp_->ResetCommand( (*lastOp_->cmd_)(doc) );
}

void UnReDoChain::NewlyExec( const Command& cmd, Document& doc )
{
	Command* nCmd = cmd(doc);
	if( nCmd != NULL )
	{
		Node *nn = new Node(nCmd,lastOp_,&headTail_);
		if( !nn )
		{	// Unable to allocate the Node!
			// delete now the command that can't be Saved...
			delete nCmd;
			return;
		}
		num_   -= (lastOp_->next_->ChainDelete(savedPos_) - 1);
		lastOp_ = lastOp_->next_ = nn;

		while( limit_ < num_ )
		{
			// The old item was deleted because the number of times limit was exceeded.
			// Deleted the old one because it exceeded the count limit.
			Node* old = headTail_.next_;
			headTail_.next_   = old->next_;
			old->next_->prev_ = &headTail_;
			if( old != &headTail_ )
				delete old;
			if( savedPos_ == &headTail_ )
				savedPos_ = NULL;
			else if( savedPos_ == old )
				savedPos_ = &headTail_;
			--num_;
		}
	}
}



bool Document::isUndoAble() const
	{ return urdo_.isUndoAble(); }

bool Document::isRedoAble() const
	{ return urdo_.isRedoAble(); }

bool Document::isModified() const
	{ return urdo_.isModified(); }

void Document::SetUndoLimit( long lim )
	{ urdo_.SetLimit( lim ); }

void Document::ClearModifyFlag()
{
	bool b = urdo_.isModified();
	urdo_.SavedHere();
	if( b != urdo_.isModified() )
		Fire_MODIFYFLAGCHANGE();
}

void Document::Undo()
{
	if( urdo_.isUndoAble() )
	{
		bool b = urdo_.isModified();
		urdo_.Undo(*this);
		if( b != urdo_.isModified() )
			Fire_MODIFYFLAGCHANGE();
	}
}

void Document::Redo()
{
	if( urdo_.isRedoAble() )
	{
		bool b = urdo_.isModified();
		urdo_.Redo(*this);
		if( b != urdo_.isModified() )
			Fire_MODIFYFLAGCHANGE();
	}
}

void Document::Execute( const Command& cmd )
{
	bool b = urdo_.isModified();
	urdo_.NewlyExec( cmd, *this );
	if( b != urdo_.isModified() )
		Fire_MODIFYFLAGCHANGE();
}



//-------------------------------------------------------------------------
// Cursor movement helper, Cursor movement helper
//-------------------------------------------------------------------------

DPos Document::leftOf( const DPos& dp, bool wide ) const
{
	if( dp.ad == 0 )
	{
		// At the beginning of a line but not at the beginning of a file
		// To the end of the previous line
		// If beginning of a line, but not the beginning of a file
		// Go to the end of the previous line.
		if( dp.tl > 0 )
			return DPos( dp.tl-1, len(dp.tl-1) );
		return dp;
	}
	else if( !wide )
	{
		// When going back one character normally in the middle of a line
		// If you want to go back one character "normally", in the middle of a line
		const unicode* l = tl(dp.tl);
		if( dp.ad>=2 && isLowSurrogate(l[dp.ad-1]) && isHighSurrogate(l[dp.ad-2]) )
			return DPos( dp.tl, dp.ad-2 );
		return DPos( dp.tl, dp.ad-1 );
	}
	else
	{
		// To go back one word in the middle of a line, To go back one word in the middle of a line
		const uchar* f = pl(dp.tl);
			  ulong  s = dp.ad;
		while( --s && (f[s]>>5)==0 );
		return DPos( dp.tl, s );
	}
}

DPos Document::rightOf( const DPos& dp, bool wide ) const
{
	if( dp.ad == len(dp.tl) )
	{
		// End of line but not end of file
		// Go to the beginning of the next line
		// At the end of a line, but not at the end of a file
		// Go to the beginning of the next line.
		if( dp.tl < tln()-1 )
			return DPos( dp.tl+1, 0 );
		return dp;
	}
	else if( !wide )
	{
		// When normally advancing one character in the middle of a line
		// If you advance one character normally in the middle of a line
		const unicode* l = tl(dp.tl);
		// There is no need to check the length since the guard 0x007f is at the end of l.
		// No need to check the length of l with the #0x007f guard at the end of l.
		if( isHighSurrogate(l[dp.ad]) /*&& dp.ad+1 <= len(dp.tl)*/ && isLowSurrogate(l[dp.ad+1]) )
			return DPos( dp.tl, dp.ad+2 );
		return DPos( dp.tl, dp.ad+1 );
	}
	else
	{
		// If you normally advance one word in the middle of a line
		// If you advance one word normally in the middle of a line
		const uchar* f = pl(dp.tl);
		const ulong  e = len(dp.tl);
			  ulong  s = dp.ad;
		const ulong  t = (f[s]>>5);
		s += t;
		if( s >= e )
			s = e;
		else if( t==7 || t==0 )
			while( s<e && (f[s]>>5)==0 )
				++s;
		return DPos( dp.tl, s );
	}
}

DPos Document::wordStartOf( const DPos& dp ) const
{
	if( dp.ad == 0 )
	{
		// beginning of line
		return dp;
	}
	else
	{
		// midline, midline
		const uchar* f = pl(dp.tl);
			  ulong  s = dp.ad+1;
		while( --s && (f[s]>>5)==0 );

		return DPos( dp.tl, s );
	}
}

DPos Document::findMatchingBrace( const DPos &dp ) const
{
	// Currently selected character.
	DPos np=dp;
	bool backward, backside=0;
	unicode q, p;
	tryagain:
	backward = 0;
	q = text_[np.tl].str()[np.ad];
	switch( q )
	{
		case '(': p=')'; backward=0; break;
		case '[': p=']'; backward=0; break;
		case '{': p='}'; backward=0; break;
		case '<': p='>'; backward=0; break;

		case ')': p='('; backward=1; break;
		case ']': p='['; backward=1; break;
		case '}': p='{'; backward=1; break;
		case '>': p='<'; backward=1; break;
		default:
			if( !backside )
			{	// Check if the parenthesis is at the backside of dp.
				np = leftOf(dp, false);
				backside=1;
				goto tryagain; // Try again!
			}
			// No brace to match, just go to next " '  * /
			if( q == L'\"' || q == L'\'' || q == L'*' )
			{
				if( findNextBrace( np, q, q ) )
					return np;
			}
			return dp;
	}

	ulong match;
	unicode b;
	if( !backward )
	{
		match = 1; // We already have 1 opening parenthesis
		while( b = findNextBrace( np, q, p ) ) // q='(' p=')'
		{
			// Keep track of opening/closing parenthesis
			match += 2*(b==q) - 1; // b=='(' ++ else --
			if( match == 0 ) // same amount of () => done!
				break;
		}
	}
	else
	{
		match = 1; // We already have 1 closing parenthesis
		while( b = findPrevBrace( np, q, p ) ) // q=')' p='('
		{
			// Keep track of opening/closing parenthesis
			match += 2*(b==q) - 1; // b==')' ++ else --
			if( match == 0 ) // same amount of () => done!
				break;
		}
	}

	if( match == 0 )
	{
		if( !backside ) // |(abc) => (abc)| (abc|) => (|abc)
			return rightOf(np, false);
		return np;
	}

	return dp;
}

unicode Document::findNextBrace( DPos &dp, unicode q, unicode p ) const
{
	// Loop forward word by word to find the next brace
	// specified in the brace parameter.
	DPos np;

	while( (np=rightOf( dp , true )) > dp )
	{
		dp = np;
		unicode ch = text_[dp.tl].str()[dp.ad];
		if( ch == q || ch == p )
			return ch;
	}
	return L'\0';
}

unicode Document::findPrevBrace( DPos &dp, unicode q, unicode p ) const
{
	// Loop back word by word to find the previous brace,
	// specified in the brace parameters.
	DPos np;
	while( (np=leftOf( dp , true )) < dp )
	{
		dp = np;
		unicode ch = text_[dp.tl].str()[dp.ad];
		if( ch == q || ch == p )
			return ch;
	}
	return L'\0';
}

//-------------------------------------------------------------------------
// A set of functions for working with ins, del, etc.
//-------------------------------------------------------------------------

ulong Document::getRangeLength( const DPos& s, const DPos& e )
{
	// Just add it all up.
	ulong ans=0, tl=s.tl, te=e.tl;
	for( ; tl<=te; ++tl )
		ans += len(tl);
	// subtract the first line
	ans -= s.ad;
	// Subtract the last line
	ans -= len(te) - e.ad;
	// Add the portion of the line feed code (CRLF)
	ans += (e.tl-s.tl) * 2;
	// The end, The end
	return ans;
}

void Document::getText( unicode* buf, const DPos& s, const DPos& e )
{
	if( !buf ) return;
	if( s.tl == e.tl )
	{
		// If you have only one line
		text_[s.tl].CopyAt( s.ad, e.ad-s.ad, buf );
		buf[e.ad-s.ad] = L'\0';
	}
	else
	{
		// Copy the end of the first line
		buf += text_[s.tl].CopyToTail( s.ad, buf );
		*buf++ = L'\r', *buf++ = L'\n';
		// Copy the middle
		for( ulong i=s.tl+1; i<e.tl; i++ )
		{
			buf += text_[i].CopyToTail( 0, buf );
			*buf++ = L'\r', *buf++ = L'\n';
		}
		// Copy the beginning of the end line
		buf += text_[e.tl].CopyAt( 0, e.ad, buf );
		*buf = L'\0';
	}
}

void Document::CorrectPos( DPos& pos )
{
	// Fix to fall within normal range.
	pos.tl = Min( pos.tl, tln()-1 );
	pos.ad = Min( pos.ad, len(pos.tl) );
}

void Document::CorrectPos( DPos& s, DPos& e )
{
	// Fix it so that s<=e.
	if( s > e )
	{
		ulong t;
		t=s.ad, s.ad=e.ad, e.ad=t;
		t=s.tl, s.tl=e.tl, e.tl=t;
	}
}

bool Document::DeletingOperation
	( DPos& s, DPos& e, unicode*& undobuf, ulong& undosiz )
{
	AutoLock lk( this );

	// position correction, position correction
	CorrectPos( s );
	CorrectPos( e );
	CorrectPos( s, e );

	// count amount to be deleted, Amount to be deleted
	undosiz = getRangeLength( s, e );

	// Allocate buffer for Undo operation
	undobuf = reinterpret_cast<unicode*>( mem().Alloc( (undosiz+1)*2 ) );
	if( undobuf )
	{ // We got enough memory...
		getText( undobuf, s, e );// get text to del for undo
	}

	// delete, delete
	if( s.tl == e.tl )
	{
		// line-by-line deletion
		text_[s.tl].RemoveAt( s.ad, e.ad-s.ad );
	}
	else
	{
		// Remove the end of the first line
		text_[s.tl].RemoveToTail( s.ad );
		// Attach the rest of the end line
		text_[s.tl].InsertToTail( tl(e.tl)+e.ad, len(e.tl)-e.ad );
		// delete a line that doesn't belong
		text_.RemoveAt( s.tl+1, e.tl-s.tl );
	}

	// reanalysis, reanalysis
	return ReParse( s.tl, s.tl );
}

bool Document::InsertingOperation
	( DPos& s, const unicode* str, ulong len, DPos& e, bool reparse )
{
	AutoLock lk( this );

	// position correction, position correction
	CorrectPos( s );

	// Okay, boom! , All right, Go!
	e.ad = s.ad;
	e.tl = s.tl;

	// Preparing to separate the specified string at line breaks
	// Prepare to separate the specified string with a newline.
	const unicode* lineStr=NULL;
	size_t lineLen=0;
	UniReader r( str, len, &lineStr, &lineLen );

	// The first line..., The first line...
	r.getLine();
	text_[e.tl].InsertAt( e.ad, lineStr, lineLen );
	e.ad += lineLen;

	if( !r.isEmpty() )
	{
		// Second to last line, Second to last line
		do
		{
			r.getLine();
			text_.InsertAt( ++e.tl, Line(lineStr, lineLen) );
		} while( !r.isEmpty() );

		// Move the string remaining at the end of the first line to the last line
		// Move the remaining string from the end
		// of the first line to the last line.
		Line& fl = text_[s.tl];
		const ulong ln = fl.size()-e.ad;
		if( ln )
		{
			Line& ed = text_[e.tl];
			ed.InsertToTail( fl.str()+e.ad, ln );
			fl.RemoveToTail( e.ad );
		}

		// Record end position
		e.ad = lineLen;
	}

	// reanalysis, reanalysis
	return reparse && ReParse( s.tl, e.tl );
}


//-------------------------------------------------------------------------
// insert command, insert command
//-------------------------------------------------------------------------

Insert::Insert( const DPos& s, const unicode* str, ulong len, bool del )
	: stt_( s )
	, buf_( str )
	, len_( len )
	, del_( del )
{
}

Insert::~Insert()
{
	if( del_ )
		mem().DeAlloc( const_cast<unicode*>(buf_), (len_+1)*2 );
}

Command* Insert::operator()( Document& d ) const
{
	// insertion
	DPos s=stt_, e;
	bool aa = d.InsertingOperation( s, buf_, len_, e );

	// event firing, event firing
	d.Fire_TEXTUPDATE( s, s, e, aa, true );

	// Return the reverse operation object
	return new Delete( s, e );
}



//-------------------------------------------------------------------------
// delete command, delete command
//-------------------------------------------------------------------------

Delete::Delete( const DPos& s, const DPos& e )
	: stt_( s )
	, end_( e )
{
}

Command* Delete::operator()( Document& d ) const
{
	// deletion
	unicode* buf;
	ulong    siz;
	DPos s = stt_, e=end_;
	bool aa = d.DeletingOperation( s, e, buf, siz );

	// event firing, event firing
	d.Fire_TEXTUPDATE( s, e, s, aa, true );

	// Return the reverse operation object
	Insert *rev = NULL;
	if( buf != NULL )
	{	// Make reverse operation...
		rev = new Insert( s, buf, siz, true );
		if( !rev ) // Clear Undo buffer on failure
			mem().DeAlloc( buf, (siz+1)*2 );
	}
	return rev;
}



//-------------------------------------------------------------------------
// Replace command, Replace command
//-------------------------------------------------------------------------

Replace::Replace(
	const DPos& s, const DPos& e, const unicode* str, ulong len, bool del )
	: stt_( s )
	, end_( e )
	, buf_( str )
	, len_( len )
	, del_( del )
{
}

Replace::~Replace()
{
	if( del_ )
		mem().DeAlloc( const_cast<unicode*>(buf_), (len_+1)*2 );
}

Command* Replace::operator()( Document& d ) const
{
	// deletion
	unicode* buf;
	ulong    siz;
	DPos s=stt_, e=end_;
	bool aa = d.DeletingOperation( s, e, buf, siz );

	// insertion
	DPos e2;
	aa = (d.InsertingOperation( s, buf_, len_, e2 ) || aa);

	// event firing, event firing
	d.Fire_TEXTUPDATE( s, e, e2, aa, true );

	// Return the reverse operation object
	Replace *rev = NULL;
	if( buf )
	{	// Make reverse operation...
		rev = new Replace( s, e2, buf, siz, true );
		if( !rev ) // Clear Undo buffer on failure
			mem().DeAlloc( buf, (siz+1)*2 );
	}
	return rev;
}



//-------------------------------------------------------------------------
// macro command, macro command
//-------------------------------------------------------------------------

Command* MacroCommand::operator()( Document& doc ) const
{
	doc.setBusyFlag(true);

	MacroCommand* undo = new MacroCommand;
	if( !undo ) return NULL;
	undo->arr_.ForceSize( size() );

	size_t e = arr_.size();
	if( e > 4 )
	{
		// Accumulate TEXTUPDATE events
		// Except for the first and last command,
		// so that the cursor behaves as exected.
		undo->arr_[e-1] = (*arr_[0])(doc); // 1st command

		doc.acc_Fire_TEXTUPDATE_begin();
		for( size_t i=1; i<e-1; ++i )
			undo->arr_[e-i-1] = (*arr_[i])(doc);
		doc.acc_Fire_TEXTUPDATE_end();

		undo->arr_[0] = (*arr_[e-1])(doc); // Last command
	}
	else
	{
		for( size_t i=0; i<e; ++i )
			undo->arr_[e-i-1] = (*arr_[i])(doc);
	}

	doc.setBusyFlag(false);
	return undo;
}



//-------------------------------------------------------------------------
// Save to file, Save to file
//-------------------------------------------------------------------------

void Document::SaveFile( ki::TextFileW& tf )
{
	urdo_.SavedHere();
	for( ulong i=0,iLast=tln()-1; i<=iLast; ++i )
		tf.WriteLine( tl(i), len(i), i==iLast );
}



//-------------------------------------------------------------------------
// Destroy all buffer contents (temporary)
//-------------------------------------------------------------------------

void Document::ClearAll()
{
	// delete everything, delete everything
	Execute( Delete( DPos(0,0), DPos(0xffffffff,0xffffffff) ) );

	// Clear Undo-Redo chain
	// Clear the Undo-Redo chain
	urdo_.Clear();
	urdo_.SavedHere();
	Fire_MODIFYFLAGCHANGE();
}



//-------------------------------------------------------------------------
// Open a file (temporary)
//-------------------------------------------------------------------------

void Document::OpenFile( TextFileR& tf )
{
	// ToDo: multi-threaded, ToDo: multi-threaded
	//currentOpeningFile_ = tf;
	//thd().Run( *this );
	// insertion
	DPos e(0,0);

	// Super small stack buffer in case the malloc fails
	#define SBUF_SZ 1800
	unicode sbuf[SBUF_SZ];

	// Use big buffer (much faster on long lines)
#ifdef WIN64
	size_t buf_sz = 2097152;
	// static unicode buf[2097152]; // 4MB on x64
#else
	// static unicode buf[131072]; // 256KB on i386
	size_t buf_sz = 88000;
#endif
	// Do not allocate more mem than twice the file size in bytes.
	// Should help with loaing small files on Win32s.
	buf_sz = Min( buf_sz, (size_t)(tf.size()+16)<<1 );
	unicode *buf=NULL;
	if( buf_sz > SBUF_SZ )
		buf = (unicode *)TS.alloc( sizeof(unicode) * buf_sz );
	if( !buf )
	{
		buf = sbuf;
		buf_sz = SBUF_SZ;
	}
//	DWORD otime = GetTickCount();
	size_t L;
	setBusyFlag(true);
	for( ulong i=0; L = tf.ReadBuf( buf, buf_sz ); )
	{
		DPos p( i, len(e.tl) ); // end of document
		InsertingOperation( p, buf, (ulong)L, e, /*reparse=*/false );
		i = tln() - 1;

		// Process Messages to avoid locking on large files
//		MSG msg;
//		while ( PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE ) )
//		{
//			BOOL result = GetMessage(&msg, NULL, 0, 0);
//			if (result == 0) // WM_QUIT
//			{
//				PostQuitMessage(msg.wParam);
//				goto fail;
//			}
//			else if (result == -1) // ERROR
//			{
//				goto fail;
//			}
//			else
//			{
//				TranslateMessage(&msg);
//				DispatchMessage(&msg);
//			}
//		}
	}
	// Parse All lines, because we skipped it
	ReParse( 0, tln()-1 );
	setBusyFlag(false);
//	fail:
	if( buf != sbuf )
		TS.freelast( buf, sizeof(unicode) * buf_sz );

//	MessageBox(GetActiveWindow(),  SInt2Str(GetTickCount()-otime).c_str(), TEXT("Time in ms:"), 0);
	// event firing, event firing
	Fire_TEXTUPDATE( DPos(0,0), DPos(0,0), e, true, false );
}


/*
void Document::StartThread()
{
	// ToDo:
	aptr<TextFileR> tf = currentOpeningFile_;

	// insert
	unicode buf[1024];
	while( !isExitRequested() && tf->state() )
	{
		ulong i, j;
		DPos s(i=tln()-1, j=len(i)), e;
		if( ulong L = tf->ReadLine( buf, countof(buf) ) )
			InsertingOperation( s, buf, L, e );
		if( tf->state() == 1 )
			InsertingOperation( DPos(i,0xffffffff), L"\n", 1, e );

		// event firing
		Fire_TEXTUPDATE( s, s, e, true, false );
	}

}
*/
