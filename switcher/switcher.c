#include <stdint.h>
#include <stdarg.h>
#include <windows.h>
#include <tlhelp32.h>

#include "switcher.h"

#include "3d_accel.h"

#include "nocrt.h"

/* globals */
static HMODULE thisDLL = NULL;
static HMODULE userlib = NULL;
static HMODULE systemlib = NULL;

/* settings */
BOOL unkToSystemDll = TRUE;

/* build in blacklist */
static const char *blacklist_exe[] = {
	"iexplore.exe",
	"explorer.exe",
	NULL
};

static const char *blacklist_dll[] = {
	"opengl32.dll",
	"mesa3d.dll",
	"vmwsgl32.dll",
	NULL
};

/* debug */
void dbg_printf(const char *fmt, ...)
{
	va_list vl;
	static char modulename[MAX_PATH+1];
	
	FILE *fa = fopen(log_name, "ab");
	if(fa)
	{
		va_start(vl, fmt);
		vfprintf(fa, fmt, vl);
		va_end(vl);
		
		fputc('\r', fa);
		fputc('\n', fa);
		
		fclose(fa);
	}
}

/* detection tools */
BOOL isWindows95()
{
	DWORD GV = GetVersion();
	DWORD major = (DWORD)(LOBYTE(LOWORD(GV)));
  DWORD minor = (DWORD)(HIBYTE(LOWORD(GV)));
	
	if(major == 4 && minor < 10) /* 10 = Windows 98*/
	{
		return TRUE;
	}
	
	return FALSE;
}

BOOL isWindows98me()
{
	DWORD GV = GetVersion();
	DWORD major = (DWORD)(LOBYTE(LOWORD(GV)));
  DWORD minor = (DWORD)(HIBYTE(LOWORD(GV)));
	
	if(major == 4 && minor >= 10) /* 10 = Windows 98 */
	{
		return TRUE;
	}
	
	return FALSE;
}

BOOL isWindows2k()
{
	DWORD GV = GetVersion();
	DWORD major = (DWORD)(LOBYTE(LOWORD(GV)));
	
	if(major >= 5)
	{
		return TRUE;
	}
	
	return FALSE;
}

