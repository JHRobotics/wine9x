%macro trampoline 1
	global _ %+ %1
	_ %+ %1 :
;		mov eax,[esp]
;		push eax
;		call _initSW
;		add esp,4

		mov eax,fnlistStruct.p %+ %1
		push eax
		call _fnprint
		
		mov eax,[esp]
		push esp
		call _fncheck
		add esp,8
		test eax, eax
		jz %1 %+ _sys
		
			mov eax,_fnlist
			mov eax,[eax + fnlistStruct.p %+ %1 ]
			test eax, eax
			jz %1 %+ _sys
			jmp eax
		
		%1 %+ _sys:
			mov eax,_fnlist_sys
			mov eax,[eax + fnlistStruct.p %+ %1 ]
			jmp eax
		
%endmacro
