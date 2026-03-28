#ifndef _KILIB_STDAFX_H_
#define _KILIB_STDAFX_H_

#undef   WINVER
#define  WINVER      0x0A00
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0A00

#define  OEMRESOURCE
#define  NOMINMAX
#ifdef SUPERTINY
  #define memset memset_default
#endif

#ifdef _MSC_VER
#define A_NOVTABLE __declspec(novtable)
#else
#define A_NOVTABLE
#endif

#ifdef __GNUC__
// Define some cool gcc attributes
#define A_HOT __attribute__((hot))
#define A_COLD __attribute__((cold))
#define A_PURE __attribute__((pure))
#define A_XPURE __attribute__((const))
#define A_FLATTEN __attribute__((flatten))
#define A_NONNULL __attribute__((nonnull))
#define A_NONNULL1(x) __attribute__((nonnull(x)))
#define A_NONNULL2(x, y) __attribute__((nonnull(x, y)))
#define A_NONNULL3(x, y, z) __attribute__((nonnull(x, y, z)))
#define A_WUNUSED  __attribute__((warn_unused))
#define ASSUME(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#define UNREACHABLE() __builtin_unreachable()
#define restrict __restrict
#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

#else //__GNUC__
#define ASSUME(x)
#define UNREACHABLE()
#define A_HOT
#define A_COLD
#define A_PURE
#define A_XPURE
#define A_FLATTEN
#define restrict
#define A_NONNULL
#define A_NONNULL1(x)
#define A_NONNULL2(x, y)
#define A_NONNULL3(x, y, z)
#define A_WUNUSED
#define likely(x)   (x)
#define unlikely(x) (x)

#endif

#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1900)
	// Define the cool new 'final' class attribute
	// Introduced in C++11
	#define A_FINAL final
#else
	#define A_FINAL
	#define override
	#define noexcept
#endif

#include <windows.h>
#include <shlobj.h>
#include <commdlg.h>
#include <commctrl.h>
#include <imm.h>
// If dimm.h is missing and an error occurs, define USEGLOBALIME in the project settings.
// If you remove it or install the latest Platform SDK, the build will pass.
#ifdef USEGLOBALIME
#include <dimm.h>
#endif

#ifdef _UNICODE
#endif

#ifndef NO_MLANG
#include <mlang.h>
#endif

#ifdef SUPERTINY
  #undef memset
#endif


#ifdef _MSC_VER
#pragma warning( disable: 4355 )
#endif

#define myDragQueryFile  DragQueryFileW
#define myDragQueryFileW DragQueryFileW

#endif // _KILIB_STDAFX_H_
