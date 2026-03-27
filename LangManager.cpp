// LangManager.cpp
#include "LangManager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
// Impl: all STL containers live here, hidden from the header
// ---------------------------------------------------------------------------
struct LangManager::Impl {
    std::unordered_map<UINT, std::wstring> strings_;
    std::unordered_map<UINT, std::wstring> menuCmds_;
    std::unordered_map<std::wstring, std::wstring> menuPopups_;
    std::unordered_map<UINT, std::unordered_map<std::wstring, std::wstring>> dialogs_;
    std::wstring langName_;
    std::wstring langCode_;
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
    pImpl_->langName_.clear();
    pImpl_->langCode_.clear();
}

// ---------------------------------------------------------------------------
// Parse escape sequences (\t -> TAB, \n -> newline, \\ -> \)
// ---------------------------------------------------------------------------
static std::wstring ParseValue(const std::wstring& raw) {
    std::wstring result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == L'\\' && i + 1 < raw.size()) {
            ++i;
            switch (raw[i]) {
            case L't':  result += L'\t'; break;
            case L'n':  result += L'\n'; break;
            case L'\\': result += L'\\'; break;
            default:    result += L'\\'; result += raw[i]; break;
            }
        } else {
            result += raw[i];
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Load .lng file
// ---------------------------------------------------------------------------
bool LangManager::Load(const wchar_t* path) {
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD fileSize = GetFileSize(hFile, nullptr);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        CloseHandle(hFile);
        return false;
    }

    std::string rawBytes(fileSize, '\0');
    DWORD bytesRead = 0;
    ReadFile(hFile, &rawBytes[0], fileSize, &bytesRead, nullptr);
    CloseHandle(hFile);

    int wlen = MultiByteToWideChar(CP_UTF8, 0, rawBytes.c_str(), (int)bytesRead, nullptr, 0);
    if (wlen <= 0)
        return false;
    std::wstring content(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, rawBytes.c_str(), (int)bytesRead, &content[0], wlen);

    Reset();

    enum class Section { None, Info, Strings, Menu, Dialog } section = Section::None;
    UINT currentDialogId = 0;

    size_t pos = 0;
    if (!content.empty() && content[0] == L'\xFEFF') // skip UTF-8 BOM
        pos = 1;

    while (pos <= content.size()) {
        size_t lineEnd = content.find(L'\n', pos);
        std::wstring line;
        if (lineEnd == std::wstring::npos) {
            line = content.substr(pos);
            pos = content.size() + 1;
        } else {
            line = content.substr(pos, lineEnd - pos);
            pos = lineEnd + 1;
        }
        if (!line.empty() && line.back() == L'\r')
            line.pop_back();

        size_t start = line.find_first_not_of(L" \t");
        if (start == std::wstring::npos) continue;
        line = line.substr(start);
        if (line[0] == L'#' || line[0] == L';') continue;

        if (line[0] == L'[') {
            size_t end = line.find(L']');
            if (end == std::wstring::npos) continue;
            std::wstring secName = line.substr(1, end - 1);
            currentDialogId = 0;
            if (secName == L"Info")         section = Section::Info;
            else if (secName == L"Strings") section = Section::Strings;
            else if (secName == L"Menu")    section = Section::Menu;
            else if (secName.size() > 7 && secName.substr(0, 7) == L"Dialog.") {
                section = Section::Dialog;
                currentDialogId = LookupNameT(secName.substr(7).c_str(), kDialogNames);
            } else {
                section = Section::None;
            }
            continue;
        }

        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;
        std::wstring key = line.substr(0, eq);
        std::wstring val = line.substr(eq + 1);

        size_t kend = key.find_last_not_of(L" \t");
        if (kend == std::wstring::npos) continue;
        key = key.substr(0, kend + 1);

        size_t vstart = val.find_first_not_of(L" \t");
        if (vstart != std::wstring::npos)
            val = val.substr(vstart);

        val = ParseValue(val);

        switch (section) {
        case Section::Info:
            if (key == L"Language")     pImpl_->langName_ = val;
            else if (key == L"LangCode") pImpl_->langCode_ = val;
            break;

        case Section::Strings: {
            UINT id = LookupNameT(key.c_str(), kStringNames);
            if (id != 0) pImpl_->strings_[id] = val;
            break;
        }

        case Section::Menu:
            if (key.size() > 6 && key.substr(0, 6) == L"popup.")
                pImpl_->menuPopups_[key.substr(6)] = val;
            else {
                UINT id = LookupNameT(key.c_str(), kCommandNames);
                if (id != 0) pImpl_->menuCmds_[id] = val;
            }
            break;

        case Section::Dialog:
            if (currentDialogId == 0) break;
            {
                auto& dlgMap = pImpl_->dialogs_[currentDialogId];
                if (key == L"Caption" ||
                    (key.size() > 7 && key.substr(0, 7) == L"static.")) {
                    dlgMap[key] = val;
                } else {
                    UINT ctrlId = LookupNameT(key.c_str(), kControlNames);
                    if (ctrlId != 0) {
                        wchar_t idStr[16];
                        wsprintfW(idStr, L"%u", ctrlId);
                        dlgMap[idStr] = val;
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// GetString
// ---------------------------------------------------------------------------
const wchar_t* LangManager::GetString(UINT id) const {
    auto it = pImpl_->strings_.find(id);
    if (it == pImpl_->strings_.end()) return nullptr;
    return it->second.c_str();
}

// ---------------------------------------------------------------------------
// GetLanguageName
// ---------------------------------------------------------------------------
const wchar_t* LangManager::GetLanguageName() const {
    return pImpl_->langName_.c_str();
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
        // Build position path: "" -> "0", "0" -> "0.3", etc.
        wchar_t idx[16];
        wsprintfW(idx, L"%d", i);
        std::wstring itemPath;
        if (path && path[0]) {
            itemPath = path;
            itemPath += L'.';
            itemPath += idx;
        } else {
            itemPath = idx;
        }

        MENUITEMINFOW mii = { sizeof(mii) };
        mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU;
        if (!GetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mii)) continue;

        if (mii.hSubMenu) {
            auto it = pImpl_->menuPopups_.find(itemPath);
            if (it != pImpl_->menuPopups_.end()) {
                MENUITEMINFOW mi2 = { sizeof(mi2) };
                mi2.fMask = MIIM_STRING;
                mi2.dwTypeData = const_cast<LPWSTR>(it->second.c_str());
                SetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mi2);
            }
            ApplyToMenuLevel(mii.hSubMenu, itemPath.c_str());
        } else if (!(mii.fType & MFT_SEPARATOR)) {
            auto it = pImpl_->menuCmds_.find(mii.wID);
            if (it != pImpl_->menuCmds_.end()) {
                MENUITEMINFOW mi2 = { sizeof(mi2) };
                mi2.fMask = MIIM_STRING;
                mi2.dwTypeData = const_cast<LPWSTR>(it->second.c_str());
                SetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mi2);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ApplyToDialog
// ---------------------------------------------------------------------------
void LangManager::ApplyToDialog(HWND hDlg, UINT dialogId) const {
    auto dit = pImpl_->dialogs_.find(dialogId);
    if (dit == pImpl_->dialogs_.end()) return;
    const auto& dlgMap = dit->second;

    auto cap = dlgMap.find(L"Caption");
    if (cap != dlgMap.end())
        SetWindowTextW(hDlg, cap->second.c_str());

    // Enumerate child controls in creation (template) order.
    // GetWindow(GW_CHILD) -> topmost = last created; GW_HWNDNEXT = lower z-order.
    // Collect then reverse to get template order.
    std::vector<HWND> children;
    for (HWND child = GetWindow(hDlg, GW_CHILD);
         child != nullptr;
         child = GetWindow(child, GW_HWNDNEXT))
    {
        children.push_back(child);
    }
    std::reverse(children.begin(), children.end());

    int staticIdx = 0;
    for (HWND child : children) {
        int id = GetDlgCtrlID(child);
        wchar_t key[32];
        if (id == -1 || id == 0) {
            wsprintfW(key, L"static.%d", staticIdx++);
        } else {
            wsprintfW(key, L"%u", (UINT)id);
        }
        auto it = dlgMap.find(key);
        if (it != dlgMap.end())
            SetWindowTextW(child, it->second.c_str());
    }
}

// ---------------------------------------------------------------------------
// DetectLocale - returns static string literals
// ---------------------------------------------------------------------------
const wchar_t* LangManager::DetectLocale() {
    LANGID langId = GetUserDefaultUILanguage();
    WORD lang = PRIMARYLANGID(langId);
    WORD sublang = SUBLANGID(langId);
    switch (lang) {
    case LANG_JAPANESE: return L"ja_JP";
    case LANG_KOREAN:   return L"ko_KR";
    case LANG_CHINESE:
        if (sublang == SUBLANG_CHINESE_TRADITIONAL ||
            sublang == SUBLANG_CHINESE_HONGKONG    ||
            sublang == SUBLANG_CHINESE_MACAU)
            return L"zh_TW";
        return L"zh_CN";
    default:
        return nullptr;
    }
}

// ---------------------------------------------------------------------------
// AutoLoad
// ---------------------------------------------------------------------------
bool LangManager::AutoLoad(const wchar_t* langDir, const wchar_t* locale) {
    const wchar_t* loc = (locale && locale[0]) ? locale : DetectLocale();
    if (!loc) return false; // English - no lng file needed

    // Try exact match: lang/ja_JP.lng
    std::wstring path = std::wstring(langDir) + L"\\" + loc + L".lng";
    if (Get().Load(path.c_str()))
        return true;

    // Try language-only fallback: lang/ja.lng
    const wchar_t* under = wcschr(loc, L'_');
    if (under) {
        std::wstring langOnly(loc, under);
        path = std::wstring(langDir) + L"\\" + langOnly + L".lng";
        if (Get().Load(path.c_str()))
            return true;
    }

    return false;
}
