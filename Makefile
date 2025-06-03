#
# This is GNU make file DON'T use it with nmake/wmake of whatever even if you
# plan to use MS compiler!
#

################################################################################
# Copyright (c) 2022-2024 Jaroslav Hensl                                       #
#                                                                              #
# See LICENCE file for law informations                                        #
# See README.md file for more build instructions                               #
#                                                                              #
################################################################################

#
# Usage:
#   1) copy config.mk-sample to config.mk
#   2) edit config.mk
#   3) run make
#
include config.mk

DEPS=Makefile config.mk

# some usually free base address to easier debug
BASE_ddraw_xp.dll  = 0x73720000
BASE_ddraw_98.dll  = 0xBAAA0000
BASE_ddraw_95.dll  = 0xBAAA0000

BASE_d3d8_xp.dll  = 0x6DF20000
BASE_d3d8_98.dll  = 0x00400000
BASE_d3d8_95.dll  = 0x00400000

BASE_d3d9_xp.dll  = 0x4FD70000
BASE_d3d9_98.dll  = 0x00400000

BASE_wined3d.dll  = 0x01A00000
#BASE_winedd.dll   = 0xBA9A0000
BASE_winedd.dll   = 0x01400000
BASE_wined8.dll   = 0x01400000
BASE_wined9.dll   = 0x01400000

NULLOUT=$(if $(filter $(OS),Windows_NT),NUL,/dev/null)

GIT      ?= git
GIT_IS   := $(shell $(GIT) rev-parse --is-inside-work-tree 2> $(NULLOUT))
ifeq ($(GIT_IS),true)
  VERSION_BUILD := $(shell $(GIT) rev-list --count main)
endif

ifdef MSC
  DLLFLAGS = /link /DLL /MACHINE:X86 /IMPLIB:$(@:dll=lib) /OUT:$@ /PDB:$(@:dll=pdb) /DEF:$(DEF_$@)
  DLLFLAGS_NOCRT = /link /DLL /MACHINE:X86 /IMPLIB:$(@:dll=lib) /OUT:$@ /PDB:$(@:dll=pdb) /DEF:$(DEF_$@)

  CFLAGS = /nologo $(MSC_RUNTIME) /Imsc /Iinclude /Iinclude/wine /Icompact /D__i386__ /D_X86_ /D_WIN32 /DWIN32 /D__WINESRC__ \
    /DWINE_NOWINSOCK /DUSE_WIN32_OPENGL \
    /DWINE_UNICODE_API="" /DDLLDIR=\"\" /DBINDIR=\"\" /DLIB_TO_BINDIR=\"\" \
    /DLIB_TO_DLLDIR=\"\" /DBIN_TO_DLLDIR=\"\" /DLIB_TO_DATADIR=\"\" /DBIN_TO_DATADIR=\"\" \
    /DWINVER=0x0500

  ifdef SPEED
    CFLAGS += /O2
  else
    CFLAGS += /Z7 /Od
  endif

  WINED3D_LIBS = kernel32.lib user32.lib gdi32.lib advapi32.lib opengl32.lib
  SWITCHER_LIBS = advapi32.lib kernel32.lib user32.lib
  
  ifdef DEBUG
    CFLAGS += /DDEBUG
  else
    CFLAGS += /DNDEBUG
  endif
  
  OBJ := .obj

  LIBSUFFIX := .lib
  LIBPREFIX := 
  
  ifdef WINED3D_SILENT
    CFLAGS += /DWINE_SILENT /DWINE_NO_TRACE_MSGS /DWINE_NO_DEBUG_MSGS
  endif
  
  # Enable imported patches from VirtualBox
  CFLAGS += \
    /DVBOX_WITH_WINE_FIX_IBMTMR \
    /DVBOX_WITH_WINE_FIX_QUIRKS \
    /DVBOX_WITH_WINE_FIX_PBOPSM \
    /DVBOX_WITH_WINE_FIX_INITCLEAR \
    /DVBOX_WITH_WINE_FIX_BUFOFFSET \
    /DVBOX_WITH_WINE_FIX_STRINFOBUF \
    /DVBOX_WITH_WINE_FIX_CURVBO \
    /DVBOX_WITH_WINE_FIX_FTOA \
    /DVBOX_WITH_WINE_FIX_SURFUPDATA \
    /DVBOX_WITH_WINE_FIX_TEXCLEAR \
    /DVBOX_WITH_WINE_FIX_SHADERCLEANUP \
    /DVBOX_WITH_WINE_FIX_SHADER_DECL \
    /DVBOX_WITH_WINE_FIXES \
    /DVBOX_WITH_WINE_FIX_POLYOFFSET_SCALE \
    /DVBOX_WITH_WINE_FIX_ZEROVERTATTR \
    /DVBOX_WITH_WINE_FIX_MUTE_ERRORS
  
  %.c.obj: %.c $(DEPS)
		$(CC) $(CFLAGS) /Fo"$@" /c $<
  
  %.asm:
	
  %.asm.obj: %.asm $(DEPS)
		$(ASM) $< -f win32 -o $@
  
  %.res: %.rc $(DEPS)
		$(WINDRES) /nologo /fo $@ $<
  
  LIBSTATIC = LIB.EXE /OUT:$@
  
  ifdef WINED3D_STATIC
    DX_LIBS = kernel32.lib user32.lib gdi32.lib advapi32.lib opengl32.lib wined3d_static.lib
    CFLAGS += /DWINED3D_STATIC
    WINED3D_LIB_NAME = $(LIBPREFIX)wined3d_static$(LIBSUFFIX)
  else
    DX_LIBS = kernel32.lib user32.lib gdi32.lib advapi32.lib wined3d.lib
    WINED3D_LIB_NAME = wined3d.dll
  endif
  
