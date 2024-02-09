#include <stdint.h>
#include <stdarg.h>
#include <windows.h>

#include "switcher.h"

#include "nocrt.h"

#define FN_TO_CHECK "Direct3DCreate8"

/* generate the list type */
#include "switcher_mf_struct.h"
#pragma pack(push)
#pragma pack(1)
typedef struct _fnlistStruct {
	#include "d3d8_list.h"
} fnlistStruct;
#pragma pack(pop)
#include "switcher_mf_clean.h"

/* globals */
const char reg_base[] = "Software\\D8Switcher";
extern fnlistStruct fnlist;
extern fnlistStruct fnlist_sys;

HMODULE switcherGetUserLib(const char *exe, const char *settings)
{
	HANDLE lib = NULL;
	char path_to_dll[MAX_PATH];
	
	if(settings != NULL && stricmp(settings, "default") != 0)
	{
		if(stricmp(settings, "system") == 0)
		{
			return switcherGetSystemLib(exe, settings);
		}
		
		if(
			(stricmp(settings, "wine") != 0)      &&
			(stricmp(settings, "nine") != 0)      &&
			(stricmp(settings, "ninemore") != 0)  &&
			(stricmp(settings, "nineforce") != 0) )
		{
			RET_ON_SUC(lib, tryLoad(settings, FN_TO_CHECK));
		}
	}
	
	RET_ON_SUC(lib, tryLoad("wined8.dll", FN_TO_CHECK));
	
	getSysPath(path_to_dll, "wined8.dll");
	RET_ON_SUC(lib, tryLoad(path_to_dll, FN_TO_CHECK));
	
	return switcherGetSystemLib(exe, settings);
}

HMODULE switcherGetSystemLib(const char *exe, const char *settings)
{
	HANDLE lib = NULL;
	char path_to_dll[MAX_PATH];
	
	
	RET_ON_SUC(lib, tryLoad("msd3d8.dll", FN_TO_CHECK));
	
	getSysPath(path_to_dll, "msd3d8.dll");
	RET_ON_SUC(lib, tryLoad(path_to_dll, FN_TO_CHECK));
	
	RET_ON_SUC(lib, tryLoad("msd3d8.dll", FN_TO_CHECK));
		
	getSysPath(path_to_dll, "msd3d8.dll");
	RET_ON_SUC(lib, tryLoad(path_to_dll, FN_TO_CHECK));
	
	return NULL;
}

#include "switcher_mf_address.h"

void switcherLoadUserLib(HMODULE lib)
{
	void *proc;
	
	fnlistStruct *fnlist_ptr = &fnlist;
	
	if(lib != NULL)
	{
		#include "d3d8_list.h"
	}
	
	DBG("Direct3DCreate8: %p", fnlist.pDirect3DCreate8);
	
}

void switcherLoadSystemLib(HMODULE lib)
{
	void *proc;
	
	fnlistStruct *fnlist_ptr = &fnlist_sys;
	
	if(lib != NULL)
	{
		#include "d3d8_list.h"
	}
}

#include "switcher_mf_clean.h"
