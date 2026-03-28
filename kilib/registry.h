#ifndef _KILIB_REGISTRY_H_
#define _KILIB_REGISTRY_H_
#include "types.h"
#include "memory.h"
#include "path.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.WinUtil //@}
//@{
//	INI file reading
//
//@}
//=========================================================================

class A_WUNUSED IniFile
{
public:

	//@{ constructor //@}
	IniFile( /*const TCHAR* ini=NULL, bool exeppath=true */ )
		{ SetFileName( /*ini, exepath*/ ); }

	//@{ Set ini file name //@}
	void SetFileName( /*const TCHAR* ini=NULL, bool exepath=true*/ )
		{ GetIniPath( iniName_ ); }

	static DWORD GetIniPath(TCHAR *ini);

	//@{ Set section name //@}
	void SetSection( const TCHAR* section )
		{ my_lstrcpys( section_, countof(section_) , section ); }

	//@{ Cache section //@}
	void CacheSection();

	//@{ Set section name to username //@}
	void SetSectionAsUserName();
	bool SetSectionAsUserNameIfNotShared( const TCHAR *section);

	//@{ Is there a section with a certain name? //@}
	bool HasSectionEnabled( const TCHAR* section ) const;

	//@{ Read integer value //@}
	int    GetInt ( const TCHAR* key, int   defval ) const;
	//@{ Read boolean value //@}
	bool   GetBool( const TCHAR* key, bool  defval ) const;
	//@{ Get rect from ini file//@}
	void GetRect ( const TCHAR* key, RECT *rc, const RECT *defrc) const;
	//@{ read string //@}
	String GetStr ( const TCHAR* key, const TCHAR *defval ) const;
	//@{ Load path string //@}
	Path  GetPath ( const TCHAR* key, const TCHAR *defval ) const;
	//@{ Get String in section //@}
	String GetStrinSect( const TCHAR* key, const TCHAR* sect, const TCHAR *defval ) const;
	TCHAR *GetSStrHere(const TCHAR* key, const TCHAR* sect, const TCHAR *defval, TCHAR buf[MAX_PATH]) const;

	//@{Write integer value //@}
	bool PutInt ( const TCHAR* key, int val );
	//@{ Write boolean value //@}
	bool PutBool( const TCHAR* key, bool val );
	//@{ Save rect to ini file //@}
	bool PutRect( const TCHAR* key, const RECT *rc);
	//@{ String writing //@}
	bool PutStr ( const TCHAR* key, const TCHAR* val );
	//@{Write path //@}
	bool PutPath( const TCHAR* key, const Path& val );
	//@{ Put string in section //@}
	bool PutStrinSect( const TCHAR* key, const TCHAR *sect, const TCHAR* val );

	const TCHAR *getName() const { return iniName_; };

private:

	TCHAR  iniName_[MAX_PATH];
	TCHAR  section_[64];
};

//=========================================================================

}      // namespace ki
#endif // _KILIB_REGISTRY_H_