else
  DLLFLAGS = -o $@ -shared -Wl,--out-implib,lib$(@:dll=a),--enable-stdcall-fixup,--image-base,$(BASE_$@)$(TUNE_LD) $(DEF_$@)
  DLLFLAGS_NOCRT = -o $@ -shared -nostdlib -nodefaultlibs -lgcc -Wl,--out-implib,lib$(@:dll=a),--enable-stdcall-fixup,--image-base,$(BASE_$@)$(TUNE_LD) $(DEF_$@)

  ifdef SPEED
    CFLAGS = -std=c99 -O3 -fomit-frame-pointer -Wno-discarded-qualifiers -Werror=implicit-function-declaration
  else
    CFLAGS = -std=c99 -g -O0
  endif
  
  ifdef DEBUG
    CFLAGS += -DDEBUG
  else
    CFLAGS += -DNDEBUG
  endif
  
  ifdef LTO
    CFLAGS += -flto=auto -fno-fat-lto-objects -pipe -Werror=implicit-function-declaration
  endif

  CFLAGS += -Wno-write-strings -Wno-cast-qual -Imingw -Iinclude -Iinclude/wine -Icompact -Ipthread9x/include -D_WIN32 -DWIN32 -D__WINESRC__ \
    -DUSE_WIN32_OPENGL -DWINE_NOWINSOCK  -DHAVE_CRTEX \
    -DWINE_UNICODE_API="" -DDLLDIR=\"\" -DBINDIR=\"\" -DLIB_TO_BINDIR=\"\" \
    -DLIB_TO_DLLDIR=\"\" -DBIN_TO_DLLDIR=\"\" -DLIB_TO_DATADIR=\"\" -DBIN_TO_DATADIR=\"\" -DWINVER=0x0400 \
    $(TUNE)
  
  ifdef WINED3D_SILENT
    CFLAGS += -DWINE_SILENT -DWINE_NO_TRACE_MSGS -DWINE_NO_DEBUG_MSGS
  endif
  
  # Enable imported patches from VirtualBox
  CFLAGS += \
    -DVBOX_WITH_WINE_FIX_IBMTMR \
    -DVBOX_WITH_WINE_FIX_QUIRKS \
    -DVBOX_WITH_WINE_FIX_PBOPSM \
    -DVBOX_WITH_WINE_FIX_INITCLEAR \
    -DVBOX_WITH_WINE_FIX_BUFOFFSET \
    -DVBOX_WITH_WINE_FIX_STRINFOBUF \
    -DVBOX_WITH_WINE_FIX_CURVBO \
    -DVBOX_WITH_WINE_FIX_FTOA \
    -DVBOX_WITH_WINE_FIX_TEXCLEAR \
    -DVBOX_WITH_WINE_FIX_SHADERCLEANUP \
    -DVBOX_WITH_WINE_FIX_SHADER_DECL \
    -DVBOX_WITH_WINE_FIXES \
    -DVBOX_WITH_WINE_FIX_POLYOFFSET_SCALE \
    -DVBOX_WITH_WINE_FIX_ZEROVERTATTR \
    -DVBOX_WITH_WINE_FIX_MUTE_ERRORS \
    -DVBOX_WITH_WINE_FIX_BLIT_ALPHATEST \
    -DVBOX_WITH_WINE_FIX_SURFUPDATA \
    -DUSE_HOOKS

  ifdef VERSION_BUILD
    CFLAGS  += -DWINE9X_BUILD=$(VERSION_BUILD)
    RESDEF  += -DWINE9X_BUILD=$(VERSION_BUILD)
  endif

  WINED3D_LIBS  = pthread9x/crtfix$(OBJ) -static-libgcc -L. -Lpthread9x -lpthread -lgdi32 -lopengl32
  SWITCHER_LIBS = -ladvapi32 -lkernel32 -luser32 -lgdi32

  WINETRAY_LIBS = -static -nostdlib -nodefaultlibs -lgcc -luser32 -lkernel32 -lgdi32 -lole32 -lshell32 -Wl,-subsystem,windows

  WINELIB_DEPS = pthread9x/crtfix$(OBJ) pthread9x/$(LIBPREFIX)pthread$(LIBSUFFIX)

  OBJ := .o

  LIBSUFFIX := .a
  LIBPREFIX := lib

  ifdef WINED3D_STATIC
    DX_LIBS = -static-libgcc -L. -lwined3d_static -lgdi32 -lopengl32
    CFLAGS += -DWINED3D_STATIC
    WINED3D_LIB_NAME = $(LIBPREFIX)wined3d_static$(LIBSUFFIX)
  else
    DX_LIBS = -static-libgcc -L. pthread9x/crtfix$(OBJ) -lwined3d -lgdi32
    WINED3D_LIB_NAME = wined3d.dll
  endif

  %.c.o: %.c $(DEPS)
		$(CC) $(CFLAGS) -c -o $@ $<
	
  %.c_sw.o: %.c $(DEPS)
		$(CC) -Inocrt -DNOCRT -DNOCRT_FILE -DNOCRT_FLOAT -DNOCRT_MEM -ffreestanding $(CFLAGS) -c -o $@ $<
	
  %.asm:
	
  %.asm.o: %.asm $(DEPS)
		$(ASM) -Iswitcher $< -f win32 -o $@
		
  %.res: %.rc $(DEPS)
		$(WINDRES) -DWINDRES -Icompact $(RESDEF) --input $< --output $@ --output-format=coff

  LIBSTATIC = $(AR) rcs -o $@
