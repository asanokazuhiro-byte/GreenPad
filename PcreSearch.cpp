#include "kilib/stdafx.h"
#include "PcreSearch.h"

#define PCRE2_CODE_UNIT_WIDTH 16
#include "pcre2.h"



//=========================================================================
// Function pointer types for dynamic DLL loading
//=========================================================================

typedef pcre2_code_16*       (*Fn_compile)(PCRE2_SPTR16, PCRE2_SIZE, uint32_t, int*, PCRE2_SIZE*, pcre2_general_context_16*);
typedef pcre2_match_data_16* (*Fn_mdata_create)(const pcre2_code_16*, pcre2_general_context_16*);
typedef int                  (*Fn_match)(const pcre2_code_16*, PCRE2_SPTR16, PCRE2_SIZE, PCRE2_SIZE, uint32_t, pcre2_match_data_16*, pcre2_match_context_16*);
typedef PCRE2_SIZE*          (*Fn_ovector)(pcre2_match_data_16*);
typedef void                 (*Fn_mdata_free)(pcre2_match_data_16*);
typedef void                 (*Fn_code_free)(pcre2_code_16*);
typedef int                  (*Fn_config)(uint32_t, void*);

static HMODULE         s_hDll         = NULL;
static Fn_compile      s_compile      = NULL;
static Fn_mdata_create s_mdata_create = NULL;
static Fn_match        s_match        = NULL;
static Fn_ovector      s_ovector      = NULL;
static Fn_mdata_free   s_mdata_free   = NULL;
static Fn_code_free    s_code_free    = NULL;
static Fn_config       s_config       = NULL;
static bool            s_tried        = false;

static bool LoadPcre2Dll()
{
	if( s_tried ) return s_hDll != NULL;
	s_tried = true;

	s_hDll = ::LoadLibraryW( L"pcre2-16.dll" );
	if( !s_hDll ) return false;

	s_compile      = (Fn_compile)      ::GetProcAddress( s_hDll, "pcre2_compile_16" );
	s_mdata_create = (Fn_mdata_create) ::GetProcAddress( s_hDll, "pcre2_match_data_create_from_pattern_16" );
	s_match        = (Fn_match)        ::GetProcAddress( s_hDll, "pcre2_match_16" );
	s_ovector      = (Fn_ovector)      ::GetProcAddress( s_hDll, "pcre2_get_ovector_pointer_16" );
	s_mdata_free   = (Fn_mdata_free)   ::GetProcAddress( s_hDll, "pcre2_match_data_free_16" );
	s_code_free    = (Fn_code_free)    ::GetProcAddress( s_hDll, "pcre2_code_free_16" );
	s_config       = (Fn_config)       ::GetProcAddress( s_hDll, "pcre2_config_16" );

	if( !s_compile || !s_mdata_create || !s_match ||
	    !s_ovector || !s_mdata_free   || !s_code_free )
	{
		::FreeLibrary( s_hDll );
		s_hDll = NULL;
		return false;
	}
	return true;
}



//=========================================================================

bool PcreSearch::IsAvailable()
{
	return LoadPcre2Dll();
}

bool PcreSearch::GetVersionStr( wchar_t* buf, int bufSize )
{
	if( !LoadPcre2Dll() || !s_config ) return false;
	wchar_t tmp[64];
	if( s_config( PCRE2_CONFIG_VERSION, tmp ) < 0 ) return false;
	// バージョン文字列 (例: "10.45 2024-06-18") の日付部分を除去
	for( wchar_t* p = tmp; *p; ++p )
		if( *p == ' ' ) { *p = '\0'; break; }
	::lstrcpynW( buf, tmp, bufSize );
	return true;
}

PcreSearch::PcreSearch( const unicode* pattern, bool caseSensitive, bool down )
	: re_     ( NULL )
	, mdata_  ( NULL )
	, caseS_  ( caseSensitive )
	, down_   ( down )
	, valid_  ( false )
	, last_rc_( 0 )
{
	if( !LoadPcre2Dll() ) return;

	int errcode;
	PCRE2_SIZE erroffset;
	uint32_t options = PCRE2_UTF | PCRE2_MULTILINE;
	if( !caseS_ )
		options |= PCRE2_CASELESS;

	re_ = s_compile(
		(PCRE2_SPTR16)pattern,
		PCRE2_ZERO_TERMINATED,
		options,
		&errcode,
		&erroffset,
		NULL );

	if( re_ )
	{
		mdata_ = s_mdata_create( (pcre2_code_16*)re_, NULL );
		valid_ = ( mdata_ != NULL );
	}
}

PcreSearch::~PcreSearch()
{
	if( mdata_ ) s_mdata_free( (pcre2_match_data_16*)mdata_ );
	if( re_ )    s_code_free( (pcre2_code_16*)re_ );
}



//=========================================================================

bool PcreSearch::Search(
	const unicode* str, ulong len, ulong stt, ulong* mbg, ulong* med )
{
	if( !valid_ )
		return false;

	pcre2_code_16*       re    = (pcre2_code_16*)re_;
	pcre2_match_data_16* mdata = (pcre2_match_data_16*)mdata_;

	if( down_ )
	{
		// Forward: find first match at or after stt
		int rc = s_match(
			re,
			(PCRE2_SPTR16)str, (PCRE2_SIZE)len,
			(PCRE2_SIZE)stt,
			0,
			mdata,
			NULL );

		if( rc < 0 )
			return false;

		PCRE2_SIZE* ov = s_ovector( mdata );
		if( ov[0] == ov[1] )
			return false;

		last_rc_ = (ulong)rc;
		*mbg = (ulong)ov[0];
		*med = (ulong)ov[1];
		return true;
	}

	// Reverse: find last non-empty match starting at or before stt
	ulong      last_mbg = 0;
	ulong      last_med = 0;
	bool       found    = false;
	PCRE2_SIZE pos      = 0;

	while( pos <= (PCRE2_SIZE)stt )
	{
		int rc = s_match(
			re,
			(PCRE2_SPTR16)str, (PCRE2_SIZE)len,
			pos,
			0,
			mdata,
			NULL );

		if( rc < 0 )
			break;

		PCRE2_SIZE* ov = s_ovector( mdata );
		if( ov[0] > (PCRE2_SIZE)stt )
			break;

		if( ov[0] != ov[1] )
		{
			last_mbg = (ulong)ov[0];
			last_med = (ulong)ov[1];
			found    = true;
		}

		// Keep overlaps for reverse search, like forward search does.
		// Move from the previous match start by one code unit.
		// For zero-length matches, force a single-step advance.
		if( ov[0] == ov[1] )
			pos = pos + 1;
		else
			pos = ov[0] + 1;
	}

	if( found )
	{
		*mbg = last_mbg;
		*med = last_med;
	}
	return found;
}



//=========================================================================

ulong PcreSearch::captureCount() const
{
	return last_rc_ > 1 ? last_rc_ - 1 : 0;
}

bool PcreSearch::getCapture( ulong n, ulong* mbg, ulong* med ) const
{
	if( !valid_ || !mdata_ || n >= last_rc_ )
		return false;

	PCRE2_SIZE* ov = s_ovector( (pcre2_match_data_16*)mdata_ );
	if( ov[2*n] == PCRE2_UNSET || ov[2*n+1] == PCRE2_UNSET )
		return false;

	*mbg = (ulong)ov[2*n];
	*med = (ulong)ov[2*n+1];
	return true;
}
