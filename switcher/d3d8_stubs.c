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
	#include "d3d8_list.h"
} fnlistStruct;
#pragma pack(pop)

#include "switcher_mf_clean.h"

/* generate function stubs */
#include "switcher_mf_stub.h"
	#include "d3d8_list.h"
#include "switcher_mf_clean.h"

/* declade and define the list */
#include "switcher_mf_init.h"

fnlistStruct fnlist = {
	#include "d3d8_list.h"
};

fnlistStruct fnlist_sys = {
	#include "d3d8_list.h"
};

#include "switcher_mf_clean.h"

