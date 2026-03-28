#ifndef _KILIB_LOG_H_
#define _KILIB_LOG_H_
#include "types.h"
#ifndef __ccdoc__
namespace ki {
#endif


//=========================================================================
//@{ @pkg ki.Core //@}
//@{
//	Logging utility (for debugging)
//@}
//=========================================================================

class Logger
{
public:

	Logger() {}
	void WriteLine( const TCHAR* str );
	void WriteLine( const TCHAR* str, int siz );
	void __cdecl WriteLineFmtErr(const TCHAR *fmt, ...);
private:

	NOCOPY(Logger);
};

#ifdef DO_LOGGING
	#define LOGGER(str) ki::Logger().WriteLine(TEXT(str))
	#define LOGGERS(str) ki::Logger().WriteLine(str)
	#define LOGGERF ki::Logger().WriteLineFmtErr
#else
	#define LOGGER(x)
	#define LOGGERS(x)
	#define LOGGERF ki::Logger().WriteLineFmtErr
//	static void __cdecl LOGGERF(const TCHAR *fmt, ...){}
#endif

//=========================================================================

}      // namespace ki
#endif // _KILIB_LOG_H_
