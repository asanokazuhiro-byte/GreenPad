
#include "kilib/stdafx.h"
#include "rsrc/resource.h"
#include "Search.h"
#include "NSearch.h"
#include "RSearch.h"
#include "PcreSearch.h"

using namespace ki;
using namespace editwing;
using view::VPos;



//-------------------------------------------------------------------------

SearchManager::SearchManager( ki::Window& w, editwing::EwEdit& e )
	: DlgImpl( IDD_FINDREPLACE )
	, edit_( e )
	, searcher_( NULL )
	, bIgnoreCase_( true ) // 1.08 default true
	, bRegExp_( false )
	, bDownSearch_( true )
	, bChanged_ (false )
	, inichanged_( false )
{
}

SearchManager::~SearchManager()
{
	if(searcher_)
		delete searcher_;
}

void SearchManager::SaveToINI()
{
	if (inichanged_)
	{
		ki::IniFile ini;
		inichanged_ = false;
		ini.SetSectionAsUserName();
		ini.PutBool( TEXT("SearchIgnoreCase"), bIgnoreCase_ );
		ini.PutBool( TEXT("SearchRegExp"), bRegExp_ );
	}
}

void SearchManager::LoadFromINI()
{
	ki::IniFile ini;
	inichanged_ = false;
	ini.SetSectionAsUserName();
	bIgnoreCase_ = ini.GetBool( TEXT("SearchIgnoreCase"), bIgnoreCase_ );
	bRegExp_     = ini.GetBool( TEXT("SearchRegExp"), bRegExp_ );
}

//-------------------------------------------------------------------------
// ダイアログ関係
//-------------------------------------------------------------------------

void SearchManager::ShowDlg()
{
//	GoModal( ::GetParent(edit_.hwnd()) );
	if( isAlive() )
	{
		SetActive();
	}
	else
	{
		GoModeless( ::GetParent(edit_.hwnd()) );
		SetCenter(hwnd(), edit_.hwnd());
		ShowUp();
	}
}

bool SearchManager::TrapMsg(MSG* msg)
{
	if( ! isAlive() || type()==MODAL )
		return false;
	return DlgImpl::PreTranslateMessage(msg);
}

void SearchManager::on_init()
{
	if( bIgnoreCase_ ) CheckItem( IDC_IGNORECASE );
	if( bRegExp_ )     CheckItem( IDC_REGEXP );

	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );
	// Set non multiline selection as find string.
	if( edit_.getCursor().isSelected() && stt->tl == end->tl )
	{
		// 選択されている状態では、基本的にそれをボックスに表示
		ulong dmy;
		aarr<unicode> str = edit_.getCursor().getSelectedStr();

		ulong len=0;
		for( ; str[len]!=L'\0' && str[len]!=L'\n'; ++len );
		str[len] = L'\0';

		if( searcher_ &&
		    searcher_->Search( str.get(), len, 0, &dmy, &dmy ) )
		{
			SetItemText( IDC_FINDBOX, findStr_.c_str() );
		}
		else
		{
		#ifdef _UNICODE
			SetItemText( IDC_FINDBOX, str.get() );
		#else
			char *ab = (char*)TS.alloc( (len+1) * 3 * sizeof(TCHAR) );
			if( ab )
			{
				::WideCharToMultiByte( CP_ACP, 0, str.get(), -1,
					ab, (len+1)*3, NULL, NULL );
				SetItemText( IDC_FINDBOX, ab );
				TS.freelast( ab, (len+1) * 3 * sizeof(TCHAR) );
			}
		#endif
		}
	}
	else
	{
		SetItemText( IDC_FINDBOX, findStr_.c_str() );
	}

	SetItemText( IDC_REPLACEBOX, replStr_.c_str() );

	fillFromHistoric(IDC_FINDBOX,    findHistoric_, countof(findHistoric_) );
	fillFromHistoric(IDC_REPLACEBOX, replHistoric_, countof(replHistoric_) );

	::SetFocus( item(IDC_FINDBOX) );
	SendMsgToItem( IDC_FINDBOX, EM_SETSEL, 0, ::GetWindowTextLength(item(IDC_FINDBOX)) );
	
	bChanged_ = true;
}

void SearchManager::on_destroy()
{
	bChanged_ = false;
}

