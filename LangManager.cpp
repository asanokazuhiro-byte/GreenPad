// LangManager.cpp
// STL-free implementation; uses only new[]/delete[] and Win32/CRT wide-string
// functions so no libstdc++ / libc++ runtime DLL is needed.
#include "LangManager.h"

// ---------------------------------------------------------------------------
// Internal string helper
// ---------------------------------------------------------------------------
static wchar_t* wstrdup(const wchar_t* s, int len = -1)
{
    if (!s) return nullptr;
    if (len < 0) len = (int)wcslen(s);
    wchar_t* p = new wchar_t[len + 1];
    wcsncpy(p, s, (size_t)len);
    p[len] = L'\0';
    return p;
}

// ---------------------------------------------------------------------------
// UMap: UINT key -> wchar_t* value  (dynamic array, linear search)
// ---------------------------------------------------------------------------
struct UMap {
    struct E { UINT k; wchar_t* v; };
    E* d; int n, cap;

    void clear() {
        for (int i = 0; i < n; ++i) delete[] d[i].v;
        delete[] d; d = nullptr; n = cap = 0;
    }
    void set(UINT k, const wchar_t* v) {
        for (int i = 0; i < n; ++i) {
            if (d[i].k == k) { delete[] d[i].v; d[i].v = wstrdup(v); return; }
        }
        if (n == cap) {
            int c = cap ? cap * 2 : 8;
            E* nd = new E[c];
            for (int i = 0; i < n; ++i) nd[i] = d[i];
            delete[] d; d = nd; cap = c;
        }
        d[n].k = k; d[n].v = wstrdup(v); ++n;
    }
    const wchar_t* get(UINT k) const {
        for (int i = 0; i < n; ++i) if (d[i].k == k) return d[i].v;
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
// WMap: wchar_t* key -> wchar_t* value  (dynamic array, linear search)
// ---------------------------------------------------------------------------
struct WMap {
    struct E { wchar_t* k; wchar_t* v; };
    E* d; int n, cap;

    void clear() {
        for (int i = 0; i < n; ++i) { delete[] d[i].k; delete[] d[i].v; }
        delete[] d; d = nullptr; n = cap = 0;
    }
    void set(const wchar_t* k, const wchar_t* v) {
        for (int i = 0; i < n; ++i) {
            if (wcscmp(d[i].k, k) == 0) { delete[] d[i].v; d[i].v = wstrdup(v); return; }
        }
        if (n == cap) {
            int c = cap ? cap * 2 : 8;
            E* nd = new E[c];
            for (int i = 0; i < n; ++i) nd[i] = d[i];
            delete[] d; d = nd; cap = c;
        }
        d[n].k = wstrdup(k); d[n].v = wstrdup(v); ++n;
    }
    const wchar_t* get(const wchar_t* k) const {
        for (int i = 0; i < n; ++i) if (wcscmp(d[i].k, k) == 0) return d[i].v;
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
// DlgMap: UINT dialog-id -> WMap of control translations
// ---------------------------------------------------------------------------
struct DlgMap {
    struct E { UINT id; WMap m; };
    E* d; int n, cap;

    void clear() {
        for (int i = 0; i < n; ++i) d[i].m.clear();
        delete[] d; d = nullptr; n = cap = 0;
    }
    WMap* getOrCreate(UINT id) {
        for (int i = 0; i < n; ++i) if (d[i].id == id) return &d[i].m;
        if (n == cap) {
            int c = cap ? cap * 2 : 4;
            E* nd = new E[c];
            for (int i = 0; i < n; ++i) nd[i] = d[i];
            for (int i = n; i < c; ++i) { nd[i].id = 0; nd[i].m.d = nullptr; nd[i].m.n = nd[i].m.cap = 0; }
            delete[] d; d = nd; cap = c;
        }
        d[n].id = id; d[n].m.d = nullptr; d[n].m.n = d[n].m.cap = 0;
        return &d[n++].m;
    }
    const WMap* get(UINT id) const {
        for (int i = 0; i < n; ++i) if (d[i].id == id) return &d[i].m;
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
// Impl: all data
// ---------------------------------------------------------------------------
struct LangManager::Impl {
    UMap strings_;
    UMap menuCmds_;
    WMap menuPopups_;
    DlgMap dialogs_;
    wchar_t langName_[64];
    wchar_t langCode_[32];

    Impl() {
        strings_.d = menuCmds_.d = nullptr;
        strings_.n = strings_.cap = menuCmds_.n = menuCmds_.cap = 0;
        menuPopups_.d = nullptr; menuPopups_.n = menuPopups_.cap = 0;
        dialogs_.d = nullptr; dialogs_.n = dialogs_.cap = 0;
        langName_[0] = langCode_[0] = L'\0';
    }
    ~Impl() {
        strings_.clear(); menuCmds_.clear(); menuPopups_.clear(); dialogs_.clear();
    }
};

// ---------------------------------------------------------------------------
// Static lookup tables: symbolic names <-> numeric IDs
// ---------------------------------------------------------------------------
struct NameIdPair { const wchar_t* name; UINT id; };

static const NameIdPair kStringNames[] = {
    {L"IDS_ASKTOSAVE",        1},
    {L"IDS_APPNAME",          2},
    {L"IDS_SAVEERROR",        3},
    {L"IDS_ALLFILES",         4},
    {L"IDS_TXTFILES",         5},
    {L"IDS_OPENERROR",        6},
    {L"IDS_DEFAULT",          7},
    {L"IDS_NOTFOUND",         8},
    {L"IDS_REPLACEALLDONE",   9},
    {L"IDS_OKTODEL",         10},
    {L"IDS_LOADING",         11},
    {L"IDS_NOTFOUNDDOWN",    12},
    {L"IDS_NOWRITEACESS",    13},
    {L"IDS_ERRORNUM",        14},
    {L"IDS_CANTOPENDIR",     15},
    {L"IDS_INVALIDCP",       16},
    {L"IDS_MODIFIEDOUT",     17},
    {L"IDS_INSERTUNI",       18},
    {L"IDS_INSSERT",         19},
    {L"IDS_ZOOMPC",          20},
    {L"IDS_UNTITLED",        21},
    {L"IDS_CHARSET_CAPTION", 22},
    {L"IDS_CRLF_CAPTION",    23},
};

static const NameIdPair kCommandNames[] = {
    {L"ID_CMD_OPENELEVATED",  40001},
    {L"ID_CMD_REOPENFILE",    40002},
    {L"ID_CMD_NEWFILE",       40004},
    {L"ID_CMD_OPENFILE",      40005},
    {L"ID_CMD_SAVEFILE",      40006},
    {L"ID_CMD_SAVEFILEAS",    40007},
    {L"ID_CMD_EXIT",          40008},
    {L"ID_CMD_UNDO",          40009},
    {L"ID_CMD_REDO",          40010},
    {L"ID_CMD_CUT",           40011},
    {L"ID_CMD_COPY",          40012},
    {L"ID_CMD_PASTE",         40013},
    {L"ID_CMD_DELETE",        40014},
    {L"ID_CMD_SELECTALL",     40015},
    {L"ID_CMD_FIND",          40016},
    {L"ID_CMD_FINDNEXT",      40017},
    {L"ID_CMD_FINDPREV",      40018},
    {L"ID_CMD_NOWRAP",        40019},
    {L"ID_CMD_WRAPWIDTH",     40020},
    {L"ID_CMD_WRAPWINDOW",    40021},
    {L"ID_CMD_CONFIG",        40022},
    {L"ID_CMD_JUMP",          40023},
    {L"ID_CMD_DATETIME",      40024},
    {L"ID_MENUITEM40025",     40025},
    {L"ID_CMD_NEXTWINDOW",    40026},
    {L"ID_CMD_PREVWINDOW",    40027},
    {L"ID_CMD_GREP",          40028},
    {L"ID_MENUITEM40030",     40030},
    {L"ID_CMD_STATUSBAR",     40032},
    {L"ID_CMD_SAVEEXIT",      40033},
    {L"ID_CMD_DISCARDEXIT",   40034},
    {L"ID_CMD_PRINT",         40035},
    {L"ID_CMD_RECONV",        40036},
    {L"ID_CMD_TOGGLEIME",     40037},
    {L"ID_CMD_HELPABOUT",     40038},
    {L"ID_CMD_UPPERCASE",     50001},
    {L"ID_CMD_LOWERCASE",     50002},
    {L"ID_CMD_INVERTCASE",    50003},
    {L"ID_CMD_TTSPACES",      50004},
    {L"ID_CMD_REFRESHFILE",   50005},
    {L"ID_CMD_PAGESETUP",     50006},
    {L"ID_CMD_SFCHAR",        50008},
    {L"ID_CMD_QUOTE",         50009},
    {L"ID_CMD_UNQUOTE",       50010},
    {L"ID_CMD_SLCHAR",        50011},
    {L"ID_CMD_HELP",          50016},
    {L"ID_CMD_OPENSELECTION", 50017},
    {L"ID_CMD_ASCIIFY",       50019},
    {L"ID_CMD_INSERTUNI",     50020},
    {L"ID_CMD_ZOOMDLG",       50021},
    {L"ID_CMD_ZOOMRZ",        50022},
    {L"ID_CMD_ZOOMUP",        50023},
    {L"ID_CMD_ZOOMDN",        50024},
    {L"ID_CMD_UNINORMC",      50025},
    {L"ID_CMD_UNINORMD",      50026},
    {L"ID_CMD_UNINORMKC",     50027},
    {L"ID_CMD_UNINORMKD",     50028},
};

static const NameIdPair kDialogNames[] = {
    {L"IDD_REOPENDLG",   107},
    {L"IDD_FINDREPLACE", 108},
    {L"IDD_JUMP",        109},
    {L"IDD_CONFIG",      110},
    {L"IDD_ADDDOCTYPE",  111},
    {L"IDD_ABOUTDLG",    112},
    {L"IDD_EDITLAYOUT",  113},
};

static const NameIdPair kControlNames[] = {
    {L"IDOK",                  1},
    {L"IDCANCEL",              2},
    {L"ID_FINDNEXT",           3},
    {L"ID_REPLACENEXT",        4},
    {L"ID_REPLACEALL",         5},
    {L"IDC_IGNORECASE",     1006},
    {L"IDC_REGEXP",         1007},
    {L"IDC_LINLABEL",       1008},
    {L"ID_FINDPREV",        1008},
    {L"IDC_UNDOLIM1",       1011},
    {L"IDC_UNDOLIM2",       1012},
    {L"IDC_COUNTBYLETTER",  1027},
    {L"IDC_OPENSAME",       1026},
    {L"IDC_REMSIZE",        1030},
    {L"IDC_REMPLACE",       1031},
    {L"IDC_COUNTBYLETTER2", 1032},
    {L"IDC_NEWDOCTYPE",     1021},
    {L"IDC_DELDOCTYPE",     1023},
    {L"IDC_EDITKWD",        1024},
    {L"IDC_EDITLAY",        1025},
};

template<size_t N>
static UINT LookupNameT(const wchar_t* name, const NameIdPair (&table)[N]) {
    for (size_t i = 0; i < N; ++i)
        if (wcscmp(table[i].name, name) == 0)
            return table[i].id;
    return 0;
}

// ---------------------------------------------------------------------------
// Singleton / constructor / destructor
// ---------------------------------------------------------------------------
LangManager& LangManager::Get() {
    static LangManager instance;
    return instance;
}

LangManager::LangManager() : pImpl_(new Impl()) {}
LangManager::~LangManager() { delete pImpl_; }

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------
void LangManager::Reset() {
    pImpl_->strings_.clear();
    pImpl_->menuCmds_.clear();
    pImpl_->menuPopups_.clear();
    pImpl_->dialogs_.clear();
    pImpl_->langName_[0] = pImpl_->langCode_[0] = L'\0';
}

// ---------------------------------------------------------------------------
// Parse escape sequences (\t \n \\) into a newly allocated string
// ---------------------------------------------------------------------------
static wchar_t* ParseValue(const wchar_t* raw)
{
    size_t len = wcslen(raw);
    wchar_t* result = new wchar_t[len + 1];
    size_t j = 0;
    for (size_t i = 0; i < len; ++i) {
        if (raw[i] == L'\\' && i + 1 < len) {
            ++i;
            switch (raw[i]) {
            case L't':  result[j++] = L'\t'; break;
            case L'n':  result[j++] = L'\n'; break;
            case L'\\': result[j++] = L'\\'; break;
            default:    result[j++] = L'\\'; result[j++] = raw[i]; break;
            }
        } else {
            result[j++] = raw[i];
        }
    }
    result[j] = L'\0';
    return result;
}

// ---------------------------------------------------------------------------
// Load .lng file
// ---------------------------------------------------------------------------
bool LangManager::Load(const wchar_t* path)
{
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD fileSize = GetFileSize(hFile, nullptr);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) { CloseHandle(hFile); return false; }

    char* rawBytes = new char[fileSize];
    DWORD bytesRead = 0;
    ReadFile(hFile, rawBytes, fileSize, &bytesRead, nullptr);
    CloseHandle(hFile);

    int wlen = MultiByteToWideChar(CP_UTF8, 0, rawBytes, (int)bytesRead, nullptr, 0);
    if (wlen <= 0) { delete[] rawBytes; return false; }

    wchar_t* buf = new wchar_t[wlen + 1];
    MultiByteToWideChar(CP_UTF8, 0, rawBytes, (int)bytesRead, buf, wlen);
    delete[] rawBytes;
    buf[wlen] = L'\0';

    Reset();

    enum { NONE, INFO, STRINGS, MENU, DIALOG } section = NONE;
    UINT currentDialogId = 0;

    wchar_t* p = buf;
    if (p[0] == L'\xFEFF') p++; // skip BOM
    wchar_t* bufEnd = buf + wlen;

    while (p < bufEnd) {
        // Find line end and null-terminate
        wchar_t* lineStart = p;
        while (p < bufEnd && *p != L'\n') p++;
        wchar_t* lineEnd = p;
        if (p < bufEnd) p++; // advance past '\n'
        *lineEnd = L'\0';
        // Strip trailing \r
        if (lineEnd > lineStart && *(lineEnd - 1) == L'\r') *(lineEnd - 1) = L'\0';

        // Trim leading whitespace
        while (*lineStart == L' ' || *lineStart == L'\t') lineStart++;

        // Skip empty/comment lines
        if (!*lineStart || *lineStart == L'#' || *lineStart == L';') continue;

        // Section header
        if (*lineStart == L'[') {
            wchar_t* rb = wcschr(lineStart + 1, L']');
            if (!rb) continue;
            *rb = L'\0';
            const wchar_t* secName = lineStart + 1;
            currentDialogId = 0;
            if      (wcscmp(secName, L"Info")    == 0) section = INFO;
            else if (wcscmp(secName, L"Strings") == 0) section = STRINGS;
            else if (wcscmp(secName, L"Menu")    == 0) section = MENU;
            else if (wcsncmp(secName, L"Dialog.", 7) == 0) {
                section = DIALOG;
                currentDialogId = LookupNameT(secName + 7, kDialogNames);
            } else {
                section = NONE;
            }
            continue;
        }

        // Key = Value
        wchar_t* eq = wcschr(lineStart, L'=');
        if (!eq) continue;

        // Trim trailing whitespace from key
        wchar_t* keyEnd = eq;
        while (keyEnd > lineStart && (*(keyEnd-1) == L' ' || *(keyEnd-1) == L'\t')) keyEnd--;
        *keyEnd = L'\0';
        const wchar_t* key = lineStart;
        if (!*key) continue;

        // Trim leading whitespace from value
        wchar_t* val = eq + 1;
        while (*val == L' ' || *val == L'\t') val++;

        wchar_t* parsedVal = ParseValue(val);

        switch (section) {
        case INFO:
            if      (wcscmp(key, L"Language") == 0)
                wcsncpy(pImpl_->langName_, parsedVal, 63), pImpl_->langName_[63] = L'\0';
            else if (wcscmp(key, L"LangCode") == 0)
                wcsncpy(pImpl_->langCode_, parsedVal, 31), pImpl_->langCode_[31] = L'\0';
            break;

        case STRINGS: {
            UINT id = LookupNameT(key, kStringNames);
            if (id != 0) pImpl_->strings_.set(id, parsedVal);
            break;
        }

        case MENU:
            if (wcsncmp(key, L"popup.", 6) == 0)
                pImpl_->menuPopups_.set(key + 6, parsedVal);
            else {
                UINT id = LookupNameT(key, kCommandNames);
                if (id != 0) pImpl_->menuCmds_.set(id, parsedVal);
            }
            break;

        case DIALOG:
            if (currentDialogId == 0) break;
            if (wcscmp(key, L"Caption") == 0 || wcsncmp(key, L"static.", 7) == 0) {
                pImpl_->dialogs_.getOrCreate(currentDialogId)->set(key, parsedVal);
            } else {
                UINT ctrlId = LookupNameT(key, kControlNames);
                if (ctrlId != 0) {
                    wchar_t idStr[16];
                    wsprintfW(idStr, L"%u", ctrlId);
                    pImpl_->dialogs_.getOrCreate(currentDialogId)->set(idStr, parsedVal);
                }
            }
            break;

        default:
            break;
        }

        delete[] parsedVal;
    }

    delete[] buf;
    return true;
}

// ---------------------------------------------------------------------------
// GetString
// ---------------------------------------------------------------------------
const wchar_t* LangManager::GetString(UINT id) const {
    return pImpl_->strings_.get(id);
}

// ---------------------------------------------------------------------------
// GetLanguageName
// ---------------------------------------------------------------------------
const wchar_t* LangManager::GetLanguageName() const {
    return pImpl_->langName_;
}

// ---------------------------------------------------------------------------
// ApplyToMenu
// ---------------------------------------------------------------------------
void LangManager::ApplyToMenu(HMENU hMenu) const {
    ApplyToMenuLevel(hMenu, L"");
}

void LangManager::ApplyToMenuLevel(HMENU hMenu, const wchar_t* path) const {
    int count = GetMenuItemCount(hMenu);
    for (int i = 0; i < count; ++i) {
        wchar_t itemPath[64];
        if (path[0])
            wsprintfW(itemPath, L"%s.%d", path, i);
        else
            wsprintfW(itemPath, L"%d", i);

        MENUITEMINFOW mii = { sizeof(mii) };
        mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU;
        if (!GetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mii)) continue;

        if (mii.hSubMenu) {
            const wchar_t* text = pImpl_->menuPopups_.get(itemPath);
            if (text) {
                MENUITEMINFOW mi2 = { sizeof(mi2) };
                mi2.fMask = MIIM_STRING;
                mi2.dwTypeData = const_cast<LPWSTR>(text);
                SetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mi2);
            }
            ApplyToMenuLevel(mii.hSubMenu, itemPath);
        } else if (!(mii.fType & MFT_SEPARATOR)) {
            const wchar_t* text = pImpl_->menuCmds_.get(mii.wID);
            if (text) {
                MENUITEMINFOW mi2 = { sizeof(mi2) };
                mi2.fMask = MIIM_STRING;
                mi2.dwTypeData = const_cast<LPWSTR>(text);
                SetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mi2);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ApplyToDialog
// ---------------------------------------------------------------------------
void LangManager::ApplyToDialog(HWND hDlg, UINT dialogId) const {
    const WMap* dlgMap = pImpl_->dialogs_.get(dialogId);
    if (!dlgMap) return;

    const wchar_t* cap = dlgMap->get(L"Caption");
    if (cap) SetWindowTextW(hDlg, cap);

    // Collect children in dialog-template order.
    // For dialog boxes, GetWindow(hDlg, GW_CHILD)/GW_HWNDNEXT enumerates
    // children in template order (first control first), so no reversal needed.
    HWND children[256];
    int childCount = 0;
    for (HWND child = GetWindow(hDlg, GW_CHILD);
         child && childCount < 256;
         child = GetWindow(child, GW_HWNDNEXT))
    {
        children[childCount++] = child;
    }

    int staticIdx = 0;
    for (int i = 0; i < childCount; ++i) {
        int id = GetDlgCtrlID(children[i]);
        wchar_t key[32];
        if (id == 0 || (WORD)id == 0xFFFF) // IDC_STATIC from dialog template
            wsprintfW(key, L"static.%d", staticIdx++);
        else
            wsprintfW(key, L"%u", (UINT)id);

        const wchar_t* text = dlgMap->get(key);
        if (text) SetWindowTextW(children[i], text);
    }
}

// ---------------------------------------------------------------------------
// DetectLocale - returns BCP 47 locale name based on the UI display language
//   Uses GetUserDefaultUILanguage() (display language) so that users who have
//   a non-English regional format but an English UI see English, not the
//   regional-format language.
// ---------------------------------------------------------------------------
const wchar_t* LangManager::DetectLocale() {
    static wchar_t locBuf[LOCALE_NAME_MAX_LENGTH] = {};
    if (locBuf[0] == L'\0') {
        // Prefer UI language (display language) over regional-format locale
        LANGID uiLang = GetUserDefaultUILanguage();
        LCID   lcid   = MAKELCID(uiLang, SORT_DEFAULT);
        if (LCIDToLocaleName(lcid, locBuf, LOCALE_NAME_MAX_LENGTH, 0) == 0) {
            // Fallback: regional-format locale
            if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME,
                                locBuf, LOCALE_NAME_MAX_LENGTH) == 0)
                return nullptr;
        }
    }
    return locBuf[0] ? locBuf : nullptr;
}

// ---------------------------------------------------------------------------
// AutoLoad
// ---------------------------------------------------------------------------
bool LangManager::AutoLoad(const wchar_t* langDir, const wchar_t* locale)
{
    const wchar_t* loc = (locale && locale[0]) ? locale : DetectLocale();
    if (!loc || !loc[0]) return false;

    wchar_t path[MAX_PATH];

    // Try exact match: lang\ja-JP.lng
    wsprintfW(path, L"%s\\%s.lng", langDir, loc);
    if (Get().Load(path)) return true;

    // For 3-part BCP 47 (e.g. zh-Hans-CN), strip script subtag -> zh-CN.lng
    const wchar_t* first = wcschr(loc, L'-');
    if (first) {
        const wchar_t* second = wcschr(first + 1, L'-');
        if (second) {
            wchar_t reduced[LOCALE_NAME_MAX_LENGTH];
            int langLen = (int)(first - loc);
            wcsncpy(reduced, loc, (size_t)langLen);
            wcscpy(reduced + langLen, second); // e.g. "-CN"
            wsprintfW(path, L"%s\\%s.lng", langDir, reduced);
            if (Get().Load(path)) return true;
        }
        // Language-only fallback: lang\ja.lng
        wchar_t langOnly[LOCALE_NAME_MAX_LENGTH];
        int len = (int)(first - loc);
        wcsncpy(langOnly, loc, (size_t)len);
        langOnly[len] = L'\0';
        wsprintfW(path, L"%s\\%s.lng", langDir, langOnly);
        if (Get().Load(path)) return true;
    }

    return false;
}
