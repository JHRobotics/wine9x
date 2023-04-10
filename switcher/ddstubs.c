#include <windows.h>
#include "ddstubs.h"

HRESULT WINAPI DllGetClassObjectStub(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	return FALSE;
}

HRESULT WINAPI DirectDrawCreateStub(GUID *driver_guid, IDirectDraw **ddraw, IUnknown *outer)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI DirectDrawCreateClipperStub(DWORD flags, IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI DirectDrawCreateExStub(GUID *guid, LPVOID *dd, REFIID iid, IUnknown *UnkOuter)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI DirectDrawEnumerateAStub(LPDDENUMCALLBACKA callback, void *context)
{
	return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI DirectDrawEnumerateExAStub(LPDDENUMCALLBACKEXA callback, void *context, DWORD flags)
{
	return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI DirectDrawEnumerateWStub(LPDDENUMCALLBACKW callback, void *context)
{
	return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI DirectDrawEnumerateExWStub(LPDDENUMCALLBACKEXW callback, void *context, DWORD flags)
{
	return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI DllCanUnloadNowStub(void)
{
	return S_FALSE;
}

HRESULT WINAPI DllRegisterServerStub(void)
{
	return S_FALSE;
}

HRESULT WINAPI DllUnregisterServerStub(void)
{
	return S_FALSE;
}

HRESULT WINAPI AcquireDDThreadLockStub(void)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI CompleteCreateSystemSurfaceStub(void)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI DDInternalLockStub(void *unk1, void *unk2)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI DDInternalUnlockStub(void *unk1)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI DSoundHelpStub(void *unk1, void *unk2, void *unk3)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI RegisterSpecialCaseStub(void)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI ReleaseDDThreadLockStub(void)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI SetAppCompatDataStub(void)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI GetDDSurfaceLocalStub(void *unk1, void *unk2, void *unk3)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI GetOLEThunkDataStub(void)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI GetSurfaceFromDCStub(void *unk1, void *unk2, void *unk3)
{
	return DDERR_GENERIC;
}

HRESULT WINAPI D3DParseUnknownCommandStub(void *unk[2]){return DDERR_GENERIC;}
HRESULT WINAPI DDGetAttachedSurfaceLclStub(void *unk[3]){return DDERR_GENERIC;}
HRESULT WINAPI DDHAL32_VidMemAllocStub(void *unk[4]){return DDERR_GENERIC;}
HRESULT WINAPI DDHAL32_VidMemFreeStub(void *unk[3]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry1Stub(void *unk[6]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry2Stub(void *unk[11]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry3Stub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry4Stub(void *unk[2]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry5Stub(void *unk[2]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry6Stub(void *unk[2]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry7Stub(void *unk[3]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry8Stub(void *unk[7]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry9Stub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry10Stub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry11Stub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry12Stub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry13Stub(void *unk[7]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry16Stub(void *unk[4]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry17Stub(void *unk[3]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry18Stub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry19Stub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry20Stub(void *unk[2]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry21Stub(void *unk[3]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry22Stub(void *unk[15]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry23Stub(void *unk[6]){return DDERR_GENERIC;}
HRESULT WINAPI DdEntry24Stub(void *unk[5]){return DDERR_GENERIC;}
HRESULT WINAPI GetAliasedVidMemStub(void){return DDERR_GENERIC;}
HRESULT WINAPI GetNextMipMapStub(void){return DDERR_GENERIC;}
HRESULT WINAPI HeapVidMemAllocAlignedStub(void *unk[2]){return DDERR_GENERIC;}
HRESULT WINAPI InternalLockStub(void){return DDERR_GENERIC;}
HRESULT WINAPI InternalUnlockStub(void){return DDERR_GENERIC;}
HRESULT WINAPI LateAllocateSurfaceMemStub(void *unk[4]){return DDERR_GENERIC;}
HRESULT WINAPI LockCBStub(void){return DDERR_GENERIC;}
HRESULT WINAPI UnlockCBStub(void){return DDERR_GENERIC;}
HRESULT WINAPI VidMemAllocStub(void *unk[3]){return DDERR_GENERIC;}
HRESULT WINAPI VidMemAmountFreeStub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI VidMemFiniStub(void *unk[1]){return DDERR_GENERIC;}
HRESULT WINAPI VidMemFreeStub(void *unk[2]){return DDERR_GENERIC;}
HRESULT WINAPI VidMemInitStub(void *unk[5]){return DDERR_GENERIC;}
HRESULT WINAPI VidMemLargestFreeStub(void *unk[1]){return DDERR_GENERIC;}

#define ddfunc(_name) _name##Stub,

const ddlib_t ddlib_stubs = {
#include "ddfunclist.h"
};

#undef ddfunc
