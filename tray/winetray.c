#include <initguid.h>
#include <windows.h>
#include <wingdi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ddraw.h>

#include "nocrt.h"

typedef HRESULT (WINAPI *HDirectDrawCreate)(GUID*,LPDIRECTDRAW*,IUnknown*);
typedef HRESULT (WINAPI *HDirectDrawCreateEx)(GUID *lpGuid, LPVOID *lplpDD, REFIID iid, IUnknown *pUnkOuter);

typedef BOOL (WINAPI *HInstallWineHook)(void);

typedef BOOL (WINAPI *HUninstallWineHool)(void);

int main(int argc, char **argv)
{
	if(CoInitialize(NULL) != S_OK)
	{
		fprintf(stderr, "FAILED: CoInitialize()\n");
		return EXIT_FAILURE;
	}
	
	HMODULE hDDraw = LoadLibrary("ddraw.dll");
	HDirectDrawCreate hDirectDrawCreate = NULL;
	
	if(hDDraw)
	{
		hDirectDrawCreate = (HDirectDrawCreate)GetProcAddress(hDDraw, "DirectDrawCreate");
		printf("ddraw.dll=0x%08lX, DirectDrawCreate=0x%08lX\n", 
			(DWORD)hDDraw, (DWORD)hDirectDrawCreate);
		if(hDirectDrawCreate)
		{
			LPDIRECTDRAW dd = NULL;
			if(hDirectDrawCreate((LPGUID)DDCREATE_HARDWAREONLY, &dd, NULL) == DD_OK)
			{
				HMODULE hVMHAL = LoadLibrary("vmhal9x.dll");
				if(hVMHAL)
				{
					HInstallWineHook hInstallWineHook = 
						(HInstallWineHook)GetProcAddress(hVMHAL, "InstallWineHook");
					if(hInstallWineHook)
					{
						if(hInstallWineHook())
						{
							printf("SUCCESS\n");
						} else fprintf(stderr, "FAILED: hInstallWineHook()\n");
					} else fprintf(stderr, "FAILED: GetProcAddress(\"InstallWineHook\")\n");
					FreeLibrary(hVMHAL);
				} else fprintf(stderr, "FAILED: LoadLibrary(\"vmhal9x.dll\")\n");
				IDirectDraw_Release(dd);
			} else fprintf(stderr, "FAILED: DirectDrawCreate(...)\n");
		} else fprintf(stderr, "FAILED: GetProcAddress(\"DirectDrawCreate\")\n");
		FreeLibrary(hDDraw);
	} else fprintf(stderr, "FAILED: LoadLibrary(\"ddraw.dll\")\n");
	
	CoUninitialize();
	
	ExitProcess(EXIT_SUCCESS);
	
	return EXIT_SUCCESS;
}
