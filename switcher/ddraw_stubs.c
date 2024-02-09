#include <stdint.h>
#include <stdarg.h>
#include <windows.h>

#include <ddraw.h>

#include "switcher.h"

#include "nocrt.h"

/* generate the list type */
#include "switcher_mf_struct.h"

#pragma pack(push)
#pragma pack(1)
typedef struct _fnlistStruct {
	#include "ddraw_list.h"
} fnlistStruct;
#pragma pack(pop)

#include "switcher_mf_clean.h"

/* generate function stubs */
#include "switcher_mf_stub.h"
	#include "ddraw_list.h"
#include "switcher_mf_clean.h"

/* declade and define the list */
#include "switcher_mf_init.h"

fnlistStruct fnlist = {
	#include "ddraw_list.h"
};

fnlistStruct fnlist_sys = {
	#include "ddraw_list.h"
};

#include "switcher_mf_clean.h"


/* 32 <-> 16 data stubs */
DWORD thk1632_ThunkData32[16] = {0};
DWORD thk3216_ThunkData32[16] = {0};
