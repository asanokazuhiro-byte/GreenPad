#ifndef _KILIB_PATH_H_
#define _KILIB_PATH_H_
#include "types.h"
#include "kstring.h"

DWORD GetFileAttributesUNC(LPCTSTR fname) A_NONNULL;
HANDLE CreateFileUNC(
	LPCTSTR fname,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
);

#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	File name processing
//
//	Basically it is a string, but it is especially useful when treating it as a file name.
//	It's a class. Extract only the name part, extract only the extension, etc.
//	Retrieving attributes and various other things.
//@}
//=========================================================================

class A_WUNUSED Path A_FINAL: public String
{
public:

	Path() {}

	//@{ Copy another Path //@}
	Path( const Path&   s )              : String( s ){}
	Path( const String& s )              : String( s ){}
	Path( const TCHAR*  s, long siz=-1 ) : String( s, siz ){}

	//@{simple assignment //@}
	Path& operator=( const Path&   s )
		{ return static_cast<Path&>(String::operator=(s)); }
	Path& operator=( const String& s )
		{ return static_cast<Path&>(String::operator=(s)); }
	Path& operator=( const TCHAR*  s )
		{ return static_cast<Path&>(String::operator=(s)); }

	//@{ addition assignment //@}
	Path& operator+=( const Path&   s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( const String& s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( const TCHAR*  s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( TCHAR c )
		{ return static_cast<Path&>(String::operator+=(c)); }

	//@{ Get special path and initialize //@}
	explicit Path( int nPATH, bool bs=true )
		{ BeSpecialPath( nPATH, bs ); }

	//@{ Get special path //@}
	Path& BeSpecialPath( int nPATH, bool bs=true );

	//@{ Constant for special path specification //@}
	enum { Win=0x1787, Sys, Tmp, Exe, Cur, ExeName,
			Snd=CSIDL_SENDTO, Dsk=CSIDL_DESKTOPDIRECTORY };

	//@{ Add a backslash at the end (true)/Do not add a backslash (false) //@}
	Path& BeBackSlash( bool add );

	//@{ Drive name or root only //@}
	Path& BeDriveOnly();

	//@{ directory name only //@}
	Path& BeDirOnly();

	//@{ short path name //@}
	Path& BeShortStyle();

	//@{ Make sure the file name is long //@}
	Path& BeShortLongStyle();

	//@{ ... to make it shorter //@}
	void CompactIfPossible(TCHAR *buf, size_t Mx) A_NONNULL;

	//@{ Other than directory information //@}
	inline const TCHAR* name() const { return name(c_str()); }

	//@{ last extension //@}
	inline const TCHAR* ext() const { return ext(c_str()); }

	//@{ All extensions after the first . //@}
	inline const TCHAR* ext_all() const { return ext_all(c_str()); }

	//@{ Name part excluding directory information and final extension //@}
	Path body() const;

	//@{ Name part excluding directory information and everything after the first . //@}
	Path body_all() const;

public:

	//@{ File or not //@}
	inline bool isFile() const
		{ return c_str()[len()-1] != TEXT('\\') && c_str()[len()-1] != TEXT('/')
	      && 0==(GetFileAttributesUNC(c_str())&FILE_ATTRIBUTE_DIRECTORY); }


	//@{ Directory or not //@}
	inline bool isDirectory() const { return isDirectory( c_str() ); }
	inline static bool isDirectory( const TCHAR *fn ) A_NONNULL
		{ DWORD x=GetFileAttributesUNC( fn );
		  return x!=0xffffffff && (x&FILE_ATTRIBUTE_DIRECTORY)!=0; }


	//@{ Exists or not. isFile() || isDirectory() //@}
	inline bool exist() const { return exist( c_str() ); }
	inline static bool exist( const TCHAR *fn ) A_NONNULL
		{ return 0xffffffff != GetFileAttributesUNC(fn); }

	//@{ Read-only or not //@}
	inline bool isReadOnly() const { return isReadOnly( c_str() ); }
	inline static bool isReadOnly( const TCHAR *fn )
		{ DWORD x = GetFileAttributesUNC( fn );
		  return x!=0xffffffff && (x&FILE_ATTRIBUTE_READONLY)!=0; }

	inline FILETIME getLastWriteTime() const { return getLastWriteTime( c_str() ); }
	static FILETIME getLastWriteTime( const TCHAR *fn );

public:

	static const TCHAR* name( const TCHAR* str );
	static const TCHAR* ext( const TCHAR* str );
	static const TCHAR* ext_all( const TCHAR* str );
	static DWORD GetExeName( TCHAR buf[MAX_PATH] ) A_NONNULL;
};


//=========================================================================

}      // namespace ki
#endif // _KILIB_PATH_H_
