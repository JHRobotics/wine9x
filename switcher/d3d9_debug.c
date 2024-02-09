#include <stdint.h>
#include <stdarg.h>
#include <windows.h>

#include "switcher.h"

#include "nocrt.h"

#include "switcher_mf_name.h"
const char *fn_names[] = {
	#include "d3d9_list.h"
};
#include "switcher_mf_clean.h"
