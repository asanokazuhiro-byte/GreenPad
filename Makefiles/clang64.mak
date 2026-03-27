
NAME       = clang64
OBJ_SUFFIX = o

###############################################################################
TARGET = release/GreenPad.exe
INTDIR = obj

all: PRE $(TARGET)

OBJS = \
 $(INTDIR)/thread.$(OBJ_SUFFIX)       \
 $(INTDIR)/log.$(OBJ_SUFFIX)          \
 $(INTDIR)/winutil.$(OBJ_SUFFIX)      \
 $(INTDIR)/textfile.$(OBJ_SUFFIX)     \
 $(INTDIR)/path.$(OBJ_SUFFIX)         \
 $(INTDIR)/cmdarg.$(OBJ_SUFFIX)       \
 $(INTDIR)/file.$(OBJ_SUFFIX)         \
 $(INTDIR)/find.$(OBJ_SUFFIX)         \
 $(INTDIR)/ctrl.$(OBJ_SUFFIX)         \
 $(INTDIR)/registry.$(OBJ_SUFFIX)     \
 $(INTDIR)/window.$(OBJ_SUFFIX)       \
 $(INTDIR)/string.$(OBJ_SUFFIX)       \
 $(INTDIR)/memory.$(OBJ_SUFFIX)       \
 $(INTDIR)/ip_cursor.$(OBJ_SUFFIX)    \
 $(INTDIR)/ip_scroll.$(OBJ_SUFFIX)    \
 $(INTDIR)/ip_wrap.$(OBJ_SUFFIX)      \
 $(INTDIR)/ip_draw.$(OBJ_SUFFIX)      \
 $(INTDIR)/ip_ctrl1.$(OBJ_SUFFIX)     \
 $(INTDIR)/ip_text.$(OBJ_SUFFIX)      \
 $(INTDIR)/ip_parse.$(OBJ_SUFFIX)     \
 $(INTDIR)/GpMain.$(OBJ_SUFFIX)       \
 $(INTDIR)/OpenSaveDlg.$(OBJ_SUFFIX)  \
 $(INTDIR)/Search.$(OBJ_SUFFIX)       \
 $(INTDIR)/RSearch.$(OBJ_SUFFIX)      \
 $(INTDIR)/ConfigManager.$(OBJ_SUFFIX) \
 $(INTDIR)/PcreSearch.$(OBJ_SUFFIX)   \
 $(INTDIR)/LangManager.$(OBJ_SUFFIX)  \
 $(INTDIR)/app.$(OBJ_SUFFIX)

LIBS = \
-lkernel32 \
-nostdlib \
-lmsvcrt \
-lc++ \
-Wl,-eentryp \
-flto \
-fuse-linker-plugin \
 -L. \
 -luser32   \
 -lgdi32    \
 -lshell32  \
 -ladvapi32 \
 -lcomdlg32 \
 -lcomctl32 \
 -lole32    \
 -luuid     \
 -limm32    \
 -Wl,-dynamicbase,-nxcompat,--no-seh,--enable-auto-import \
 -Wl,--disable-reloc-section,--disable-runtime-pseudo-reloc \
 -Wl,--gc-sections,--tsaware,--large-address-aware,-s -s

WARNINGS = \
 -Wall \
 -Wextra \
 -Wno-pragma-pack \
 -Wno-expansion-to-defined \
 -Wno-macro-redefined \
 -Wno-unused-parameter \
 -Wno-unused-value \
 -Wno-missing-field-initializers \
 -Wno-implicit-fallthrough \
 -Wno-register \
 -Wno-parentheses \
 -Wno-ignored-attributes \
 -Wuninitialized \
 -Wtype-limits \
 -Winit-self \
 -Wnull-dereference \
 -Wnonnull \
 -Wno-unknown-pragmas

DEFINES = \
 -D_UNICODE -DUNICODE \
 -UDEBUG -U_DEBUG \
 -DUSEGLOBALIME \
 -DUSE_ORIGINAL_MEMMAN


PRE:
	-@if [ ! -d release   ]; then   mkdir release; fi;
	-@if [ ! -d obj       ]; then   mkdir obj; fi;
	-@if [ ! -d $(INTDIR) ]; then   mkdir $(INTDIR); fi;
###############################################################################

PCRE2_INC = -Ithird_party/pcre2/include

RES = $(INTDIR)/gp_rsrc.o

VPATH    = editwing:kilib
CXXFLAGS = \
 -nostdlib -m64 -c -Os -fdata-sections -ffunction-sections \
 -flto \
 -mtune=generic \
 -march=x86-64 \
 -mno-stack-arg-probe \
 -momit-leaf-frame-pointer \
 -fomit-frame-pointer \
 -fno-stack-check \
 -fno-stack-protector \
 -fno-access-control \
 -fno-use-cxa-atexit \
 -fno-exceptions \
 -fno-dwarf2-cfi-asm \
 -fno-asynchronous-unwind-tables \
 -fno-rtti \
 -fno-ident \
 $(WARNINGS) $(DEFINES) $(MINGWI) \
 -idirafter kilib \
 $(PCRE2_INC)

LOPT = -m64 -mwindows

$(TARGET) : $(OBJS) $(RES)
	clang $(LOPT) -o$(TARGET) $(OBJS) $(RES) $(LIBS)
#	strip -s $(TARGET)
$(INTDIR)/%.o: rsrc/%.rc
	windres --codepage 65001 -Fpe-x86-64 -l 0x411 -I rsrc $< -O coff -o$@
$(INTDIR)/%.o: %.cpp
	@echo CC $@ $<
	@clang $(CXXFLAGS) -o$@ $<

clean : 
	-rm $(RES)
	-rm $(OBJS)
	-rm $(TARGET)
	-rmdir $(INTDIR)
