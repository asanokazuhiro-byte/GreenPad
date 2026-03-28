#ifndef _EDITWING_IP_DOC_H_
#define _EDITWING_IP_DOC_H_
#include "ewDoc.h"

#ifndef __ccdoc__
namespace editwing {
namespace doc {
#endif



//@{ @pkg editwing.Doc.Impl //@}
class Parser;



//=========================================================================
//@{
//	Row buffer structure
//
//	Holds text data as raw UCS-2. Also maintains a buffer for parse results
//	used to distinguish keywords specified by the keyword file.
//	Character data is not NUL-terminated; instead, U+007F is appended
//	after the last character to speed up parsing.
//@}
//=========================================================================
#ifdef USE_ORIGINAL_MEMMAN
	// be sure to align alloc evenly because unicode is 2 bytes long
	// and we append bytes, but alignement should remain 2bytes
	// Otherwise some functions might fail such as TextOutW
	//#define EVEN(x) ( ((x)+1)&(~1ul) )
	// No more needed as we round up all allocations
	#define EVEN(x) (x)
#else
	#define EVEN(x) (x)
#endif
class Line //: public ki::Object
{
public:

	//@{ Initialize with specified text //@}
	Line( const unicode* str, size_t len )
		: commentBitReady_( 0 )
		, isLineHeadCommented_( 0 )
		, alen_( len )
		, commentTransition_( 0 )
		, len_ ( len )
		, str_ ( len==0? empty_buf() :static_cast<unicode*>( ki::mem().Alloc(EVEN((alen_+1)*2+alen_)) ) )
		{
			if( !str_ )
			{	// Allocation failed!
				len_ = 0;
				alen_ = 0;
				str_ = empty_buf();
				return;
			}
			memmove( str_, str, len*2 );
			str_[ len ] = 0x007f;
		}

	void Clear() // Manually destroy line
	{
		if( str_ != empty_buf() )
			ki::mem().DeAlloc( str_, EVEN((alen_+1)*2+alen_) );
	}

	//@{ Insert text (specified position, specified size) //@}
	void InsertAt( size_t at, const unicode* buf, size_t siz )
	{
		uchar *flgs = flg(); // str_+alen_+1;
		if( len_+siz > alen_ )
		{
			// buffer expansion
			size_t psiz = (alen_+1)*2+alen_;
			size_t nalen = Max( (size_t)(alen_+(alen_>>1)), len_+siz ); // len_+siz;
			unicode* tmpS;
			TRYAGAIN:
				tmpS = static_cast<unicode*>( ki::mem().Alloc( EVEN((nalen+1)*2+nalen) ) );
			if( !tmpS )
			{
				if( nalen > len_+siz )
				{	// Try again with minimal buffer size...
					nalen = len_+siz;
					goto TRYAGAIN;
				}
				return;
			}
			uchar*   tmpF =
				reinterpret_cast<uchar*>(tmpS+nalen+1);
			alen_ = nalen;
			// copy
			memmove( tmpS,        str_,             at*2 );
			memmove( tmpS+at+siz, str_+at, (len_-at+1)*2 );
			memmove( tmpF,        flgs,             at   );
			// delete old
			if( str_ != empty_buf() )
				ki::mem().DeAlloc( str_, EVEN(psiz) );
			str_ = tmpS;
		}
		else
		{
			memmove( str_+at+siz, str_+at, (len_-at+1)*2 );
			memmove( flgs+at+siz, flgs+at, (len_-at)     );
		}
		memmove( str_+at, buf, siz*sizeof(unicode) );
		len_ += siz;
	}

	//@{ Insert text (at the end) //@}
	void InsertToTail( const unicode* buf, size_t siz )
	{
		InsertAt( len_, buf, siz );
	}

	//@{ Delete text (specified size from specified position) //@}
	void RemoveAt( size_t at, size_t siz )
	{
		uchar *flgs = flg();
		memmove( str_+at, str_+at+siz, (len_-siz-at+1)*2 );
		memmove( flgs+at, flgs+at+siz, (len_-siz-at)     );
		len_ -= siz;
	}

	//@{ Delete text (from specified position to end) //@}
	void RemoveToTail( size_t at )
	{
		if( at < len_ )
			str_[ len_=at ] = 0x007f;
	}

	//@{ Copy to buffer (specified size from specified position) //@}
	size_t CopyAt( size_t at, size_t siz, unicode* buf )
	{
		memmove( buf, str_+at, siz*sizeof(unicode) );
		return siz;
	}

	//@{ Copy to buffer (from specified position to end) //@}
	size_t CopyToTail( size_t at, unicode* buf )
	{
		return CopyAt( at, len_-at, buf );
	}