bool SearchManager::on_command( UINT cmd, UINT id, HWND ctrl )
{
	if( cmd==CBN_SELCHANGE || cmd == CBN_EDITCHANGE )
	{
		// 文字列変更があったことを記憶
		bChanged_ = true;
	}
	else if( cmd==BN_CLICKED )
	{
		switch( id )
		{
		// チェックボックスの変更があったことを記憶
		case IDC_IGNORECASE:
		case IDC_REGEXP:
			bChanged_ = true;
			break;
		// ボタンが押された場合
		case ID_FINDNEXT:
			on_findnext();
			break;
		case ID_FINDPREV:
			on_findprev();
			break;
		case ID_REPLACENEXT:
			on_replacenext();
			break;
		case ID_REPLACEALL:
			on_replaceall();
			break;
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool SearchManager::on_cancel()
{
	UpdateData();

	saveToHistoric(IDC_FINDBOX,    findHistoric_, countof(findHistoric_) );
	saveToHistoric(IDC_REPLACEBOX, replHistoric_, countof(replHistoric_) );

	return true;
}

void SearchManager::on_findnext()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
	{
		FindNextImpl();
//		End( IDOK );
	}
}

void SearchManager::on_findprev()
{
	UpdateData();
	ConstructSearcher( false );
	if( isReady() )
		FindPrevImpl();
}

void SearchManager::on_replacenext()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
		ReplaceImpl();
}

void SearchManager::on_replaceall()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
		ReplaceAllImpl();
}

void SearchManager::UpdateData()
{
	// ダイアログから変更点を取り込み
	bool IgnoreCase = isItemChecked( IDC_IGNORECASE );
	bool RegExp     = isItemChecked( IDC_REGEXP );

	// Must we save save to ini?
	inichanged_ = bIgnoreCase_ != IgnoreCase || RegExp != bRegExp_;
	bIgnoreCase_ = IgnoreCase;
	bRegExp_ = RegExp;

	SaveToINI();

	TCHAR* str;
	LRESULT n = SendMsgToItem( IDC_FINDBOX, WM_GETTEXTLENGTH );
	str = (TCHAR*)TS.alloc( sizeof(TCHAR) * (n+1) );
	if( str )
	{
		GetItemText( IDC_FINDBOX, n+1, str );
		findStr_ = str;
		AddToComboBoxHistoric(IDC_FINDBOX, str);
		TS.freelast( str, sizeof(TCHAR) * (n+1) );
	}

	n = SendMsgToItem( IDC_REPLACEBOX, WM_GETTEXTLENGTH );
	str = (TCHAR*)TS.alloc( sizeof(TCHAR) * (n+1) );
	if( str )
	{
		GetItemText( IDC_REPLACEBOX, n+1, str );
		replStr_ = str;
		AddToComboBoxHistoric(IDC_REPLACEBOX, str);
		TS.freelast( str, sizeof(TCHAR) * (n+1) );
	}
}

void SearchManager::ConstructSearcher( bool down )
{
	bChanged_ = (bChanged_ || (bDownSearch_ != down));
	if( (bChanged_ || !isReady()) && findStr_.len()!=0 )
	{
		// 検索者作成
		bDownSearch_ = down;
		const unicode *u = findStr_.ConvToWChar();

		if( searcher_ )
			delete searcher_;
		searcher_ = NULL;
		if( bRegExp_ )
		{
			if( PcreSearch::IsAvailable() )
				searcher_ = new PcreSearch( u, !bIgnoreCase_, bDownSearch_ );
			else
			searcher_ = new RSearch( u, !bIgnoreCase_, bDownSearch_ );
		}
		else
			if( bDownSearch_ )
				if( bIgnoreCase_ )
					searcher_ = new NSearch<IgnoreCase>(u);
				else
					searcher_ = new NSearch<CaseSensitive>(u);
			else
				if( bIgnoreCase_ )
					searcher_ = new NSearchRev<IgnoreCase>(u);
				else
					searcher_ = new NSearchRev<CaseSensitive>(u);

		findStr_.FreeWCMem(u);

		// 変更終了フラグ
		bChanged_ = false;
	}
}



//-------------------------------------------------------------------------

void SearchManager::FindNext()
{
	if( !isReady() )
	{
		ShowDlg();
	}
	else
	{
		ConstructSearcher();
		if( isReady() )
			FindNextImpl();
	}
}

void SearchManager::FindPrev()
{
	if( !isReady() )
	{
		ShowDlg();
	}
	else
	{
		ConstructSearcher( false );
		if( isReady() )
			FindPrevImpl();
	}
}



//-------------------------------------------------------------------------
// 実際の処理の実装
//-------------------------------------------------------------------------