BOOL getVMDISP9xFlags(DWORD *pFlags)
{
	HWND hDesktop = GetDesktopWindow();
	HDC hdc = GetDC(hDesktop);
	FBHDA_t *hda = NULL;

	if(ExtEscape(hdc, OP_FBHDA_SETUP, 0, NULL, sizeof(FBHDA_t *), (LPVOID)&hda))
	{
		if(hda != NULL)
		{
			if(hda->cb == sizeof(FBHDA_t))
			{
				*pFlags = hda->flags;
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

BOOL isNineSafe(DWORD drv_flags)
{
	/* is forced to use software Mesa OpenGL */
	if(drv_flags & FB_FORCE_SOFTWARE)
		return TRUE;
	
	/* no Nine on ViRGE */
	if(drv_flags & FB_ACCEL_VIRGE)
		return FALSE;
	
	/* no Nine on VirtualBox 6.0 */
	if(drv_flags & FB_ACCEL_CHROMIUM)
		return FALSE;
	
	/* no Nine on 3dfx QEMU */
	if(drv_flags & FB_ACCEL_QEMU3DFX)
		return FALSE;
	
	/* not using Nine when vGPU10 is disabled */
	if((drv_flags & (FB_ACCEL_VMSVGA | FB_ACCEL_VMSVGA3D)) ==
			(FB_ACCEL_VMSVGA | FB_ACCEL_VMSVGA3D)
		)
	{
		if((drv_flags & FB_ACCEL_VMSVGA10) == 0)
		{
			return FALSE;
		}
	}
	
	/* vGPU10 or software Mesa */
	return TRUE;
}

HMODULE tryLoad(const char *path, const char *check_entry)
{
	HMODULE lib = LoadLibraryA(path);
	if(lib != NULL)
	{
		if(lib != thisDLL)
		{
			if(GetProcAddress(lib, check_entry) != NULL)
			{
				return lib;
			}
		}
		
		FreeLibrary(lib);
	}
	else
	{	
		DBG("LoadLibraryA(%s): %ld", path, GetLastError());
	}
	
	return NULL;
}

/**
 This unroll call stack and check modules against blacklist.
 
 @return: TRUE - call valid
          FALSE - loop call
 **/
BOOL fncheck(BYTE *returnaddress, DWORD fn)
{
	DBG("returnaddress: %X fn: %X", returnaddress, fn);
	
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	MODULEENTRY32 modentry = { 0 };
	
	(void)fn;
	
	if(hSnapshot != NULL)
	{
		modentry.dwSize = sizeof(MODULEENTRY32);
		Module32First(hSnapshot, &modentry);
		do
		{
			if ((modentry.modBaseAddr <= returnaddress) &&
				(modentry.modBaseAddr + modentry.modBaseSize > returnaddress))
			{
				int i = 0;
				const char *blentry;
				for(i = 0; (blentry = blacklist_dll[i]) != NULL; i++)
				{
					/* WARNING: check for module name case insensitive! */
					if(stricmp(modentry.szModule, blentry) == 0)
					{
						CloseHandle(hSnapshot);
						DBG("LOOP");
						return FALSE;
					}
					
				}
			}
		} while (Module32Next(hSnapshot, &modentry));
			
		CloseHandle(hSnapshot);
	}
	
	DBG("PASS");
	return TRUE;
}

void fnprint(DWORD fn)
{
	DWORD p = fn/(sizeof(void*));
	
	DBG("CALL: %s", fn_names[p]);
}

/* registry and settings */
static char *getExeName(char *buffer)
{
	char pathbuf[MAX_PATH];
	if(GetModuleFileNameA(NULL, pathbuf, MAX_PATH))
	{
		const char *exe = strrchr(pathbuf, '\\');
		if(exe != NULL && strlen(exe) > 1)
		{
			strcpy(buffer, exe+1);
			return buffer;
		}
	}
	
	return NULL;
}

void getSysPath(char *buf, char *dllname)
{
	if(GetSystemDirectoryA(buf, MAX_PATH) != 0)
	{
		strcat(buf, "\\");
		strcat(buf, dllname);
	}
	else
	{
		strcpy(buf, dllname);	
	}
}

static char *getSettings(char *buffer, const char *exe)
{
	DWORD type;
	LSTATUS lResult;
	HKEY hKey;
	DWORD size;
	char *result = NULL;
	
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg_base, 0, KEY_READ, &hKey);
	if(lResult == ERROR_SUCCESS)
	{
		if(exe != NULL)
		{
			size = PATH_MAX;
			lResult = RegQueryValueExA(hKey, exe, NULL, &type, buffer, &size);
	    if(lResult == ERROR_SUCCESS)
	    {
	      switch(type)
	      {
	        case REG_SZ:
	        case REG_MULTI_SZ:
	        case REG_EXPAND_SZ:
	        {
	        	result = buffer;
	        	break;
	        }
	        case REG_DWORD:
	        {
	        	/* conv DWORD to string */
	        	DWORD tmp = *((DWORD*)buffer);
	        	sprintf(buffer, "%u", tmp);	
	        	result = buffer;
	          break;
	        }
	      }
			}
		}
		
		if(result == NULL)
		{
			size = PATH_MAX;
			lResult = RegQueryValueExA(hKey, "global", NULL, &type, buffer, &size);
	    if(lResult == ERROR_SUCCESS)
	    {
	      switch(type)
	      {
	        case REG_SZ:
	        case REG_MULTI_SZ:
	        case REG_EXPAND_SZ:
	        {
	        	result = buffer;
	        	break;
	        }
	        case REG_DWORD:
	        {
	        	/* conv DWORD to string */
	        	DWORD tmp = *((DWORD*)buffer);
	        	sprintf(buffer, "%u", tmp);	     
	        	result = buffer;   	
	          break;
	        }
	      }
			}
		}
		
		RegCloseKey(hKey);
	} // RegOpenKeyEx(...) == ERROR_SUCCESS
	
	return result;
}

BOOL switcher_init()
{
	DBG("switcher_start");
	
	char exename_buf[MAX_PATH];
	char settings_buf[MAX_PATH];
	char *exe = getExeName(exename_buf);
	char *settings = getSettings(settings_buf, exe);
	
	DBG("exe: %s, settings: %s", exe, settings);
	
	userlib = switcherGetUserLib(exe, settings);
	systemlib = switcherGetSystemLib(exe, settings);
	
	DBG("userlib: %X, systemlib: %X", userlib, systemlib);

	if(userlib == NULL && systemlib == NULL)
	{
		return FALSE;
	}
	
	switcherLoadUserLib(userlib);
	switcherLoadSystemLib(systemlib);
	
	return TRUE;
}

static volatile DWORD need_init = 0;

void switcher_reinit()
{
	if(InterlockedExchange(&need_init, 1) == 0)
	{
		switcher_init();
	}
}

void switcher_free()
{
	if(userlib != NULL)
	{
		FreeLibrary(userlib);
		userlib = NULL;
	}
	
	if(systemlib != NULL)
	{
		FreeLibrary(systemlib);
		systemlib = NULL;
	}
}

/* DLL main */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
  switch (reason)
  {
    case DLL_PROCESS_ATTACH:
    	thisDLL = (HMODULE)inst;
    	/* 
    	JH: We cannot call LoadLibrary here! (on 9x works, on XP no)
    	if(!switcher_init())
    		return FALSE;
    	*/
    		
    	break;
    case DLL_PROCESS_DETACH:
    	switcher_free();
    	break;
   }
	
	return TRUE;
}

