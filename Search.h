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
//	検索オブジェクト
//@}
//=========================================================================

class Searchable
{
public:
	//@{
	//	検索を行う
	//	@param str 対象文字列
	//	@param len 対象文字列の長さ
	//	@param stt 検索開始index。0なら先頭から
	//	@param mbg マッチ結果の先頭index
	//	@param med マッチ結果の終端indexの１個後ろ
	//	@return マッチしたかどうか
	//
	//	下方向サーチオブジェクトの場合、stt <= *beg の範囲
	//	上方向サーチオブジェクトの場合、*beg <= stt の範囲を検索
	//@}
	virtual bool Search( const unicode* str, ulong len, ulong stt,
		ulong* mbg, ulong* med ) = 0;

	virtual ~Searchable() {};
};



//=========================================================================
//@{
//	検索管理人
//
//	前回検索したときのオプションや検索文字列を覚えておくのが
//	このクラスの担当。検索・置換ダイアログの表示等もここで
//	やるかもしれない。
//@}
//=========================================================================

class SearchManager A_FINAL: public ki::DlgImpl
{
	typedef editwing::DPos DPos;

public:
	//@{ コンストラクタ。特記事項無し //@}
	SearchManager( ki::Window& w, editwing::EwEdit& e );

	//@{ デストラクタ。特記事項無し //@}
	~SearchManager();

	//@{ 検索ダイアログ表示 //@}
	void ShowDlg();

	//@{ [次を検索]コマンド //@}
	void FindNext();

	//@{ [前を検索]コマンド //@}
	void FindPrev();

	//@{ 今すぐ検索可能か？ //@}
	bool isReady() const
		{ return searcher_ != NULL; }

	//@{ 設定Save //@}
	void SaveToINI();

	//@{ 設定Load //@}
	void LoadFromINI();

	//@{ 苦肉の策^^; //@}
	bool TrapMsg(MSG* msg);

	//@{ 見つかりませんでしたダイアログ //@}
	void NotFound(bool GoingDown=false);

private:

	//@{ [置換]コマンド //@}
	void ReplaceImpl();

	//@{ [全置換]コマンド //@}
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
	ki::Window& mainWnd_;

	bool bIgnoreCase_; // 大文字小文字を同一視？
	bool bRegExp_;     // 正規表現？
	bool bDownSearch_; // 検索方向
	bool bChanged_;    // 前回のsearcher構築時から変更があったらtrue
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
