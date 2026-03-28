
#include "kilib/stdafx.h"
#include "rsrc/resource.h"
#include "GsMain.h"
#include "LangManager.h"
using namespace ki;
using namespace editwing;

//-------------------------------------------------------------------------
// Start new process
//-------------------------------------------------------------------------

void BootNewProcess( const TCHAR* cmd = TEXT("") )
{
	String fcmd = TEXT("\"");
	fcmd += Path(Path::ExeName);
	fcmd += TEXT("\" ");
	fcmd += cmd;

	PROCESS_INFORMATION psi;
	STARTUPINFO         sti;
	::GetStartupInfo( &sti );
	if( ::CreateProcess( NULL, const_cast<TCHAR*>(fcmd.c_str()),
			NULL, NULL, 0, NORMAL_PRIORITY_CLASS, NULL, NULL,
			&sti, &psi ) )
	{
		::CloseHandle( psi.hThread );
		::CloseHandle( psi.hProcess );
	}
}

static HMENU LoadLocalizedMainMenu(HINSTANCE hInst)
{
	HMENU hMenu = ::LoadMenu(hInst, MAKEINTRESOURCE(IDR_MAIN));
	if (hMenu)
		LangManager::Get().ApplyToMenu(hMenu);
	return hMenu;
}

static HMENU getDocTypeSubMenu(HWND hwnd) { return GetSubMenu( ::GetSubMenu(::GetMenu(hwnd),3),9 ); }

//-------------------------------------------------------------------------
// status bar control
//-------------------------------------------------------------------------

inline GsStBar::GsStBar()
	: str_(NULL)
	, lb_(2)
	, zoom_(100)
{
}

inline void GsStBar::SetCsText( const TCHAR* str )
{
	SetText( str_=str, CS_PART );
}

inline void GsStBar::SetLbText( int lb )
{
	static const TCHAR* const lbstr[] = {TEXT("CR"),TEXT("LF"),TEXT("CRLF")};
	SetText( lbstr[lb_=lb], LB_PART );
}

void GsStBar::SetUnicode( const unicode *uni )
{
	TCHAR buf[ULONG_DIGITS+2+1];

	ulong  cc = uni[0];
	if( isHighSurrogate(uni[0]) )
		cc = 0x10000 + ( ((uni[0]-0xD800)&0x3ff)<<10 ) + ( (uni[1]-0xDC00)&0x3ff );

	TCHAR *t = const_cast<TCHAR*>(LPTR2Hex( buf+2, cc ));
	*--t = TEXT('+'); *--t = TEXT('U');
	SetText( t, UNI_PART );
}

void GsStBar::SetZoom( short z )
{
	zoom_ = z;
	TCHAR buf[INT_DIGITS+1 + 2];
	const TCHAR *b = Int2lStr(buf, z);
	buf[INT_DIGITS+2] = TEXT('\0');
	buf[INT_DIGITS+1] = TEXT('%');
	buf[INT_DIGITS+0] = TEXT(' ');
	SetText(b, GsStBar::ZOOM_PART);
}

int GsStBar::AutoResize( bool maximized )
{
	int h = StatusBar::AutoResize( maximized );
	int w[] = { width()-150, width()-50, width()-50, width()-50, width() };

	HDC dc = ::GetDC( hwnd() );
	SIZE s;
	if( ::GetTextExtentPoint( dc, TEXT("CRLF1M"), 6, &s ) )
		w[3] = w[4] - s.cx;
	if( ::GetTextExtentPoint( dc, TEXT("BBBWWW (100)"), 12, &s ) )
		w[1] = w[2] = w[3] - s.cx;
	if( ::GetTextExtentPoint( dc, TEXT("U+100000"), 8, &s ) )
		w[1] = w[2] - s.cx;
	if( ::GetTextExtentPoint( dc, TEXT("990 %"), 5, &s ) )
		w[0] = Max( w[1] - s.cx, (long)(width()/3) );

	::ReleaseDC( hwnd(), dc );

	SetParts( countof(w), w );
	SetCsText( str_ );
	SetLbText( lb_ );
	SetZoom( zoom_ );
	return h;
}



//-------------------------------------------------------------------------
// dispatcher
//-------------------------------------------------------------------------

LRESULT GreenStarWnd::on_message( UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg )
	{
	case WM_ACTIVATE:
		if( LOWORD(wp) != WA_INACTIVE )
			edit_.SetFocus();
		break;

	case WM_ACTIVATEAPP:
		if( cfg_.warnOnModified() && (BOOL)wp == TRUE )
			PostMessage(hwnd(), WMU_CHECKFILETIMESTAMP, 0, 0);
		return WndImpl::on_message( msg, wp, lp );

	case WM_SIZE:
		if( wp==SIZE_MAXIMIZED || wp==SIZE_RESTORED )
		{
			int ht = stb_.AutoResize( wp==SIZE_MAXIMIZED );
			edit_.MoveTo( 0, 0, LOWORD(lp), HIWORD(lp)-ht );
			cfg_.RememberWnd(this);
		}
		break;

	case WM_MOUSEWHEEL:
		if( wp & MK_CONTROL )
		{
			short delta_zoom = (SHORT)HIWORD(wp) / 12;
			if(delta_zoom == 0)
				delta_zoom += (SHORT)HIWORD(wp) > 0 ? 1 : -1;
			on_setzoom( cfg_.GetZoom() + delta_zoom );
			return 0;
		}
		break;

	#ifdef PM_DPIAWARE
	case 0x02E0: // WM_DPICHANGED
		if( lp )
		{
			edit_.getView().SetFont( cfg_.vConfig(), cfg_.GetZoom() );
			RECT *rc = reinterpret_cast<RECT *>(lp);
			::SetWindowPos( hwnd(), NULL,
				rc->left, rc->top, rc->right-rc->left, rc->bottom-rc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		} break;
	#endif

	case WM_MOVE:
		{
			RECT rc;
			getPos(&rc);
			cfg_.RememberWnd(this);
		}
		break;

	case WM_SYSCOMMAND:
		if( wp==SC_CLOSE || wp==SC_DEFAULT )
			on_exit();
		else
			return WndImpl::on_message( msg, wp, lp );
		break;

	case WM_CONTEXTMENU:
		if( reinterpret_cast<HWND>(wp) == edit_.hwnd() )
			::TrackPopupMenu(
				::GetSubMenu( ::GetMenu(hwnd()), 1 ),
				GetSystemMetrics(SM_MENUDROPALIGNMENT)|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
				static_cast<SHORT>(LOWORD(lp)), static_cast<SHORT>(HIWORD(lp)), 0, hwnd(), NULL );
		else
			return WndImpl::on_message( msg, wp, lp );
		break;

	case WM_INITMENU:
	case WM_INITMENUPOPUP:
		on_initmenu( reinterpret_cast<HMENU>(wp), msg==WM_INITMENUPOPUP );
		break;

	case WM_DROPFILES:
		on_drop( reinterpret_cast<HDROP>(wp) );
		break;

	#ifndef NO_OLEDNDSRC
	case WM_NCRBUTTONDOWN: {
		if( wp == HTSYSMENU
		&& !isUntitled()
		&& coolDragDetect( hwnd(), /*pt=*/lp, WM_NCRBUTTONUP, PM_NOREMOVE )  )
		{
			const unicode *fnu = filename_.ConvToWChar();
			if( fnu )
			{
				OleDnDSourceTxt doDrag( fnu, my_lstrlenW(fnu), DROPEFFECT_COPY );
				filename_.FreeWCMem(fnu);
			}
			break;
		}
	} return WndImpl::on_message( msg, wp, lp );
	#endif

	case GPM_MRUCHANGED:
		SetupMRUMenu();
		break;

	case WM_NOTIFY:
		if( wp == 1787
		&& (reinterpret_cast<NMHDR*>(lp))->code == NM_DBLCLK )
		{
			RECT rc;
			DWORD msgpos = GetMessagePos();
			POINT pt = { (short)LOWORD(msgpos), (short)HIWORD(msgpos) };
			ScreenToClient(stb_.hwnd(), &pt);
			if( stb_.SendMsg( SB_GETRECT, 0, reinterpret_cast<LPARAM>(&rc) ) && PtInRect(&rc, pt) )
				on_jump();
			else if( stb_.SendMsg( SB_GETRECT, 1, reinterpret_cast<LPARAM>(&rc) ) && PtInRect(&rc, pt) )
				on_zoom();
			else if( stb_.SendMsg( SB_GETRECT, 2, reinterpret_cast<LPARAM>(&rc) ) && PtInRect(&rc, pt) )
				on_insertuni();
			else
				on_reopenfile();
		}
		break;

	case WMU_CHECKFILETIMESTAMP:
		if( !isUntitled()
		&&  old_filetime_.dwLowDateTime != 0
		&&  old_filetime_.dwHighDateTime != 0 )
		{
			FILETIME new_filetime = filename_.getLastWriteTime();
			if( ::CompareFileTime(&old_filetime_, &new_filetime) < 0 )
			{
				SendMessage(edit_.getView().hwnd(), WM_LBUTTONUP, 0, GetMessagePos());
				old_filetime_.dwLowDateTime=old_filetime_.dwHighDateTime=0;
				if( IDYES == MsgBox(RzsString(IDS_MODIFIEDOUT).c_str(), RzsString(IDS_APPNAME).c_str(), MB_YESNO) )
					on_refreshfile();
				old_filetime_ = filename_.getLastWriteTime();
			}
		}
		break;

	default:
		return WndImpl::on_message( msg, wp, lp );
	}

	return 0;
}

