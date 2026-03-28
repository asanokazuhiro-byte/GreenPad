#ifndef _KILIB_TEXTFILE_H_
#define _KILIB_TEXTFILE_H_
#include "types.h"
#include "memory.h"
#include "file.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	Available code sets
//
//	However, some of the things listed here do not work properly on Windows.
//	Only those with language support installed are actually compatible.
//	A code with a value less than -100 is the language of the code page immediately above it.
//	It relies on support to perform the conversion.
//@}
//=========================================================================

enum charset {
	AutoDetect = 0,    // Automatic judgment
	                   // SJIS/EucJP/IsoJP/IsoKR/IsoCN
					   // UTF5/UTF8/UTF8N/UTF16b/UTF16l/UTF32b/UTF32l
					   // Determine. I don't know about the others. (^^;

	ASCIICP    = 20127,// Normal plain ASCII.

	WesternDOS = 850,  // Europe and America (CP850 != ISO-8859-1)
	Western    = 1252, // Europe and America (Windows1252 >> ISO-8859-1)
//	WesternOS2 = 1004  // CP1004 OS/2 encoding ~ Windows1252
	WesternMAC = 10000,// Europe and America (x-mac-roman != ISO-8859-1)
	WesternISO = 28605,// Europe and America (ISO-8859-15)
	TurkishDOS = 857,  // Turkish (CP857 != ISO-8859-9)
	Turkish    = 1254, // Turkish (Windows1254 >> ISO-8859-9)
	TurkishMAC = 10081,// Turkish (x-mac-turkish != ISO-8859-9)
	TurkishISO = 28599,// Turkish (ISO-8859-9)
	EsperantoISO = 28593,// Esperanto (ISO-8859-3)
	HebrewDOS  = 862,  // Hebrew (CP862 !=> ISO-8859-8)
	Hebrew     = 1255, // Hebrew (Windows1255 >> ISO-8859-8)
	HebrewMAC  = 10005,// Hebrew (x-mac-hebrew !=> ISO-8859-8)
	ArabicIBM  = 720,  // Arabic (CP720 != ISO-8859-6)
	ArabicMSDOS= 864,  // Arabic (CP864 != ISO-8859-6)
	Arabic     = 1256, // Arabic (Windows1256 ~ ISO-8859-6)
	ArabicMAC  = 10004,// Arabic (x-mac-arabic != ISO-8859-6)
	ArabicISO  = 28596,// Arabic (ISO-8859-6)
	BalticIBM  = 775,  // Baltic (CP775 != ISO-8859-13)
	Baltic     = 1257, // Baltic (Windows1257 >> ISO-8859-13)
	Vietnamese = 1258, // Vietnamese (Windows1258 != VISCII)
	CentralDOS = 852,  // Central Europe (CP852 != ISO-8859-2)
	Central    = 1250, // Central Europe (Windows1250 ~ ISO-8859-2)
	CentralISO = 28592,// Central Europe (ISO-8859-2)
	LatinMAC   = 10029,// Central Europe (x-mac-ce != ISO-8859-2)
	RomaniaMAC = 10010,// Romanian (x-mac-romania != ISO-8859-2)
	CroatiaMAC = 10082,// Croatian (x-mac-croatia != ISO-8859-2)
	GreekIBM   = 737,  // Greek (CP737 = ISO-8859-7 ?)
	GreekMSDOS = 869,  // Greek (CP869 != ISO-8859-7)
	Greek      = 1253, // Greek (Windows1253 ~ ISO-8859-7)
	GreekMAC   = 10006,// Greek (x-mac-greek != ISO-8859-7)
	GreekISO   = 28597,// Greek (ISO-8859-7)
	Thai       = 874,  // Thai
	ThaiISO    = 28601,  // Thai (ISO-8859-11)
	Portuguese = 860,  // Portuguese (CP860)
	Icelandic  = 861,  // Icelandic (CP861)
	IcelandicMAC= 10079,  // Icelandic (x-mac-icelandic)
	CanadianFrench= 863, // French (Canada) (CP863)
	Nordic     = 865, // MS-DOS Nordic (CP865)

