# WineD3D port for Windows 9x

This is port of Wine 1.7.55 to Windows 95/98/Me. It requires OpenGL 2.1 with BGRA extensions.

## Full accelerated package
This is only DirectX wrapper, if you need ready-to-use package for running DirectX, OpenGL or Glide application and games use this: https://github.com/JHRobotics/softgpu

## Manual installation
**Important: some application still need original DDRAW.DLL, don't just overwrite it!**

0) Install latest DirectX redistributable (DX8 for Windows 95, DX9 for Windows 98/Me)
1) Copy ddraw.dll to ddsys.dll
2) Edit ddraw.dll in hex editor and replace all (ASCII) "DDRAW.DLL" to "DDSYS.DLL" (this is because system DDRAW register some function for 16-bit applications, if this registration goes to wrong DLL, results are fatal!)
3) Now copy all files to WINDOWS/SYSTEM and overwrite existing
4) Windows Me only: you cannot overwrite DDRAW.DLL, but is hack for it, add this to registry (ddrawme.reg):
```
REGEDIT4

[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\SessionManager\KnownDLLs]
"DDRAW"="ddrawme.dll"
```
This basically place `ddrawme.dll` instead `ddraw.dll` for most applications.
5) Run `dxdiag` and check result

## Files
- `wined3d.dll` - main runtime
- `d3d8.dll` - DX8 interface
- `d3d9.dll` - DX9 interface
- `ddraw.dll` - switching library between Wine replacement and system (if needed)
- `dwine.dll` - DirectDraw, DX6, DX7 interface
- `ddrawme.dll` - same as `ddraw.dll` if this DLL cannot be replaced (Windows Me)

## DirectDraw switcher
Some DirectDraw application are broken when run in Wine (Half-Life for example), bud most of need doesn't require 3D and works with 2D software rendering. To switch individual application from Wine to System DDraw, set under `HKLM\Software\DDSwitcher` string key with *application exe name* to `system`. Example for already mentioned Half-Life:
```
REGEDIT4

[HKEY_LOCAL_MACHINE\Software\DDSwitcher]
"hl.exe"="system"
```

## Compile from source

You need:
- MinGW
- GNU Make
- NASM

Copy `config.mk-sample` to `config.mk`, now edit this file and adjust settings. After run make:
```
make
```