endif

TARGETS := winedd.dll wined8.dll wined9.dll
TARGETS += ddraw_95.dll ddraw_98.dll ddraw_xp.dll
TARGETS += d3d8_95.dll d3d8_98.dll d3d8_xp.dll
TARGETS += d3d9_98.dll d3d9_xp.dll
#TARGETS += winetray.exe

all: $(TARGETS)
.PHONY: all clean

wined3d_SRC = \
	wined3d/arb_program_shader.c \
	wined3d/ati_fragment_shader.c \
	wined3d/buffer.c \
	wined3d/context.c \
	wined3d/cs.c \
	wined3d/device.c \
	wined3d/directx.c \
	wined3d/drawprim.c \
	wined3d/gl_compat.c \
	wined3d/glsl_shader.c \
	wined3d/nvidia_texture_shader.c \
	wined3d/palette.c \
	wined3d/query.c \
	wined3d/resource.c \
	wined3d/shader.c \
	wined3d/shader_sm1.c \
	wined3d/shader_sm4.c \
	wined3d/state.c \
	wined3d/stateblock.c \
	wined3d/surface.c \
	wined3d/swapchain.c \
	wined3d/texture.c \
	wined3d/utils.c \
	wined3d/vertexdeclaration.c \
	wined3d/volume.c \
	wined3d/view.c \
	wined3d/wined3d_main.c \
	wined3d/sampler.c

d3d8_SRC = \
	d3d8/d3d8_main.c \
	d3d8/buffer.c \
	d3d8/device.c \
	d3d8/directx.c \
	d3d8/shader.c \
	d3d8/surface.c \
	d3d8/swapchain.c \
	d3d8/texture.c \
	d3d8/vertexdeclaration.c \
	d3d8/volume.c \
	compact/debug.c

d3d9_SRC = \
	d3d9/d3d9_main.c \
	d3d9/buffer.c \
	d3d9/device.c \
	d3d9/directx.c \
	d3d9/query.c \
	d3d9/shader.c \
	d3d9/stateblock.c \
	d3d9/surface.c \
	d3d9/swapchain.c \
	d3d9/texture.c \
	d3d9/vertexdeclaration.c \
	d3d9/volume.c \
	compact/debug.c

ddraw_SRC = \
	ddraw/clipper.c \
	ddraw/ddraw.c \
	ddraw/device.c \
	ddraw/executebuffer.c \
	ddraw/light.c \
	ddraw/main.c \
	ddraw/material.c \
	ddraw/palette.c \
	ddraw/surface.c \
	ddraw/utils.c \
	ddraw/vertexbuffer.c \
	ddraw/viewport.c \
	ddraw/memmgr.c \
	compact/debug.c \
	compact/exception.asm