	CyrillicIBM= 855,  // Cyrillic (IBM) (CP855 = ISO-8859-5 ?)
	CyrillicDOS= 866,  // Cyrillic (MS-DOS) (CP866 != ISO-8859-5)
	Cyrillic   = 1251, // Cyrillic (Windows1251 != ISO-8859-5)
	CyrillicMAC= 10007, // Cyrillic (x-mac-cyrillic != ISO-8859-5)
	CyrillicISO= 28595, // Cyrillic (ISO-8859-5)
	Koi8R      = 20866,// Cyrillic (KOI8-R)
	Koi8U      = 21866,// Cyrillic (KOI8-U Ukrainian)
	MacUA      = 10017,// Cyrillic (x-mac-ukraine Ukrainian)

	UHC        = 949,  // Korean 1 (Unified Hangle Code >> EUC-KR)
	IsoKR      = -950, // Korean 2 (ISO-2022-KR)
	Johab      = 1361, // Korean 3 (Johab)
	MacKR      = 10003,// Korean 4 (x-mac-korean)

	GBK        = 936,  // Chinese 1 (Simplified GBK >> EUC-CN)
	IsoCN      = -936, // Chinese 2 (Simplified ISO-2022-CN)
	HZ         = -937, // Chinese 3 (Simplified Chinese HZ-GB2312)
	Big5       = 950,  // Chinese 4 (Traditional Big5)
	CNS        = 20000,// Chinese 5 (Traditional EUC-TW/CNS)
	TCA        = 20001,// Chinese 6 (Traditional TCA)
	ETen       = 20002,// Chinese 7 (Traditional ETen)
	IBM5550    = 20003,// Chinese 8 (Traditional IBM5550)
	Teletext   = 20004,// Chinese 9 (Traditional Teletext)
	Wang       = 20005,// Chinese 10 (Traditional Wang)
	GB18030    = 54936,// Chinese 11 (Simplified Chinese GB18030 >> GBK >> EUC-CN) No BOM
	GB18030Y   =-54936,// Chinese 11 (Simplified Chinese GB18030 >> GBK >> EUC-CN) With BOM
	MacTW      = 10002,// Chinese 12 (Traditional x-mac-taiwan ~ Big5)
	MacCN      = 10008,// Chinese 13 (Simplified x-mac-prc ~ GB2312)

	SJIS       = 932,  // Japanese 1 (Shift_JIS)
	EucJP      = -932, // Japanese 2 (Japanese EUC)
	IsoJP      = -933, // Japanese 3 (ISO-2022-JP)
	MacJP      = 10001,// Japanese 4 (x-mac-japanese)

	UTF1       = -1,   // Unicode (UTF-1): No BOM
	UTF5       = -2,   // Unicode (UTF-5): No BOM
	UTF5Y      = -14,   // Unicode (UTF-5): with BOM
	UTF7       = 65000,// Unicode (UTF-7): No BOM
	UTF8       =-65001,// Unicode (UTF-8): with BOM
	UTF8N      = 65001,// Unicode (UTF-8N): No BOM
	UTF16b     = -3,   // Unicode (UTF-16): BE with BOM
	UTF16l     = -4,   // Unicode (UTF-16): with BOM LE
	UTF16BE    = -5,   // Unicode (UTF-16BE): No BOM
	UTF16LE    = -6,   // Unicode (UTF-16LE): No BOM
	UTF32b     = -7,   // Unicode (UTF-32): BE with BOM
	UTF32l     = -8,   // Unicode (UTF-32): with BOM LE
	UTF32BE    = -9,   // Unicode (UTF-32BE): No BOM
	UTF32LE    = -10,  // Unicode (UTF-32LE): No BOM
	UTF9       = -11,  // Unicode (UTF-9): No BOM
	OFSSUTF    = -12,  // Unicode (Old FSS-UTF): No BOM
	UTF1Y      =-64999,// Unicode (UTF-1): with BOM
	UTF9Y      =-65002,// Unicode (UTF-9): with BOM
	OFSSUTFY   = -13,  // Unicode (Old FSS-UTF): with BOM
	UTFEBCDIC  = -15,  // Unicode (UTF-EBCDIC): No BOM
	UTFEBCDICY = -16,  // Unicode (UTF-EBCDIC): with BOM

