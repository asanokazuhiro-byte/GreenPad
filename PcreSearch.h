#ifndef GREENPAD_PCRESEARCH_H_
#define GREENPAD_PCRESEARCH_H_
#include "Search.h"



//=========================================================================
// Regular expression search using PCRE2 (pcre2-16)
//=========================================================================

class PcreSearch A_FINAL: public Searchable
{
public:
	explicit PcreSearch( const unicode* pattern, bool caseSensitive, bool down );
	~PcreSearch() override;

	bool valid() const { return valid_; }
	static bool IsAvailable();
	static bool GetVersionStr( wchar_t* buf, int bufSize );

	ulong captureCount() const override;
	bool  getCapture( ulong n, ulong* mbg, ulong* med ) const override;

private:
	bool Search( const unicode* str, ulong len, ulong stt,
		ulong* mbg, ulong* med ) override;

private:
	void*  re_;      // pcre2_code_16*
	void*  mdata_;   // pcre2_match_data_16*
	bool   caseS_;
	bool   down_;
	bool   valid_;
	ulong  last_rc_; // return value of last successful forward match

	NOCOPY(PcreSearch);
};



#endif // GREENPAD_PCRESEARCH_H_
