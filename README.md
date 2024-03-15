# WineD3D port for Windows 9x

This is port of Wine 1.7.55 to Windows 95/98/Me. It requires OpenGL 2.1 with BGRA extensions.

## Full accelerated package
This is only DirectX wrapper, if you need ready-to-use package for running DirectX, OpenGL or Glide application and games use this: https://github.com/JHRobotics/softgpu

## Manual installation (Windows 9x)
**Important: some application still need original DDRAW.DLL, don't just overwrite it!**

0) Install latest DirectX redistributable (DX8 for Windows 95, DX9 for Windows 98/Me
1) Go to `WINDOWS\system` folder
2) Copy `ddraw.dll` to `ddsys.dll`
3) Edit `ddraw.dll` in hex editor and replace all (ASCII) "DDRAW.DLL" to "DDSYS.DLL" (this is because system DDRAW register some function for 16-bit applications, if this registration goes to wrong DLL, results are fatal!)
4) Rename `d3d8.dll` to `msd3d8.dll`
5) Rename `d3d9.dll` to `msd3d9.dll`
6) **(only for 98, 95)** Copy `ddraw_98.dll` to this folder and rename to `ddraw.dll` 
7) **(only for Me)** Copy `ddraw_98.dll` to this folder and rename to `ddrawme.dll` 
8) Copy `d3d8_98.dll` to this folder and rename to `d3d8.dll` 
9) Copy `d3d9_98.dll` to this folder and rename to `d3d9.dll`
10) Copy `wined3d.dll`, `winedd.dll`, `wined8.dll`, `wined9.dll` to this folder
11) **(only for Me)** you cannot overwrite DDRAW.DLL, but is hack for it, add this to registry (`ddrawme.reg`):
```
REGEDIT4

[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\SessionManager\KnownDLLs]
"DDRAW"="ddrawme.dll"
```
This basically place `ddrawme.dll` instead `ddraw.dll` for most applications.

12) Run `dxdiag` and check result


## Windows XP

Although this fork of Wine is mainly designed for use with Windows 9x (95, 98, Me) it is possible to use it with Windows 2k/XP. It is useful when is available OpenGL acceleration but not DirectX acceleration (VirtualBox 6.0) or something in DX support is missing (usually DX6 and DX7: VirtualBox 5.1, VMware Workstation).

### Installation on XP

0) Boot to save mode (hold F8 during POST)
1) Go to `WINDOW/system32`
2) rename `ddraw.dll` to `ddsys.dll` **(this is important!)**
3) Rename `d3d8.dll` to `msd3d8.dll`
4) Rename `d3d9.dll` to `msd3d9.dll`
5) Copy `ddraw_xp.dll` to this folder and rename to `ddraw.dll` 
6) Copy `d3d8_xp.dll` to this folder and rename to `d3d8.dll` 
7) Copy `d3d9_xp.dll` to this folder and rename to `d3d9.dll`
8) Copy `wined3d.dll`, `winedd.dll`, `wined8.dll`, `wined9.dll` to this folder
9) Go to `WINDOW/system32/dllcache` (folder is hidden)
10) Copy new `d3d8.dll`, `d3d9.dll` and `ddraw.dll` from `system32` to here and overwrite files (OR delete these files in this folder). If you don't do it, the system simple recover replaced files back,
11) Reboot to normal mode
12) Run `dxdiag` and check result


### opengl32.dll dependencies

Very important things to known is that in Windows XP system `opengl32.dll` is calling `ddraw.dll` so if you only simply replace `ddraw.dll` with Wine you will get `ddraw.dll` ↔ `opengl32.dll` ↔ `ddraw.dll` ↔ `opengl32.dll` loop. As solution to this problem special `ddraw.dll` which is only wrapper to another DLL but it decide based on stack trace which DLL (Wine or system) call.

[Look to switcher.c to see how it works](blob/main/switcher/switcher.c#L174)

## Files
- `wined3d.dll` - main runtime
- `wined8.dll` - DX8 interface
- `wined9.dll` - DX9 interface
- `winedd.dll` - DirectDraw, DX6, DX7 interface
- `ddraw_95.dll` - switching library between Wine replacement and system (version 4.8.0.400, for 95)
- `ddraw_98.dll`- switching library between Wine replacement and system (version 4.9.0.904, for 98/Me)
- `ddraw_xp.dll` - switching library between Wine replacement and system (version 5.4.0.904, for XP)
- `d3d8_95.dll` - DX8 switcher between Wine and MS DLL (4.8.0.400, 95)
- `d3d8_98.dll` - DX8 switcher between Wine and MS DLL (4.9.0.904, 98)
- `d3d8_xp.dll` - DX8 switcher between Wine and MS DLL (5.4.0.904, XP)
- `d3d9_98.dll` - DX9 switcher between Wine and MS DLL (4.9.0.904, 98)
- `d3d9_xp.dll` - DX9 switcher between Wine and MS DLL (5.4.0.904, XP)

(There is no `d3d9_95.dll`, because DX9 isn't supported on Windows 95, you can use `d3d9_98.dll` if you want experimenting with DX9 and Win95.)


## DLL switcher
Some application are broken when run in Wine (Half-Life for example), bud most of need doesn't require 3D and works with 2D software rendering. To switch individual application from Wine to System DDraw, set under `HKLM\Software\DDSwitcher` string key with *application exe name* to `system`. Example for already mentioned Half-Life:
```
REGEDIT4

[HKEY_LOCAL_MACHINE\Software\DDSwitcher]
"hl.exe"="system"
```

It is also possible do it for DX8 and DX9, only difference is registry key which is `HKEY_LOCAL_MACHINE\Software\D8Switcher` or `HKEY_LOCAL_MACHINE\Software\D9Switcher`.

Possible values are
- `wine` = calls go to Wine DLL
- `system` = calls go to original system DLL
- `nine` = DX9 calls go to mesa99.dll (Mesa3D Nine driver)
- `ninemore` = DX8 and DX9 calls go to Mesa Nine driver
- `nineforce` = same as `ninemore` but force to use Nine driver even if is not supported
- `someother.dll` = file name of another wrapper. Can be use as combination with *SwiftShader* for example.

## Compile from source

You need:
- MinGW
- GNU Make
- NASM

Copy `config.mk-sample` to `config.mk`, now edit this file and adjust settings. After run make:
```
make
```