	DOSUS      = 437,  // DOSLatinUS (CP437)

	SCSUY      = -60000,// Standard Compression Scheme for Unicode with BOM
	BOCU1Y     = -60001,// Binary Ordered Compression for Unicode-1 with BOM
	SCSU       = -60002,// Standard Compression Scheme for Unicode without BOM
	BOCU1      = -60003 // Binary Ordered Compression for Unicode-1 No BOM
};

//=========================================================================
//@{
//	new line code
//@}
//=========================================================================

enum lbcode {
	CR   = 0,
	LF   = 1,
	CRLF = 2,
	NL   = 3,
	CRNL = 4
};

struct TextFileRPimpl;
struct TextFileWPimpl;



//=========================================================================
//@{
//	Read text file
//
//	Interpret the file with the specified character code and convert it into a Unicode string.
//	Returns each line. Automatic determination of character codes and line feed codes is also possible.
//@}
//=========================================================================

class TextFileR
{
public:

	//@{ constructor (code specification) //@}
	explicit TextFileR( int charset=AutoDetect );

	//@{ destructor //@}
	~TextFileR();

	//@{ open //@}
	bool Open( const TCHAR* fname, bool always=false );

	//@{ close //@}
	void Close();

	//@{
	//	Read (return read length)
	//
	//	Please specify a buffer with a size of at least 20.
	//@}
	size_t ReadBuf( unicode* buf, size_t siz );

public:

	//@{ Code page of the file being read //@}
	inline int codepage() const { return cs_; }

	//@{ Line feed code (0:CR, 1:LF, 2:CRLF) //@}
	inline int linebreak() const { return lb_; }

	//@{ file size //@}
	inline ulong size() const { return fp_.size(); }

	//@{ Flag for which no line breaks were found //@}
	inline bool nolb_found() const { return nolbFound_; }

	void setLBfromFreq(uint freq[256], uchar cr, uchar lf);

	static int neededCodepage(int cs) A_XPURE;
	static bool isEBCDIC( int cs ) A_XPURE;
	static bool IsChardetAvailable();
	static bool GetChardetVersionStr( wchar_t* buf, int bufSize );

	const uchar* rawData() const
		{ return fp_.base(); }

private:

	TextFileRPimpl    *impl_;
	FileR                fp_;
	int                  cs_;
	int                  lb_;
	bool          nolbFound_;

private:

	int AutoDetection( int cs, const uchar* ptr, size_t siz );
	int MLangAutoDetection( const uchar* ptr, size_t siz );
	int chardetAutoDetection( const uchar* ptr, size_t siz );

	bool IsNonUnicodeRange(qbyte u) const A_XPURE;
	bool IsAscii(uchar c) const A_XPURE;
	bool IsSurrogateLead(qbyte w) const A_XPURE;
	bool CheckUTFConfidence(const uchar* ptr, size_t siz, unsigned char uChrSize, bool LE) const A_XPURE;
	bool IsValidUTF8( const uchar* ptr, size_t siz ) const A_XPURE;

private:

	NOCOPY(TextFileR);
};


//=========================================================================
//@{
//	Text file writing
//
//	Receives a Unicode string and outputs it while converting it to the specified character code.
//@}
//=========================================================================

class TextFileW
{
public:

	//@{ constructor (character, line feed code specification) //@}
	TextFileW( int charset, int linebreak );
	~TextFileW();

	//@{ open //@}
	bool Open( const TCHAR* fname );

	//@{ close //@}
	void Close();

	//@{ Write one line //@}
	void WriteLine( const unicode* buf, size_t siz, bool lastline );

private:

	TextFileWPimpl    *impl_;
	FileW                fp_;
	const int            cs_;
	const int            lb_;

private:

	NOCOPY(TextFileW);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_TEXTFILE_H_