bool GreenStarWnd::on_command( UINT id, HWND ctrl )
{
	if( edit_.getDoc().isBusy() ) return false;

	switch( id )
	{
	// Window
	case ID_CMD_NEXTWINDOW: on_nextwnd(); break;
	case ID_CMD_PREVWINDOW: on_prevwnd(); break;

	// File
	case ID_CMD_NEWFILE:    on_newfile();   break;
	case ID_CMD_OPENFILE:   on_openfile();  break;
	case ID_CMD_REOPENFILE: on_reopenfile();break;
	case ID_CMD_OPENELEVATED: on_openelevated(filename_); break;
	case ID_CMD_REFRESHFILE:on_refreshfile();break;
	case ID_CMD_SAVEFILE:   on_savefile();  break;
	case ID_CMD_SAVEFILEAS: on_savefileas();break;
	case ID_CMD_PRINT:      on_print();     break;
	case ID_CMD_PAGESETUP:  on_pagesetup(); break;
	case ID_CMD_SAVEEXIT:   if(Save_showDlgIfNeeded()) on_exit();  break;
	case ID_CMD_DISCARDEXIT: Destroy();     break;
	case ID_CMD_EXIT:       on_exit();      break;

	// Edit
	case ID_CMD_UNDO:       edit_.getDoc().Undo();              break;
	case ID_CMD_REDO:       edit_.getDoc().Redo();              break;
	case ID_CMD_CUT:        edit_.getCursor().Cut();            break;
	case ID_CMD_COPY:       edit_.getCursor().Copy();           break;
	case ID_CMD_PASTE:      edit_.getCursor().Paste();          break;
	case ID_CMD_DELETE: if( edit_.getCursor().isSelected() ){ edit_.getCursor().Del(false);} break;
	case ID_CMD_SELECTALL:  edit_.getCursor().Home(true,false);
	                        edit_.getCursor().End(true,true);   break;
	case ID_CMD_DATETIME:   on_datetime();                      break;
	case ID_CMD_INSERTUNI:  on_insertuni();                     break;
	case ID_CMD_ZOOMDLG:    on_zoom();                          break;
	case ID_CMD_ZOOMRZ:     on_setzoom( 100 );                  break;
	case ID_CMD_ZOOMUP:     on_setzoom( cfg_.GetZoom() + 10 );  break;
	case ID_CMD_ZOOMDN:     on_setzoom( cfg_.GetZoom() - 10 );  break;
#ifndef NO_IME
	case ID_CMD_RECONV:     on_reconv();                        break;
	case ID_CMD_TOGGLEIME:  on_toggleime();                     break;
#endif
	// More edit
	case ID_CMD_UPPERCASE:  edit_.getCursor().UpperCaseSel();      break;
	case ID_CMD_LOWERCASE:  edit_.getCursor().LowerCaseSel();      break;
	case ID_CMD_INVERTCASE: edit_.getCursor().InvertCaseSel();     break;
	case ID_CMD_TTSPACES:   edit_.getCursor().TTSpacesSel();       break;
	case ID_CMD_SFCHAR:     edit_.getCursor().StripFirstChar();    break;
	case ID_CMD_SLCHAR:     edit_.getCursor().StripLastChar();     break;
	case ID_CMD_ASCIIFY:    edit_.getCursor().ASCIIFy();           break;

	case ID_CMD_UNINORMC:   edit_.getCursor().UnicodeNormalize(1); break;
	case ID_CMD_UNINORMD:   edit_.getCursor().UnicodeNormalize(2); break;
	case ID_CMD_UNINORMKC:  edit_.getCursor().UnicodeNormalize(5); break;
	case ID_CMD_UNINORMKD:  edit_.getCursor().UnicodeNormalize(6); break;

	case ID_CMD_QUOTE:      edit_.getCursor().QuoteSelection(false);break;
	case ID_CMD_UNQUOTE:    edit_.getCursor().QuoteSelection(true); break;
	case ID_CMD_DELENDLINE: edit_.getCursor().DelToEndline(false); break;
	case ID_CMD_DELSTALINE: edit_.getCursor().DelToStartline(false); break;
	case ID_CMD_DELENDFILE: edit_.getCursor().DelToEndline(true); break;
	case ID_CMD_DELSTAFILE: edit_.getCursor().DelToStartline(true); break;

	// Search
	case ID_CMD_FIND:       search_.ShowDlg();  break;
	case ID_CMD_FINDNEXT:   search_.FindNext(); break;
	case ID_CMD_FINDPREV:   search_.FindPrev(); break;
	case ID_CMD_JUMP:       on_jump(); break;
	case ID_CMD_GREP:       on_grep();break;
	case ID_CMD_HELP:       on_help();break;
	case ID_CMD_OPENSELECTION: on_openselection(); break;
	case ID_CMD_SELECTIONLEN: on_showselectionlen(); break;

	// View
	case ID_CMD_NOWRAP:     edit_.getView().SetWrapType( wrap_=-1 ); break;
	case ID_CMD_WRAPWIDTH:  edit_.getView().SetWrapType( wrap_=cfg_.wrapWidth() ); break;
	case ID_CMD_WRAPWINDOW: edit_.getView().SetWrapType( wrap_=0 ); break;
	case ID_CMD_CONFIG:     on_config();    break;
	case ID_CMD_STATUSBAR:  on_statusBar(); break;

	// Help
	case ID_CMD_HELPABOUT: on_helpabout(); break;

	// DocType
	default: if( ID_CMD_DOCTYPE <= id ) {
			on_doctype( id - ID_CMD_DOCTYPE );
			break;
		} else if( ID_CMD_MRU <= id ) {
			on_mru( id - ID_CMD_MRU );
			break;
		}

		return false;
	}
	return true;
}

bool GreenStarWnd::PreTranslateMessage( MSG* msg )
{
	if( search_.TrapMsg(msg) )
		return true;
	if( HandleWsKey(msg) )
		return true;
	return 0 != ::TranslateAccelerator( hwnd(), accel_, msg );
}



//-------------------------------------------------------------------------
// WordStar key handler
//
// Two-stroke key state machine:
//   wsKeyState_ == 0    : normal mode
//   wsKeyState_ == 'K'  : after Ctrl+K prefix
//   wsKeyState_ == 'Q'  : after Ctrl+Q prefix
//
// Diamond cursor (normal mode, Ctrl held):
//   E=Up  X=Down  S=Left  D=Right
//   A=WordLeft  F=WordRight  R=PageUp  C=PageDown
//
// Ctrl+K prefix:
//   S=Save  D=SaveAs  X=SaveExit  Q=DiscardExit
//
// Ctrl+Q prefix:
//   S=BOL  D=EOL  R=BOF  C=EOF  F=Find  A=SelectAll
//-------------------------------------------------------------------------

