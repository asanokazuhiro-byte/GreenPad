#ifndef AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
#define AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
#include "editwing/editwing.h"
#include "kilib/window.h"
#include "kilib/memory.h"
#include "kilib/kstring.h"
#include "kilib/registry.h"



//=========================================================================
//@{ @pkg Gp.Search //@}
//@{
//	search object
//@}
//=========================================================================

class Searchable
{
public:
	//@{
	//	Do a search
	//	@param str Target string
	//	@param len Length of target string
	//	@param stt Search start index. If 0, start from the beginning
	//	@param mbg First index of match result
	//	@param med One position after the ending index of the match result
	//	@return Matched?
	//
	//	For downward search objects, the range stt <= *beg
	//	For upward search objects, search range *beg <= stt
	//@}
	virtual bool Search( const unicode* str, ulong len, ulong stt,
		ulong* mbg, ulong* med ) = 0;

	virtual ulong captureCount() const { return 0; }
	virtual bool  getCapture( ulong n, ulong* mbg, ulong* med ) const
		{ (void)n; (void)mbg; (void)med; return false; }

	virtual ~Searchable() {};
};



//=========================================================================
//@{
//	search manager
//
//	Remembering the options and search string from the last search
//	In charge of this class. You can also display the search/replace dialog here.
//	I might do it.
//@}
//=========================================================================

class SearchManager A_FINAL: public ki::DlgImpl
{
	typedef editwing::DPos DPos;

public:
	//@{ constructor; No special notes //@}
	SearchManager( ki::Window& w, editwing::EwEdit& e );

	//@{ destructor; No special notes //@}
	~SearchManager();

	//@{ Display search dialog //@}
	void ShowDlg();

	//@{ [Find Next] command //@}
	void FindNext();

	//@{ [Find Previous] command //@}
	void FindPrev();

	//@{ Can I search now? //@}
	bool isReady() const
		{ return searcher_ != NULL; }

	//@{Save settings //@}
	void SaveToINI();

	//@{SettingsLoad //@}
	void LoadFromINI();

	//@{A last resort^^; //@}
	bool TrapMsg(MSG* msg);

	//@{ Not Found Dialog //@}
	void NotFound(bool GoingDown=false);

private:

	//@{ [Replace] command //@}
	void ReplaceImpl();

	//@{ [Replace all] command //@}
	void ReplaceAllImpl();

private:

	void on_init() override;
	void on_destroy() override;
	bool on_command( UINT cmd, UINT id, HWND ctrl ) override;
	bool on_cancel() override;

	void on_findnext();
	void on_findprev();
	void on_replacenext();
	void on_replaceall();
	void UpdateData();
	void ConstructSearcher( bool down=true );
	void FindNextImpl( bool redo=false );
	void FindPrevImpl();
	bool FindNextFromImpl( DPos s, DPos* beg, DPos* end );
	bool FindPrevFromImpl( DPos s, DPos* beg, DPos* end );

	void AddToComboBoxHistoric(UINT idc_cbb, const TCHAR *str)
	{
		if( !str || !*str )
			return;
		
		// Save edit selection state:
		HRESULT oldsel = SendMsgToItem( idc_cbb, CB_GETEDITSEL, 0, 0 );
		
		int found_index = SendMsgToItem( idc_cbb, CB_FINDSTRINGEXACT, 0, (LPARAM)str );
		if( found_index == CB_ERR )
		{
			// Item was not found in historic.
			int item_count = SendMsgToItem( idc_cbb, CB_GETCOUNT, 0, 0);
			if( item_count > 15 )
				SendMsgToItem( idc_cbb, CB_DELETESTRING, item_count-1, 0);
		}
		else
		{
			// Item was found in historic.
			// Remove old entry
			SendMsgToItem( idc_cbb, CB_DELETESTRING, found_index, 0);
		}
		// Alway Insert last searched string at the begining of the list
		SendMsgToItem( idc_cbb, CB_INSERTSTRING, /*index*/0, (LPARAM)str );

		// Restore combobox edit state
		SendMsgToItem( idc_cbb, CB_SETCURSEL, 0, 0 );
		SendMsgToItem( idc_cbb, CB_SETEDITSEL, 0, (LPARAM)oldsel );
	}

	void fillFromHistoric( UINT idc_cbb, ki::String *hist, size_t hist_len )
	{
		for( size_t i=0; i < hist_len; i++ )
		{
			if( hist[i].len() > 0 )
				SendMsgToItem( idc_cbb, CB_ADDSTRING, 0, (LPARAM)hist[i].c_str() );
		}
	}

	void saveToHistoric( UINT idc_cbb, ki::String *hist, size_t hist_len )
	{
		int item_count = SendMsgToItem( idc_cbb, CB_GETCOUNT, 0, 0);
		if(item_count <= 0)
			return;

		for( size_t i=0; i < Min((size_t)item_count, hist_len) ; i++ )
		{
			LPARAM txtlen = SendMsgToItem(idc_cbb, CB_GETLBTEXTLEN, i, 0);
			TCHAR *txt = (TCHAR *)ki::TS.alloc( txtlen+1 );
			if( txt )
			{
				SendMsgToItem(idc_cbb, CB_GETLBTEXT, i, (LPARAM)txt);
				hist[i] = txt; // save
				ki::TS.freelast( txt, txtlen+1 );
			}
		}
	}

private:
	editwing::EwEdit& edit_;
	Searchable *searcher_;

	bool bIgnoreCase_; // Are uppercase and lowercase letters the same?
	bool bRegExp_;     // Regular expression?
	bool bDownSearch_; // Search direction
	bool bChanged_;    // True if there has been a change since the last searcher construction
	bool inichanged_;  // Set to true when the ini must be saved

	ki::String findStr_;
	ki::String replStr_;

	ki::String findHistoric_[16];
	ki::String replHistoric_[16];

private:
	NOCOPY(SearchManager);
};



//=========================================================================

#endif // AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
