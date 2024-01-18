#include "config.h"
#include "wine/port.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

typedef unsigned long	FLATPTR;
typedef uint32_t DWORD;
typedef uint32_t DDSCAPS;
typedef void* LPVOID;
typedef long* LPLONG;

#define WINAPI __stdcall

typedef struct _SURFACEALIGNMENT
{
	union
	{
		struct
		{
			DWORD       dwStartAlignment;
			DWORD       dwPitchAlignment;
			DWORD       dwReserved1;
			DWORD       dwReserved2;
		} Linear;
		struct
		{
			DWORD       dwXAlignment;
			DWORD       dwYAlignment;
			DWORD       dwReserved1;
			DWORD       dwReserved2;
		} Rectangular;
	};
} SURFACEALIGNMENT;
typedef struct _SURFACEALIGNMENT *LPSURFACEALIGNMENT;

typedef struct _HEAPALIGNMENT
{
	DWORD                dwSize;
	DDSCAPS              ddsCaps;       /* Indicates which alignment fields are valid.*/
	DWORD                dwReserved;
	SURFACEALIGNMENT     ExecuteBuffer; /* Surfaces tagged with DDSCAPS_EXECUTEBUFFER */
	SURFACEALIGNMENT     Overlay;       /* Surfaces tagged with DDSCAPS_OVERLAY       */
	SURFACEALIGNMENT     Texture;       /* Surfaces tagged with DDSCAPS_TEXTURE       */
	SURFACEALIGNMENT     ZBuffer;       /* Surfaces tagged with DDSCAPS_ZBUFFER       */
	SURFACEALIGNMENT     AlphaBuffer;   /* Surfaces tagged with DDSCAPS_ALPHA         */
	SURFACEALIGNMENT     Offscreen;     /* Surfaces tagged with DDSCAPS_OFFSCREENPLAIN*/
	SURFACEALIGNMENT     FlipTarget;    /* Surfaces whose bits are potential primaries i.e. back buffers*/
} HEAPALIGNMENT;
typedef struct _HEAPALIGNMENT *LPHEAPALIGNMENT;

typedef struct _VMEMHEAP
{
	DWORD		        dwFlags;
	DWORD                       stride;
	LPVOID		        freeList;
	LPVOID		        allocList;
	DWORD                       dwTotalSize;
	FLATPTR                     fpGARTLin;      /* AGP: GART linear base of heap (app. visible)   */
	FLATPTR                     fpGARTDev;      /* AGP: GART device base of heap (driver visible) */
	DWORD                       dwCommitedSize; /* AGP: Number of bytes commited to heap          */
	/*
	 * Extended alignment data:
	 * Filled in by DirectDraw in response to a GetHeapAlignment HAL call.
	 */
	DWORD                       dwCoalesceCount;
	HEAPALIGNMENT               Alignment;
} VMEMHEAP;
typedef struct _VMEMHEAP LPVMEMHEAP;

/*
 * video memory data structures (passed in DDHALINFO)
 */
typedef struct _VIDMEM
{
	DWORD		dwFlags;	// flags
	FLATPTR		fpStart;	// start of memory chunk
	union
	{
		FLATPTR		fpEnd;		// end of memory chunk
		DWORD		dwWidth;	// width of chunk (rectanglar memory)
	};
	DDSCAPS		ddsCaps;		// what this memory CANNOT be used for
	DDSCAPS		ddsCapsAlt;	// what this memory CANNOT be used for if it must
	union
	{
		LPVMEMHEAP	lpHeap;		// heap pointer, used by DDRAW
		DWORD		dwHeight;	// height of chunk (rectanguler memory)
	};
} VIDMEM;
typedef VIDMEM *LPVIDMEM;


static void dbg_printf(const char *fmt, ...)
{
  va_list vl;
  FILE *fa;  
  
  fa = fopen("C:\\wine.log", "ab");
  if(!fa) return;
    
  va_start(vl, fmt);
  vfprintf(fa, fmt, vl);
  va_end(vl);
  
  fputs("\r\n", fa);
  fclose(fa);
}

/*
 * These legacy DLL exports don't handle nonlocal heaps
 */
FLATPTR WINAPI VidMemAlloc(LPVMEMHEAP pvmh, DWORD width, DWORD height)
{
	dbg_printf("%s", __FUNCTION__);
	return 0;
}

void WINAPI VidMemFree(LPVMEMHEAP pvmh, FLATPTR ptr)
{
	dbg_printf("%s", __FUNCTION__);
}

/*
 * This DLL export can be used by drivers to allocate aligned surfaces from heaps which
 * they have previously exposed to DDRAW.DLL. This function can allocate from nonlocal heaps.
 */
FLATPTR WINAPI HeapVidMemAllocAligned(struct _VIDMEM* lpVidMem,
	DWORD dwWidth, DWORD dwHeight, LPSURFACEALIGNMENT lpAlignment, LPLONG lpNewPitch)
{
	dbg_printf("%s", __FUNCTION__);
	return 0;
}

FLATPTR WINAPI DDHAL_VidMemAlloc(LPVOID lpDD, int heap, DWORD dwWidth, DWORD dwHeight)
{
	dbg_printf("%s", __FUNCTION__);
	return 0;
}


void WINAPI DDHAL_VidMemFree(LPVOID lpDD, int heap, FLATPTR fpMem)
{
	dbg_printf("%s", __FUNCTION__);
}

