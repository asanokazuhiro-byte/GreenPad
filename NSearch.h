#ifndef AFX_NSEARCH_H__8336E133_90C5_4059_8605_6066BD37D042__INCLUDED_
#define AFX_NSEARCH_H__8336E133_90C5_4059_8605_6066BD37D042__INCLUDED_
#include "Search.h"

//=========================================================================
//@{ @pkg Gp.Search //@}
// BM method search policy
//=========================================================================

//@{ Case-sensitive policy //@}
struct CaseSensitive
{
	static unicode map( unicode c )
		{ return c; }
	static bool not_equal( unicode c1, unicode c2 )
		{ return c1!=c2; }
};

//@{ Case-insensitive policy //@}
struct IgnoreCase
{
	static unicode map( unicode c )
	{ // Map to uppercase everything.
		if( c < 128 ) // ASCII
		{
			if( L'a'<=c && c<=L'z' )
				return c - L'a' + L'A';
			return c;
		}
		// Non ASCII characters...
		return my_CharUpperSingleW(c);
	}
	static bool not_equal( unicode c1, unicode c2 )
		{ return map(c1)!=map(c2); }
};



//=========================================================================
//@{
//	Ordinary forward search using BM method
//@}
//=========================================================================
template<class ComparisonPolicy> class BMSearch
{
public:
	BMSearch( const unicode* key )
		: keylen_( my_lstrlenW(key) )
	{
		key_ =  (unicode *)malloc( sizeof(unicode) * (keylen_+1) );
		if( !key_ ) return;
		my_lstrcpyW( key_, key );

		ki::memFF( lastAppearance_, sizeof(lastAppearance_) );
		for( int i=0, e=keylen_; i<e; ++i )
			lastAppearance_[ ComparisonPolicy::map(key[i]) ] = i;
	}

	~BMSearch()
	{
		free( key_ );
	}

	int Search( const unicode* str, int strlen )
	{
		if( !key_ ) return -1;
		for( int i=0, e=strlen-keylen_, j, t; i<=e; i+=(j>t?j-t:1) )
		{
			for( j=keylen_-1; j>=0; --j )
			{
				if( ComparisonPolicy::not_equal( key_[j], str[i+j] ) )
					break;
			}
			if( j < 0 )
				return i;
			t = lastAppearance_[ ComparisonPolicy::map(str[i+j]) ];
		}
		return -1;
	}

	int keylen() const { return keylen_; }

private:
	int      keylen_;
	unicode* key_;
	int      lastAppearance_[65536];
private:
	NOCOPY(BMSearch);
};



//=========================================================================
//@{
//	Backward search using BM method
//@}
//=========================================================================

template<class ComparisonPolicy> class BMSearchRev
{
public:
	BMSearchRev( const unicode* key )
		: keylen_( my_lstrlenW(key) )
	{
		key_ =  (unicode *)malloc( sizeof(unicode) * (keylen_+1) );
		if( !key_ ) return;

		my_lstrcpyW( key_, key );
		ki::memFF( firstAppearance_, sizeof(firstAppearance_) );
		for( int i=keylen_-1; i>=0; --i )
			firstAppearance_[ ComparisonPolicy::map(key[i]) ] = i;
	}

	~BMSearchRev()
	{
		free( key_ );
	}

	int Search( const unicode* str, int strlen )
	{
		if( !key_ ) return -1;
		for( int i=strlen-keylen_-1, j, e, t; i>=0; i-=(t>j?t-j:1) )
		{
			for( j=0, e=keylen_; j<e; ++j )
				if( ComparisonPolicy::not_equal( key_[j], str[i+j] ) )
					break;
			if( j >= e )
				return i;
			t = firstAppearance_[ ComparisonPolicy::map(str[i+j]) ];
			if( t == -1 ) t = keylen_;
		}
		return -1;
	}

	int keylen() const { return keylen_; }

private:
	int      keylen_;
	unicode* key_;
	int      firstAppearance_[65536];
private:
	NOCOPY(BMSearchRev);
};



//=========================================================================
//@{
// Implementation as Seahcable (forward search)
//@}
//=========================================================================

template<class CompalisonPolicy>
class NSearch : public Searchable
{
public:
	NSearch( const unicode* key ) : s_(key) {}

private:
	bool Search(
		const unicode* str, ulong len, ulong stt, ulong* mbg, ulong* med ) override
	{
		int n = s_.Search( str+stt, len-stt );
		if( n < 0 )
			return false;
		*mbg = stt + n;
		*med = stt + n + s_.keylen();
		return true;
	}

private:
	BMSearch<CompalisonPolicy> s_;
};



//=========================================================================
//@{
// Implementation as Seahcable (backward search)
//@}
//=========================================================================

template<class CompalisonPolicy>
class NSearchRev : public Searchable
{
public:
	NSearchRev( const unicode* key ) : s_(key) {}

private:
	bool Search(
		const unicode* str, ulong len, ulong stt, ulong* mbg, ulong* med ) override
	{
		int n = s_.Search( str, stt+s_.keylen() );
		if( n < 0 )
			return false;
		*mbg = n;
		*med = n + s_.keylen();
		return true;
	}

private:
	BMSearchRev<CompalisonPolicy> s_;
};



//=========================================================================

#endif
