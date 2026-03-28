#ifndef _KILIB_STRING_H_
#define _KILIB_STRING_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__


#ifdef UNICODE
	#define my_lstrcpy my_lstrcpyW
	#define my_lstrcpys my_lstrcpysW
	#define my_lstrlen my_lstrlenW
	#define my_lstrcmp my_lstrcmpW
	#define my_lstrchr my_lstrchrW
//	#define my_lstrcat my_lstrcatW
	#define my_lstrkpy my_lstrkpyW
	#define my_lstrcmpiAscii my_lstrcmpiAsciiW
#else
	#define my_lstrcpy my_lstrcpyA
	#define my_lstrcpys my_lstrcpysA
	#define my_lstrlen my_lstrlenA
	#define my_lstrcmp my_lstrcmpA
	#define my_lstrchr my_lstrchrA
//	#define my_lstrcat my_lstrcatA
	#define my_lstrkpy my_lstrkpyA
	#define my_lstrcmpiAscii my_lstrcmpiAsciiA
#endif


wchar_t * WINAPI my_CharUpperWW(wchar_t *s);
wchar_t * WINAPI my_CharLowerWW(wchar_t *s);
#define my_CharUpperW my_CharUpperWW
#define my_CharLowerW my_CharLowerWW
wchar_t my_CharUpperSingleW(wchar_t c);
wchar_t my_CharLowerSingleW(wchar_t c);
BOOL my_IsCharLowerW(wchar_t c);

const TCHAR *Int2lStr(TCHAR str[INT_DIGITS+1], int n) A_NONNULL;
const TCHAR *Ulong2lStr(TCHAR str[ULONG_DIGITS+1], ulong n) A_NONNULL;
const TCHAR *LPTR2Hex(TCHAR str[ULONG_DIGITS+1], UINT_PTR n) A_NONNULL;
ulong Hex2Ulong( const TCHAR *str ) A_NONNULL;
ulong Octal2Ulong( const TCHAR *s ) A_NONNULL;

//inline static
//const char *my_lstrcatA(char *dest, const char * restrict src)
//{
//	char *orig=dest;
//	for (; *dest; ++dest) ; /* go to end of dest */
//	for (; (*dest=*src); ++src,++dest) ; /* then append from src */
//	return orig;
//}
//
//inline static
//const wchar_t *my_lstrcatW(wchar_t *dest, const wchar_t * restrict src)
//{
//	wchar_t *orig=dest;
//	for (; *dest; ++dest) ; /* go to end of dest */
//	for (; (*dest=*src); ++src,++dest) ; /* then append from src */
//	return orig;
//}

inline static
char *my_lstrkpyA(char *dest, const char * restrict src)
{
	for (; (*dest=*src); ++src,++dest);
	return dest;
}

inline static
wchar_t *my_lstrkpyW(wchar_t *dest, const wchar_t * restrict src)
{
	for (; (*dest=*src); ++src,++dest);
	return dest;
}

inline static
const char *my_lstrchrA(const char *str, char c)
{
    while(*str != c) {
        if(!*str) return NULL;
        str++;
    }
    return str;
}
inline static
const unicode *my_lstrchrW(const unicode *str, unicode c)
{
    while(*str != c) {
        if(!*str) return NULL;
        str++;
    }
    return str;
}

