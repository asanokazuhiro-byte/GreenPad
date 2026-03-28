#ifndef _GREENPAD_OPENSAVEDLG_H_
#define _GREENPAD_OPENSAVEDLG_H_
#include "kilib/ktlarray.h"
#include "kilib/kstring.h"
#include "rsrc/resource.h"



//========================================================================
//@{ @pkg Gp.Dlg //@}
//@{
//	Available character code list
//@}
//========================================================================

class CharSetList
{
public:

	struct CsInfo
	{
		const TCHAR*  longName;
		const TCHAR* shortName;
		long              type : 2;
		long                ID : sizeof(long)*8 -2;
	};

public:

	CharSetList();
	const CsInfo& operator[](size_t i) const { return list_[i]; }
	size_t size() const { return list_.size(); }
	int defaultCs() const;
	size_t defaultCsi() const;
	size_t findCsi( int cs ) const;
	void EnrollCs( int _id, uint _num);
	size_t GetCSIfromNumStr( const TCHAR *buf ) const;
	static int GetCSIFromComboBox( HWND dlg, const CharSetList& csl, uint OpenSaveMask );

private:

	enum { SAVE=1, LOAD=2, BOTH=3 };
	ki::storage<CsInfo> list_;
};



//========================================================================
//@{
//	"Open file" dialog
//
//	At the bottom of the common Windows dialog box, select a character code field.
//	Show what you added.
//@}
//========================================================================

class OpenFileDlg
{
public:
	explicit OpenFileDlg( const CharSetList& csl )
		: csl_(csl), csIndex_(0)
				{ ki::mem00( filename_, sizeof(filename_) ); }

	bool DoModal( HWND wnd, const TCHAR* filter, const TCHAR* fnm );

public:
	inline const TCHAR* filename() const { return filename_; }
	inline int csi() const { return csIndex_; }

public:
	static ki::aarr<TCHAR> ConnectWithNull( const TCHAR *lst[], size_t num );

private:
	const CharSetList& csl_;
	TCHAR filename_[MAX_PATH];
	int   csIndex_;

};


//=======
//@{
//	Save File Dialog
//
//	At the bottom of the common Windows dialog, there is a character code selection field and
//	Displays a line feed code selection field.
//@}
//========================================================================

class SaveFileDlg
{
public:
	explicit SaveFileDlg( const CharSetList& csl, int cs, int lb )
		: csl_(csl)
		, csIndex_(cs)
		, lb_(lb)
				{ ki::mem00( filename_, sizeof(filename_) ); }

	bool DoModal( HWND wnd, const TCHAR* filter, const TCHAR* fnm );

public:
	inline const TCHAR* filename() const { return filename_; }
	inline int csi() const { return csIndex_; }
	inline int lb() const { return lb_; }

public:
	static ki::aarr<TCHAR> ConnectWithNull( const TCHAR *lst[], size_t num )
		{ return OpenFileDlg::ConnectWithNull( lst, num ); }

private:
	const CharSetList& csl_;
	TCHAR filename_[MAX_PATH];
	int   csIndex_;
	int   lb_;

private:
};


//========================================================================
//@{
//	"Reopen" dialog
//
//	Character code selection field display
//@}
//========================================================================

class ReopenDlg A_FINAL: public ki::DlgImpl
{
public:
	ReopenDlg( const CharSetList& csl, int csi );
	inline int csi() const { return csIndex_; }

private:
	void on_init() override;
	bool on_ok() override;

private:
	const CharSetList& csl_;
	int   csIndex_;
};

//========================================================================

#endif // _GREENPAD_OPENSAVEDLG_H_
