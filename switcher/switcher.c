#include <stdint.h>
#include <stdarg.h>
#include <windows.h>
#include <tlhelp32.h>
#include "nocrt.h"
#include "ddstubs.h"

typedef struct _ddlink_t
{
	HANDLE dll;
	char name[64];
	char dllname[MAX_PATH];
	ddlib_t lib;
	int usable;
	int loaded;
	int priority;
	int system;
} ddlink_t;

ddlib_t *ddraw_active_lib = NULL;

#define SWITCHER_MAX 12
#define SWITCHER_MAGIC 0x4312BEC4
#define TEST_MAGIC() (switcher_ctx.magic == SWITCHER_MAGIC)

#define FILE_LOG "C:\\ddswitch.log"

#ifdef DEBUG
//#define DBG_printf printf
HANDLE log_mutex = INVALID_HANDLE_VALUE;

void DBG_printf(const char *format, ...)
{
	va_list vl;
	static char modulename[MAX_PATH+1];
	
	if(log_mutex != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(log_mutex, INFINITE);
	}
	
# ifdef FILE_LOG
	FILE *fa = fopen(FILE_LOG, "ab");
	if(fa)
	{
		if(GetModuleFileNameA(NULL, modulename, MAX_PATH) != 0)
		{
			fprintf(fa, "%s: ", modulename);
		}
		
		va_start(vl,format);
		vfprintf(fa, format, vl);
		va_end(vl);
		fclose(fa);
	}
# else
 vprintf(format, vl);
# endif
	
	if(log_mutex != INVALID_HANDLE_VALUE)
	{
		ReleaseMutex(log_mutex);
	}
	
}
#else
#define DBG_printf(format, ...)
#endif


typedef struct _switcher_t
{
	unsigned long magic;
	ddlink_t ddlinks[SWITCHER_MAX];
	size_t   ddlinks_cnt;
} switcher_t;

#define ddfunc(_name) #_name,

#pragma pack(push)
#pragma pack(1)
const char *dddraw_names[] = {
#include "ddfunclist.h"
	NULL
};
#pragma pack(pop)

#undef ddfunc

switcher_t switcher_ctx;

#define ddfunc(_n) tmp = GetProcAddress(dll, #_n);	if(tmp != NULL){ lib->pf##_n = (p##_n)tmp;} /*std_puts(#_n ": ");std_vardump(VAR_32|VAR_FMT_HEX, &tmp);std_puts("\n");*/

static void LoadDDraw(HANDLE dll, ddlib_t *lib)
{
	void *tmp;
	memcpy(lib, &ddlib_stubs, sizeof(ddlib_t));
	
  #include "ddfunclist.h"
}
#undef ddfunc

