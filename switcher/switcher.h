#ifndef __SWITCHER_H__INCLUDED__
#define __SWITCHER_H__INCLUDED__

/* switcher.c */
BOOL isWindows95();
BOOL isWindows98me();
BOOL isWindows2k();
void getSysPath(char *buf, char *dllname);
BOOL fncheck(BYTE *returnaddress, DWORD fn);
void fnprint(DWORD fn);
HMODULE tryLoad(const char *path, const char *check_entry);

/* modules */
void switcherLoadUserLib(HMODULE lib);
void switcherLoadSystemLib(HMODULE lib);
HMODULE switcherGetUserLib(const char *exe, const char *settings);
HMODULE switcherGetSystemLib(const char *exe, const char *settings);

#define RET_ON_SUC(_v, _e) _v=(_e); if(_v != NULL){return _v;}
#define COPY_ON_SUC(_tmp, _dst, _e) _tmp=(_e); if(_tmp != NULL){_dst = _tmp;};

void dbg_printf(const char *fmt, ...);

#ifdef DEBUG
# define DBG(_fmt, ...) dbg_printf(_fmt __VA_OPT__(,) __VA_ARGS__)
#else
# define DBG(_fmt, ...) dbg_printf(_fmt __VA_OPT__(,) __VA_ARGS__)
#endif

/* globals from modules */
extern const char  reg_base[];
extern const char *fn_names[];

#endif /* __SWITCHER_H__INCLUDED__ */
