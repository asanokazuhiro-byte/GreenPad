#ifndef _KILIB_FILE_H_
#define _KILIB_FILE_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	Simple file reading
//
//	Since it uses file mapping, it is easy to handle and relatively fast.
//	However, the problem is that you can only open up to 4GB.
//@}
//=========================================================================

class FileR
{
public:

	FileR()
		: handle_ ( INVALID_HANDLE_VALUE )
		, fmo_    ( NULL )
		, size_   ( 0 )
		, basePtr_( NULL ) {}
	~FileR() { Close(); }

	//@{
	//	open
	//	@param fname file name
	//	@return Opened or not?
	//@}
	bool Open( const TCHAR* fname, bool always=false );

	//@{
	//	close
	//@}
	void Close();

public:

	//@{ file size //@}
	size_t size() const
		{ return size_; };

	//@{ Get address mapped with file contents //@}
	const uchar* base() const
		{ return static_cast<const uchar*>(basePtr_); }

private:

	HANDLE      handle_;
	HANDLE      fmo_;
	size_t      size_;
	const void* basePtr_;

private:

	NOCOPY(FileR);
};



//=========================================================================
//@{
//	Simple file writing
//
//	While buffering a lot.
//@}
//=========================================================================

class FileW
{
public:

	FileW();
	~FileW();

	//@{ open //@}
	bool Open( const TCHAR* fname, bool creat=true );

	//@{ close //@}
	void Close();

	//@{ write //@}
	void Write( const void* buf, size_t siz );

	//@{Write one character //@}
	void WriteC( const uchar ch );

	//@{ Write a character without checking bufer //@}
	inline void WriteCN( const uchar ch )
		{ buf_[bPos_++] = ch; }

	//@{ Flush if needed to get the specified space //@}
	inline void NeedSpace( const size_t sz )
		{ if( (BUFSIZE-bPos_) <= sz ) Flush(); }

	//@{ Writes to the file using a specific output codepage //@}
	void WriteInCodepageFromUnicode( int cp, const unicode* str, size_t len );

public:

	void Flush();

private:

	enum { BUFSIZE = 32768 };
	HANDLE       handle_;
	uchar* const buf_;
	size_t       bPos_;

private:

	NOCOPY(FileW);
};

//=========================================================================

}      // namespace ki
#endif // _KILIB_FILE_H_