void SearchManager::FindNextImpl(bool redo)
{
	// カーソル位置取得
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	// 選択範囲ありなら、選択範囲先頭の１文字先から検索
	// そうでなければカーソル位置から検索
	DPos s = *stt;
	if( *stt != *end )
	{
		if( stt->ad == edit_.getDoc().len(stt->tl) )
			s = DPos( stt->tl+1, 0 );
		else
			s = DPos( stt->tl, stt->ad+1 );
	}
	// 検索
	DPos b, e;
	if( FindNextFromImpl( s, &b, &e ) )
	{
		// 見つかったら選択
		edit_.getCursor().MoveCur( b, false );
		edit_.getCursor().MoveCur( e, true );
		return;
	}

	// 見つからなかった場合
	NotFound(!redo);
}

void SearchManager::NotFound(bool GoingDown)
{
	//MsgBox( RzsString(IDS_NOTFOUND).c_str() );
	if (GoingDown) {
		if (IDOK == MsgBox( RzsString(IDS_NOTFOUNDDOWN).c_str(), NULL, MB_OKCANCEL )) {
			edit_.getCursor().MoveCur( DPos(0,0), false );
			FindNextImpl(true);
		}
	} else {
	    MsgBox(RzsString(IDS_NOTFOUND).c_str(), NULL, MB_OK);
	}
}

void SearchManager::FindPrevImpl()
{
	// カーソル位置取得
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	if( stt->ad!=0 || stt->tl!=0 )
	{
		// 選択範囲先頭の１文字前から検索
		DPos s;
		if( stt->ad == 0 )
			s = DPos( stt->tl-1, edit_.getDoc().len(stt->tl-1) );
		else
			s = DPos( stt->tl, stt->ad-1 );

		// 検索
		DPos b, e;
		if( FindPrevFromImpl( s, &b, &e ) )
		{
			// 見つかったら選択
			edit_.getCursor().MoveCur( b, false );
			edit_.getCursor().MoveCur( e, true );
			return;
		}
	}

	// 見つからなかった場合
	NotFound();
}

bool SearchManager::FindNextFromImpl( DPos s, DPos* beg, DPos* end )
{
	// １行ずつサー, Search one line at a time
	const doc::Document& d = edit_.getDoc();
	for( ulong mbg,med,e=d.tln(); s.tl<e; ++s.tl, s.ad=0 )
		if( searcher_ && searcher_->Search(
			d.tl(s.tl), d.len(s.tl), s.ad, &mbg, &med ) )
		{
			beg->tl = end->tl = s.tl;
			beg->ad = mbg;
			end->ad = med;
			return true; // 発見, Found!
		}
	return false;
}

bool SearchManager::FindPrevFromImpl( DPos s, DPos* beg, DPos* end )
{
	// １行ずつサーチ, Search one line at a time
	const doc::Document& d = edit_.getDoc();
	for( ulong mbg,med; ; s.ad=d.len(--s.tl) )
	{
		if( searcher_ && searcher_->Search(
			d.tl(s.tl), d.len(s.tl), s.ad, &mbg, &med ) )
		{
			beg->tl = end->tl = s.tl;
			beg->ad = mbg;
			end->ad = med;
			return true; // 発見, Found!
		}
		if( s.tl==0 )
			break;
	}
	return false;
}

// Expand \0-\9 backreferences in replacement string.
// \0 = full match, \1-\9 = capture groups.
// \\ = literal backslash.
// If out is NULL, only computes and returns the expanded length.
static ulong DoExpandRepl(
	const unicode* repl, ulong replLen,
	const unicode* line,
	Searchable* searcher,
	unicode* out )
{
	ulong outLen = 0;
	for( ulong i = 0; i < replLen; i++ )
	{
		if( repl[i] == L'\\' && i + 1 < replLen )
		{
			unicode next = repl[i+1];
			if( next >= L'0' && next <= L'9' )
			{
				ulong n = (ulong)(next - L'0');
				ulong mbg, med;
				if( searcher->getCapture(n, &mbg, &med) )
				{
					ulong len = med - mbg;
					if( out ) ::memcpy( out + outLen, line + mbg, len * sizeof(unicode) );
					outLen += len;
				}
				else
				{
					if( out ) { out[outLen] = L'\\'; out[outLen+1] = next; }
					outLen += 2;
				}
				i++;
			}
			else if( next == L'\\' )
			{
				if( out ) out[outLen] = L'\\';
				outLen++;
				i++;
			}
			else
			{
				if( out ) out[outLen] = repl[i];
				outLen++;
			}
		}
		else
		{
			if( out ) out[outLen] = repl[i];
			outLen++;
		}
	}
	return outLen;
}