static BOOL FileExists(const char *path)
{
  DWORD dwAttrib = GetFileAttributes(path);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static char getExeNameBuf[MAX_PATH];
static const char *getExeName()
{
	if(GetModuleFileNameA(NULL, getExeNameBuf, MAX_PATH))
	{
		const char *exe = strrchr(getExeNameBuf, '\\');
		if(exe != NULL && strlen(exe) > 1)
		{
			return exe+1;
		}
	}
	return NULL;
}

#define REG_BUFFER_SIZE 128

static void ctx_init()
{
	switcher_ctx.ddlinks_cnt = 0;
	switcher_ctx.magic = SWITCHER_MAGIC;
}

static void ctx_add(const char *name, const char *dllname, int priority, int issystem, int isloaded)
{
	int id = switcher_ctx.ddlinks_cnt;
	
	if(id >= SWITCHER_MAX)
	{
		return;
	}
	
	strcpy(switcher_ctx.ddlinks[id].name,    name);
	strcpy(switcher_ctx.ddlinks[id].dllname, dllname);
	switcher_ctx.ddlinks[id].dll      = NULL;
	switcher_ctx.ddlinks[id].usable   = 1;
	switcher_ctx.ddlinks[id].loaded   = isloaded;
	switcher_ctx.ddlinks[id].priority = priority;
	switcher_ctx.ddlinks[id].system   = issystem;
	
	if(isloaded)
	{
		memcpy(&switcher_ctx.ddlinks[id].lib, &ddlib_stubs, sizeof(ddlib_t));
	}
	
	switcher_ctx.ddlinks_cnt++;
}

static BOOL isWindows95()
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

static BOOL isWindows98me()
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

static BOOL isWindows2k()
{
	DWORD GV = GetVersion();
	DWORD major = (DWORD)(LOBYTE(LOWORD(GV)));
	
	if(major >= 5)
	{
		return TRUE;
	}
	
	return FALSE;
}

static void InitSwitcher(HMODULE inst)
{
	char myfilename[MAX_PATH]; 
	char tmppath[MAX_PATH];
	char regbuff[REG_BUFFER_SIZE];
	size_t i = 0;
	int syspmul = 1;
	LSTATUS lResult;
	HKEY  hKey;
	
	ddraw_active_lib = (ddlib_t*)&ddlib_stubs;
	
	ctx_init();
	ctx_add("default", "", 1, 1, 1);
	
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\DDSwitcher", 0, KEY_READ, &hKey);
	if(lResult == ERROR_SUCCESS)
	{
		DWORD size = REG_BUFFER_SIZE;
		DWORD type = 0;
		DWORD temp_dw = 0;
		const char *name = getExeName();
		
		if(name != NULL)
		{
			lResult = RegQueryValueExA(hKey, name, NULL, &type, regbuff, &size);
	    if(lResult == ERROR_SUCCESS)
	    {
	      switch(type)
	      {
	        case REG_SZ:
	        case REG_MULTI_SZ:
	        case REG_EXPAND_SZ:
	        	if(stricmp(regbuff, "ddraw.dll") == 0 ||
	        		 stricmp(regbuff, "ddsys.dll") == 0 ||
	        		 stricmp(regbuff, "system") == 0 ||
	        		 stricmp(regbuff, "ddraw") == 0)
	        	{
	        		syspmul = 10;
	        	}
	        	else
	        	{
	        		ctx_add("registry", regbuff, 60, 0, 0);
	        	}
	          break;
	      }
			}
		}
		
		RegCloseKey(hKey);
	}
	
	
	if(GetModuleFileNameA(inst, myfilename, MAX_PATH) != ERROR_SUCCESS)
	{
		myfilename[0] = '\0';
	}
	
	if(FileExists("ddsys.dll"))
	{
		ctx_add("msddraw", "ddsys.dll", 5*syspmul, 1, 0);
	}
	
	if(FileExists("dwine.dll"))
	{
		ctx_add("wine", "dwine.dll", 10, 0, 0);
	}
	
	GetSystemDirectoryA(tmppath, MAX_PATH);
	strcat(tmppath, "\\ddsys.dll");
	if(FileExists(tmppath))
	{
		ctx_add("msddraw", tmppath, 3*syspmul, 1, 0);
	}
	
	if(isWindows2k()) /* on 9x if oridinal DDL is loaded, cannot be unloaded, so why load in only on 2k+ */
	{
		GetSystemDirectoryA(tmppath, MAX_PATH);
		strcat(tmppath, "\\ddraw.dll");
		if(FileExists(tmppath) && stricmp(tmppath, myfilename) != 0)
		{
			ctx_add("msddraw", tmppath, 2*syspmul, 1, 0);
		}
	}
	
	if(isWindows98me())
	{
		GetSystemDirectoryA(tmppath, MAX_PATH);
		strcat(tmppath, "\\ddr98.dll");
		if(FileExists(tmppath) && stricmp(tmppath, myfilename) != 0)
		{
			ctx_add("msddraw", tmppath, 2*syspmul, 1, 0);
		}
	}
	
	if(isWindows95())
	{
		GetSystemDirectoryA(tmppath, MAX_PATH);
		strcat(tmppath, "\\ddr95.dll");
		if(FileExists(tmppath) && stricmp(tmppath, myfilename) != 0)
		{
			ctx_add("msddraw", tmppath, 2*syspmul, 1, 0);
		}
	}
	
	GetSystemDirectoryA(tmppath, MAX_PATH);
	strcat(tmppath, "\\dwine.dll");
	if(FileExists(tmppath))
	{
		ctx_add("wine", "dwine.dll", 8, 0, 0);
	}
}

static int ChooseLib(int need_system)
{
	size_t i;
	int pp = 0;
	int id = 0;
	
	for(i = 0; i < switcher_ctx.ddlinks_cnt; i++)
	{
		if(switcher_ctx.ddlinks[i].priority > pp)
		{
			if(switcher_ctx.ddlinks[i].usable)
			{
				if(need_system)
				{
					if(switcher_ctx.ddlinks[i].system)
					{
						pp = switcher_ctx.ddlinks[i].priority;
						id = i;
					}
				}
				else
				{
					pp = switcher_ctx.ddlinks[i].priority;
					id = i;
				}
			}
		}
	}
	
	return id;	
}

const char *blacklist[] = {
	"iexplore.exe",
	"explorer.exe",
	NULL
};

static BOOL Blacklisted()
{
	const char *exe = getExeName();
	const char *item;
	int i;
	
	if(exe != NULL)
	{
		DBG_printf("Exe: %s\n", exe);
			
		for(item = blacklist[0], i = 0; item != NULL; item = blacklist[++i])
		{
			if(stricmp(exe, item) == 0)
			{
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

static BOOL CallLoop(BYTE *returnaddress)
{
	BOOL r = FALSE;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	MODULEENTRY32 modentry = { 0 };
	
	if(hSnapshot != NULL)
	{
		modentry.dwSize = sizeof(MODULEENTRY32);
		Module32First(hSnapshot, &modentry);
		do
		{
			if ((modentry.modBaseAddr <= returnaddress) &&
				(modentry.modBaseAddr + modentry.modBaseSize > returnaddress))
			{
				/* WARNING: check for module name case insensitive! */
				if(stricmp(modentry.szModule, "opengl32.dll") == 0 ||
					stricmp(modentry.szModule, "mesa3d.dll") == 0)
				{
					DBG_printf("backfire <- %s\n", modentry.szModule);
					
					r = TRUE;
					break;
				}
			}
		} while (Module32Next(hSnapshot, &modentry));
			
		CloseHandle(hSnapshot);
	}
	
	DBG_printf("CallLoop: %d\n", r);
	return r;
}

void InitLib(BYTE *addr)
{
	int need_system = 0;
	int id = 0;
	ddlink_t *lnk = NULL;
	
	if(!TEST_MAGIC()) return;

#ifdef DEBUG
	SetUnhandledExceptionFilter(NULL);
#endif
	
	do
	{
		if(CallLoop(addr)) need_system = 1;
		if(Blacklisted()) need_system  = 1;
	
		id = ChooseLib(need_system);
		
		lnk = &(switcher_ctx.ddlinks[id]);
	
		if(!lnk->loaded)
		{
			DBG_printf("Loading: %s\n", lnk->dllname);
			
			lnk->dll = LoadLibraryA(lnk->dllname);
			
			if(!lnk->dll)
			{
				DWORD err = GetLastError();
				DBG_printf("Load Failure: %d\nFailed DLL: %s\n", lnk->dllname);
				lnk->usable = 0;
				lnk = NULL;
			}
			else
			{
				lnk->loaded = 1;
				LoadDDraw(lnk->dll, &(lnk->lib));
			}
		}
		
		if(lnk)
		{
			ddraw_active_lib = &(lnk->lib);
		}
		
	} while(id != 0 && lnk == NULL);
}

static void dump_switcher()
{
	int i;
	int t;
	for(i = 0; i < switcher_ctx.ddlinks_cnt; i++)
	{
		DBG_printf("%d\n", i);
		DBG_printf("--------\n");
		DBG_printf("%s\n%s\n", switcher_ctx.ddlinks[i].name, switcher_ctx.ddlinks[i].dllname);
		DBG_printf("system: %d\n", switcher_ctx.ddlinks[i].system);
		DBG_printf("========\n");
	}
	
	t = ChooseLib(0);
	DBG_printf("Default: %d\n", t);
	
	t = ChooseLib(1);
	DBG_printf("Default system: %d\n", t);
}


BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
  switch (reason)
  {
    case DLL_PROCESS_ATTACH:
    	#ifdef DEBUG
    	log_mutex = CreateMutexA(NULL, FALSE, "global\\ddswmux");
    	#endif
    	InitSwitcher(inst);
    	#ifdef DEBUG
    	dump_switcher();
    	#endif
    	break;
    case DLL_PROCESS_DETACH:
    	break;
   }
	
	return TRUE;
}

void printFunc(uintptr_t pos)
{
	unsigned int index = pos / 4;
	DBG_printf("DLL Call: %s\n", dddraw_names[index]);
}
