#ifndef _KILIB_FIND_H_
#define _KILIB_FIND_H_
#include "types.h"
#include "memory.h"
#include "path.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.WinUtil //@}
//@{
//	file search
//
//	Using FindFirstFile etc. of Win32 API,
//	List files.
//@}
//=========================================================================

class FindFile
{
public:

	//@{ constructor //@}
	FindFile() : han_( INVALID_HANDLE_VALUE ), first_(false) {}

	//@{ destructor //@}
	~FindFile() { Close(); }

	//@{Search end process //@}
	void Close();

public: //-- Outward interface --------------------------

	bool Begin( const TCHAR* wild );
	bool Next( WIN32_FIND_DATA* pfd );

public:

	//@{static version. Get the first matching file //@}
	static bool FindFirst( const TCHAR* wild, WIN32_FIND_DATA* pfd );

private:

	HANDLE         han_;
	bool         first_;
	WIN32_FIND_DATA fd_;

private:

	NOCOPY(FindFile);
};


//=========================================================================

}      // namespace ki
#endif // _KILIB_FIND_H_