void SearchManager::ReplaceImpl()
{
	// カーソル位置取得
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	// 選択範囲先頭から検索
	DPos b, e;
	if( FindNextFromImpl( *stt, &b, &e ) )
	{
		if( e == *end )
		{
			const wchar_t* replPat = replStr_.ConvToWChar();
			const ulong replPatLen = my_lstrlenW( replPat );

			const unicode* line = edit_.getDoc().tl(b.tl);
			ulong ulen;
			unicode* ustr;
			if( bRegExp_ )
			{
				ulen = DoExpandRepl( replPat, replPatLen, line, searcher_, NULL );
				ustr = (unicode*)ki::TS.alloc( (ulen+1) * sizeof(unicode) );
				DoExpandRepl( replPat, replPatLen, line, searcher_, ustr );
				ustr[ulen] = L'\0';
			}
			else
			{
				ulen = replPatLen;
				ustr = const_cast<unicode*>(replPat);
			}

			// 置換
			edit_.getDoc().Execute( doc::Replace(b, e, ustr, ulen) );

			if( bRegExp_ )
				ki::TS.freelast( ustr, (ulen+1) * sizeof(unicode) );
			replStr_.FreeWCMem( replPat );

			if( FindNextFromImpl( DPos(b.tl,b.ad+ulen), &b, &e ) )
			{
				// 次を選択
				edit_.getCursor().MoveCur( b, false );
				edit_.getCursor().MoveCur( e, true );
				return;
			}
		}
		else
		{
			// そうでなければとりあえず選択
			edit_.getCursor().MoveCur( b, false );
			edit_.getCursor().MoveCur( e, true );
			return;
		}
	}
	// 見つからなかった場合
	NotFound();
}

void SearchManager::ReplaceAllImpl()
{
	// まず、実行する置換を全てここに登録する
	doc::MacroCommand mcr;

	// 置換後文字列
	const wchar_t* replPat = replStr_.ConvToWChar();
	const ulong replPatLen = my_lstrlenW( replPat );

	// Get selection position
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );
	DPos oristt = *stt;

	// Set begining and end for replace all
	// if multi-line selection
	DPos s(0,0), dend(0, 0);
	bool noselection = true;
	if(stt->tl != end->tl)
	{ // Multi-line selection
		noselection = false;
		s = *stt; // Start of selection
		dend = *end; // End of selection
	}

	// 文書の頭から検索, Search from the beginning of the document (or selection)
	int dif=0;
	ulong lastExpandedLen = replPatLen;
	DPos b, e;
	while( FindNextFromImpl( s, &b, &e ) && (noselection || e <= dend) )
	{ // search until the end of selection if any
		if( s.tl != b.tl ) dif = 0;
		s = e;

		// 置換コマンドを登録
		b.ad += dif, e.ad += dif;
		if( bRegExp_ )
		{
			const unicode* line = edit_.getDoc().tl(b.tl);
			ulong expLen = DoExpandRepl( replPat, replPatLen, line, searcher_, NULL );
			unicode* expBuf = (unicode*)ki::TS.alloc( (expLen+1) * sizeof(unicode) );
			DoExpandRepl( replPat, replPatLen, line, searcher_, expBuf );
			expBuf[expLen] = L'\0';
			mcr.Add( new doc::Replace(b, e, expBuf, expLen) );
			ki::TS.freelast( expBuf, (expLen+1) * sizeof(unicode) );
			dif -= e.ad - b.ad - (int)expLen;
			lastExpandedLen = expLen;
		}
		else
		{
			mcr.Add( new doc::Replace(b, e, replPat, replPatLen) );
			dif -= e.ad - b.ad - (int)replPatLen;
			lastExpandedLen = replPatLen;
		}
	}

	if( mcr.size() > 0 )
	{
		// ここで連続置換
		edit_.getDoc().Execute( mcr );
		// カーソル移動
		e.ad = b.ad + lastExpandedLen;
		if (noselection)
		{
			edit_.getCursor().MoveCur( e, false );
		}
		else
		{ // Re-select the text that was modified if needed.
			edit_.getCursor().MoveCur( oristt, false );
			edit_.getCursor().MoveCur( DPos(dend.tl, dend.ad+dif), true );
		}
		// 閉じる？
		End( IDOK );
	}

	TCHAR str[256+INT_DIGITS+1];
	::wsprintf( str, RzsString(IDS_REPLACEALLDONE).c_str(), mcr.size() );
	MsgBox( str, RzsString(IDS_APPNAME).c_str(), MB_ICONINFORMATION );

	replStr_.FreeWCMem( replPat );
}