static inline
void my_lstrcpyW( unicode* const d, const unicode* restrict s )
{
	for(unicode* n=d; (*n++ = *s++););
}
inline static
size_t my_lstrlenW( const unicode* const d )
{
	const unicode* n=d;
	for(; *n; ++n);
	return static_cast<size_t>(n-d);
}
static inline
int my_lstrcmpW(const unicode *X, const unicode *Y)
{
	while (*X && *X == *Y) { X++; Y++; }
	return *X - *Y;
}
static inline
int my_lstrncmpW(const unicode *X, const unicode *Y, size_t N)
{
	while (--N && *X && *X == *Y) { X++; Y++; }
	return *X - *Y;
}
static inline
bool my_instringW(const unicode *X, const unicode *Y)
{ // return true if we find Y in the X string.
	while (*X && *X == *Y) { X++; Y++; }
	return !*Y; // Match if we reached the end of Y
}
inline static
void my_lstrcpyA( char* const d, const char * restrict s )
{
	for(char* n=d; (*n++ = *s++););
}
inline static
size_t my_lstrlenA( const char* const d )
{
	const char* n;
	for(n=d; *n; ++n);
	return static_cast<size_t>(n-d);
}
static inline
int my_lstrcmpA(const char *X, const char *Y)
{
	while (*X && *X == *Y) { X++; Y++; }
	return *(const unsigned char*)X - *(const unsigned char*)Y;
}

#define tolowerASCII(x) ( (x) | ('A'<=(x) && (x)<='Z') << 5 )
static inline
int my_lstrcmpiAsciiA(const char *X, const char *Y)
{
	while ( *X && tolowerASCII(*X) == tolowerASCII(*Y) ) { X++; Y++; }
	return tolowerASCII( *(const unsigned char*)X )
	     - tolowerASCII( *(const unsigned char*)Y );
}
static inline
int my_lstrcmpiAsciiW(const wchar_t *X, const wchar_t *Y)
{
	while ( *X && tolowerASCII(*X) == tolowerASCII(*Y) ) { X++; Y++; }
	return tolowerASCII( *X )
	     - tolowerASCII( *Y );
}

static inline
int looseStrCmp( LPCTSTR a, LPCTSTR b )
{
	enum { FLAGS =  NORM_IGNORECASE|NORM_IGNOREKANATYPE|NORM_IGNORENONSPACE|NORM_IGNORESYMBOLS|NORM_IGNOREWIDTH };
	return CompareString( LOCALE_USER_DEFAULT, FLAGS, a, -1, b, -1) - 2; // 2 -> 0
}

static inline
void my_lstrcpysW(wchar_t *out, size_t outlen, const wchar_t * restrict in)
{
	size_t i;
	for (i=0; i<outlen-1 && in[i]; i++)
	{
		out[i] = in[i];
	}
	out[i] = TEXT('\0');
}
static inline
void my_lstrcpysA(char *out, size_t outlen, const char * restrict in)
{
	size_t i;
	for (i=0; i<outlen-1 && in[i]; i++)
	{
		out[i] = in[i];
	}
	out[i] = TEXT('\0');
}

namespace ki {
#endif
#ifdef _UNICODE
	#define XTCHAR char
#else
	#define XTCHAR wchar_t
#endif

//=========================================================================
//@{
//	String processing + α2
//
//	Create the wide text version function yourself
//@}
//=========================================================================
//#undef lstrcpy
//#undef lstrlen
//#undef lstrcmp


//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	String processing
//
//	It pretty much copies MFC's CString. For now, use operator=
//	We made simple assignments almost costless. SubStr()
//	I also thought about not copying the time, but until then
//	I feel like I don't need it...
//@}
//=========================================================================

class A_WUNUSED String
{
public:

	//@{Create empty string //@}
	inline String() { SetData( null() ); }
	~String();

	//@{ Copying another String //@}
	inline String( const String& obj ) { SetData( obj.data() ); }

	//@{ Copying another character array //@}
	String( const TCHAR* str, long siz=-1 );

	//@{ Create from resource //@}
	inline explicit String( UINT rsrcID ) { SetData( null() ), Load( rsrcID ); }

	//@{ Case-sensitive comparison //@}
	inline bool operator==( LPCTSTR s ) const
		{ return 0==my_lstrcmp( c_str(), s ); }
	inline bool operator==( const String& obj ) const
		{ return (data_==obj.data_ ? true : operator==( obj.c_str() )); }

