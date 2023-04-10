#ifndef __DDSTUBS_H__INCLUDED__
#define __DDSTUBS_H__INCLUDED__

#include <ddraw.h>

typedef HRESULT (WINAPI *pDllCanUnloadNow)(void);
typedef HRESULT (WINAPI *pDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
typedef HRESULT (WINAPI *pDllRegisterServer)(void);
typedef HRESULT (WINAPI *pDllUnregisterServer)(void);
typedef HRESULT (WINAPI *pDirectDrawCreate)(GUID *driver_guid, IDirectDraw **ddraw, IUnknown *outer);
typedef HRESULT (WINAPI *pDirectDrawCreateClipper)(DWORD flags, IDirectDrawClipper **clipper, IUnknown *outer_unknown);
typedef HRESULT (WINAPI *pDirectDrawCreateEx)(GUID *guid, LPVOID *dd, REFIID iid, IUnknown *UnkOuter);
typedef HRESULT (WINAPI *pDirectDrawEnumerateA)(LPDDENUMCALLBACKA callback, void *context);
typedef HRESULT (WINAPI *pDirectDrawEnumerateExA)(LPDDENUMCALLBACKEXA callback, void *context, DWORD flags);
typedef HRESULT (WINAPI *pDirectDrawEnumerateW)(LPDDENUMCALLBACKW callback, void *context);
typedef HRESULT (WINAPI *pDirectDrawEnumerateExW)(LPDDENUMCALLBACKEXW callback, void *context, DWORD flags);

typedef HRESULT (WINAPI *pAcquireDDThreadLock)(void);
typedef HRESULT (WINAPI *pCompleteCreateSystemSurface)(void);
typedef HRESULT (WINAPI *pDDInternalLock)(void *unk1, void *unk2);
typedef HRESULT (WINAPI *pDDInternalUnlock)(void *unk1);
typedef HRESULT (WINAPI *pDSoundHelp)(void *unk1, void *unk2, void *unk3);
typedef HRESULT (WINAPI *pRegisterSpecialCase)(void);
typedef HRESULT (WINAPI *pReleaseDDThreadLock)(void);
typedef HRESULT (WINAPI *pSetAppCompatData)(void);
typedef HRESULT (WINAPI *pGetDDSurfaceLocal)(void *unk1, void *unk2, void *unk3);
typedef HRESULT (WINAPI *pGetOLEThunkData)(void);
typedef HRESULT (WINAPI *pGetSurfaceFromDC)(void *unk1, void *unk2, void *unk3);

typedef HRESULT (WINAPI *pD3DParseUnknownCommand)(void *unk[2]);
typedef HRESULT (WINAPI *pDDGetAttachedSurfaceLcl)(void *unk[3]);
typedef HRESULT (WINAPI *pDDHAL32_VidMemAlloc)(void *unk[4]);
typedef HRESULT (WINAPI *pDDHAL32_VidMemFree)(void *unk[3]);
typedef HRESULT (WINAPI *pDdEntry1)(void *unk[6]);
typedef HRESULT (WINAPI *pDdEntry2)(void *unk[11]);
typedef HRESULT (WINAPI *pDdEntry3)(void *unk[1]);
typedef HRESULT (WINAPI *pDdEntry4)(void *unk[2]);
typedef HRESULT (WINAPI *pDdEntry5)(void *unk[2]);
typedef HRESULT (WINAPI *pDdEntry6)(void *unk[2]);
typedef HRESULT (WINAPI *pDdEntry7)(void *unk[3]);
typedef HRESULT (WINAPI *pDdEntry8)(void *unk[7]);
typedef HRESULT (WINAPI *pDdEntry9)(void *unk[1]);
typedef HRESULT (WINAPI *pDdEntry10)(void *unk[1]);
typedef HRESULT (WINAPI *pDdEntry11)(void *unk[1]);
typedef HRESULT (WINAPI *pDdEntry12)(void *unk[1]);
typedef HRESULT (WINAPI *pDdEntry13)(void *unk[7]);
typedef HRESULT (WINAPI *pDdEntry16)(void *unk[4]);
typedef HRESULT (WINAPI *pDdEntry17)(void *unk[3]);
typedef HRESULT (WINAPI *pDdEntry18)(void *unk[1]);
typedef HRESULT (WINAPI *pDdEntry19)(void *unk[1]);
typedef HRESULT (WINAPI *pDdEntry20)(void *unk[2]);
typedef HRESULT (WINAPI *pDdEntry21)(void *unk[3]);
typedef HRESULT (WINAPI *pDdEntry22)(void *unk[15]);
typedef HRESULT (WINAPI *pDdEntry23)(void *unk[6]);
typedef HRESULT (WINAPI *pDdEntry24)(void *unk[5]);
typedef HRESULT (WINAPI *pGetAliasedVidMem)(void);
typedef HRESULT (WINAPI *pGetNextMipMap)(void);
typedef HRESULT (WINAPI *pHeapVidMemAllocAligned)(void *unk[2]);
typedef HRESULT (WINAPI *pInternalLock)(void);
typedef HRESULT (WINAPI *pInternalUnlock)(void);
typedef HRESULT (WINAPI *pLateAllocateSurfaceMem)(void *unk[4]);
typedef HRESULT (WINAPI *pLockCB)(void);
typedef HRESULT (WINAPI *pUnlockCB)(void);
typedef HRESULT (WINAPI *pVidMemAlloc)(void *unk[3]);
typedef HRESULT (WINAPI *pVidMemAmountFree)(void *unk[1]);
typedef HRESULT (WINAPI *pVidMemFini)(void *unk[1]);
typedef HRESULT (WINAPI *pVidMemFree)(void *unk[2]);
typedef HRESULT (WINAPI *pVidMemInit)(void *unk[5]);
typedef HRESULT (WINAPI *pVidMemLargestFree)(void *unk[1]);

#define ddfunc(_name) p##_name pf##_name;

#pragma pack(push)
#pragma pack(1)
typedef struct _ddlib_t
{
#include "ddfunclist.h"
} ddlib_t;
#pragma pack(pop)

#undef ddfunc

extern const ddlib_t ddlib_stubs;

#endif /* __DDSTUBS_H__INCLUDED__ */
