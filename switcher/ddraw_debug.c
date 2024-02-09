#include <stdint.h>
#include <stdarg.h>
#include <windows.h>

#include <ddraw.h>

#include "switcher.h"

#include "nocrt.h"

#include "switcher_mf_name.h"
const char *fn_names[] = {
	#include "ddraw_list.h"
};
#include "switcher_mf_clean.h"
