// LangManager.h
// Runtime language manager: loads .lng text files and provides translations
// at runtime. Falls back to RC-compiled (English) strings when no translation
// is available.
//
// Uses PIMPL to avoid exposing STL headers to callers compiled with /EHs-c-.
#pragma once
#include <windows.h>

class LangManager {
public:
    static LangManager& Get();

    // Load a .lng file. Returns false if file not found or cannot be parsed.
    bool Load(const wchar_t* path);

    // Clear all loaded translations (revert to built-in English).
    void Reset();

    // Returns translated string for IDS_* resource id,
    // or nullptr if no translation is available.
    const wchar_t* GetString(UINT id) const;

    // Apply translations to a menu (recursive walk of all items/submenus).
    // Call after loading the base English menu from RC.
    void ApplyToMenu(HMENU hMenu) const;

    // Apply translations to a dialog's controls.
    // Called automatically by DlgImpl::MainProc after on_init().
    void ApplyToDialog(HWND hDlg, UINT dialogId) const;

    // Display name of current language (e.g., L"Japanese").
    const wchar_t* GetLanguageName() const;

    // Detect locale string from Windows UI language (e.g., L"ja_JP").
    // Returns nullptr if no known locale matches (i.e., use English).
    static const wchar_t* DetectLocale();

    // Find and load appropriate .lng file from langDir directory.
    // locale: e.g. L"ja_JP". Pass nullptr to auto-detect.
    // Returns true if a translation file was loaded.
    static bool AutoLoad(const wchar_t* langDir, const wchar_t* locale = nullptr);

private:
    struct Impl;   // defined in LangManager.cpp; holds all STL containers
    Impl* pImpl_;

    LangManager();
    ~LangManager();
    LangManager(const LangManager&) = delete;
    LangManager& operator=(const LangManager&) = delete;

    void ApplyToMenuLevel(HMENU hMenu, const wchar_t* path) const;
};