	//@{ Case-insensitive comparison //@}
	bool isSame( LPCTSTR s ) const
		{ return 0==::lstrcmpi( c_str(), s ); }
	bool isSame( const String& obj ) const
		{ return (data_==obj.data_ ? true : operator==( obj.c_str() )); }

	//@{simple assignment //@}
	String& operator=( const String& obj );
	String& operator=( const TCHAR* s ) { return SetString( s, my_lstrlen(s) ); }
	String& operator=( const XTCHAR* s );

	//@{ addition assignment //@}
	String& operator+=( const String& obj )
		{ return CatString( obj.c_str(), obj.len() ); }
	String& operator+=( const TCHAR* s )
		{ return CatString( s, my_lstrlen(s) ); }
	String& operator+=( TCHAR c )
		{ return CatString( &c, 1 ); }

#ifdef _UNICODE
	String& operator+=( const char* s );
#else
	String& operator+=( const wchar_t* s );
#endif

	//@{ Resource load //@}
	String& Load( UINT rsrcID );

	//@{ cut right //@}
	void TrimRight( size_t siz );

	//@{ Convert int to string //@}
	String& SetInt( int n );

	String& SetUlong( ulong n );

	//@{ Convert string to int //@}
	int GetInt() const { return GetInt( data_->buf() ); }
	int GetULong() const { return GetULong( data_->buf() ); }

public:

	//@{ return string buffer //@}
	inline const TCHAR* c_str() const { return data_->buf(); }

	//@{ length //@}
	inline size_t len() const { return data_->len-1; }

	//@{ element //@}
	inline TCHAR operator[](int n) const { return data_->buf()[n]; }

	//@{ Convert to wide string and return //@}
	const wchar_t* ConvToWChar() const;
	const char* ConvToChar() const;

	//@{ Release the return value buffer of ConvTo(W)Char //@}
	void FreeWCMem( const wchar_t* wc ) const;
	void FreeCMem( const char* str ) const;

	static bool isCompatibleWithACP(const TCHAR *uni, size_t len);
	bool isCompatibleWithACP() const;

public:

	//@{ Next character //@}
	static TCHAR*       next( TCHAR* p ) A_NONNULL;
	static const TCHAR* next( const TCHAR* p ) A_NONNULL;

	//@{ Is it the beginning of a double-byte character? //@}
	static bool isLB( TCHAR c );

	//@{ Convert string to int //@}
	static int GetInt( const TCHAR* p ) A_NONNULL;
	static ulong GetULong( const TCHAR* x ) A_NONNULL;

protected:

	// Prepare a writable buffer for at least minimum characters including the end
	inline TCHAR* AllocMem( size_t minimum )
		{ return AllocMemHelper( minimum, TEXT(""), 1 ); }
	TCHAR* ReallocMem( size_t minimum=0 );

	// After writing, reset the length
	void UnlockMem( long siz=-1 )
		{ data_->len = 1 + (siz==-1 ? my_lstrlen(c_str()) : siz); }

private:

	struct StringData
	{
		long  ref;         // reference counter
		size_t len;        // Length including terminating '\0'
		size_t alen;       // Size of allocated memory
		TCHAR* buf() const // TCHAR buf[alen]
			{ return reinterpret_cast<TCHAR*>(
				const_cast<StringData*>(this+1)
			); }
	};

private:

	TCHAR*  AllocMemHelper( size_t minimum, const TCHAR* str, size_t siz ) A_NONNULL;
	String& CatString( const TCHAR* str, size_t siz ) A_NONNULL;
	String& SetString( const TCHAR* str, size_t siz ) A_NONNULL;
	inline void   SetData( StringData* d ) A_NONNULL { data_=d, data_->ref++; } // Initialization
	void          ReleaseData();
	inline static StringData* null() { return nullData_; } // 0 character data
	inline        StringData* data() const { return data_; } // Internal data structure

private:

	StringData*        data_;
	static StringData* nullData_;
#if !defined(_UNICODE) && defined(_MBCS)
	static char        lb_[256];
#endif

private:

