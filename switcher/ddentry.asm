%define ddfunc(_fn) .p %+ _fn resd 1

struc ddlib
%include "switcher/ddfunclist.h"
endstruc

extern _ddraw_active_lib
extern _InitLib
extern _printFunc

%macro ddentryproc 1
	global _ %+ %1
	_ %+ %1 :
		mov eax,[esp]
		push eax
		call _InitLib
		add esp,4

		mov eax,ddlib.p %+ %1
		push eax
		call _printFunc
		add esp,4
		
		mov eax,[_ddraw_active_lib]
		mov eax,[eax + ddlib.p %+ %1 ]
		jmp eax
%endmacro

section .text

ddentryproc DirectDrawCreate
ddentryproc DirectDrawCreateClipper
ddentryproc DirectDrawCreateEx
ddentryproc DirectDrawEnumerateA
ddentryproc DirectDrawEnumerateExA
ddentryproc DirectDrawEnumerateExW
ddentryproc DirectDrawEnumerateW
ddentryproc AcquireDDThreadLock
ddentryproc DDInternalLock
ddentryproc DDInternalUnlock
ddentryproc DSoundHelp
ddentryproc RegisterSpecialCase
ddentryproc ReleaseDDThreadLock
ddentryproc SetAppCompatData
ddentryproc GetDDSurfaceLocal
ddentryproc GetOLEThunkData
ddentryproc GetSurfaceFromDC
ddentryproc DllCanUnloadNow
ddentryproc DllGetClassObject
ddentryproc DllRegisterServer
ddentryproc DllUnregisterServer

ddentryproc D3DParseUnknownCommand
ddentryproc DDGetAttachedSurfaceLcl
ddentryproc DDHAL32_VidMemAlloc
ddentryproc DDHAL32_VidMemFree
ddentryproc DdEntry1
ddentryproc DdEntry2
ddentryproc DdEntry3
ddentryproc DdEntry4
ddentryproc DdEntry5
ddentryproc DdEntry6
ddentryproc DdEntry7
ddentryproc DdEntry8
ddentryproc DdEntry9
ddentryproc DdEntry10
ddentryproc DdEntry11
ddentryproc DdEntry12
ddentryproc DdEntry13
ddentryproc DdEntry16
ddentryproc DdEntry17
ddentryproc DdEntry18
ddentryproc DdEntry19
ddentryproc DdEntry20
ddentryproc DdEntry21
ddentryproc DdEntry22
ddentryproc DdEntry23
ddentryproc DdEntry24
ddentryproc GetAliasedVidMem
ddentryproc GetNextMipMap
ddentryproc HeapVidMemAllocAligned
ddentryproc InternalLock
ddentryproc InternalUnlock
ddentryproc LateAllocateSurfaceMem
ddentryproc LockCB
ddentryproc UnlockCB
ddentryproc VidMemAlloc
ddentryproc VidMemAmountFree
ddentryproc VidMemFini
ddentryproc VidMemFree
ddentryproc VidMemInit
ddentryproc VidMemLargestFree

section .data

global _thk1632_ThunkData32
global _thk3216_ThunkData32

_thk1632_ThunkData32: dd 0,0,0,0,0,0,0,0,0
_thk3216_ThunkData32: dd 0,0,0,0,0,0,0,0,0