bool GreenStarWnd::HandleWsKey( MSG* msg )
{
	if( msg->message != WM_KEYDOWN )
		return false;

	const bool ctrl  = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
	const bool shift = (::GetKeyState(VK_SHIFT)   & 0x8000) != 0;
	const UINT vk    = static_cast<UINT>(msg->wParam);

	// --- Second stroke of a two-stroke sequence ---
	if( wsKeyState_ != 0 )
	{
		const int prevState = wsKeyState_;
		wsKeyState_ = 0;
		// Restore cursor-position display in MAIN_PART
		{ DPos c = old_cur_, s = old_sel_; old_cur_.tl = (ulong)-1; on_move(c, s); }

		if( prevState == 'K' )
		{
			switch( vk )
			{
			case 'N': on_newfile();                               return true;
			case 'O': on_openfile();                              return true;
			case 'S': on_savefile();                              return true;
			case 'D': on_savefileas();                            return true;
			case 'P': on_print();                                 return true;
			case 'X': if(Save_showDlgIfNeeded()) on_exit();      return true;
			case 'Q': Destroy();                                  return true;
			// Block operations: TODO
			// case 'B': mark block begin
			// case 'K': mark block end
			// case 'C': copy block
			// case 'V': move block
			// case 'Y': delete block
			}
		}
		else if( prevState == 'Q' )
		{
			switch( vk )
			{
			case 'S': edit_.getCursor().Home(false, shift);       return true; // BOL
			case 'D': edit_.getCursor().End (false, shift);       return true; // EOL
			case 'R': edit_.getCursor().Home(true,  shift);       return true; // BOF
			case 'C': edit_.getCursor().End (true,  shift);       return true; // EOF
			case 'F': search_.ShowDlg();                          return true; // Find
			case 'A': edit_.getCursor().Home(true,  false);
			          edit_.getCursor().End (true,  true);        return true; // Select All
			case 'G': on_grep();                                  return true;
			// case 'E': top of screen  (TODO: requires view scroll position)
			// case 'X': bottom of screen (TODO)
			}
		}
		// Consume unrecognized second stroke silently.
		return true;
	}

	// --- Single-stroke Ctrl keys (diamond cursor + editing) ---
	if( !ctrl )
		return false;

	switch( vk )
	{
	// Diamond cursor
	case 'E': edit_.getCursor().Up   (false, shift); return true; // Up
	case 'X': edit_.getCursor().Down (false, shift); return true; // Down
	case 'S': edit_.getCursor().Left (false, shift); return true; // Left (char)
	case 'D': edit_.getCursor().Right(false, shift); return true; // Right (char)

	// Word movement
	case 'A': edit_.getCursor().Left (true,  shift); return true; // Word left
	case 'F': edit_.getCursor().Right(true,  shift); return true; // Word right

	// Page movement
	case 'R': edit_.getCursor().PageUp  (shift);     return true; // Page up
	case 'C': edit_.getCursor().PageDown(shift);     return true; // Page down

	// Editing
	case 'G': edit_.getCursor().Del    (false);      return true; // Delete char forward
	case 'H': edit_.getCursor().DelBack(false);      return true; // Backspace
	case 'T': edit_.getCursor().Del    (true);       return true; // Delete word right
	case 'Y': // Delete current line (go to BOL, delete to EOL, then eat newline)
		edit_.getCursor().Home(false, false);
		edit_.getCursor().DelToEndline(false);
		edit_.getCursor().Del(false);
		return true;
	case 'I': edit_.getCursor().Tabulation(false);   return true; // Tab
	case 'M': edit_.getCursor().Return();             return true; // Enter
	case 'V': edit_.getCursor().SetInsMode( !edit_.getCursor().isInsMode() );
	          return true; // Toggle insert/overtype

	// Undo / Redo (WordStar uses Ctrl+U for undo, Ctrl+W for redo)
	case 'U': edit_.getDoc().Undo();                 return true;
	case 'W': edit_.getDoc().Redo();                 return true;

	// Two-stroke prefixes
	case 'K': wsKeyState_ = 'K'; stb_.SetText(TEXT("^K")); return true;
	case 'Q': wsKeyState_ = 'Q'; stb_.SetText(TEXT("^Q")); return true;
	}

	return false;
}



//-------------------------------------------------------------------------
// Command processing
//-------------------------------------------------------------------------

void GreenStarWnd::on_dirtyflag_change( bool )
{
	UpdateWindowName();
}

void GreenStarWnd::on_newfile()
{
	BootNewProcess();
}

void GreenStarWnd::on_openfile()
{
	Path fn;
	int  cs = 0;
	if( ShowOpenDlg( &fn, &cs ) )
		Open( fn, cs, true );
}

void GreenStarWnd::on_helpabout()
{
	#define SHARP(x) #x
	#define STR(x) SHARP(x)

	#if defined(UNICODE)
		#define UNIANSI TEXT(" (Unicode)")
	#elif defined(_MBCS)
		#define UNIANSI TEXT(" (MBCS)")
	#else
		#define UNIANSI TEXT(" (ANSI)")
	#endif

	#if defined(__clang__)
		#define COMPILER TEXT("LLVM Clang - ")  TEXT(STR(__clang_major__)) TEXT(".") TEXT(STR(__clang_minor__))
	#elif defined(__GNUC__)
		#define COMPILER TEXT( "GNU C Compiler - " __VERSION__ )
	#elif defined(_MSC_VER)
		#define COMPILER TEXT("Visual C++ - ")  + (String)SInt2Str((_MSC_VER-600)/100).c_str() + TEXT(".") + SInt2Str(_MSC_VER%100).c_str() +
	#else
		#define COMPILER TEXT( "?\n" )
	#endif

	#if defined(UNICODE)
		#define TARGETOS TEXT("Windows NT")
	#else
		#define TARGETOS TEXT("Windows 9x")
	#endif

	#if defined(_M_AMD64) || defined(_M_X64) || defined(WIN64)
		#define TGVER TEXT(" 5.1")
	#elif defined(_M_IA64)
		#define TGVER TEXT(" 5.0+")
	#elif defined(_M_ARM64) || defined(_M_ARM)
		#define TGVER TEXT(" 6.2+")
	#else
		#define TGVER TEXT(" 3.51+")
	#endif

	#if defined(NO_OLEDNDSRC) && defined(NO_OLEDNDTAR)
		#define USEOLE TEXT(" ")
	#else
		#define USEOLE TEXT(" OLE ")
	#endif

	#if defined(_M_AMD64) || defined(_M_X64)
		#define PALT TEXT( " - x86_64" )
	#elif defined(_M_IA64)
		#define PALT TEXT( "- IA64" )
	#elif defined(_M_ARM64)
		#define PALT TEXT( " - ARM64" )
	#elif defined(_M_ARM)
		#define PALT TEXT( " - ARM" )
	#elif defined(_M_IX86)
		#define PALT TEXT( " - i386" )
	#else
		#define PALT TEXT( "" )
	#endif

	struct AboutDlg A_FINAL: public DlgImpl {
		AboutDlg(HWND parent) : DlgImpl(IDD_ABOUTDLG), parent_( parent ) { GoModal(parent_); }
		void on_init() override
		{
			String s = RzsString(IDS_APPNAME).c_str();
			s += TEXT(" - ") TEXT( VER_FILEVERSIONSTR ) UNIANSI TEXT("\r\n")
			     COMPILER TEXT(" on ") TEXT( __DATE__ ) TEXT("\r\n")
			     TARGETOS TGVER USEOLE PALT TEXT("\r\n")
			     TEXT("Running on ");

			s += TEXT("Windows NT ");
			WORD osver = app().getOSVer();
			WORD osbuild = app().getOSBuild();
			s += SInt2Str( HIBYTE(osver) ).c_str(); s += TEXT(".");
			s += SInt2Str( LOBYTE(osver) ).c_str(); s += TEXT(".");
			s += SInt2Str( osbuild ).c_str();

			SetItemText(IDC_ABOUTSTR, s.c_str());
			SetItemText(IDC_ABOUTURL, RzsString(IDS_PROJECT_URL).c_str());
			SetCenter(hwnd(), parent_);
			String title = TEXT("About ");
			title += RzsString(IDS_APPNAME).c_str();
			SetText(title.c_str());
		}
		HWND parent_;
	} ahdlg (hwnd());

	#undef UNIANSI
	#undef COMPILER
	#undef TGVER
	#undef USEOLE
	#undef PALT
}

void GreenStarWnd::on_reopenfile()
{
	ReopenDlg dlg( charSets_, csi_ );
	dlg.GoModal( hwnd() );
	if( dlg.endcode() == IDOK )
	{
		csi_ = dlg.csi();
		if( !isUntitled() )
		{
			if( AskToSave() )
			{
				int cs = resolveCSI(csi_);
				OpenByMyself( filename_, cs, false );
			}
		}
		else
		{
			UpdateWindowName();
		}
	}
}

int GreenStarWnd::resolveCSI(int csi) const
{
	return ( (UINT)csi >= 0xf0f00000 && (UINT)csi < 0xf1000000 )? csi & 0xfffff
	     : (0 < csi && csi < (int)charSets_.size())? charSets_[csi].ID: 0;
}

void GreenStarWnd::on_openelevated(const ki::Path& fn)
{
	const view::VPos *cur, *sel;
	edit_.getCursor().getCurPosUnordered(&cur, &sel);
	int cp = resolveCSI(csi_);

	String cmdl  = TEXT( "-c"); cmdl += SInt2Str(cp).c_str();
	       cmdl += TEXT(" -l"); cmdl += SInt2Str(cur->tl+1).c_str();
	       cmdl += TEXT(" \""); cmdl += fn.c_str(); cmdl += TEXT("\"");

	TCHAR exename[MAX_PATH];
	Path::GetExeName( exename );

	HINSTANCE ret = ShellExecute(NULL, TEXT("runas"), exename, cmdl.c_str(), NULL, SW_SHOWNORMAL);
	if( (LONG_PTR)ret > 32 )
		::ExitProcess(0);
}

