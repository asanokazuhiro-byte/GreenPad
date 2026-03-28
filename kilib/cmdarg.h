#ifndef _KILIB_CMDARG_H_
#define _KILIB_CMDARG_H_
#include "types.h"
#include "memory.h"
#include "ktlarray.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	Splitting command line strings
//
//	Just delimit them with spaces and double quotes in mind.
//@}
//=========================================================================

class A_WUNUSED Argv
{
public:

	//@{ Split the specified string //@}
	explicit Argv( const TCHAR* cmd = GetCommandLine() );
	~Argv() { free( buf_ ); }

	//@{ Argument Get //@}
	const TCHAR* operator[]( size_t i ) const { return arg_[i]; }

	//@{ Number of arguments //@}
	size_t size() const { return argNum_; }

private:

	enum { MAX_ARGS = 16 };
	TCHAR                *  buf_;
	size_t        argNum_;
	const TCHAR * arg_[MAX_ARGS];

private:

	NOCOPY(Argv);
};

//=========================================================================

}      // namespace ki
#endif // _KILIB_CMDARG_H_
