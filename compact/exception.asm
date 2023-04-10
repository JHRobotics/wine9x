%define E_INVALIDARG 0x80070057
extern _oldexcepfilter
extern _SetUnhandledExceptionFilter@4

section .text

global _setMingwRecoveryPoint
_setMingwRecoveryPoint:
  ; save actual ebp
  mov [saved_ebp],ebp
  ret

global _DirectDrawEnumerateExReturn
_DirectDrawEnumerateExReturn:
  ; recover old exception filter;
  mov eax,[_oldexcepfilter]
  push eax
  call _SetUnhandledExceptionFilter@4
  
  ; load error code from stack (__cdecl)
  mov eax,[esp-4]
  
  ; restore stack frame and return from function
  mov ebp,[saved_ebp]
  mov esp,ebp
  pop ebp
  retn 12 ; (LPDDENUMCALLBACKEXA callback, void *context, DWORD flags)

section .data

saved_ebp: dd 0