void GreenStarWnd::on_refreshfile()
{
	if( !isUntitled() )
	{
		const view::VPos *cur, *sel;
		bool selected = edit_.getCursor().getCurPosUnordered(&cur, &sel);
		unsigned sline = cur->tl, scol = cur->ad;
		unsigned eline = sel->tl, ecol = sel->ad;
		int cp = resolveCSI(csi_);

		OpenByMyself(filename_, cp, false);
		if (selected) edit_.getCursor().MoveCur(DPos(eline, ecol), false);
		edit_.getCursor().MoveCur(DPos(sline, scol), selected);
	}
}

void GreenStarWnd::on_savefile()
{
	Save_showDlgIfNeeded();
}

void GreenStarWnd::on_savefileas()
{
	if( ShowSaveDlg() )
	{
		Save();
		ReloadConfig();
	}
}

BOOL GreenStarWnd::myPageSetupDlg(LPPAGESETUPDLG lppsd)
{
	return PageSetupDlg(lppsd);
}

void GreenStarWnd::on_pagesetup()
{
	PAGESETUPDLG psd;
	mem00(&psd, sizeof(psd));
	psd.lStructSize = sizeof(psd);
	psd.Flags = PSD_INTHOUSANDTHSOFINCHES|PSD_DISABLEORIENTATION|PSD_DISABLEPAPER|PSD_DISABLEPRINTER|PSD_MARGINS;
	psd.hwndOwner = hwnd();
	CopyRect(&psd.rtMargin, cfg_.PMargins());
	if( myPageSetupDlg(&psd) )
		cfg_.SetPrintMargins(&psd.rtMargin);
}