switcher_SRC = \
  switcher/switcher.c

winetray_SRC = \
  tray/winetray.c

ifndef MSC
  NOCRT_SRC += \
    nocrt/nocrt.c \
    nocrt/nocrt_file_win.c \
    nocrt/nocrt_mem_win.c \
    nocrt/nocrt_math.c
 
  switcher_SRC += $(NOCRT_SRC) nocrt/nocrt_dll.c
	
  winetray_SRC += $(NOCRT_SRC) nocrt/nocrt_exe.c
endif

switcher_dd_SRC = \
	$(switcher_SRC) \
	switcher/ddraw_stubs.c \
	switcher/ddraw_sw.c \
	switcher/ddraw_debug.c \
	switcher/ddraw_sw.asm

switcher_d8_SRC = \
	$(switcher_SRC) \
	switcher/d3d8_stubs.c \
	switcher/d3d8_sw.c \
	switcher/d3d8_debug.c \
	switcher/d3d8_sw.asm

switcher_d9_SRC = \
	$(switcher_SRC) \
	switcher/d3d9_stubs.c \
	switcher/d3d9_sw.c \
	switcher/d3d9_debug.c \
	switcher/d3d9_sw.asm

DEF_wined3d.dll    = wined3d/wined3d.def
DEF_winedd.dll     = ddraw/ddraw.def
DEF_wined8.dll     = d3d8/d3d8.def
DEF_wined9.dll     = d3d9/d3d9.def

DEF_ddraw_xp.dll   = switcher/ddraw_xp.def
DEF_ddraw_98.dll   = switcher/ddraw_98.def
DEF_ddraw_95.dll   = switcher/ddraw_95.def

DEF_d3d8_xp.dll   = switcher/d3d8_xp.def
DEF_d3d8_98.dll   = switcher/d3d8_98.def
DEF_d3d8_95.dll   = switcher/d3d8_95.def

DEF_d3d9_xp.dll   = switcher/d3d9_xp.def
DEF_d3d9_98.dll   = switcher/d3d9_98.def

wined3d_OBJS  := $(wined3d_SRC:.c=.c$(OBJ))
d3d8_OBJS     := $(d3d8_SRC:.c=.c$(OBJ))
d3d9_OBJS     := $(d3d9_SRC:.c=.c$(OBJ))
ddraw_OBJS    := $(ddraw_SRC:.c=.c$(OBJ))
ddraw_OBJS    := $(ddraw_OBJS:.asm=.asm$(OBJ))
switcher_dd_OBJS := $(switcher_dd_SRC:.c=.c_sw$(OBJ))
switcher_dd_OBJS := $(switcher_dd_OBJS:.asm=.asm$(OBJ))
switcher_d8_OBJS := $(switcher_d8_SRC:.c=.c_sw$(OBJ))
switcher_d8_OBJS := $(switcher_d8_OBJS:.asm=.asm$(OBJ))
switcher_d9_OBJS := $(switcher_d9_SRC:.c=.c_sw$(OBJ))
switcher_d9_OBJS := $(switcher_d9_OBJS:.asm=.asm$(OBJ))

winetray_OBJS := $(winetray_SRC:.c=.c_sw$(OBJ))

ddraw_OBJS    += ddraw/dwine.res
d3d8_OBJS     += d3d8/d3d8.res
d3d9_OBJS     += d3d9/d3d9.res
wined3d_OBJS  += wined3d/wined3d.res
winetray_OBJS += tray/winetray.res

LD_DEPS := pthread9x/$(LIBPREFIX)pthread$(LIBSUFFIX)

wined3d.dll: $(wined3d_OBJS) compact/debug.c$(OBJ) $(WINELIB_DEPS) $(LD_DEPS)
	$(LD) $(CFLAGS) $(wined3d_OBJS) compact/debug.c$(OBJ) $(WINED3D_LIBS) $(DLLFLAGS)

$(LIBPREFIX)wined3d_static$(LIBSUFFIX): $(wined3d_OBJS) $(WINELIB_DEPS)
	-$(RM) $@
	$(LIBSTATIC) $(wined3d_OBJS)

winedd.dll: $(ddraw_OBJS) $(WINED3D_LIB_NAME) $(LD_DEPS)
	$(LD) $(CFLAGS) $(ddraw_OBJS) $(DX_LIBS) $(DLLFLAGS)
	
