#ifndef AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
#define AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
#include "editwing/editwing.h"
#include "OpenSaveDlg.h"
#include "kilib/registry.h"

void SetFontSize(LOGFONT *font, HDC hDC, int fsiz, int fx);

// application message
#define GPM_MRUCHANGED WM_APP+0

//=========================================================================
//@{ @pkg Gp.Main //@}
//@{
//	Centralized management of settings
//
//	Calling SetDocType switches the active document type and updates
//	all document-type-dependent settings accordingly.
//@}
//=========================================================================

class ConfigManager
{
public:

	ConfigManager() A_COLD;
	~ConfigManager() A_COLD;

	//@{ Load document type for file with specified name //@}
	int SetDocType( const ki::Path& fname )  A_COLD;

	//@{ Load document type with specified number //@}
	void SetDocTypeByMenu( int pos, HMENU m )  A_COLD;

	//@{ Load document type with specified name //@}
	bool SetDocTypeByName( const ki::String& nam )  A_COLD;

	//@{Create menu item //@}
	void SetDocTypeMenu( HMENU m, UINT idstart )  A_COLD;

	//@{Modify menu item check //@}
	void CheckMenu( HMENU m, int pos )  A_COLD;

	//@{ Display settings dialog //@}
	bool DoDialog( const ki::Window& parent )  A_COLD;

	static size_t GetLayData(const TCHAR *name, unicode *buf, size_t buf_len);

public:

	//@{ Undo count limit //@}
	inline int undoLimit() const { return undoLimit_; }

	//@{ How to count the number of characters //@}
	inline bool countByUnicode() const { return countbyunicode_; }

	//@{ Setting the filter to be displayed in the open/save dialog //@}
	inline const ki::String& txtFileFilter() const { return txtFilter_; }

	//@{ Number of characters to wrap when specifying number of characters //@}
	inline short wrapWidth() const { return curDt_->wrapWidth; }

	//@{ Get smart warp flag //@}
	inline bool wrapSmart() const { return curDt_->wrapSmart; }

	//@{ Wrapping method //@}
	inline short wrapType() const { return curDt_->wrapType>0 ? wrapWidth() : curDt_->wrapType; }

	//@{ Display line number? //@}
	inline bool showLN() const { return curDt_->showLN; }

	//@{Display color, font, etc. //@}
	inline const editwing::VConfig& vConfig() const { return curDt_->vc; }

	//@{ keyword file name (full path) //@}
	inline ki::Path kwdFile() const
		{
		ki::Path p(ki::Path::Exe);
		p += TEXT("type\\");
		p += curDt_->kwdfile;
		return p;
		}

	//@{ External executable file name for Grep //@}
	inline const ki::Path& grepExe() const { return grepExe_; }

	//@{Help external executable file name //@}
	inline const ki::Path& helpExe() const { return helpExe_; }

	//@{ Open in same window mode //@}
	inline bool openSame() const { return openSame_; }

	//@{Status bar display //@}
	inline bool showStatusBar() const { return showStatusBar_; }
	inline void ShowStatusBarSwitch() { showStatusBar_ = !showStatusBar_; inichanged_=1; SaveIni(); }

	//@{ date //@}
	inline const ki::String& dateFormat() const { return dateFormat_; }

public:
	//@{Character code index of new file //@}
	inline int GetNewfileCsi() const { return charSets_.findCsi( newfileCharset_ ); }

	//@{ Line break code for new file //@}
	inline ki::lbcode GetNewfileLB() const { return newfileLB_; }

public:
	//@{ Add to [Recent Files] //@}
	bool AddMRU( const ki::Path& fname )  A_COLD;

	//@{ Building the Recent Files menu //@}
	int SetUpMRUMenu( HMENU m, UINT id )  A_COLD;

	//@{ Get [recently used files] //@}
	ki::Path GetMRU( int no ) const A_COLD;

	//@{ Get list of supported character sets //@}
	inline CharSetList& GetCharSetList() { return charSets_; }

public:
	//@{ Window position/size restoration processing //@}
	inline int GetWndX() const { return rememberWindowPlace_ ? wndPos_.left : CW_USEDEFAULT; }
	inline int GetWndY() const { return rememberWindowPlace_ ? wndPos_.top : CW_USEDEFAULT; }
	inline int GetWndW() const {
		if (rememberWindowSize_) {
			return wndPos_.right - wndPos_.left;
		}
		if (wndPos_.right > wndPos_.left) {
			return wndPos_.right - wndPos_.left;
		}
		return CW_USEDEFAULT;
	}
	inline int GetWndH() const {
		if (rememberWindowSize_) {
			return wndPos_.bottom - wndPos_.top;
		}
		if (wndPos_.bottom > wndPos_.top) {
			return wndPos_.bottom - wndPos_.top;
		}
		return CW_USEDEFAULT;
	}
	inline bool GetWndM() const { return rememberWindowSize_ && wndM_; }
	void RememberWnd( const ki::Window* wnd );
	inline const RECT *PMargins() const { return &rcPMargins_; }
	inline void SetPrintMargins(const RECT *rc) { CopyRect(&rcPMargins_, rc); inichanged_=1; SaveIni(); }
	inline bool useQuickExit() const { return useQuickExit_; }
	inline bool warnOnModified() const { return warnOnModified_; }
	inline short GetZoom() const { return zoom_; };
	inline void SetZoom(short zoom) { zoom_ = zoom; inichanged_ = true; };

	inline const ki::String& GetLanguage() const { return language_; }
	inline void SetLanguage(const ki::String& lang) { language_ = lang; inichanged_ = 1; }

private:

	CharSetList charSets_;

	// Overall settings
	int        undoLimit_;
	ki::String txtFilter_;
	ki::Path   grepExe_;
	ki::Path   helpExe_;
	ki::String dateFormat_;
//	ki::String timeFormat_;
//	bool datePrior_;

	short      zoom_;
	ki::String language_;
	bool       sharedConfigMode_;
	bool       inichanged_; // keep track of save to ini.

	bool       openSame_;
	bool       countbyunicode_;
	bool       showStatusBar_;
	bool       rememberWindowSize_;
	bool       rememberWindowPlace_;
	bool       useQuickExit_;
	bool       warnOnModified_;

	// Window size memory
	bool wndM_; // maximized?
	RECT wndPos_;

	// List of document types
	struct DocType
	{
		// Definition file name etc.
		ki::String        name;
		ki::String        pattern;
		ki::String        kwdfile;
		ki::String        layfile;

		// Setting items
		editwing::VConfig vc;
		short             wrapWidth;
		signed char       wrapType;
		bool              wrapSmart;
		bool              showLN;
		uchar             fontCS;
		uchar             fontQual;
		bool              loaded;
	};
	typedef ki::olist<DocType> DtList;

	DtList           dtList_;
	DtList::iterator curDt_;

	RECT rcPMargins_;

	// list of recently used files
	int mrus_;
	ki::Path mru_[20];

	// New file related
	int        newfileCharset_;
	ki::String newfileDoctype_;
	ki::lbcode newfileLB_;

private:

	void LoadIni() A_COLD;
	void SaveIni() A_COLD;
	void ReadAllDocTypes( const TCHAR *ininame ) A_COLD;
	void LoadLayout( DocType* dt ) A_COLD;
	bool MatchDocType( const unicode* fname, const unicode* pat );

private:

	friend struct ConfigDlg;
	NOCOPY(ConfigManager);
};

//=========================================================================

#endif // AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
