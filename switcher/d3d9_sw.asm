%include "switcher.inc"

extern _fnlist
extern _fnlist_sys
extern _fnprint
extern _fncheck

%include "switcher_mf_struct.inc"
struc fnlistStruct
%include "d3d9_list.h"
endstruc
%include "switcher_mf_clean.inc"

%include "switcher_mf_proc.inc"
%include "d3d9_list.h"
%include "switcher_mf_clean.inc"

