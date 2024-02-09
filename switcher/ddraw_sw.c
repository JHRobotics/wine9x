#include <stdint.h>
#include <stdarg.h>
#include <windows.h>

#include "switcher.h"

#include "nocrt.h"

#define FN_TO_CHECK "DirectDrawCreate"

/* generate the list type */
#include "switcher_mf_struct.h"
#pragma pack(push)
#pragma pack(1)
typedef struct _fnlistStruct {
	#include "ddraw_list.h"
} fnlistStruct;
#pragma pack(pop)
#include "switcher_mf_clean.h"

/* globals */
const char reg_base[] = "Software\\DDSwitcher";
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
	
	RET_ON_SUC(lib, tryLoad("winedd.dll", FN_TO_CHECK));
	
	getSysPath(path_to_dll, "winedd.dll");
	RET_ON_SUC(lib, tryLoad(path_to_dll, FN_TO_CHECK));
	
	return switcherGetSystemLib(exe, settings);
}

HMODULE switcherGetSystemLib(const char *exe, const char *settings)
{
	HANDLE lib = NULL;
	char path_to_dll[MAX_PATH];
	
	if(isWindows95())
	{
		RET_ON_SUC(lib, tryLoad("ddr95.dll", FN_TO_CHECK));
	}
	
	if(isWindows98me())
	{
		RET_ON_SUC(lib, tryLoad("ddr98.dll", FN_TO_CHECK));
	}
	
	RET_ON_SUC(lib, tryLoad("ddsys.dll", FN_TO_CHECK));
	
	getSysPath(path_to_dll, "ddsys.dll");
	RET_ON_SUC(lib, tryLoad(path_to_dll, FN_TO_CHECK));
	
	if(isWindows2k())
	{
		/* loading ddraw.dll is dangerous on WIN9X, because
		   when is loaded, it cannot be unloaded!
		 */
		RET_ON_SUC(lib, tryLoad("ddraw.dll", FN_TO_CHECK));
		
		getSysPath(path_to_dll, "ddraw.dll");
		RET_ON_SUC(lib, tryLoad(path_to_dll, FN_TO_CHECK));
	}
	
	return NULL;
}

#include "switcher_mf_address.h"

void switcherLoadUserLib(HMODULE lib)
{
	void *proc;
	
	fnlistStruct *fnlist_ptr = &fnlist;
	
	if(lib != NULL)
	{
		#include "ddraw_list.h"
	}
}

void switcherLoadSystemLib(HMODULE lib)
{
	void *proc;
	
	fnlistStruct *fnlist_ptr = &fnlist_sys;
	
	if(lib != NULL)
	{
		#include "ddraw_list.h"
	}
}

#include "switcher_mf_clean.h"