	//@{ length //@}
	size_t size() const
		{ return len_; }

	//@{ text //@}
	unicode* str()
		{ return str_; }

	//@{ text(const) //@}
	const unicode* str() const
		{ return str_; }

	//@{ Analysis result //@}
	uchar* flg()
		{ return reinterpret_cast<uchar*>(str_+alen_+1); }

	//@{ Analysis result (const) //@}
	const uchar* flg() const
		{ return reinterpret_cast<const uchar*>(str_+alen_+1); }

	// ask
	uchar isCmtBitReady() const
		{ return commentBitReady_; }
	uchar isLineHeadCmt() const
		{ return isLineHeadCommented_; }
	// for doc
	uchar TransitCmt( uchar start )
	{
		isLineHeadCommented_ = start;
		commentBitReady_     = false;
		return ((uchar)commentTransition_>>start)&1;
	}
	// for parser
	void SetTransitFlag( uchar flag )
		{ commentTransition_ = flag; }
	void CommentBitUpdated()
		{ commentBitReady_   = true; }

private:
	// 32 bit mode: max_line_length = 2^30 * 3 = 3GB
	// Which is larger than the max 2GB adress space.
	size_t   commentBitReady_:      1;
	size_t   isLineHeadCommented_:  1;
	size_t   alen_: sizeof(size_t)*8-2;
	size_t   commentTransition_:    2;
	size_t   len_:  sizeof(size_t)*8-2;
	unicode* str_;

	static unicode* empty_buf()
		{ static unicode empty[2] = { 0x7F, 0 }; return empty; }
};

#undef EVEN


//=========================================================================
//@{
//	Unicode text segmenter
//
//	Since processing is often performed on a line-by-line basis, separate processing for each line is
//	I cut it out. getLine() to the specified pointer and integer variable.
//	Row data is stored sequentially from the beginning.
//@}
//=========================================================================

class UniReader
{
public:

	//@{ Initialize by giving the source buffer //@}
	UniReader(
		const unicode*     str, size_t     len,
		const unicode** ansstr, size_t* anslen )
		: ptr_  ( str )
		, end_  ( str+len )
		, ans_  ( ansstr )
		, aln_  ( anslen )
		, empty_( false ) {}

	//@{Check if you have finished reading //@}
	bool isEmpty()
		{ return empty_; }

	//@{ Get one line //@}
	void A_HOT getLine()
	{
		// Get the position of the next line break
		const unicode *p=ptr_, *e=end_;
		for( ; p<e; ++p )
			if( *p == L'\r' || *p == L'\n' )
				break;
		// record
		*ans_  = ptr_;
		*aln_  = int(p-ptr_);
		// Line break code skip
		if( p == e )
			empty_ = true;
		else
			if( *(p++)==L'\r'&& p<e && *p==L'\n' )
				++p;
		ptr_  = p;
	}

private:
	const unicode *ptr_, *end_, **ans_;
	size_t *aln_;
	bool  empty_;
};



//=========================================================================
//@{
//	A class that stores Command objects for Undo/Redo.
//@}
//=========================================================================

class UnReDoChain : public ki::Object
{
public:

	//@{ constructor //@}
	UnReDoChain();

	//@{ destructor //@}
	~UnReDoChain();

	//@{ Execute Undo //@}
	void Undo( Document& doc );

	//@{ Execute Redo //@}
	void Redo( Document& doc );

	//@{ Execute new command //@}
	void NewlyExec( const Command& cmd, Document& doc );

	//@{Return to initial state //@}
	void Clear();

	//@{Set save position flag to current position //@}
	inline void SavedHere() { savedPos_ = lastOp_; }

	//@{ Specify the Undo/Redo count limit. -1 = Infinite //@}
	void SetLimit( long lim );

public:

	//@{ Is Undo operation possible? //@}
	inline bool isUndoAble() const { return (lastOp_ != &headTail_); }

	//@{ Is Redo operation possible? //@}
	inline bool isRedoAble() const { return (lastOp_->next_ != &headTail_); }

	//@{ Has it changed after saving? //@}
	inline bool isModified() const { return (lastOp_ != savedPos_); }

private:

	struct Node : public ki::Object
	{
		Node();
		Node( Command*, Node*, Node* );
		~Node();
		void  ResetCommand( Command* cmd );
		ulong ChainDelete(Node*& savedPos_ref);
		Node    *next_, *prev_;
		Command *cmd_;
	};

private:

	Node  headTail_;
	Node* savedPos_;
	Node* lastOp_;
	size_t num_;
	size_t limit_;

private:

	NOCOPY(UnReDoChain);
};


//=========================================================================
//@{
//	Implementation part of Document class
//@}
//=========================================================================

class Document A_FINAL : public ki::Object
#ifdef USE_THREADS
              , private ki::EzLockable
              , private ki::Runnable
#else
              , private ki::NoLockable
#endif
{
public:

	Document();
	~Document();

	//@{Execute operation command //@}
	void Execute( const Command& cmd );

	//@{ Switch keyword definition //@}
	void SetKeyword( const unicode* defbuf, size_t siz );

	//@{Event handler registration //@}
	void AddHandler( DocEvHandler* eh );

	//@{ Release event handler //@}
	void DelHandler( const DocEvHandler* eh );

	//@{ Open file //@}
	void OpenFile( ki::TextFileR& tf );

	//@{Save file //@}
	void SaveFile( ki::TextFileW& tf );

	//@{Discard contents //@}
	void ClearAll();

	//@{ undo //@]
	void Undo();

	//@{Redo //@]
	void Redo();

	//@{ Undo count limit //@]
	void SetUndoLimit( long lim );

	//@{ Clear change flag //@}
	void ClearModifyFlag();

public:

	//@{number of lines //@}
	inline ulong tln() const { return text_.size(); }

	//@{ line buffer //@}
	inline const unicode* tl( ulong i ) const { return text_[i].str(); }

	//@{ Row parsing result buffer //@}
	const uchar* pl( ulong i ) const
	{
		const Line& x = text_[i];
		if( !x.isCmtBitReady() )
			SetCommentBit( x );
		return x.flg();
	}

	//@{ Number of line characters //@}
	ulong len( ulong i ) const { return text_[i].size(); }

	//@{ Length of text in specified range //@}
	ulong getRangeLength( const DPos& stt, const DPos& end );

	//@{ Range of text //@}
	void getText( unicode* buf, const DPos& stt, const DPos& end );

	//@{ Get the beginning of the word at the specified position //@}
	DPos wordStartOf( const DPos& dp ) const;

	unicode findNextBrace( DPos &dp, unicode q, unicode p ) const;
	unicode findPrevBrace( DPos &dp, unicode q, unicode p ) const;
	DPos findMatchingBrace( const DPos &dp ) const;

	//@{ Get the position one position to the left of the specified position //@}
	DPos leftOf( const DPos& dp, bool wide=false ) const;

	//@{ Get the position one position to the right of the specified position //@}
	DPos rightOf( const DPos& dp, bool wide=false ) const;

	//@{ Is it possible to undo? //@}
	bool isUndoAble() const;

	//@{ Is it possible to redo? //@}
	bool isRedoAble() const;

	//@{ Modified? //@}
	bool isModified() const;

	const unicode* getCommentStr() const { return CommentStr_; }

	//@{ Busy flag (satisfied only during macro command execution) //@}
	void setBusyFlag( bool b ) { busy_ = b; }
	bool isBusy() const { return busy_; }

private:

	enum { MAX_EVHAN = 4 };
	ki::uptr<Parser>             parser_; // string analyzer

	unicode               CommentStr_[8];
	ki::gapbufobjnoref<Line>       text_;   // text data
	size_t                     evHanNum_;
	DocEvHandler*     pEvHan_[MAX_EVHAN]; // Event notification destination
	UnReDoChain                    urdo_;   // Undurido
	editwing::DPos acc_s_, acc_e2_;
	bool busy_;
	bool acc_textupdate_mode_;
	bool acc_reparsed_;
	bool acc_nmlcmd_;

public:
	void acc_Fire_TEXTUPDATE_begin();
	void acc_Fire_TEXTUPDATE_end();
private:

	// Change notification
	void Fire_KEYWORDCHANGE();
	void Fire_MODIFYFLAGCHANGE();
	void Fire_TEXTUPDATE( const DPos& s,
		const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd );

	// helper function
	bool ReParse( ulong s, ulong e );
	void SetCommentBit( const Line& x ) const;
	void CorrectPos( DPos& pos );
	void CorrectPos( DPos& stt, DPos& end );

	// Insertion/deletion work
	bool InsertingOperation(
		DPos& stt, const unicode* str, ulong len, DPos& undoend, bool reparse=true );
	bool DeletingOperation(
		DPos& stt, DPos& end, unicode*& undobuf, ulong& undosiz );

	// parallel lead
	//virtual void StartThread();

private:

	NOCOPY(Document);
	friend class Insert;
	friend class Delete;
	friend class Replace;
};

//=========================================================================

}}     // namespace editwing::doc
#endif // _EDITWING_IP_DOC_H_