void GreenStarWnd::SetFontSizeforDC(LOGFONT *font, HDC hDC, int fsiz, int fx)
{
	font->lfWidth  = 0;
	font->lfHeight = -MulDiv(fsiz, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
	if(fx) font->lfWidth = -MulDiv(fx, ::GetDeviceCaps(hDC, LOGPIXELSX), 72);
}

void GreenStarWnd::on_print()
{
	doc::Document& d = edit_.getDoc();
	const unicode* buf;
	ulong len = 0;
	short procCopies = 0, totalCopies = 0;

	PRINTDLG thePrintDlg = { sizeof(thePrintDlg) };
	thePrintDlg.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION | PD_HIDEPRINTTOFILE;
	thePrintDlg.nCopies = 1;
	thePrintDlg.hwndOwner = hwnd();
	thePrintDlg.hDevNames = NULL;
	thePrintDlg.hDevMode = NULL;

	if (PrintDlg(&thePrintDlg) == 0)
		return;

	totalCopies = thePrintDlg.nCopies;

	TCHAR name[1+MAX_PATH+6+32+1];
	GetTitleText( name );

	DOCINFO di = { sizeof(DOCINFO) };
	di.lpszDocName = name;
	di.lpszOutput = (LPTSTR) NULL;
	di.lpszDatatype = (LPTSTR) NULL;
	di.fwType = 0;

	int nError = ::StartDoc(thePrintDlg.hDC, &di);
	if (nError == SP_ERROR)
	{
		TCHAR tmp[128];
		::wsprintf(tmp,TEXT("StartDoc Error #%d - please check printer."),::GetLastError());
		MsgBox(tmp, RzsString(IDS_APPNAME).c_str(), MB_OK|MB_TASKMODAL );
		return;
	}

	LOGFONT lf;
	memmove( &lf, &cfg_.vConfig().font, sizeof(lf) );
	SetFontSizeforDC(&lf, thePrintDlg.hDC, cfg_.vConfig().fontsize, cfg_.vConfig().fontwidth);
	HFONT printfont = ::CreateFontIndirect(&lf);
	HFONT oldfont = (HFONT)::SelectObject( thePrintDlg.hDC, printfont );

	::StartPage(thePrintDlg.hDC);

	int cWidthPels  = ::GetDeviceCaps(thePrintDlg.hDC, HORZRES);
	int cHeightPels = ::GetDeviceCaps(thePrintDlg.hDC, VERTRES);
	int logpxx = GetDeviceCaps(thePrintDlg.hDC, LOGPIXELSX);
	int logpxy = GetDeviceCaps(thePrintDlg.hDC, LOGPIXELSY);

	RECT rctmp = {0, 0, cWidthPels, cHeightPels};
	::DrawTextW(thePrintDlg.hDC, L"#", 1, &rctmp, DT_CALCRECT|DT_LEFT|DT_WORDBREAK|DT_EXPANDTABS|DT_EDITCONTROL);
	int cLineHeight = rctmp.bottom-rctmp.top;

	const RECT rcMargins = {
		(cfg_.PMargins()->left  * logpxx)/1000,
		(cfg_.PMargins()->top   * logpxy)/1000,
		(cfg_.PMargins()->right * logpxx)/1000,
		(cfg_.PMargins()->left  * logpxy)/1000,
	};
	const RECT rcFullPage = {
		rcMargins.left , rcMargins.top,
		cWidthPels - rcMargins.left - rcMargins.right,
		cHeightPels - rcMargins.top - rcMargins.bottom
	};
	RECT rcPrinter;
	CopyRect(&rcPrinter, &rcFullPage);

	int nThisLineHeight, nChars = 0, nHi = 0, nLo = 0;
	const unicode* uStart;

	#define myDTFLAGS DT_WORDBREAK|DT_NOCLIP|DT_EXPANDTABS|DT_NOPREFIX|DT_EDITCONTROL
	do {
		if( procCopies )
		{
			::StartPage(thePrintDlg.hDC);
			CopyRect(&rcPrinter, &rcFullPage);
		}
		for( ulong e=d.tln(), dpStart=0; dpStart<e; )
		{
			len = d.len(dpStart);
			buf = d.tl(dpStart);
			if(!len)
			{
				rcPrinter.top += cLineHeight;
				++dpStart;
			}
			else
			{
				CopyRect(&rctmp, &rcPrinter);
				nHi = len;
				nLo = 0;
				if(!nChars)
				{
					uStart = buf;
					nChars = len;
				}
				else
				{
					uStart += nChars;
					nHi = nChars = len-nChars;
				}

				while (nLo < nHi) {
					rctmp.top = rcPrinter.top;
					nThisLineHeight = ::DrawTextW(thePrintDlg.hDC, uStart, nChars, &rctmp, DT_CALCRECT|myDTFLAGS);
					if (rcPrinter.top+nThisLineHeight < rcPrinter.bottom)
						nLo = nChars;
					if (rcPrinter.top+nThisLineHeight > rcPrinter.bottom)
						nHi = nChars;
					if (nLo == nHi - 1)
						nChars = nHi = nLo;
					if (nLo < nHi)
						nChars = nLo + (nHi - nLo)/2;
				}
				rcPrinter.top += ::DrawTextW(thePrintDlg.hDC, uStart, nChars, &rcPrinter, myDTFLAGS);
				if(uStart+nChars == buf+len)
				{
					nChars = 0;
					++dpStart;
				}
			}

			if( (dpStart<e) && (rcPrinter.top + cLineHeight + 5 > rcPrinter.bottom) )
			{
				::EndPage(thePrintDlg.hDC);
				::StartPage(thePrintDlg.hDC);
				CopyRect(&rcPrinter, &rcFullPage);
			}
		}
		::EndPage(thePrintDlg.hDC);
	} while( ++procCopies < totalCopies );
	#undef myDTFLAGS

	::SelectObject(thePrintDlg.hDC, oldfont);
	::DeleteObject(printfont);
	::EndDoc(thePrintDlg.hDC);
	::DeleteDC(thePrintDlg.hDC);
	::GlobalFree(thePrintDlg.hDevNames);
	::GlobalFree(thePrintDlg.hDevMode);
}

void GreenStarWnd::on_exit()
{
	search_.SaveToINI();
	if( AskToSave() )
		Destroy();
}

void GreenStarWnd::on_initmenu( HMENU menu, bool editmenu_only )
{
	UINT gray_when_unselected = MF_BYCOMMAND|(edit_.getCursor().isSelected()? MF_ENABLED: MF_GRAYED);
	::EnableMenuItem( menu, ID_CMD_CUT,    gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_COPY,   gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_DELETE, gray_when_unselected);
	::EnableMenuItem( menu, ID_CMD_UNDO,   MF_BYCOMMAND|(edit_.getDoc().isUndoAble()? MF_ENABLED: MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_REDO,   MF_BYCOMMAND|(edit_.getDoc().isRedoAble()? MF_ENABLED: MF_GRAYED) );

	::EnableMenuItem( menu, ID_CMD_UPPERCASE, gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_LOWERCASE, gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_INVERTCASE,gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_TTSPACES,  gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_ASCIIFY,   gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_SFCHAR,    gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_SLCHAR,    gray_when_unselected );

#ifndef NO_IME
	::EnableMenuItem( menu, ID_CMD_RECONV, MF_BYCOMMAND|(edit_.getCursor().isSelected() && ime().IsIME() && ime().CanReconv() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_TOGGLEIME, MF_BYCOMMAND|(ime().IsIME() ? MF_ENABLED : MF_GRAYED) );
#endif
	if( editmenu_only )
		return;

	::EnableMenuItem( menu, ID_CMD_SAVEFILE, MF_BYCOMMAND|(isUntitled() || edit_.getDoc().isModified() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_OPENELEVATED, MF_BYCOMMAND|MF_ENABLED );
	::EnableMenuItem( menu, ID_CMD_GREP, MF_BYCOMMAND|(cfg_.grepExe().len()>0 ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_OPENSELECTION, gray_when_unselected );

	::CheckMenuItem( menu, ID_CMD_NOWRAP, MF_BYCOMMAND|(wrap_==-1?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_WRAPWIDTH, MF_BYCOMMAND|(wrap_>0?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_WRAPWINDOW, MF_BYCOMMAND|(wrap_==0?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_STATUSBAR, cfg_.showStatusBar()?MF_CHECKED:MF_UNCHECKED );
}

void GreenStarWnd::on_drop( HDROP hd )
{
	UINT iMax = ::myDragQueryFile( hd, 0xffffffff, NULL, 0 );
	for( UINT i=0; i<iMax; ++i )
	{
		UINT len = ::myDragQueryFile( hd, i, NULL, 0)+1;
		len = Max(len, (UINT)MAX_PATH);
		TCHAR *str = (TCHAR *)TS.alloc( sizeof(TCHAR) * len );
		if( str )
		{
			::myDragQueryFile( hd, i, str, len );
			Open( str, AutoDetect );
			TS.freelast( str, sizeof(TCHAR) * len );
		}
	}
	::DragFinish( hd );
}

void GreenStarWnd::on_jump()
{
	const view::VPos *cur, *sel;
	edit_.getCursor().getCurPosUnordered(&cur, &sel);
	struct JumpDlg A_FINAL: public DlgImpl {
		JumpDlg( HWND w, int cur_line, int cur_col )
			: DlgImpl( IDD_JUMP )
			, LineNo ( cur_line )
			, ColNo  ( cur_col  )
			, w_( w )
			{ GoModal(w); }
		void on_init() override
		{
			SetCenter(hwnd(),w_); ::SetFocus(item(IDC_LINEBOX));
		}
		bool on_ok() override
		{
			TCHAR str[64], *pcol=NULL;
			::GetWindowText( item(IDC_LINEBOX), str, countof(str) );
			for( TCHAR *p=str; *p ;p++  )
			{
				if( *p == TEXT(',') )
				{
					*p = TEXT('\0');
					pcol = ++p;
					break;
				}
			}
			if( *str != TEXT(',') )
				LineNo = newnumber( LineNo, str );
			if( pcol )
				ColNo = newnumber( ColNo, pcol );
			else
				ColNo = 1;
			return true;
		}
		int newnumber( int number, const TCHAR *str )
		{
			if( str[0] == TEXT('+') || str[0] == TEXT('-'))
			{
				number += String::GetInt(str);
				if( number <= 0 ) number = 1;
			}
			else
			{
				number = String::GetInt(str);
			}
			return number;
		}

		int LineNo;
		int ColNo;
		HWND w_;
	} dlg( hwnd(), cur->tl+1, cur->ad+1 );

	if( IDOK == dlg.endcode() )
		edit_.getCursor().MoveCur( DPos(dlg.LineNo-1, dlg.ColNo-1), false );
}

void GreenStarWnd::on_openselection()
{
#define isAbsolutePath(x) ( x[0] == L'\\' || (x[0] && x[1] == L':') )
	String cmd = TEXT("-c0 \"");
	aarr<unicode> sel = edit_.getCursor().getSelectedStr();
	size_t slen = my_lstrlenW( sel.get() );
	while( slen-- && (sel[ slen ] == L'\r' || sel[ slen ] == L'\n') )
		sel[ slen ] = L'\0';

	if( my_instringW( sel.get(), L"http://")
	||  my_instringW( sel.get(), L"https://")
	||  my_instringW( sel.get(), L"ftp://")
	||  my_instringW( sel.get(), L"ftps://") )
	{
		cmd = sel.get();
		ShellExecute(NULL, TEXT("open"), cmd.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return;
	}
	if( !isAbsolutePath( sel ) )
	{
		Path d;
		if( filename_.len() )
			(d = filename_).BeDirOnly().BeBackSlash(true);
		else
			d = Path(Path::Cur);

		cmd += d;
	}

	cmd += sel.get();
	cmd += TEXT("\"");
	BootNewProcess( cmd.c_str() );
#undef isAbsolutePath
}

void GreenStarWnd::on_showselectionlen()
{
	const view::VPos *a, *b;
	edit_.getCursor().getCurPos(&a, &b);
	ulong len = edit_.getDoc().getRangeLength(*a, *b);
	TCHAR buf[ULONG_DIGITS+1];
	if( stb_.isVisible() )
		stb_.SetText( Ulong2lStr(buf, len) );
	else
		MsgBox( Ulong2lStr(buf, len), TEXT("Length in UTF-16 chars") );
}

void GreenStarWnd::on_grep()
{
	on_external_exe_start( cfg_.grepExe() );
}

void GreenStarWnd::on_help()
{
	on_external_exe_start( cfg_.helpExe() );
}

void GreenStarWnd::on_external_exe_start(const Path &g)
{
	if( g.len() != 0 )
	{
		Path d;
		if( filename_.len() )
			(d = filename_).BeDirOnly().BeBackSlash(false);
		else
			d = Path(Path::Cur);

		String fcmd;
		for( size_t i=0, e=g.len(); i<e; ++i )
		{
			if( g[i]==TEXT('%') )
			{
				if( g[i+1]==TEXT('1') || g[i+1]==TEXT('D') )
					++i, fcmd += d;
				else if( g[i+1]==TEXT('F') )
					++i, fcmd += filename_;
				else if( g[i+1]==TEXT('N') )
					++i, fcmd += filename_.name();
				else if( g[i+1]==TEXT('S') )
					++i, fcmd += edit_.getCursor().getSelectedStr().get();
			}
			else
			{
				fcmd += g[i];
			}
		}

		PROCESS_INFORMATION psi;
		STARTUPINFO         sti = {sizeof(STARTUPINFO)};
		if( ::CreateProcess( NULL, const_cast<TCHAR*>(fcmd.c_str()),
				NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
				&sti, &psi ) )
		{
			::CloseHandle( psi.hThread );
			::CloseHandle( psi.hProcess );
		}
	}
}

void GreenStarWnd::on_datetime()
{
	String g = cfg_.dateFormat();
	TCHAR buf[255], tmp[255]=TEXT("");
	const TCHAR *lpFormat = g.len()?const_cast<TCHAR*>(g.c_str()):TEXT("HH:mm yyyy/MM/dd");
	::GetTimeFormat( LOCALE_USER_DEFAULT, 0, NULL, lpFormat, buf, countof(buf));
	::GetDateFormat( LOCALE_USER_DEFAULT, 0, NULL, buf, tmp, countof(tmp));
	edit_.getCursor().Input( tmp, my_lstrlen(tmp) );
}

void GreenStarWnd::on_insertuni()
{
	struct InsertUnicode A_FINAL: public DlgImpl {
		InsertUnicode(HWND w) : DlgImpl(IDD_JUMP), utf32_(0xffffffff), w_(w) { GoModal(w); }
		void on_init() override
		{
			SetCenter( hwnd(), w_ );
			SetItemText( IDC_LINLABEL, TEXT("&U+") );
			SetItemText( IDOK, RzsString(IDS_INSSERT).c_str() );
			SetText( RzsString(IDS_INSERTUNI).c_str() );
			::SetFocus(item(IDC_LINEBOX));
		}
		bool on_ok() override
		{
			TCHAR str[32]; str[0] = TEXT('\0');
			::GetWindowText( item(IDC_LINEBOX), str, countof(str) );
			if( !*str )
				return true;
			if( str[0] == TEXT('.') )
				utf32_ = String::GetInt(str+1);
			else if( str[0] == TEXT('o') || str[0] == TEXT('O') )
				utf32_ = Octal2Ulong(str+1);
			else
				utf32_ = Hex2Ulong( str + (str[0] == TEXT('x') || str[0] == TEXT('X')) );
			return true;
		}
		qbyte utf32_;
		HWND w_;
	} dlg(hwnd());

	if( IDOK == dlg.endcode() && dlg.utf32_ != 0xffffffff )
		edit_.getCursor().InputUTF32( dlg.utf32_ );
}

void GreenStarWnd::on_zoom()
{
	struct ZoomDlg A_FINAL: public DlgImpl {
		ZoomDlg(HWND w, short zoom) : DlgImpl(IDD_JUMP), zoom_(zoom), w_(w) { GoModal(w); }
		void on_init() override
		{
			SetCenter( hwnd(), w_ );
			SetItemText( IDC_LINLABEL, TEXT("%") );
			SetItemText( IDOK, TEXT("OK") );
			SetText( RzsString(IDS_ZOOMPC).c_str() );
			SetItemText( IDC_LINEBOX, SInt2Str(zoom_).c_str() );
			HWND ed = item(IDC_LINEBOX);
			::SetFocus(ed);
			::SendMessage(ed, EM_SETSEL, 0, -1);
		}
		bool on_ok() override
		{
			TCHAR str[16]; str[0] = TEXT('\0');
			::GetWindowText( item(IDC_LINEBOX), str, countof(str) );
			if( !*str ) { zoom_ = 100; return true; }
			zoom_ = String::GetInt(str);
			return true;
		}
		short zoom_;
		HWND w_;
	} dlg(hwnd(), cfg_.GetZoom());

	short zoom = dlg.zoom_;
	if( IDOK == dlg.endcode() && zoom != cfg_.GetZoom() )
		on_setzoom( zoom );
}

void GreenStarWnd::on_setzoom( short zoom )
{
	zoom = Clamp((short)0, zoom, (short)990);
	edit_.getView().SetFont( cfg_.vConfig(), zoom );
	cfg_.SetZoom( zoom );
	stb_.SetZoom( zoom );
}

void GreenStarWnd::on_doctype( int no )
{
	if( HMENU m = getDocTypeSubMenu( hwnd() ) )
	{
		cfg_.SetDocTypeByMenu( no, m );
		ReloadConfig( true );
	}
}

void GreenStarWnd::on_config()
{
	if( cfg_.DoDialog(*this) )
	{
		SetupSubMenu();
		SetupMRUMenu();
		ReloadConfig(false);
	}
}

#define MyFindWindowEx FindWindowEx
static void MyShowWnd( HWND wnd )
{
	if( ::IsIconic(wnd) )
		::ShowWindow( wnd, SW_RESTORE );
	::BringWindowToTop( wnd );
}

void GreenStarWnd::on_nextwnd()
{
	if( HWND next = ::MyFindWindowEx( NULL, hwnd(), className_, NULL ) )
	{
		int i=0;
		HWND last=next, pos=NULL;
		while( last != NULL && i++ < 1024 )
			last = ::MyFindWindowEx( NULL, pos=last, className_, NULL );
		if( pos != next )
			::SetWindowPos( hwnd(), pos,
				0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW );
		MyShowWnd( next );
	}
}

void GreenStarWnd::on_prevwnd()
{
	HWND pos=NULL, next=::MyFindWindowEx( NULL,NULL,className_,NULL );
	int i=0;
	if( next==hwnd() )
	{
		while( next != NULL && i++ < 1024 )
			next = ::MyFindWindowEx( NULL,pos=next,className_,NULL );
		if( pos!=hwnd())
			MyShowWnd( pos );
	}
	else
	{
		while( next!=hwnd() && next!=NULL && i++ < 1024)
			next = ::MyFindWindowEx( NULL,pos=next,className_,NULL );
		if( next!=NULL )
			MyShowWnd( pos );
	}
}

void GreenStarWnd::on_statusBar()
{
	stb_.SetStatusBarVisible( !stb_.isStatusBarVisible() );
	cfg_.ShowStatusBarSwitch();

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	::GetWindowPlacement( hwnd(), &wp );
	if( wp.showCmd != SW_MINIMIZE )
	{
		const int ht = stb_.AutoResize( wp.showCmd == SW_MAXIMIZE );
		RECT rc;
		getClientRect(&rc);
		edit_.MoveTo( 0, 0, rc.right, rc.bottom-ht );
	}
}

void GreenStarWnd::on_move( const DPos& c, const DPos& s )
{
	static int busy_cnt = 0;
	if( !stb_.isVisible() )
		return;
	if( edit_.getDoc().isBusy() && ((++busy_cnt)&0xff) )
		return;

	if( c == old_cur_ && s == old_sel_ )
		return;

	if( c == s )
	{
		const unicode* su = edit_.getDoc().tl(c.tl);
		stb_.SetUnicode( su+c.ad );
	}
	else
	{
		TCHAR buf[ULONG_DIGITS+1];
		ulong N = s.tl == c.tl
			? Max(c.ad, s.ad) - Min(c.ad, s.ad)
			: Max(c.tl, s.tl) - Min(c.tl, s.tl);
		stb_.SetText( Ulong2lStr(buf, N), GsStBar::UNI_PART );
	}

	old_cur_ = c;
	old_sel_ = s;

	ulong cad = c.ad;
	if( ! cfg_.countByUnicode() )
	{
		const unicode* cu = edit_.getDoc().tl(c.tl);
		const uint tab = NZero(cfg_.vConfig().tabstep);
		cad = 0;
		for( ulong i=0; i<c.ad; ++i )
			if( cu[i] == L'\t' )
				cad = (cad/tab+1)*tab;
			else if( cu[i]<0x80 || (0xff60<=cu[i] && cu[i]<=0xff9f) )
				cad = cad + 1;
			else
				cad = cad + 2;
	}

	TCHAR str[ULONG_DIGITS*4 + 10], *end = str;
	TCHAR tmp[ULONG_DIGITS+1];
	*end++ = TEXT('(');
	end = my_lstrkpy( str+1, Ulong2lStr(tmp, c.tl+1) );
	*end++ = TEXT(',');
	end = my_lstrkpy( end, Ulong2lStr(tmp, cad+1) );
	*end++ = TEXT(')');
	*end = TEXT('\0');
	if( c != s )
	{
		ulong sad = s.ad;
		if( ! cfg_.countByUnicode() )
		{
			const unicode* su = edit_.getDoc().tl(s.tl);
			sad = 0;
			for( ulong i=0; i<s.ad; ++i )
				sad += su[i]<0x80 || (0xff60<=su[i] && su[i]<=0xff9f) ? 1 : 2;
		}
		end = my_lstrkpy( end, TEXT(" - (") );
		end = my_lstrkpy( end, Ulong2lStr(tmp, s.tl+1) );
		*end++ = TEXT(',');
		end = my_lstrkpy( end, Ulong2lStr(tmp, sad+1) );
		*end++ = TEXT(')');
		*end = TEXT('\0');
	}
	stb_.SetText( str );
}

void GreenStarWnd::on_reconv()
{
	edit_.getCursor().Reconv();
}

void GreenStarWnd::on_toggleime()
{
	edit_.getCursor().ToggleIME();
}



//-------------------------------------------------------------------------
// utility
//-------------------------------------------------------------------------

void GreenStarWnd::JumpToLine( ulong ln )
{
	edit_.getCursor().MoveCur( DPos(ln-1,0), false );
}

void GreenStarWnd::SetupSubMenu()
{
	if( HMENU m = getDocTypeSubMenu( hwnd() ) )
	{
		cfg_.SetDocTypeMenu( m, ID_CMD_DOCTYPE );
		::DrawMenuBar( hwnd() );
	}
}

void GreenStarWnd::GetTitleText( TCHAR *name )
{
	TCHAR *end = name+1;
	RzsString untitled(IDS_UNTITLED);
	const TCHAR* untitledText = untitled.c_str();
	RzsString appName(IDS_APPNAME);
	const TCHAR* appNameText = appName.c_str();
	name[0] = TEXT('[');
	if( isUntitled() )
		end = my_lstrkpy( end, untitledText );
	else
		end = my_lstrkpy( end, filename_.name() );
	if( edit_.getDoc().isModified() )
		end = my_lstrkpy( end, TEXT(" *") );
	end = my_lstrkpy( end, TEXT("] - ") );
	end = my_lstrkpy( end, appNameText );
}

void GreenStarWnd::UpdateWindowName()
{
	{
		TCHAR name[1+MAX_PATH+6+32+1];
		GetTitleText( name );
		SetText( name );
	}

	static TCHAR cpname[32];
	TCHAR tmp[INT_DIGITS+1];
	if( (UINT)csi_==0xffffffff )
	{
		stb_.SetCsText( TEXT("UNKN") );
	}
	else if((UINT)csi_ >= 0xf0f00000 && (UINT)csi_ < 0xf1000000)
	{
		cpname[0] = TEXT('C'); cpname[1] = TEXT('P');
		my_lstrkpy( cpname+2, Int2lStr(tmp, csi_ & 0xfffff) );
		stb_.SetCsText( cpname );
	}
	else if (0 <= csi_ && csi_ < (int)charSets_.size() )
	{
		TCHAR *end = my_lstrkpy(cpname, charSets_[csi_].shortName);
		*end++ = TEXT(' ');
		*end++ = TEXT('(');
		end = my_lstrkpy( end, Int2lStr(tmp, charSets_[csi_].ID) );
		*end++ = TEXT(')');
		*end = TEXT('\0');
		stb_.SetCsText( cpname );
	} else {
		stb_.SetCsText( Int2lStr(cpname, csi_) );
	}
	stb_.SetLbText( lb_ );
}

void GreenStarWnd::SetupMRUMenu()
{
	int nmru=0;
	HMENU mparent = ::GetSubMenu(::GetMenu(hwnd()),0);
	if( HMENU m = ::GetSubMenu(mparent, 13) )
	{
		nmru = cfg_.SetUpMRUMenu( m, ID_CMD_MRU );
		::DrawMenuBar( hwnd() );
	}
	::EnableMenuItem(mparent, 13, MF_BYPOSITION|(nmru?MF_ENABLED:MF_GRAYED));
}

void GreenStarWnd::on_mru( int no )
{
	Path fn = cfg_.GetMRU(no);
	if( fn.len() != 0 )
		Open( fn, AutoDetect );
}



//-------------------------------------------------------------------------
// Settings update process
//-------------------------------------------------------------------------

void GreenStarWnd::ReloadConfig( bool noSetDocType )
{
	if( !noSetDocType )
	{
		int t = cfg_.SetDocType( filename_ );
		if( HMENU m = getDocTypeSubMenu( hwnd() ) )
			cfg_.CheckMenu( m, t );
	}

	edit_.getDoc().SetUndoLimit( cfg_.undoLimit() );

	wrap_ = cfg_.wrapType();
	edit_.getView().SetWrapLNandFont( wrap_, cfg_.wrapSmart(), cfg_.showLN(), cfg_.vConfig(), cfg_.GetZoom() );

	Path kwd = cfg_.kwdFile();
	FileR fp;
	if( kwd.len()!=0 && kwd.isFile() && fp.Open(kwd.c_str()) )
		edit_.getDoc().SetKeyword(reinterpret_cast<const unicode*>(fp.base()),fp.size()/sizeof(unicode));
	else
		edit_.getDoc().SetKeyword(NULL,0);
}



//-------------------------------------------------------------------------
// Opening process
//-------------------------------------------------------------------------

bool GreenStarWnd::ShowOpenDlg( Path* fn, int* cs )
{
	RzsString txtfiles(IDS_TXTFILES);
	RzsString allfiles(IDS_ALLFILES);
	const TCHAR *flst[] = {
		txtfiles.c_str(),
		cfg_.txtFileFilter().c_str(),
		allfiles.c_str(),
		TEXT("*.*")
	};
	aarr<TCHAR> filt = OpenFileDlg::ConnectWithNull(flst, countof(flst));

	OpenFileDlg ofd( charSets_ );
	bool ok = ofd.DoModal( hwnd(), filt.get(), filename_.c_str() );
	if( ok )
	{
		*fn = ofd.filename();
		*cs = resolveCSI( ofd.csi() );
	}

	return ok;
}

bool GreenStarWnd::Open( const ki::Path& fn, int cs, bool always )
{
	if( isUntitled() && !edit_.getDoc().isModified() )
	{
		return OpenByMyself( fn, cs, true, always );
	}
	else
	{
		if( cfg_.openSame() )
			return ( AskToSave() ? OpenByMyself( fn, cs, true, true ) : true );

		String
			cmd  = TEXT("-c");
			cmd += SInt2Str( cs ).c_str();
			cmd += TEXT(" \"");
			cmd += fn;
			cmd += TEXT('\"');
		BootNewProcess( cmd.c_str() );
		return true;
	}
}

BOOL CALLBACK GreenStarWnd::PostMsgToFriendsProc(HWND hwnd, LPARAM lPmsg)
{
	TCHAR classn[256];
	if(::IsWindow(hwnd))
	{
		if( ::GetClassName(hwnd, classn, countof(classn))
		&&  !my_lstrcmp(classn, className_) )
			::PostMessage(hwnd, (UINT)lPmsg, 0, 0);
	}
	return TRUE;
}

BOOL GreenStarWnd::PostMsgToAllFriends(UINT msg)
{
	return EnumWindows(PostMsgToFriendsProc, static_cast<LPARAM>(msg));
}

bool GreenStarWnd::OpenByMyself( const ki::Path& fn, int cs, bool needReConf, bool always )
{
	LOGGERS( fn.c_str() );
	TextFileR tf(cs);

	if( !tf.Open( fn.c_str(), always ) )
	{
		int err = GetLastError();
		RzsString ids_oerr(IDS_OPENERROR);
		String fnerror = fn; fnerror+= TEXT('\n'); fnerror += ids_oerr.c_str();
		fnerror += SInt2Str(err).c_str();
		if( err == ERROR_ACCESS_DENIED )
		{
			if ( fn.isDirectory() )
			{
				fnerror += RzsString(IDS_CANTOPENDIR).c_str();
				MsgBox( fnerror.c_str(), ids_oerr.c_str(), MB_OK );
				return false;
			}
			on_openelevated(fn);
			return false;
		}
		MsgBox( fnerror.c_str(), ids_oerr.c_str() );
		return false;
	}
	else
	{
		int drivestart=0;
		bool networkpath = false;
		if( fn[0] == TEXT('\\') && fn[1] == TEXT('\\') )
		{
			if( fn[2] != '?' && fn[2] != '.' )
				networkpath = true;
			else if( fn[3] == '\\' )
			{
				if( fn[4] && fn[5] == ':')
					drivestart = 4;
				else if(
				    fn[7] == '\\'
				&& (fn[4] == 'U' || fn[4] == 'u')
				&& (fn[5] == 'N' || fn[5] == 'n')
				&& (fn[6] == 'C' || fn[6] == 'c') )
					networkpath = true;
			}
		}

		bool notry = networkpath;
		if (!notry)
		{
			TCHAR drive[4];
			drive[0] = fn[drivestart+0];
			drive[1] = fn[drivestart+1];
			drive[2] = fn[drivestart+2];
			drive[3] = TEXT('\0');
			UINT DT = GetDriveType(drive);
			notry = !(DT&DRIVE_REMOTE) || !(DT&DRIVE_CDROM);
		}

		TextFileW tfw( cs, lb_ );
		if (!notry && !tfw.Open( fn.c_str() ) && GetLastError() == ERROR_ACCESS_DENIED )
		{
			if( !networkpath )
			{
				String fnerror = fn; fnerror += RzsString(IDS_NOWRITEACESS).c_str();
				if ( IDYES == MsgBox( fnerror.c_str(), RzsString(IDS_OPENERROR).c_str(), MB_YESNO ) )
				{
					on_openelevated(fn);
					return false;
				}
			}
		}
	}

	if( fn[0]==TEXT('\\') || (fn[0] && fn[1]==TEXT(':')) )
		filename_ = fn;
	else
		filename_ = Path( Path::Cur ) + fn;

	if( tf.size() )
	{
		csi_ = charSets_.findCsi( tf.codepage() );
		if( (UINT)csi_ == 0xffffffff )
			csi_ = 0xf0f00000 | tf.codepage();
		if( tf.nolb_found() )
			lb_ = cfg_.GetNewfileLB();
		else
			lb_ = tf.linebreak();
	}
	else
	{
		csi_ = cfg_.GetNewfileCsi();
		lb_  = cfg_.GetNewfileLB();
	}
	filename_.BeShortLongStyle();

	::SetCurrentDirectory( Path(filename_).BeDriveOnly().c_str() );

	if( needReConf )
		ReloadConfig();

	edit_.getDoc().ClearAll();
	stb_.SetText( RzsString(IDS_LOADING).c_str() );
	old_filetime_ = filename_.getLastWriteTime();
	edit_.getDoc().OpenFile( tf );
	stb_.SetText( TEXT("(1,1)") );

	UpdateWindowName();

	if( cfg_.AddMRU( filename_ ) )
		PostMsgToAllFriends(GPM_MRUCHANGED);

	return true;
}



//-------------------------------------------------------------------------
// Save process
//-------------------------------------------------------------------------

bool GreenStarWnd::ShowSaveDlg()
{
	RzsString allfiles(IDS_ALLFILES);
	const TCHAR *flst[] = {
		allfiles.c_str(),
		TEXT("*.*")
	};
	aarr<TCHAR> filt = SaveFileDlg::ConnectWithNull( flst, countof(flst) );

	SaveFileDlg sfd( charSets_, csi_, lb_ );
	StatusBar::SaveRestoreText SaveRest(stb_);
	stb_.SetText( TEXT("Saving file...") );
	if( !sfd.DoModal( hwnd(), filt.get(), filename_.c_str() ) )
		return false;

	const int csi = sfd.csi();
	bool invalidCS = false;
	if( (UINT)csi == 0xffffffff )
		invalidCS = true;
	else if( (UINT)csi >= 0xf0f00000 && (UINT)csi < 0xf1000000 )
	{
		int neededcs = TextFileR::neededCodepage( resolveCSI(csi) );
		invalidCS = neededcs != 0 && !::IsValidCodePage( neededcs );
	}
	if( invalidCS )
	{
		MsgBox( RzsString(IDS_INVALIDCP).c_str(), NULL, MB_OK);
		return false;
	}

	filename_ = sfd.filename();
	csi_      = sfd.csi();
	lb_       = sfd.lb();

	return true;
}

bool GreenStarWnd::Save_showDlgIfNeeded()
{
	bool wasUntitled = isUntitled();
	if( isUntitled() )
		if( !ShowSaveDlg() )
			return false;
	if( Save() )
	{
		if( wasUntitled )
			ReloadConfig();
		return true;
	}
	return false;
}

bool GreenStarWnd::AskToSave()
{
	if( edit_.getDoc().isModified() )
	{
		int answer = MsgBox(
			RzsString(IDS_ASKTOSAVE).c_str(),
			RzsString(IDS_APPNAME).c_str(),
			MB_YESNOCANCEL|MB_ICONQUESTION
		);
		if( answer == IDYES )    return Save_showDlgIfNeeded();
		if( answer == IDCANCEL ) return false;
	}
	return true;
}

bool GreenStarWnd::Save()
{
	int save_Csi;

	if((UINT)csi_ == 0xffffffff)
		save_Csi = ::GetACP();
	else
		save_Csi = resolveCSI(csi_);

	if (!save_Csi) save_Csi = ::GetACP();

	{
		TextFileW tf( save_Csi, lb_ );

		if( !tf.Open( filename_.c_str() ) )
		{
			DWORD err = GetLastError();
			String fnerror = filename_ + TEXT("\n\nError #");
			fnerror += SInt2Str(err).c_str();
			MsgBox( fnerror.c_str(), RzsString(IDS_SAVEERROR).c_str() );
			return false;
		}
		edit_.getDoc().SaveFile( tf );
	}
	UpdateWindowName();
	if( cfg_.AddMRU( filename_ ) )
		PostMsgToAllFriends(GPM_MRUCHANGED);

	old_filetime_ = filename_.getLastWriteTime();
	return true;
}



//-------------------------------------------------------------------------
// Initializing the main window
//-------------------------------------------------------------------------

GreenStarWnd::ClsName GreenStarWnd::className_ = TEXT("GreenStar MainWnd");

GreenStarWnd::GreenStarWnd()
	#ifndef FORCE_RTL_LAYOUT
	: WndImpl  ( className_, WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES)
	#else
	: WndImpl  ( className_, WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES|WS_EX_LAYOUTRTL )
	#endif
	, search_     ( *this, edit_ )
	, charSets_   ( cfg_.GetCharSetList() )
	, old_cur_    ( DPos(0,0) )
	, old_sel_    ( DPos(0,0) )
	, csi_        ( cfg_.GetNewfileCsi() )
	, lb_         ( cfg_.GetNewfileLB() )
	, wrap_       ( -1 )
	, wsKeyState_ ( 0 )
{
	LANGID uiLang = ::GetUserDefaultUILanguage();
	::SetThreadLocale( MAKELCID(uiLang, SORT_DEFAULT) );

	static WNDCLASS wc;
	wc.style         = 0;
	wc.hIcon         = app().LoadIcon( IDR_MAIN );
	wc.hCursor       = app().LoadOemCursor( IDC_ARROW );
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = className_;
	WndImpl::Register( &wc );
#ifndef NO_IME
	ime().EnableGlobalIME( true );
#endif
}

void GreenStarWnd::on_create( CREATESTRUCT* cs )
{
	HMENU hMenu = LoadLocalizedMainMenu(app().hinst());
	if (hMenu)
		::SetMenu(hwnd(), hMenu);

	accel_ = app().LoadAccel( IDR_MAIN );
	edit_.Create( NULL, hwnd(), 0, 0, 100, 100 );
	edit_.getDoc().AddHandler( this );
	edit_.getCursor().AddHandler( this );
	stb_.SetParent(hwnd());
	stb_.SetStatusBarVisible( cfg_.showStatusBar() );

	search_.LoadFromINI();
	SetupSubMenu();
	SetupMRUMenu();
}

bool GreenStarWnd::StartUp( const Path& fn, int cs, int ln )
{
	Create( 0, 0, cfg_.GetWndX(), cfg_.GetWndY(), cfg_.GetWndW(), cfg_.GetWndH(), 0 );
	if( fn.len()==0 || !OpenByMyself( fn, cs ) )
	{
		ReloadConfig( fn.len()==0 );
		UpdateWindowName();
		stb_.SetText( TEXT("(1,1)") );
	}
	stb_.SetZoom( cfg_.GetZoom() );

	if( ln != -1 )
		JumpToLine( ln );

	ShowUp2();
	return true;
}

void GreenStarWnd::ShowUp2()
{
	Window::ShowUp( cfg_.GetWndM() ? SW_MAXIMIZE : SW_SHOW );
}



//-------------------------------------------------------------------------
// startup routine
//-------------------------------------------------------------------------

int kmain()
{
	// Load runtime language file from lang/ directory next to the exe.
	{
		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		wchar_t* lastSlash = wcsrchr(exePath, L'\\');
		if (lastSlash) *lastSlash = L'\0';

		wchar_t langDir[MAX_PATH];
		wsprintfW(langDir, L"%s\\lang", exePath);

		// 1. Check command-line: GreenStar.exe -lang ja-JP ...
		wchar_t langFromCmdLine[32] = L"";
		{
			int argc = 0;
			LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
			if (argv) {
				for (int i = 1; i < argc - 1; ++i) {
					if (lstrcmpiW(argv[i], L"-lang") == 0) {
						lstrcpynW(langFromCmdLine, argv[i + 1], 32);
						break;
					}
				}
				LocalFree(argv);
			}
		}

		// 2. Check ini file: Language=ja-JP
		wchar_t langFromIni[32] = L"";
		{
			wchar_t iniPath[MAX_PATH];
			DWORD len = GetModuleFileNameW(nullptr, iniPath, MAX_PATH);
			if (len > 3)
				lstrcpyW(iniPath + len - 3, L"ini");
			wchar_t username[256] = L"";
			DWORD unLen = 256;
			GetUserNameW(username, &unLen);
			if (username[0])
				GetPrivateProfileStringW(username, L"Language", L"", langFromIni, 32, iniPath);
			if (!langFromIni[0])
				GetPrivateProfileStringW(L"SharedConfig", L"Language", L"", langFromIni, 32, iniPath);
		}

		const wchar_t* locale =
			langFromCmdLine[0] ? langFromCmdLine :
			langFromIni[0]     ? langFromIni     :
			nullptr;

		LangManager::AutoLoad(langDir, locale);
	}

	GreenStarWnd wnd;
	{
		Argv  arg;
		ulong   i;
		int optL = -1;
		int optC = 0;

		for( i=1; i<arg.size() && arg[i][0]==TEXT('-'); ++i )
			switch( arg[i][1] )
			{
			case TEXT('c'):
				if( arg[i][2] ) optC = String::GetInt(arg[i]+2);
				else if( ++i < arg.size() ) optC = String::GetInt(arg[i]);
				break;
			case TEXT('l'):
				if( lstrcmpiW(arg[i], TEXT("-lang")) == 0 ) { ++i; break; } // -lang xx-XX (consumed by LangManager)
				if( arg[i][2] ) optL = String::GetInt(arg[i]+2);
				else if( ++i < arg.size() ) optL = String::GetInt(arg[i]);
				break;
			case TEXT('-'):
				goto option_done; // "--" stops option processing
			}
		option_done:

		Path file;
		if( i < arg.size() )
		{
			file = arg[i];
			if( !file.isFile() )
			{
				ulong j;
				for( j=i+1; j<arg.size(); ++j )
				{
					file += ' ';
					file += arg[j];
					if( file.isFile() )
						break;
				}

				if( j==arg.size() )
					file = arg[i];
				else
					i=j;
			}
		}

		if( ++i < arg.size() )
		{
			String cmd;
			for( ; i<arg.size(); ++i )
			{
				cmd += TEXT('\"');
				cmd += arg[i];
				cmd += TEXT("\" ");
			}
			::BootNewProcess( cmd.c_str() );
		}

		if( !wnd.StartUp(file, optC, optL) )
			return -1;

		TS.reset();
	}

	wnd.MsgLoop();
	return 0;
}
