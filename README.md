# GreenPad

A lightweight text editor for Windows, aiming to be a practical Notepad replacement with minimal footprint.

Original author: [k.inaba](http://www.kmonos.net/lib/gp.en.html)
Extended by: roytam1, [RamonUnch](https://github.com/RamonUnch/GreenPad)
This build: modernized 64-bit fork with PCRE2 and chardet support.

## Features

- Unicode 14.0 support
- Proportional font rendering
- Syntax highlighting (customizable via `.kwd` files)
- Regular expression search powered by PCRE2 (via `pcre2-16.dll`)
- Charset auto-detection via `chardet.dll` (optional, based on libchardet)
- Wide encoding support: UTF-8/16/32, EUC-JP, Shift-JIS, GB18030, and many more
- Word wrap (character or word boundary)
- Smart indentation
- UAC elevation support
- UI languages: English, Japanese, Simplified Chinese, Traditional Chinese, Korean

## System Requirements

- Windows Vista or later (x64)

## Optional DLLs

Place these DLLs in the same directory as `GreenPad.exe` to enable additional features:

| DLL | Purpose |
|-----|---------|
| `chardet.dll` | Charset auto-detection (libchardet-based, MPL/GPL/LGPL) |
| `pcre2-16.dll` | PCRE2 regex engine; falls back to built-in NFA if absent |

## Building

### Visual C++ (x64)

```bat
nmake -f Makefiles/vcc.mak
```

### MinGW64 (ucrt64)

```bash
make gcc64
```

### Clang64

```bash
make clang64
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+R` | Reopen |
| `Ctrl+L` | Open elevated |
| `Shift+Ctrl+S` / `F12` | Save As |
| `Ctrl+Y` | Redo |
| `F5` | Reload file |
| `F6` | Insert date & time |
| `F7` | Open selection as file/URL |
| `F8` | Display selection length |
| `Ctrl+F` | Find |
| `F3` / `Shift+F3` | Find next / prev |
| `Ctrl+H` | Replace |
| `Ctrl+J` | Jump to line |
| `Ctrl+G` | External grep |
| `Ctrl+1/2/3` | Wrap: none / fixed width / window width |
| `Ctrl+I` | Insert Unicode code point |
| `Ctrl+B` | Go to matching brace |

## Configuration

### Layout files (`.lay`)

Located in `release/type/`. Edit manually to change fonts and colors:

```
ft=Font name
sz=Font size (points)
fw=Font weight (400=normal, 700=bold)
ff=Font flags (1:Italic 2:Underline 4:Strikeout)
fx=Font width in points (0=default)
ct=Text color (RGB)
ck=Keyword color
cb=Background color
cc=Control character color
cn=Comment color
cl=Line number color
tb=Tab width
sc=11000  (Show special chars: EOF/LF/Tab/Space/fullwidth-space)
wp=Wrap type (-1:none  0:window edge  1:fixed width)
ww=Wrap width (chars)
ws=Word wrap (1=word boundaries  0=character)
ln=Show line numbers
```

### Syntax highlighting (`.kwd`)

Place `.kwd` files in `release/type/`. Format:

```
1111       # flags: CaseSensitive EnSingleQuote EnDoubleQuote EnEscape
/*         # block comment start
*/         # block comment end
//         # line comment start
keyword1
keyword2
...
```

### Command line

```
greenpad [-l <line>] [-c <charset>] <file> ...
```

Common charset codes: `-65001` (UTF-8), `-5`/`-6` (UTF-16 BE/LE), `-932` (EUC-JP), `-933` (ISO-2022-JP)

### Shared configuration

To share one `GreenPad.ini` across user accounts, add to the ini file:

```ini
[SharedConfig]
Enable=1
```

## License

NYSL Version 0.9982 — essentially public domain. See [readme.en.txt](docs/readme.en.txt) for the full text.