	static void LibInit();
	friend void APIENTRY Startup();
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__


// Pointer calculation support
#if !defined(_UNICODE) && defined(_MBCS)
	inline TCHAR* String::next( TCHAR* p )
		{ return p + lb_[*(uchar*)p]; }
	inline const TCHAR* String::next( const TCHAR* p )
		{ return p + lb_[*(const uchar*)p]; }
	inline bool String::isLB( TCHAR c )
		{ return lb_[(uchar)c]==2; }
#else // _UNICODE or _SBCS
	inline TCHAR* String::next( TCHAR* p )
		{ return p + 1; }
	inline const TCHAR* String::next( const TCHAR* p )
		{ return p + 1; }
	inline bool String::isLB( TCHAR c )
		{ return false; }
#endif


//@{ String + String //@}
inline const String operator+( const String& a, const String& b )
	{ return String(a) += b; }
//@{ String + TCHAR* //@}
inline const String operator+( const String& a, const TCHAR* b )
	{ return String(a) += b; }
//@{ TCHAR* + String //@}
inline const String operator+( const TCHAR* a, const String& b )
	{ return String(a) += b; }
inline const String operator+( const String& a, TCHAR b )
	{ return String(a) += b; }

// Freeing the return value buffer of ConvToWChar
inline void String::FreeWCMem( const wchar_t* wc ) const
#ifdef _UNICODE
	{}
#else // _MBCS or _SBCS
	{ free(const_cast<wchar_t*>(wc)); }
#endif

inline void String::FreeCMem( const char* str ) const
#ifdef _UNICODE
	{ free( const_cast<char*>(str) ); }
#else // _MBCS or _SBCS
	{}
#endif

inline bool String::isCompatibleWithACP() const
{ return isCompatibleWithACP( c_str(), len() ); }



#endif // __ccdoc__
#undef XTCHAR
//=========================================================================
//@{
//	String processing + α
//
//	This is a version of String that allows you to call the buffer allocation function in the String class.
//@}
//=========================================================================

struct RawString : public String
{
	TCHAR* AllocMem( size_t m ) { return String::AllocMem(m); }
	void UnlockMem()            { String::UnlockMem(); }
};

}      // namespace ki

//=========================================================================
//@{
//	Short stack string
//
//	Only used for short string resources loading without going to the heap.
//	The 256 chars should be enough for our application.
//@}
//=========================================================================

class A_WUNUSED RzsString
{
public:
//	~RzsString() { ki::mem00(str_, sizeof(str_)); }
	explicit RzsString( UINT rsrcID );
	const TCHAR *c_str() const { return &str_[0]; }

private:
	TCHAR str_[256];
};

//=========================================================================
//@{
//	Short stack string helper for digits
//@}
//=========================================================================

class A_WUNUSED SInt2Str
{
public:
//	~SInt2Str() { ki::mem00(buf_, sizeof(buf_)); str_ = NULL; }
	explicit SInt2Str( int num )   : str_ ( Int2lStr(buf_, num)   ) {}
#ifdef WIN64
	explicit SInt2Str( unsigned long long num ) : str_ ( Ulong2lStr(buf_, num) ) {}
#endif
	explicit SInt2Str( DWORD num ) : str_ ( Ulong2lStr(buf_, num) ) {}
	explicit SInt2Str( UINT num )  : str_ ( Ulong2lStr(buf_, num) ) {}
	explicit SInt2Str( WORD num )  : str_ ( Ulong2lStr(buf_, num) ) {}
	explicit SInt2Str( BYTE num )  : str_ ( Ulong2lStr(buf_, num) ) {}
	const TCHAR *c_str() const { return str_; }

private:
	TCHAR buf_[ULONG_DIGITS+4];
	const TCHAR *str_; // points to the begining of the number
};

//=========================================================================

#endif // _KILIB_STRING_H_