wined8.dll: $(d3d8_OBJS) $(WINED3D_LIB_NAME) $(LD_DEPS)
	$(LD) $(CFLAGS) $(d3d8_OBJS) $(DX_LIBS) $(DLLFLAGS)

wined9.dll: $(d3d9_OBJS) $(WINED3D_LIB_NAME) $(LD_DEPS)
	$(LD) $(CFLAGS) $(d3d9_OBJS) $(DX_LIBS) $(DLLFLAGS)

winetray.exe: $(winetray_OBJS)
	$(LD) $(CFLAGS) $(winetray_OBJS) -o $@ $(WINETRAY_LIBS)

pthread9x/crtfix$(OBJ): $(DEPS) pthread9x/$(LIBPREFIX)pthread$(LIBSUFFIX)

pthread9x/$(LIBPREFIX)pthread$(LIBSUFFIX): $(DEPS) pthread9x/Makefile pthread.mk
	cd pthread9x && $(MAKE)

ddraw_xp.dll: $(switcher_dd_OBJS) switcher/ddraw_xp.res
	$(LD) $(CFLAGS) $(switcher_dd_OBJS) switcher/ddraw_xp.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

ddraw_98.dll: $(switcher_dd_OBJS) switcher/ddraw_98.res
	$(LD) $(CFLAGS) $(switcher_dd_OBJS) switcher/ddraw_98.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

ddraw_95.dll: $(switcher_dd_OBJS) switcher/ddraw_95.res
	$(LD) $(CFLAGS) $(switcher_dd_OBJS) switcher/ddraw_95.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

d3d8_xp.dll: $(switcher_d8_OBJS) switcher/d3d8_xp.res
	$(LD) $(CFLAGS) $(switcher_d8_OBJS) switcher/d3d8_xp.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

d3d8_98.dll: $(switcher_d8_OBJS) switcher/d3d8_98.res
	$(LD) $(CFLAGS) $(switcher_d8_OBJS) switcher/d3d8_98.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

d3d8_95.dll: $(switcher_d8_OBJS) switcher/d3d8_95.res
	$(LD) $(CFLAGS) $(switcher_d8_OBJS) switcher/d3d8_95.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

d3d9_xp.dll: $(switcher_d9_OBJS) switcher/d3d9_xp.res
	$(LD) $(CFLAGS) $(switcher_d9_OBJS) switcher/d3d9_xp.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

d3d9_98.dll: $(switcher_d9_OBJS) switcher/d3d9_98.res
	$(LD) $(CFLAGS) $(switcher_d9_OBJS) switcher/d3d9_98.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

d3d9_95.dll: $(switcher_d9_OBJS) switcher/d3d9_95.res
	$(LD) $(CFLAGS) $(switcher_d9_OBJS) switcher/d3d9_95.res $(SWITCHER_LIBS) $(DLLFLAGS_NOCRT)

SLIBS  = $(LIBPREFIX)wined3d_static$(LIBSUFFIX) $(LIBPREFIX)wined3d$(LIBSUFFIX)
SLIBS += $(LIBPREFIX)winedd$(LIBSUFFIX) $(LIBPREFIX)wined8$(LIBSUFFIX) $(LIBPREFIX)wined9$(LIBSUFFIX)
SLIBS += $(LIBPREFIX)ddraw_95$(LIBSUFFIX) $(LIBPREFIX)ddraw_98$(LIBSUFFIX) $(LIBPREFIX)ddraw_xp$(LIBSUFFIX)
SLIBS += $(LIBPREFIX)d3d8_95$(LIBSUFFIX) $(LIBPREFIX)d3d8_98$(LIBSUFFIX) $(LIBPREFIX)d3d8_xp$(LIBSUFFIX)
SLIBS += $(LIBPREFIX)d3d9_98$(LIBSUFFIX) $(LIBPREFIX)d3d9_xp$(LIBSUFFIX)

ddreplacer.exe: ddreplacer.c$(OBJ)
	$(CC) $< -o $@

ifdef OBJ
clean:
	-$(RM) $(wined3d_OBJS)
	-$(RM) $(d3d8_OBJS)
	-$(RM) $(d3d9_OBJS)
	-$(RM) $(ddraw_OBJS)
	-$(RM) $(switcher_dd_OBJS)
	-$(RM) $(switcher_d8_OBJS)
	-$(RM) $(switcher_d9_OBJS)
	-$(RM) $(SLIBS)
	-$(RM) switcher/ddraw.res
	-$(RM) switcher/ddrawme.res
	-$(RM) switcher/ddraw_ver.res
	-$(RM) $(TARGETS)
	-$(RM) wined3d.dll
	-cd pthread9x && $(MAKE) clean
endif
