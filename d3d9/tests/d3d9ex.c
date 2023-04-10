/*
 * Copyright (C) 2008 Stefan Dösinger(for CodeWeavers)
 * Copyright (C) 2010 Louis Lenders
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* This file contains tests specific to IDirect3D9Ex and IDirect3DDevice9Ex, like
 * how to obtain them. For testing rendering with extended functions use visual.c
 */

#define COBJMACROS
#include "wine/test.h"
#include <initguid.h>
#include <d3d9.h>

static HMODULE d3d9_handle = 0;
static DEVMODEW registry_mode;

static HRESULT (WINAPI *pDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex **d3d9ex);

#define CREATE_DEVICE_FULLSCREEN        0x01
#define CREATE_DEVICE_NOWINDOWCHANGES   0x02

struct device_desc
{
    HWND device_window;
    unsigned int width;
    unsigned int height;
    DWORD flags;
};

static BOOL color_match(D3DCOLOR c1, D3DCOLOR c2, BYTE max_diff)
{
    unsigned int i;

    for (i = 0; i < 4; ++i)
    {
        if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff)
            return FALSE;
        c1 >>= 8;
        c2 >>= 8;
    }
    return TRUE;
}

static DWORD get_pixel_color(IDirect3DDevice9Ex *device, unsigned int x, unsigned int y)
{
    DWORD ret;
    IDirect3DSurface9 *surf = NULL, *target = NULL;
    HRESULT hr;
    D3DLOCKED_RECT locked_rect;
    RECT rect = {x, y, x + 1, y + 1};

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 640, 480,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, NULL);
    if (FAILED(hr) || !surf)
    {
        trace("Can't create an offscreen plain surface to read the render target data, hr %#x.\n", hr);
        return 0xdeadbeef;
    }

    hr = IDirect3DDevice9Ex_GetRenderTarget(device, 0, &target);
    if (FAILED(hr))
    {
        trace("Can't get the render target, hr %#x.\n", hr);
        ret = 0xdeadbeed;
        goto out;
    }

    hr = IDirect3DDevice9Ex_GetRenderTargetData(device, target, surf);
    if (FAILED(hr))
    {
        trace("Can't read the render target data, hr %#x.\n", hr);
        ret = 0xdeadbeec;
        goto out;
    }

    hr = IDirect3DSurface9_LockRect(surf, &locked_rect, &rect, D3DLOCK_READONLY);
    if (FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr %#x.\n", hr);
        ret = 0xdeadbeeb;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different
     * ideas how to treat it apparently, and it isn't really important
     * for these tests. */
    ret = ((DWORD *)locked_rect.pBits)[0] & 0x00ffffff;
    hr = IDirect3DSurface9_UnlockRect(surf);
    if (FAILED(hr))
        trace("Can't unlock the offscreen surface, hr %#x.\n", hr);

out:
    if (target)
        IDirect3DSurface9_Release(target);
    if (surf)
        IDirect3DSurface9_Release(surf);
    return ret;
}

static HWND create_window(void)
{
    WNDCLASSA wc = {0};

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClassA(&wc);

    return CreateWindowA("d3d9_test_wc", "d3d9_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION,
            0, 0, 640, 480, 0, 0, 0, 0);
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static IDirect3DDevice9Ex *create_device(HWND focus_window, const struct device_desc *desc)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9Ex *device;
    D3DDISPLAYMODEEX mode, *m;
    IDirect3D9Ex *d3d9;
    DWORD behavior_flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    if (FAILED(pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9)))
        return NULL;

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (desc)
    {
        present_parameters.BackBufferWidth = desc->width;
        present_parameters.BackBufferHeight = desc->height;
        present_parameters.hDeviceWindow = desc->device_window;
        present_parameters.Windowed = !(desc->flags & CREATE_DEVICE_FULLSCREEN);
        if (desc->flags & CREATE_DEVICE_NOWINDOWCHANGES)
            behavior_flags |= D3DCREATE_NOWINDOWCHANGES;
    }

    mode.Size = sizeof(mode);
    mode.Width = present_parameters.BackBufferWidth;
    mode.Height = present_parameters.BackBufferHeight;
    mode.RefreshRate = 0;
    mode.Format = D3DFMT_A8R8G8B8;
    mode.ScanLineOrdering = 0;

    m = present_parameters.Windowed ? NULL : &mode;
    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, m, &device)))
        goto done;

    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, m, &device)))
        goto done;

    behavior_flags ^= (D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING);

    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, m, &device)))
        goto done;

    device = NULL;

done:
    IDirect3D9Ex_Release(d3d9);
    return device;
}

static HRESULT reset_device(IDirect3DDevice9Ex *device, const struct device_desc *desc)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = NULL;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (desc)
    {
        present_parameters.BackBufferWidth = desc->width;
        present_parameters.BackBufferHeight = desc->height;
        present_parameters.hDeviceWindow = desc->device_window;
        present_parameters.Windowed = !(desc->flags & CREATE_DEVICE_FULLSCREEN);
    }

    return IDirect3DDevice9_Reset(device, &present_parameters);
}

static ULONG getref(IUnknown *obj) {
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

static void test_qi_base_to_ex(void)
{
    IDirect3D9 *d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    IDirect3D9Ex *d3d9ex = (void *) 0xdeadbeef;
    IDirect3DDevice9 *device;
    IDirect3DDevice9Ex *deviceEx = (void *) 0xdeadbeef;
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = (void *)0xdeadbeef;
    HRESULT hr;
    HWND window = create_window();
    D3DPRESENT_PARAMETERS present_parameters;

    if (!d3d9)
    {
        skip("Direct3D9 is not available\n");
        return;
    }

    hr = IDirect3D9_QueryInterface(d3d9, &IID_IDirect3D9Ex, (void **) &d3d9ex);
    ok(hr == E_NOINTERFACE,
       "IDirect3D9::QueryInterface for IID_IDirect3D9Ex returned %08x, expected E_NOINTERFACE\n",
       hr);
    ok(d3d9ex == NULL, "QueryInterface returned interface %p, expected NULL\n", d3d9ex);
    if(d3d9ex) IDirect3D9Ex_Release(d3d9ex);

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == E_NOINTERFACE,
       "IDirect3D9Device::QueryInterface for IID_IDirect3DDevice9Ex returned %08x, expected E_NOINTERFACE\n",
       hr);
    ok(deviceEx == NULL, "QueryInterface returned interface %p, expected NULL\n", deviceEx);
    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08x).\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
        ok(hr == E_NOINTERFACE,
                "IDirect3DSwapChain9::QueryInterface for IID_IDirect3DSwapChain9Ex returned %08x, expected E_NOINTERFACE.\n",
                hr);
        ok(swapchainEx == NULL, "QueryInterface returned interface %p, expected NULL.\n", swapchainEx);
        if (swapchainEx)
            IDirect3DSwapChain9Ex_Release(swapchainEx);
    }
    if (swapchain)
        IDirect3DSwapChain9_Release(swapchain);

    IDirect3DDevice9_Release(device);

out:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_qi_ex_to_base(void)
{
    IDirect3D9 *d3d9 = (void *) 0xdeadbeef;
    IDirect3D9Ex *d3d9ex;
    IDirect3DDevice9 *device;
    IDirect3DDevice9Ex *deviceEx = (void *) 0xdeadbeef;
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = (void *)0xdeadbeef;
    HRESULT hr;
    HWND window = create_window();
    D3DPRESENT_PARAMETERS present_parameters;
    ULONG ref;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Direct3DCreate9Ex returned %08x\n", hr);
    if(FAILED(hr)) {
        skip("Direct3D9Ex is not available\n");
        goto out;
    }

    hr = IDirect3D9Ex_QueryInterface(d3d9ex, &IID_IDirect3D9, (void **) &d3d9);
    ok(hr == D3D_OK,
       "IDirect3D9Ex::QueryInterface for IID_IDirect3D9 returned %08x, expected D3D_OK\n",
       hr);
    ok(d3d9 != NULL && d3d9 != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", d3d9);
    ref = getref((IUnknown *) d3d9ex);
    ok(ref == 2, "IDirect3D9Ex refcount is %d, expected 2\n", ref);
    ref = getref((IUnknown *) d3d9);
    ok(ref == 2, "IDirect3D9 refcount is %d, expected 2\n", ref);

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;

    /* First, try to create a normal device with IDirect3D9Ex::CreateDevice and QI it for IDirect3DDevice9Ex */
    hr = IDirect3D9Ex_CreateDevice(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == D3D_OK,
       "IDirect3D9Device::QueryInterface for IID_IDirect3DDevice9Ex returned %08x, expected D3D_OK\n",
       hr);
    ok(deviceEx != NULL && deviceEx != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", deviceEx);
    ref = getref((IUnknown *) device);
    ok(ref == 2, "IDirect3DDevice9 refcount is %d, expected 2\n", ref);
    ref = getref((IUnknown *) deviceEx);
    ok(ref == 2, "IDirect3DDevice9Ex refcount is %d, expected 2\n", ref);
    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);
    IDirect3DDevice9_Release(device);

    /* Next, try to create a normal device with IDirect3D9::CreateDevice(non-ex) and QI it */
    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == D3D_OK,
       "IDirect3D9Device::QueryInterface for IID_IDirect3DDevice9Ex returned %08x, expected D3D_OK\n",
       hr);
    ok(deviceEx != NULL && deviceEx != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", deviceEx);
    ref = getref((IUnknown *) device);
    ok(ref == 2, "IDirect3DDevice9 refcount is %d, expected 2\n", ref);
    ref = getref((IUnknown *) deviceEx);
    ok(ref == 2, "IDirect3DDevice9Ex refcount is %d, expected 2\n", ref);

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08x).\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
        ok(hr == D3D_OK,
                "IDirect3DSwapChain9::QueryInterface for IID_IDirect3DSwapChain9Ex returned %08x, expected D3D_OK.\n",
                hr);
        ok(swapchainEx != NULL && swapchainEx != (void *)0xdeadbeef,
                "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef.\n", swapchainEx);
        if (swapchainEx)
            IDirect3DSwapChain9Ex_Release(swapchainEx);
    }
    if (swapchain)
        IDirect3DSwapChain9_Release(swapchain);

    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);
    IDirect3DDevice9_Release(device);

    IDirect3D9_Release(d3d9);
    IDirect3D9Ex_Release(d3d9ex);

out:
    DestroyWindow(window);
}

static void test_get_adapter_luid(void)
{
    HWND window = create_window();
    IDirect3D9Ex *d3d9ex;
    UINT count;
    HRESULT hr;
    LUID luid;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Direct3D9Ex is not available.\n");
        DestroyWindow(window);
        return;
    }

    count = IDirect3D9Ex_GetAdapterCount(d3d9ex);
    if (!count)
    {
        skip("No adapters available.\n");
        IDirect3D9Ex_Release(d3d9ex);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D9Ex_GetAdapterLUID(d3d9ex, D3DADAPTER_DEFAULT, &luid);
    ok(SUCCEEDED(hr), "GetAdapterLUID failed, hr %#x.\n", hr);
    trace("adapter luid: %08x:%08x.\n", luid.HighPart, luid.LowPart);

    IDirect3D9Ex_Release(d3d9ex);
}

static void test_swapchain_get_displaymode_ex(void)
{
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = NULL;
    IDirect3DDevice9Ex *device;
    D3DDISPLAYMODE mode;
    D3DDISPLAYMODEEX mode_ex;
    D3DDISPLAYROTATION rotation;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    if (FAILED(hr))
    {
        skip("Failed to get the implicit swapchain, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
    IDirect3DSwapChain9_Release(swapchain);
    if (FAILED(hr))
    {
        skip("Failed to QI for IID_IDirect3DSwapChain9Ex, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    /* invalid size */
    memset(&mode_ex, 0, sizeof(mode_ex));
    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "GetDisplayModeEx returned %#x instead of D3DERR_INVALIDCALL.\n", hr);

    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);
    rotation = (D3DDISPLAYROTATION)0xdeadbeef;
    /* valid count and valid size */
    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, &rotation);
    ok(SUCCEEDED(hr), "GetDisplayModeEx failed, hr %#x.\n", hr);

    /* compare what GetDisplayMode returns with what GetDisplayModeEx returns */
    hr = IDirect3DSwapChain9Ex_GetDisplayMode(swapchainEx, &mode);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#x.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "Size is %d.\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "Width is %d instead of %d.\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "Height is %d instead of %d.\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d.\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "Format is %x instead of %x.\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0.\n");
    /* Don't know how to compare the rotation in this case, test that it is set */
    ok(rotation != (D3DDISPLAYROTATION)0xdeadbeef, "rotation is %d, expected != 0xdeadbeef.\n", rotation);

    trace("GetDisplayModeEx returned Width = %d, Height = %d, RefreshRate = %d, Format = %x, ScanLineOrdering = %x, rotation = %d.\n",
          mode_ex.Width, mode_ex.Height, mode_ex.RefreshRate, mode_ex.Format, mode_ex.ScanLineOrdering, rotation);

    /* test GetDisplayModeEx with null pointer for D3DDISPLAYROTATION */
    memset(&mode_ex, 0, sizeof(mode_ex));
    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);

    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, NULL);
    ok(SUCCEEDED(hr), "GetDisplayModeEx failed, hr %#x.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "Size is %d.\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "Width is %d instead of %d.\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "Height is %d instead of %d.\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d.\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "Format is %x instead of %x.\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0.\n");

    IDirect3DSwapChain9Ex_Release(swapchainEx);

out:
    if (device)
        IDirect3DDevice9Ex_Release(device);
    DestroyWindow(window);
}

static void test_get_adapter_displaymode_ex(void)
{
    HWND window = create_window();
    IDirect3D9 *d3d9 = (void *) 0xdeadbeef;
    IDirect3D9Ex *d3d9ex;
    UINT count;
    HRESULT hr;
    D3DDISPLAYMODE mode;
    D3DDISPLAYMODEEX mode_ex;
    D3DDISPLAYROTATION rotation;
    DEVMODEW startmode;
    LONG retval;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Direct3D9Ex is not available (%#x)\n", hr);
        DestroyWindow(window);
        return;
    }

    count = IDirect3D9Ex_GetAdapterCount(d3d9ex);
    if (!count)
    {
        skip("No adapters available.\n");
        IDirect3D9Ex_Release(d3d9ex);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D9Ex_QueryInterface(d3d9ex, &IID_IDirect3D9, (void **) &d3d9);
    ok(hr == D3D_OK,
       "IDirect3D9Ex::QueryInterface for IID_IDirect3D9 returned %08x, expected D3D_OK\n",
       hr);
    ok(d3d9 != NULL && d3d9 != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", d3d9);

    memset(&startmode, 0, sizeof(startmode));
    startmode.dmSize = sizeof(startmode);
    retval = EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &startmode, 0);
    ok(retval, "Failed to retrieve current display mode, retval %d.\n", retval);
    if (!retval) goto out;

    startmode.dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT;
    S2(U1(startmode)).dmDisplayOrientation = DMDO_180;
    retval = ChangeDisplaySettingsExW(NULL, &startmode, NULL, 0, NULL);

    if(retval == DISP_CHANGE_BADMODE)
    {
        trace(" Test skipped: graphics mode is not supported\n");
        goto out;
    }

    ok(retval == DISP_CHANGE_SUCCESSFUL,"ChangeDisplaySettingsEx failed with %d\n", retval);
    /* try retrieve orientation info with EnumDisplaySettingsEx*/
    startmode.dmFields = 0;
    S2(U1(startmode)).dmDisplayOrientation = 0;
    ok(EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &startmode, EDS_ROTATEDMODE), "EnumDisplaySettingsEx failed\n");

    /*now that orientation has changed start tests for GetAdapterDisplayModeEx: invalid Size*/
    memset(&mode_ex, 0, sizeof(mode_ex));
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "GetAdapterDisplayModeEx returned %#x instead of D3DERR_INVALIDCALL\n", hr);

    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);
    /* invalid count*/
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, count + 1, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "GetAdapterDisplayModeEx returned %#x instead of D3DERR_INVALIDCALL\n", hr);
    /*valid count and valid Size*/
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, &rotation);
    ok(SUCCEEDED(hr), "GetAdapterDisplayModeEx failed, hr %#x.\n", hr);

    /* Compare what GetAdapterDisplayMode returns with what GetAdapterDisplayModeEx returns*/
    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &mode);
    ok(SUCCEEDED(hr), "GetAdapterDisplayMode failed, hr %#x.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "size is %d\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "width is %d instead of %d\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "height is %d instead of %d\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "format is %x instead of %x\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetAdapterDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0\n");
    /* Check that orientation is returned correctly by GetAdapterDisplayModeEx
     * and EnumDisplaySettingsEx(). */
    todo_wine ok(S2(U1(startmode)).dmDisplayOrientation == DMDO_180 && rotation == D3DDISPLAYROTATION_180,
            "rotation is %d instead of %d\n", rotation, S2(U1(startmode)).dmDisplayOrientation);

    trace("GetAdapterDisplayModeEx returned Width = %d,Height = %d, RefreshRate = %d, Format = %x, ScanLineOrdering = %x, rotation = %d\n",
          mode_ex.Width, mode_ex.Height, mode_ex.RefreshRate, mode_ex.Format, mode_ex.ScanLineOrdering, rotation);

    /* test GetAdapterDisplayModeEx with null pointer for D3DDISPLAYROTATION */
    memset(&mode_ex, 0, sizeof(mode_ex));
    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);

    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, NULL);
    ok(SUCCEEDED(hr), "GetAdapterDisplayModeEx failed, hr %#x.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "size is %d\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "width is %d instead of %d\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "height is %d instead of %d\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "format is %x instead of %x\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetAdapterDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0\n");

    /* return to the default mode */
    ChangeDisplaySettingsExW(NULL, NULL, NULL, 0, NULL);
out:
    IDirect3D9_Release(d3d9);
    IDirect3D9Ex_Release(d3d9ex);
}

static void test_user_memory(void)
{
    static const struct
    {
        float x, y, z;
        float u, v;
    }
    quad[] =
    {
        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
        {-1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f, 0.0f},
    };
    IDirect3DDevice9Ex *device;
    IDirect3DTexture9 *texture, *texture2;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DVolumeTexture9 *volume_texture;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    UINT refcount;
    HWND window;
    HRESULT hr;
    void *mem;
    char *ptr;
    D3DCAPS9 caps;
    unsigned int x, y;
    D3DCOLOR color;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);

    mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 128 * 128 * 4);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 1, 1, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 2, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
    ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    if (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        hr = IDirect3DDevice9Ex_CreateCubeTexture(device, 2, 1, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM, &cube_texture, &mem);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }
    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        hr = IDirect3DDevice9Ex_CreateVolumeTexture(device, 2, 2, 2, 1, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM, &volume_texture, &mem);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }

    hr = IDirect3DDevice9Ex_CreateIndexBuffer(device, 16, 0, D3DFMT_INDEX32, D3DPOOL_SYSTEMMEM,
            &index_buffer, &mem);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, 16, 0, 0, D3DPOOL_SYSTEMMEM,
            &vertex_buffer, &mem);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &surface, &mem);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
    ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &surface, &mem, 0);
    todo_wine ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
        ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
        IDirect3DSurface9_Release(surface);
    }

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &surface, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &surface, &mem, 0);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    ptr = mem;
    for (y = 0; y < 33; ++y)
        for (x = 0; x < 33; ++x)
            *ptr++ = x * 255 / 32;

    hr = IDirect3DDevice9Ex_CreateTexture(device, 33, 33, 1, 0, D3DFMT_L8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    ok(locked_rect.Pitch == 33, "Got unexpected pitch %d.\n", locked_rect.Pitch);
    ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_CreateTexture(device, 33, 33, 1, 0, D3DFMT_L8,
            D3DPOOL_DEFAULT, &texture2, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)texture,
            (IDirect3DBaseTexture9 *)texture2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x000000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_pixel_color(device, 320, 240);
    ok(color_match(color, 0x007f7f7f, 2), "Got unexpected color %#x.\n", color);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture2);
    IDirect3DTexture9_Release(texture);
    HeapFree(GetProcessHeap(), 0, mem);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

done:
    DestroyWindow(window);
}

static void test_reset(void)
{
    static const DWORD simple_vs[] =
    {
        0xfffe0101,                                     /* vs_1_1             */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position0 v0   */
        0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000, /* dp4 oPos.x, v0, c0 */
        0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001, /* dp4 oPos.y, v0, c1 */
        0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002, /* dp4 oPos.z, v0, c2 */
        0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003, /* dp4 oPos.w, v0, c3 */
        0x0000ffff,                                     /* end                */
    };

    DWORD height, orig_height = GetSystemMetrics(SM_CYSCREEN);
    DWORD width, orig_width = GetSystemMetrics(SM_CXSCREEN);
    IDirect3DVertexShader9 *shader;
    IDirect3DSwapChain9 *swapchain;
    D3DDISPLAYMODE d3ddm, d3ddm2;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9Ex *device;
    IDirect3DSurface9 *surface;
    UINT i, adapter_mode_count;
    IDirect3D9 *d3d9;
    D3DVIEWPORT9 vp;
    D3DCAPS9 caps;
    UINT refcount;
    DWORD value;
    HWND window;
    HRESULT hr;
    RECT rect;
    struct
    {
        UINT w;
        UINT h;
    } *modes = NULL;
    UINT mode_count = 0;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_GetDirect3D(device, &d3d9);
    ok(SUCCEEDED(hr), "Failed to get d3d9, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &d3ddm);
    adapter_mode_count = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, d3ddm.Format);
    modes = HeapAlloc(GetProcessHeap(), 0, sizeof(*modes) * adapter_mode_count);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        UINT j;

        hr = IDirect3D9_EnumAdapterModes(d3d9, D3DADAPTER_DEFAULT, d3ddm.Format, i, &d3ddm2);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#x.\n", hr);

        for (j = 0; j < mode_count; ++j)
        {
            if (modes[j].w == d3ddm2.Width && modes[j].h == d3ddm2.Height)
                break;
        }
        if (j == mode_count)
        {
            modes[j].w = d3ddm2.Width;
            modes[j].h = d3ddm2.Height;
            ++mode_count;
        }

        /* We use them as invalid modes. */
        if ((d3ddm2.Width == 801 && d3ddm2.Height == 600)
                || (d3ddm2.Width == 32 && d3ddm2.Height == 32))
        {
            skip("This system supports a screen resolution of %dx%d, not running mode tests.\n",
                 d3ddm2.Width, d3ddm2.Height);
            goto cleanup;
        }
    }

    if (mode_count < 2)
    {
        skip("Less than 2 modes supported, skipping mode tests.\n");
        goto cleanup;
    }

    i = 0;
    if (modes[i].w == orig_width && modes[i].h == orig_height)
        ++i;

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = FALSE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Got screen width %u, expected %u.\n", width, modes[i].w);
    ok(height == modes[i].h, "Got screen height %u, expected %u.\n", height, modes[i].h);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].w && rect.bottom == modes[i].h,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %u.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == modes[i].w, "Got vp.Width %u, expected %u.\n", vp.Width, modes[i].w);
    ok(vp.Height == modes[i].h, "Got vp.Height %u, expected %u.\n", vp.Height, modes[i].h);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp,MaxZ %.8e.\n", vp.MaxZ);

    i = 1;
    vp.X = 10;
    vp.Y = 20;
    vp.MinZ = 2.0f;
    vp.MaxZ = 3.0f;
    hr = IDirect3DDevice9Ex_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    SetRect(&rect, 10, 20, 30, 40);
    hr = IDirect3DDevice9Ex_SetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to set scissor rect, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    /* Render states are preserved in d3d9ex. */
    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].w && rect.bottom == modes[i].h,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %u.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == modes[i].w, "Got vp.Width %u, expected %u.\n", vp.Width, modes[i].w);
    ok(vp.Height == modes[i].h, "Got vp.Height %u, expected %u.\n", vp.Height, modes[i].h);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp,MaxZ %.8e.\n", vp.MaxZ);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Got screen width %u, expected %u.\n", width, modes[i].w);
    ok(height == modes[i].h, "Got screen height %u, expected %u.\n", height, modes[i].h);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == modes[i].w, "Got backbuffer width %u, expected %u.\n",
            d3dpp.BackBufferWidth, modes[i].w);
    ok(d3dpp.BackBufferHeight == modes[i].h, "Got backbuffer height %u, expected %u.\n",
            d3dpp.BackBufferHeight, modes[i].h);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Got screen width %u, expected %u.\n", width, orig_width);
    ok(height == orig_height, "Got screen height %u, expected %u.\n", height, orig_height);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 400 && rect.bottom == 300,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %u.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 400, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 300, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp,MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == 400, "Got unexpected backbuffer width %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 300, "Got unexpected backbuffer height %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    SetRect(&rect, 0, 0, 200, 150);
    ok(AdjustWindowRect(&rect, GetWindowLongW(window, GWL_STYLE), FALSE), "Failed to adjust window rect.\n");
    ok(SetWindowPos(window, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER), "Failed to set window position.\n");

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 0;
    d3dpp.BackBufferHeight = 0;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    ok(d3dpp.BackBufferWidth == 200, "Got unexpected BackBufferWidth %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 150, "Got unexpected BackBufferHeight %u.\n", d3dpp.BackBufferHeight);
    ok(d3dpp.BackBufferFormat == d3ddm.Format, "Got unexpected BackBufferFormat %#x, expected %#x.\n",
            d3dpp.BackBufferFormat, d3ddm.Format);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", d3dpp.BackBufferCount);
    ok(!d3dpp.MultiSampleType, "Got unexpected MultiSampleType %u.\n", d3dpp.MultiSampleType);
    ok(!d3dpp.MultiSampleQuality, "Got unexpected MultiSampleQuality %u.\n", d3dpp.MultiSampleQuality);
    ok(d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", d3dpp.SwapEffect);
    ok(!d3dpp.hDeviceWindow, "Got unexpected hDeviceWindow %p.\n", d3dpp.hDeviceWindow);
    ok(d3dpp.Windowed, "Got unexpected Windowed %#x.\n", d3dpp.Windowed);
    ok(!d3dpp.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", d3dpp.EnableAutoDepthStencil);
    ok(!d3dpp.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", d3dpp.AutoDepthStencilFormat);
    ok(!d3dpp.Flags, "Got unexpected Flags %#x.\n", d3dpp.Flags);
    ok(!d3dpp.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            d3dpp.FullScreen_RefreshRateInHz);
    ok(!d3dpp.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", d3dpp.PresentationInterval);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 200 && rect.bottom == 150,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %u.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 200, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 150, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp,MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == 200, "Got unexpected backbuffer width %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 150, "Got unexpected backbuffer height %u.\n", d3dpp.BackBufferHeight);
    ok(d3dpp.BackBufferFormat == d3ddm.Format, "Got unexpected BackBufferFormat %#x, expected %#x.\n",
            d3dpp.BackBufferFormat, d3ddm.Format);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", d3dpp.BackBufferCount);
    ok(!d3dpp.MultiSampleType, "Got unexpected MultiSampleType %u.\n", d3dpp.MultiSampleType);
    ok(!d3dpp.MultiSampleQuality, "Got unexpected MultiSampleQuality %u.\n", d3dpp.MultiSampleQuality);
    ok(d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", d3dpp.SwapEffect);
    ok(d3dpp.hDeviceWindow == window, "Got unexpected hDeviceWindow %p, expected %p.\n", d3dpp.hDeviceWindow, window);
    ok(d3dpp.Windowed, "Got unexpected Windowed %#x.\n", d3dpp.Windowed);
    ok(!d3dpp.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", d3dpp.EnableAutoDepthStencil);
    ok(!d3dpp.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", d3dpp.AutoDepthStencilFormat);
    ok(!d3dpp.Flags, "Got unexpected Flags %#x.\n", d3dpp.Flags);
    ok(!d3dpp.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            d3dpp.FullScreen_RefreshRateInHz);
    ok(!d3dpp.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", d3dpp.PresentationInterval);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;

    /* Reset with resources in the default pool succeeds in d3d9ex. */
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        IDirect3DVolumeTexture9 *volume_texture;

        hr = IDirect3DDevice9Ex_CreateVolumeTexture(device, 16, 16, 4, 1, 0,
                D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &volume_texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
        hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
        hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
        ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
        IDirect3DVolumeTexture9_Release(volume_texture);
    }
    else
    {
        skip("Volume textures not supported.\n");
    }

    /* Scratch and sysmem pools are fine too. */
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    /* The depth stencil should get reset to the auto depth stencil when present. */
    hr = IDirect3DDevice9Ex_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil surface, hr %#x.\n", hr);

    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_GetDepthStencilSurface(device, &surface);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil surface, hr %#x.\n", hr);
    ok(!!surface, "Depth/stencil surface should not be NULL.\n");
    IDirect3DSurface9_Release(surface);

    d3dpp.EnableAutoDepthStencil = FALSE;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_GetDepthStencilSurface(device, &surface);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!surface, "Depth/stencil surface should be NULL.\n");

    /* References to implicit surfaces are allowed in d3d9ex. */
    hr = IDirect3DDevice9Ex_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    /* Shaders are fine. */
    hr = IDirect3DDevice9Ex_CreateVertexShader(device, simple_vs, &shader);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
    IDirect3DVertexShader9_Release(shader);

    /* Try setting invalid modes. */
    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 32;
    d3dpp.BackBufferHeight = 32;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 801;
    d3dpp.BackBufferHeight = 600;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected cooperative level %#x.\n", hr);

    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &d3ddm);
    ok(SUCCEEDED(hr), "Failed to get display mode, hr %#x.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

cleanup:
    HeapFree(GetProcessHeap(), 0, modes);
    IDirect3D9_Release(d3d9);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_reset_resources(void)
{
    IDirect3DSurface9 *surface, *rt;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9Ex *device;
    unsigned int i;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 128, 128, D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth/stencil surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, surface);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil surface, hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    for (i = 0; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create render target texture %u, hr %#x.\n", i, hr);
        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        ok(SUCCEEDED(hr), "Failed to get surface %u, hr %#x.\n", i, hr);
        IDirect3DTexture9_Release(texture);
        hr = IDirect3DDevice9_SetRenderTarget(device, i, surface);
        ok(SUCCEEDED(hr), "Failed to set render target surface %u, hr %#x.\n", i, hr);
        IDirect3DSurface9_Release(surface);
    }

    hr = reset_device(device, NULL);
    ok(SUCCEEDED(hr), "Failed to reset device.\n");

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &rt);
    ok(SUCCEEDED(hr), "Failed to get back buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p for render target.\n", surface);
    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(rt);

    for (i = 1; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &surface);
        ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    }

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    DestroyWindow(window);
}

static void test_vidmem_accounting(void)
{
    IDirect3DDevice9Ex *device;
    unsigned int i;
    HWND window;
    HRESULT hr = D3D_OK;
    ULONG ref;
    UINT vidmem_start, vidmem_end;
    INT diff;
    IDirect3DTexture9 *textures[20];

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    vidmem_start = IDirect3DDevice9_GetAvailableTextureMem(device);
    memset(textures, 0, sizeof(textures));
    for (i = 0; i < 20 && SUCCEEDED(hr); i++)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 1024, 1024, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &textures[i], NULL);
        /* No D3DERR_OUTOFVIDEOMEMORY in d3d9ex */
        ok(SUCCEEDED(hr) || hr == E_OUTOFMEMORY, "Failed to create texture, hr %#x.\n", hr);
    }
    vidmem_end = IDirect3DDevice9_GetAvailableTextureMem(device);

    diff = vidmem_start - vidmem_end;
    diff = abs(diff);
    ok(diff < 1024 * 1024, "Expected a video memory difference of less than 1 MB, got %u MB.\n",
            diff / 1024 / 1024);

    for (i = 0; i < 20; i++)
    {
        if (textures[i])
            IDirect3DTexture9_Release(textures[i]);
    }

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    DestroyWindow(window);
}

static void test_user_memory_getdc(void)
{
    IDirect3DDevice9Ex *device;
    HWND window;
    HRESULT hr;
    ULONG ref;
    IDirect3DSurface9 *surface;
    DWORD *data;
    HDC dc;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data) * 16 * 16);
    memset(data, 0xaa, sizeof(*data) * 16 * 16);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, (HANDLE *)&data);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DSurface9_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get dc, hr %#x.\n", hr);
    BitBlt(dc, 0, 0, 16, 8, NULL, 0, 0, WHITENESS);
    BitBlt(dc, 0, 8, 16, 8, NULL, 0, 0, BLACKNESS);
    hr = IDirect3DSurface9_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release dc, hr %#x.\n", hr);

    ok(data[0] == 0xffffffff, "Expected color 0xffffffff, got %#x.\n", data[0]);
    ok(data[8 * 16] == 0x00000000, "Expected color 0x00000000, got %#x.\n", data[8 * 16]);

    IDirect3DSurface9_Release(surface);
    HeapFree(GetProcessHeap(), 0, data);

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    DestroyWindow(window);
}

static void test_lost_device(void)
{
    IDirect3DDevice9Ex *device;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;
    struct device_desc desc;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    desc.device_window = window;
    desc.width = 640;
    desc.height = 480;
    desc.flags = CREATE_DEVICE_FULLSCREEN;
    if (!(device = create_device(window, &desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    desc.width = 1024;
    desc.height = 768;
    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    desc.flags = 0;
    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);
    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    DestroyWindow(window);
}

static void test_unsupported_shaders(void)
{
    static const DWORD simple_vs[] =
    {
        0xfffe0101,                             /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,     /* dcl_position0 v0     */
        0x00000001, 0xc00f0000, 0x90e40000,     /* mov oPos, v0         */
        0x0000ffff,                             /* end                  */
    };
    static const DWORD simple_ps[] =
    {
        0xffff0101,                             /* ps_1_1               */
        0x00000001, 0x800f0000, 0x90e40000,     /* mul r0, t0, r0       */
        0x0000ffff,                             /* end                  */
    };
    static const DWORD vs_3_0[] =
    {
        0xfffe0300,                             /* vs_3_0               */
        0x0200001f, 0x80000000, 0x900f0000,     /* dcl_position v0      */
        0x0200001f, 0x80000000, 0xe00f0000,     /* dcl_position o0      */
        0x02000001, 0xe00f0000, 0x90e40000,     /* mov o0, v0           */
        0x0000ffff,                             /* end                  */
    };

#if 0
    float4 main(const float4 color : COLOR) : SV_TARGET
    {
        float4 o;

        o = color;

        return o;
    }
#endif
    static const DWORD ps_4_0[] =
    {
        0x43425844, 0x4da9446f, 0xfbe1f259, 0x3fdb3009, 0x517521fa, 0x00000001, 0x000001ac, 0x00000005,
        0x00000034, 0x0000008c, 0x000000bc, 0x000000f0, 0x00000130, 0x46454452, 0x00000050, 0x00000000,
        0x00000000, 0x00000000, 0x0000001c, 0xffff0400, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f,
        0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
        0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000002, 0x00000000,
        0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000,
    };
#if 0
    vs_1_1
    dcl_position v0
    def c255, 1.0, 1.0, 1.0, 1.0
    add r0, v0, c255
    mov oPos, r0
#endif
    static const DWORD vs_1_255[] =
    {
        0xfffe0101,
        0x0000001f, 0x80000000, 0x900f0000,
        0x00000051, 0xa00f00ff, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x00000002, 0x800f0000, 0x90e40000, 0xa0e400ff,
        0x00000001, 0xc00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    vs_1_1
    dcl_position v0
    def c256, 1.0, 1.0, 1.0, 1.0
    add r0, v0, c256
    mov oPos, r0
#endif
    static const DWORD vs_1_256[] =
    {
        0xfffe0101,
        0x0000001f, 0x80000000, 0x900f0000,
        0x00000051, 0xa00f0100, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x00000002, 0x800f0000, 0x90e40000, 0xa0e40100,
        0x00000001, 0xc00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    vs_3_0
    dcl_position v0
    dcl_position o0
    def c256, 1.0, 1.0, 1.0, 1.0
    add r0, v0, c256
    mov o0, r0
#endif
    static const DWORD vs_3_256[] =
    {
        0xfffe0300,
        0x0200001f, 0x80000000, 0x900f0000,
        0x0200001f, 0x80000000, 0xe00f0000,
        0x05000051, 0xa00f0100, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x03000002, 0x800f0000, 0x90e40000, 0xa0e40100,
        0x02000001, 0xe00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    vs_3_0
    dcl_position v0
    dcl_position o0
    defi i16, 1, 1, 1, 1
    rep i16
    add r0, r0, v0
    endrep
    mov o0, r0
#endif
    static const DWORD vs_3_i16[] =
    {
        0xfffe0300,
        0x0200001f, 0x80000000, 0x900f0000,
        0x0200001f, 0x80000000, 0xe00f0000,
        0x05000030, 0xf00f0010, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x01000026, 0xf0e40010,
        0x03000002, 0x800f0000, 0x80e40000, 0x90e40000,
        0x00000027,
        0x02000001, 0xe00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    vs_3_0
    dcl_position v0
    dcl_position o0
    defb b16, true
    mov r0, v0
    if b16
    add r0, r0, v0
    endif
    mov o0, r0
#endif
    static const DWORD vs_3_b16[] =
    {
        0xfffe0300,
        0x0200001f, 0x80000000, 0x900f0000,
        0x0200001f, 0x80000000, 0xe00f0000,
        0x0200002f, 0xe00f0810, 0x00000001,
        0x02000001, 0x800f0000, 0x90e40000,
        0x01000028, 0xe0e40810,
        0x03000002, 0x800f0000, 0x80e40000, 0x90e40000,
        0x0000002b,
        0x02000001, 0xe00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    ps_1_1
    def c8, 1.0, 1.0, 1.0, 1.0
    add r0, v0, c8
#endif
    static const DWORD ps_1_8[] =
    {
        0xffff0101,
        0x00000051, 0xa00f0008, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x00000002, 0x800f0000, 0x90e40000, 0xa0e40008,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    ps_2_0
    def c32, 1.0, 1.0, 1.0, 1.0
    add oC0, v0, c32
#endif
    static const DWORD ps_2_32[] =
    {
        0xffff0200,
        0x05000051, 0xa00f0020, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x03000002, 0x800f0000, 0x90e40000, 0xa0e40020,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    ps_3_0
    dcl_color0 v0
    def c224, 1.0, 1.0, 1.0, 1.0
    add oC0, v0, c224
#endif
    static const DWORD ps_3_224[] =
    {
        0xffff0300,
        0x0200001f, 0x8000000a, 0x900f0000,
        0x05000051, 0xa00f00e0, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x03000002, 0x800f0800, 0x90e40000, 0xa0e400e0,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    ps_2_0
    defb b0, true
    defi i0, 1, 1, 1, 1
    rep i0
    if b0
    add r0, r0, v0
    endif
    endrep
    mov oC0, r0
#endif
    static const DWORD ps_2_0_boolint[] =
    {
        0xffff0200,
        0x0200002f, 0xe00f0800, 0x00000001,
        0x05000030, 0xf00f0000, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x01000026, 0xf0e40000,
        0x01000028, 0xe0e40800,
        0x03000002, 0x800f0000, 0x80e40000, 0x90e40000,
        0x0000002b,
        0x00000027,
        0x02000001, 0x800f0800, 0x80e40000,
        0x0000ffff
    };

    IDirect3DVertexShader9 *vs = NULL;
    IDirect3DPixelShader9 *ps = NULL;
    IDirect3DDevice9Ex *device;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_CreateVertexShader(device, simple_ps, &vs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, simple_vs, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_4_0, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9Ex_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    if (caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_3_0, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        if (caps.VertexShaderVersion <= D3DVS_VERSION(1, 1) && caps.MaxVertexShaderConst < 256)
        {
            hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_1_255, &vs);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        }
        else
        {
            skip("GPU supports SM2+, skipping SM1 test.\n");
        }

        skip("This GPU doesn't support SM3, skipping test with shader using unsupported constants.\n");
    }
    else
    {
        skip("This GPU supports SM3, skipping unsupported shader test.\n");

        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_1_255, &vs);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        IDirect3DVertexShader9_Release(vs);
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_1_256, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_3_256, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_3_i16, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_3_b16, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }

    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0))
    {
        skip("This GPU doesn't support SM3, skipping test with shader using unsupported constants.\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_1_8, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_2_32, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_3_224, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_2_0_boolint, &ps);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    if (ps)
        IDirect3DPixelShader9_Release(ps);

cleanup:
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static HWND filter_messages;

enum message_window
{
    DEVICE_WINDOW,
    FOCUS_WINDOW,
};

struct message
{
    UINT message;
    enum message_window window;
    BOOL check_wparam;
    WPARAM expect_wparam;
};

static const struct message *expect_messages;
static HWND device_window, focus_window;
static LONG windowposchanged_received, syscommand_received;

struct wndproc_thread_param
{
    HWND dummy_window;
    HANDLE window_created;
    HANDLE test_finished;
    BOOL running_in_foreground;
};

static LRESULT CALLBACK test_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (filter_messages && filter_messages == hwnd)
    {
        if (message != WM_DISPLAYCHANGE && message != WM_IME_NOTIFY)
            todo_wine ok(0, "Received unexpected message %#x for window %p.\n", message, hwnd);
    }

    if (expect_messages)
    {
        HWND w;

        switch (expect_messages->window)
        {
            case DEVICE_WINDOW:
                w = device_window;
                break;

            case FOCUS_WINDOW:
                w = focus_window;
                break;

            default:
                w = NULL;
                break;
        };

        if (hwnd == w && expect_messages->message == message)
        {
            if (expect_messages->check_wparam)
                ok(wparam == expect_messages->expect_wparam,
                        "Got unexpected wparam %lx for message %x, expected %lx.\n",
                        wparam, message, expect_messages->expect_wparam);

            ++expect_messages;
        }
    }

    /* KDE randomly does something with the hidden window during the
     * mode change that sometimes generates a WM_WINDOWPOSCHANGING
     * message. A WM_WINDOWPOSCHANGED message is not generated, so
     * just flag WM_WINDOWPOSCHANGED as bad. */
    if (message == WM_WINDOWPOSCHANGED)
        InterlockedIncrement(&windowposchanged_received);
    else if (message == WM_SYSCOMMAND)
        InterlockedIncrement(&syscommand_received);

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static DWORD WINAPI wndproc_thread(void *param)
{
    struct wndproc_thread_param *p = param;
    DWORD res;
    BOOL ret;

    p->dummy_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, 0, 0, 0, 0);
    p->running_in_foreground = SetForegroundWindow(p->dummy_window);

    ret = SetEvent(p->window_created);
    ok(ret, "SetEvent failed, last error %#x.\n", GetLastError());

    for (;;)
    {
        MSG msg;

        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        res = WaitForSingleObject(p->test_finished, 100);
        if (res == WAIT_OBJECT_0)
            break;
        if (res != WAIT_TIMEOUT)
        {
            ok(0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());
            break;
        }
    }

    DestroyWindow(p->dummy_window);

    return 0;
}

static void test_wndproc(void)
{
    struct wndproc_thread_param thread_params;
    struct device_desc device_desc;
    IDirect3DDevice9Ex *device;
    WNDCLASSA wc = {0};
    HANDLE thread;
    LONG_PTR proc;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;
    UINT i, adapter_mode_count;
    HRESULT hr;
    D3DDISPLAYMODE d3ddm;
    DWORD d3d_width = 0, d3d_height = 0, user32_width = 0, user32_height = 0;
    DEVMODEW devmode;
    LONG change_ret;
    BOOL ret;
    IDirect3D9Ex *d3d9ex;

    static const struct message create_messages[] =
    {
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW,   FALSE,  0},
        /* Do not test wparam here. If device creation succeeds,
         * wparam is WA_ACTIVE. If device creation fails (testbot)
         * wparam is set to WA_INACTIVE on some Windows versions. */
        {WM_ACTIVATE,           FOCUS_WINDOW,   FALSE,  0},
        {WM_SETFOCUS,           FOCUS_WINDOW,   FALSE,  0},
        {0,                     0,              FALSE,  0},
    };
    static const struct message focus_loss_messages[] =
    {
        /* WM_ACTIVATE (wparam = WA_INACTIVE) is sent on Windows. It is
         * not reliable on X11 WMs. When the window focus follows the
         * mouse pointer the message is not sent.
         * {WM_ACTIVATE,           FOCUS_WINDOW,   TRUE,   WA_INACTIVE}, */
        {WM_DISPLAYCHANGE,      DEVICE_WINDOW,  FALSE,  0},
        /* WM_DISPLAYCHANGE is sent to the focus window too, but the order is
         * not deterministic. */
        {WM_WINDOWPOSCHANGING,  DEVICE_WINDOW,  FALSE,  0},
        /* Windows sends WM_ACTIVATE to the device window, indicating that
         * SW_SHOWMINIMIZED is used instead of SW_MINIMIZE. Yet afterwards
         * the foreground and focus window are NULL. On Wine SW_SHOWMINIMIZED
         * leaves the device window active, breaking re-activation in the
         * lost device test.
         * {WM_ACTIVATE,           DEVICE_WINDOW,  TRUE,   0x200000 | WA_ACTIVE}, */
        {WM_WINDOWPOSCHANGED,   DEVICE_WINDOW,  FALSE,  0},
        {WM_SIZE,               DEVICE_WINDOW,  TRUE,   SIZE_MINIMIZED},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE},
        /* WM_ACTIVATEAPP is sent to the device window too, but the order is
         * not deterministic. It may be sent after the focus window handling
         * or before. */
        {0,                     0,              FALSE,  0},
    };
    static const struct message focus_loss_messages_nowc[] =
    {
        /* WM_ACTIVATE (wparam = WA_INACTIVE) is sent on Windows. It is
         * not reliable on X11 WMs. When the window focus follows the
         * mouse pointer the message is not sent.
         * {WM_ACTIVATE,           FOCUS_WINDOW,   TRUE,   WA_INACTIVE}, */
        {WM_DISPLAYCHANGE,      DEVICE_WINDOW,  FALSE,  0},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message focus_loss_messages_hidden[] =
    {
        {WM_DISPLAYCHANGE,      DEVICE_WINDOW,  FALSE,  0},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message focus_loss_messages_filtered[] =
    {
        /* WM_ACTIVATE is delivered to the window proc because it is
         * generated by SetForegroundWindow before the d3d routine
         * starts it work. Don't check for it due to focus-follows-mouse
         * WMs though. */
        {WM_DISPLAYCHANGE,      FOCUS_WINDOW,   FALSE,  0},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message sc_restore_messages[] =
    {
        /* WM_SYSCOMMAND is delivered only once, after d3d has already
         * processed it. Our wndproc has no way to prevent d3d from
         * handling the message. The second DefWindowProc call done by
         * our wndproc doesn't do any changes to the window because it
         * is already restored due to d3d's handling. */
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW,   FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FOCUS_WINDOW,   FALSE,  0},
        {WM_SIZE,               FOCUS_WINDOW,   TRUE,   SIZE_RESTORED},
        {WM_SYSCOMMAND,         FOCUS_WINDOW,   TRUE,   SC_RESTORE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message sc_minimize_messages[] =
    {
        {WM_SYSCOMMAND,         FOCUS_WINDOW,   TRUE,   SC_MINIMIZE},
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW,   FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FOCUS_WINDOW,   FALSE,  0},
        {WM_MOVE,               FOCUS_WINDOW,   FALSE,  0},
        {WM_SIZE,               FOCUS_WINDOW,   TRUE,   SIZE_MINIMIZED},
        {0,                     0,              FALSE,  0},
    };
    static const struct message sc_maximize_messages[] =
    {
        {WM_SYSCOMMAND,         FOCUS_WINDOW,   TRUE,   SC_MAXIMIZE},
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW,   FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FOCUS_WINDOW,   FALSE,  0},
        {WM_MOVE,               FOCUS_WINDOW,   FALSE,  0},
        /* WM_SIZE(SIZE_MAXIMIZED) is unreliable on native. */
        {0,                     0,              FALSE,  0},
    };
    static const struct
    {
        DWORD create_flags;
        const struct message *focus_loss_messages;
        BOOL iconic;
    }
    tests[] =
    {
        {0,                               focus_loss_messages,          TRUE},
        {CREATE_DEVICE_NOWINDOWCHANGES,   focus_loss_messages_nowc,     FALSE},
    };

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Direct3D9Ex is not available (%#x)\n", hr);
        return;
    }

    adapter_mode_count = IDirect3D9Ex_GetAdapterModeCount(d3d9ex, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        hr = IDirect3D9Ex_EnumAdapterModes(d3d9ex, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &d3ddm);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#x.\n", hr);

        if (d3ddm.Width == registry_mode.dmPelsWidth && d3ddm.Height == registry_mode.dmPelsHeight)
            continue;
        /* The r200 driver on Windows XP enumerates modes like 320x200 and 320x240 but
         * refuses to create a device at these sizes. */
        if (d3ddm.Width < 640 || d3ddm.Height < 480)
            continue;

        if (!user32_width)
        {
            user32_width = d3ddm.Width;
            user32_height = d3ddm.Height;
            continue;
        }

        /* Make sure the d3d mode is smaller in width or height and at most
         * equal in the other dimension than the mode passed to
         * ChangeDisplaySettings. Otherwise Windows shrinks the window to
         * the ChangeDisplaySettings parameters + 12. */
        if (d3ddm.Width == user32_width && d3ddm.Height == user32_height)
            continue;
        if (d3ddm.Width <= user32_width && d3ddm.Height <= user32_height)
        {
            d3d_width = d3ddm.Width;
            d3d_height = d3ddm.Height;
            break;
        }
        if (user32_width <= d3ddm.Width && user32_height <= d3ddm.Height)
        {
            d3d_width = user32_width;
            d3d_height = user32_height;
            user32_width = d3ddm.Width;
            user32_height = d3ddm.Height;
            break;
        }
    }

    IDirect3D9Ex_Release(d3d9ex);

    if (!d3d_width)
    {
        skip("Could not find adequate modes, skipping mode tests.\n");
        return;
    }

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmPelsWidth = user32_width;
        devmode.dmPelsHeight = user32_height;
        change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

        focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
                WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, user32_width, user32_height, 0, 0, 0, 0);
        device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
                WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, user32_width, user32_height, 0, 0, 0, 0);
        thread = CreateThread(NULL, 0, wndproc_thread, &thread_params, 0, &tid);
        ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());

        res = WaitForSingleObject(thread_params.window_created, INFINITE);
        ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());

        proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
                (LONG_PTR)test_proc, proc);
        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
                (LONG_PTR)test_proc, proc);

        trace("device_window %p, focus_window %p, dummy_window %p.\n",
                device_window, focus_window, thread_params.dummy_window);

        tmp = GetFocus();
        ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
        if (thread_params.running_in_foreground)
        {
            tmp = GetForegroundWindow();
            ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
                    thread_params.dummy_window, tmp);
        }
        else
            skip("Not running in foreground, skip foreground window test\n");

        flush_events();

        expect_messages = create_messages;

        device_desc.device_window = device_window;
        device_desc.width = d3d_width;
        device_desc.height = d3d_height;
        device_desc.flags = CREATE_DEVICE_FULLSCREEN | tests[i].create_flags;
        if (!(device = create_device(focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            goto done;
        }

        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;

        if (0) /* Disabled until we can make this work in a reliable way on Wine. */
        {
            tmp = GetFocus();
            ok(tmp == focus_window, "Expected focus %p, got %p.\n", focus_window, tmp);
            tmp = GetForegroundWindow();
            ok(tmp == focus_window, "Expected foreground window %p, got %p.\n", focus_window, tmp);
        }
        SetForegroundWindow(focus_window);
        flush_events();

        proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
                (LONG_PTR)test_proc, proc);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n",
                (LONG_PTR)test_proc);

        /* Change the mode while the device is in use and then drop focus. */
        devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmPelsWidth = user32_width;
        devmode.dmPelsHeight = user32_height;
        change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x, i=%u.\n", change_ret, i);

        /* Native needs a present call to pick up the mode change. */
        hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
        todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x, i=%u.\n", hr, i);
        hr = IDirect3DDevice9Ex_CheckDeviceState(device, device_window);
        todo_wine ok(hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#x, i=%u.\n", hr, i);

        expect_messages = tests[i].focus_loss_messages;
        /* SetForegroundWindow is a poor replacement for the user pressing alt-tab or
         * manually changing the focus. It generates the same messages, but the task
         * bar still shows the previous foreground window as active, and the window has
         * an inactive titlebar if reactivated with SetForegroundWindow. Reactivating
         * the device is difficult, see below. */
        SetForegroundWindow(GetDesktopWindow());
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        tmp = GetFocus();
        ok(tmp != device_window, "The device window is active, i=%u.\n", i);
        ok(tmp != focus_window, "The focus window is active, i=%u.\n", i);

        hr = IDirect3DDevice9Ex_CheckDeviceState(device, device_window);
        ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#x, i=%u.\n", hr, i);

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth
                && devmode.dmPelsHeight == registry_mode.dmPelsHeight, "Got unexpect screen size %ux%u.\n",
                devmode.dmPelsWidth, devmode.dmPelsHeight);

        /* In d3d9ex the device and focus windows have to be minimized and restored,
         * otherwise native does not notice that focus has been restored. This is
         * independent of D3DCREATE_NOWINDOWCHANGES. */
        ShowWindow(device_window, SW_MINIMIZE);
        ShowWindow(device_window, SW_RESTORE);

        /* Reactivation messages like in d3d8/9 are random in native d3d9ex.
         * Sometimes they are sent, sometimes they are not (tested on Vista
         * and Windows 7). The minimizing and restoring of the device window
         * may have something to do with this, but if the messages are sent,
         * they are generated by the 3 calls below. */
        ShowWindow(focus_window, SW_MINIMIZE);
        ShowWindow(focus_window, SW_RESTORE);
        /* Set focus twice to make KDE and fvwm in focus-follows-mouse mode happy. */
        SetForegroundWindow(focus_window);
        flush_events();
        SetForegroundWindow(focus_window);
        flush_events();

        /* Calling Reset is not necessary in d3d9ex. */
        hr = IDirect3DDevice9Ex_CheckDeviceState(device, device_window);
        ok(hr == S_OK, "Got unexpected hr %#x, i=%u.\n", hr, i);

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == d3d_width
                && devmode.dmPelsHeight == d3d_height, "Got unexpect screen size %ux%u.\n",
                devmode.dmPelsWidth, devmode.dmPelsHeight);

        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

        ShowWindow(device_window, SW_HIDE);
        flush_events();

        expect_messages = focus_loss_messages_hidden;
        windowposchanged_received = 0;
        SetForegroundWindow(GetDesktopWindow());
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(!windowposchanged_received, "Received WM_WINDOWPOSCHANGED but did not expect it, i=%u.\n", i);
        expect_messages = NULL;
        flush_events();

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth, "Got unexpect width %u.\n", devmode.dmPelsWidth);
        ok(devmode.dmPelsHeight == registry_mode.dmPelsHeight, "Got unexpect height %u.\n", devmode.dmPelsHeight);

        /* SW_SHOWMINNOACTIVE is needed to make FVWM happy. SW_SHOWNOACTIVATE is needed to make windows
         * send SIZE_RESTORED after ShowWindow(SW_SHOWMINNOACTIVE). */
        ShowWindow(focus_window, SW_SHOWNOACTIVATE);
        ShowWindow(focus_window, SW_SHOWMINNOACTIVE);
        flush_events();

        syscommand_received = 0;
        expect_messages = sc_restore_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_RESTORE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(syscommand_received == 1, "Got unexpected number of WM_SYSCOMMAND messages: %d.\n", syscommand_received);
        expect_messages = NULL;
        flush_events();

        expect_messages = sc_minimize_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        flush_events();

        expect_messages = sc_maximize_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        flush_events();

        SetForegroundWindow(GetDesktopWindow());
        ShowWindow(device_window, SW_MINIMIZE);
        ShowWindow(device_window, SW_RESTORE);
        ShowWindow(focus_window, SW_MINIMIZE);
        ShowWindow(focus_window, SW_RESTORE);
        SetForegroundWindow(focus_window);
        flush_events();

        filter_messages = focus_window;
        ref = IDirect3DDevice9Ex_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u, i=%u.\n", ref, i);

        /* Fix up the mode until Wine's device release behavior is fixed. */
        change_ret = ChangeDisplaySettingsW(NULL, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx, i=%u.\n",
                (LONG_PTR)test_proc, proc, i);

        /* Hide the device window. It prevents WM_ACTIVATEAPP messages from being sent
         * on native in the test below. It isn't needed anyways. Creating the third
         * device will show it again. */
        filter_messages = NULL;
        ShowWindow(device_window, SW_HIDE);
        /* Remove the maximized state from the SYSCOMMAND test while we're not
         * interfering with a device. */
        ShowWindow(focus_window, SW_SHOWNORMAL);
        filter_messages = focus_window;

        device_desc.device_window = focus_window;
        if (!(device = create_device(focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            goto done;
        }
        filter_messages = NULL;
        SetForegroundWindow(focus_window); /* For KDE. */
        flush_events();

        expect_messages = focus_loss_messages_filtered;
        windowposchanged_received = 0;
        SetForegroundWindow(GetDesktopWindow());
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(!windowposchanged_received, "Received WM_WINDOWPOSCHANGED but did not expect it, i=%u.\n", i);
        expect_messages = NULL;

        /* The window is iconic even though no message was sent. */
        ok(!IsIconic(focus_window) == !tests[i].iconic,
                "Expected IsIconic %u, got %u, i=%u.\n", tests[i].iconic, IsIconic(focus_window), i);

        ShowWindow(focus_window, SW_SHOWNOACTIVATE);
        ShowWindow(focus_window, SW_SHOWMINNOACTIVE);
        flush_events();

        syscommand_received = 0;
        expect_messages = sc_restore_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_RESTORE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(syscommand_received == 1, "Got unexpected number of WM_SYSCOMMAND messages: %d.\n", syscommand_received);
        expect_messages = NULL;
        flush_events();

        /* For FVWM. */
        ShowWindow(focus_window, SW_RESTORE);
        flush_events();

        expect_messages = sc_minimize_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        flush_events();

        expect_messages = sc_maximize_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        flush_events();

        /* This test can't activate, drop focus and restore focus like in plain d3d9 because d3d9ex
         * immediately restores the device on activation. There are plenty of WM_WINDOWPOSCHANGED
         * messages that are generated by ShowWindow, so testing for their absence is pointless. */
        ShowWindow(focus_window, SW_MINIMIZE);
        ShowWindow(focus_window, SW_RESTORE);
        SetForegroundWindow(focus_window);
        flush_events();

        filter_messages = focus_window;
        ref = IDirect3DDevice9Ex_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u, i=%u.\n", ref, i);

        device_desc.device_window = device_window;
        if (!(device = create_device(focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            goto done;
        }

        proc = SetWindowLongPtrA(focus_window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
        ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n",
                (LONG_PTR)test_proc);

        ref = IDirect3DDevice9Ex_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
                (LONG_PTR)DefWindowProcA, proc);

done:
        filter_messages = NULL;
        DestroyWindow(device_window);
        DestroyWindow(focus_window);
        SetEvent(thread_params.test_finished);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }

    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);

    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_wndproc_windowed(void)
{
    struct wndproc_thread_param thread_params;
    struct device_desc device_desc;
    IDirect3DDevice9Ex *device;
    WNDCLASSA wc = {0};
    HANDLE thread;
    LONG_PTR proc;
    HRESULT hr;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());

    focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, 0, 0, 0, 0);
    thread = CreateThread(NULL, 0, wndproc_thread, &thread_params, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());

    res = WaitForSingleObject(thread_params.window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    trace("device_window %p, focus_window %p, dummy_window %p.\n",
            device_window, focus_window, thread_params.dummy_window);

    tmp = GetFocus();
    ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
    if (thread_params.running_in_foreground)
    {
        tmp = GetForegroundWindow();
        ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
                thread_params.dummy_window, tmp);
    }
    else
        skip("Not running in foreground, skip foreground window test\n");

    filter_messages = focus_window;

    device_desc.device_window = device_window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = 0;
    if (!(device = create_device(focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    tmp = GetFocus();
    ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
    tmp = GetForegroundWindow();
    ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
            thread_params.dummy_window, tmp);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = NULL;

    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = focus_window;

    ref = IDirect3DDevice9Ex_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    filter_messages = device_window;

    device_desc.device_window = focus_window;
    if (!(device = create_device(focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    filter_messages = NULL;

    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = device_window;

    ref = IDirect3DDevice9Ex_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    device_desc.device_window = device_window;
    if (!(device = create_device(focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    filter_messages = NULL;

    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = device_window;

    ref = IDirect3DDevice9Ex_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    filter_messages = NULL;

    SetEvent(thread_params.test_finished);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    CloseHandle(thread);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_window_style(void)
{
    RECT focus_rect, device_rect, fullscreen_rect, r, r2;
    LONG device_style, device_exstyle, expected_style;
    LONG focus_style, focus_exstyle;
    struct device_desc device_desc;
    LONG style;
    IDirect3DDevice9Ex *device;
    HRESULT hr;
    ULONG ref;
    BOOL ret;
    static const struct
    {
        LONG style_flags;
        DWORD device_flags;
        LONG focus_loss_style;
        LONG create2_style, create2_exstyle;
    }
    tests[] =
    {
        {0,             0,                              0,              WS_VISIBLE, WS_EX_TOPMOST},
        {WS_VISIBLE,    0,                              WS_MINIMIZE,    WS_VISIBLE, WS_EX_TOPMOST},
        {0,             CREATE_DEVICE_NOWINDOWCHANGES,  0,              0,          0},
        {WS_VISIBLE,    CREATE_DEVICE_NOWINDOWCHANGES,  0,              0,          0},
    };
    unsigned int i;

    SetRect(&fullscreen_rect, 0, 0, registry_mode.dmPelsWidth, registry_mode.dmPelsHeight);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        focus_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].style_flags,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);
        device_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].style_flags,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);

        device_style = GetWindowLongA(device_window, GWL_STYLE);
        device_exstyle = GetWindowLongA(device_window, GWL_EXSTYLE);
        focus_style = GetWindowLongA(focus_window, GWL_STYLE);
        focus_exstyle = GetWindowLongA(focus_window, GWL_EXSTYLE);

        GetWindowRect(focus_window, &focus_rect);
        GetWindowRect(device_window, &device_rect);

        device_desc.device_window = device_window;
        device_desc.width = registry_mode.dmPelsWidth;
        device_desc.height = registry_mode.dmPelsHeight;
        device_desc.flags = CREATE_DEVICE_FULLSCREEN | tests[i].device_flags;
        if (!(device = create_device(focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            DestroyWindow(device_window);
            DestroyWindow(focus_window);
            return;
        }

        style = GetWindowLongA(device_window, GWL_STYLE);
        todo_wine ok(style == device_style, "Expected device window style %#x, got %#x, i=%u.\n",
                device_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        todo_wine ok(style == device_exstyle, "Expected device window extended style %#x, got %#x, i=%u.\n",
                device_exstyle, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#x, got %#x, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x, i=%u.\n",
                focus_exstyle, style, i);

        GetWindowRect(device_window, &r);
        if (tests[i].device_flags & CREATE_DEVICE_NOWINDOWCHANGES)
            todo_wine ok(EqualRect(&r, &device_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}, i=%u.\n",
                    device_rect.left, device_rect.top, device_rect.right, device_rect.bottom,
                    r.left, r.top, r.right, r.bottom, i);
        else
            ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}, i=%u.\n",
                    fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
                    r.left, r.top, r.right, r.bottom, i);
        GetClientRect(device_window, &r2);
        todo_wine ok(!EqualRect(&r, &r2), "Client rect and window rect are equal, i=%u.\n", i);
        GetWindowRect(focus_window, &r);
        ok(EqualRect(&r, &focus_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}, i=%u.\n",
                focus_rect.left, focus_rect.top, focus_rect.right, focus_rect.bottom,
                r.left, r.top, r.right, r.bottom, i);

        device_desc.flags = 0;
        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

        style = GetWindowLongA(device_window, GWL_STYLE);
        if (tests[i].style_flags & WS_VISIBLE)
            ok(style == device_style, "Expected device window style %#x, got %#x, i=%u.\n",
                    device_style, style, i);
        else
            todo_wine ok(style == device_style, "Expected device window style %#x, got %#x, i=%u.\n",
                    device_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        todo_wine ok(style == device_exstyle, "Expected device window extended style %#x, got %#x, i=%u.\n",
                device_exstyle, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#x, got %#x, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x, i=%u.\n",
                focus_exstyle, style, i);

        ref = IDirect3DDevice9Ex_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

        style = GetWindowLongA(device_window, GWL_STYLE);
        if (device_style & WS_VISIBLE)
            ok(style == device_style, "Expected device window style %#x, got %#x, i=%u.\n",
                device_style, style, i);
        else
            todo_wine ok(style == device_style, "Expected device window style %#x, got %#x, i=%u.\n",
                    device_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        todo_wine ok(style == device_exstyle, "Expected device window extended style %#x, got %#x, i=%u.\n",
                device_exstyle, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#x, got %#x, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x, i=%u.\n",
                focus_exstyle, style, i);

        /* The second time a device is created on the window the window becomes visible and
         * topmost if D3DCREATE_NOWINDOWCHANGES is not set. */
        device_desc.flags = CREATE_DEVICE_FULLSCREEN | tests[i].device_flags;
        device = create_device(focus_window, &device_desc);
        ok(!!device, "Failed to create a D3D device.\n");
        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style | tests[i].create2_style;
        todo_wine ok(style == expected_style, "Expected device window style %#x, got %#x, i=%u.\n",
                expected_style, style, i);
        expected_style = device_exstyle | tests[i].create2_exstyle;
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        todo_wine ok(style == expected_style, "Expected device window extended style %#x, got %#x, i=%u.\n",
                expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#x, got %#x, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x, i=%u.\n",
                focus_exstyle, style, i);
        ref = IDirect3DDevice9Ex_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

        DestroyWindow(device_window);
        DestroyWindow(focus_window);
        focus_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].style_flags,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);
        device_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].style_flags,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);

        device_desc.device_window = device_window;
        device = create_device(focus_window, &device_desc);
        ok(!!device, "Failed to create a D3D device.\n");
        ret = SetForegroundWindow(GetDesktopWindow());
        ok(ret, "Failed to set foreground window.\n");

        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style | tests[i].focus_loss_style;
        todo_wine ok(style == expected_style, "Expected device window style %#x, got %#x, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        todo_wine ok(style == device_exstyle, "Expected device window extended style %#x, got %#x, i=%u.\n",
                device_exstyle, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#x, got %#x, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x, i=%u.\n",
                focus_exstyle, style, i);

        ref = IDirect3DDevice9Ex_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

        DestroyWindow(device_window);
        DestroyWindow(focus_window);
    }
}

static void test_swapchain_parameters(void)
{
    IDirect3DDevice9Ex *device;
    IDirect3D9Ex *d3d9ex;
    HWND window;
    HRESULT hr;
    unsigned int i;
    D3DPRESENT_PARAMETERS present_parameters, present_parameters_windowed = {0}, present_parameters2;
    IDirect3DSwapChain9 *swapchain;
    D3DDISPLAYMODEEX mode = {0};
    static const struct
    {
        BOOL windowed;
        UINT backbuffer_count;
        D3DSWAPEFFECT swap_effect;
        HRESULT hr;
    }
    tests[] =
    {
        /* Swap effect 0 is not allowed. */
        {TRUE,  1,  0,                        D3DERR_INVALIDCALL},
        {FALSE, 1,  0,                        D3DERR_INVALIDCALL},

        /* All (non-ex) swap effects are allowed in
         * windowed and fullscreen mode. */
        {TRUE,  1,  D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  1,  D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 1,  D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {FALSE, 1,  D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 1,  D3DSWAPEFFECT_COPY,       D3D_OK},

        /* Only one backbuffer in copy mode. */
        {TRUE,  0,  D3DSWAPEFFECT_COPY,       D3D_OK},
        {TRUE,  1,  D3DSWAPEFFECT_COPY,       D3D_OK},
        {TRUE,  2,  D3DSWAPEFFECT_COPY,       D3DERR_INVALIDCALL},
        {FALSE, 2,  D3DSWAPEFFECT_COPY,       D3DERR_INVALIDCALL},

        /* Ok with the others, in fullscreen and windowed mode. */
        {TRUE,  2,  D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  2,  D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 2,  D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {FALSE, 2,  D3DSWAPEFFECT_FLIP,       D3D_OK},

        /* D3D9Ex swap effects. Flipex works, Overlay is complicated
         * and depends on HW features, pixel format, etc. */
        {TRUE,  1,  D3DSWAPEFFECT_FLIPEX,     D3D_OK},
        {TRUE,  1,  D3DSWAPEFFECT_FLIPEX + 1, D3DERR_INVALIDCALL},
        {FALSE, 1,  D3DSWAPEFFECT_FLIPEX,     D3D_OK},
        {FALSE, 1,  D3DSWAPEFFECT_FLIPEX + 1, D3DERR_INVALIDCALL},

        /* 30 is the highest allowed backbuffer count. */
        {TRUE,  30, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  31, D3DSWAPEFFECT_DISCARD,    D3DERR_INVALIDCALL},
        {TRUE,  30, D3DSWAPEFFECT_FLIP,       D3D_OK},
        {TRUE,  31, D3DSWAPEFFECT_FLIP,       D3DERR_INVALIDCALL},
        {FALSE, 30, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {FALSE, 31, D3DSWAPEFFECT_DISCARD,    D3DERR_INVALIDCALL},
        {FALSE, 30, D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 31, D3DSWAPEFFECT_FLIP,       D3DERR_INVALIDCALL},
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Direct3D9Ex is not available (%#x)\n", hr);
        return;
    }

    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9Ex_Release(d3d9ex);
        DestroyWindow(window);
        return;
    }
    IDirect3DDevice9Ex_Release(device);

    present_parameters_windowed.BackBufferWidth = registry_mode.dmPelsWidth;
    present_parameters_windowed.BackBufferHeight = registry_mode.dmPelsHeight;
    present_parameters_windowed.hDeviceWindow = window;
    present_parameters_windowed.BackBufferFormat = D3DFMT_X8R8G8B8;
    present_parameters_windowed.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters_windowed.Windowed = TRUE;
    present_parameters_windowed.BackBufferCount = 1;

    mode.Size = sizeof(mode);
    mode.Width = registry_mode.dmPelsWidth;
    mode.Height = registry_mode.dmPelsHeight;
    mode.RefreshRate = 0;
    mode.Format = D3DFMT_X8R8G8B8;
    mode.ScanLineOrdering = 0;

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        memset(&present_parameters, 0, sizeof(present_parameters));
        present_parameters.BackBufferWidth = registry_mode.dmPelsWidth;
        present_parameters.BackBufferHeight = registry_mode.dmPelsHeight;
        present_parameters.hDeviceWindow = window;
        present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;

        present_parameters.SwapEffect = tests[i].swap_effect;
        present_parameters.Windowed = tests[i].windowed;
        present_parameters.BackBufferCount = tests[i].backbuffer_count;

        hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters,
                tests[i].windowed ? NULL : &mode, &device);
        ok(hr == tests[i].hr, "Expected hr %x, got %x, test %u.\n", tests[i].hr, hr, i);
        if (SUCCEEDED(hr))
        {
            UINT bb_count = tests[i].backbuffer_count ? tests[i].backbuffer_count : 1;

            hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
            ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x, test %u.\n", hr, i);

            hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_parameters2);
            ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x, test %u.\n", hr, i);
            ok(present_parameters2.SwapEffect == tests[i].swap_effect, "Swap effect changed from %u to %u, test %u.\n",
                    tests[i].swap_effect, present_parameters2.SwapEffect, i);
            ok(present_parameters2.BackBufferCount == bb_count, "Backbuffer count changed from %u to %u, test %u.\n",
                    bb_count, present_parameters2.BackBufferCount, i);
            ok(present_parameters2.Windowed == tests[i].windowed, "Windowed changed from %u to %u, test %u.\n",
                    tests[i].windowed, present_parameters2.Windowed, i);

            IDirect3DSwapChain9_Release(swapchain);
            IDirect3DDevice9Ex_Release(device);
        }

        hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters_windowed, NULL, &device);
        ok(SUCCEEDED(hr), "Failed to create device, hr %#x, test %u.\n", hr, i);

        memset(&present_parameters, 0, sizeof(present_parameters));
        present_parameters.BackBufferWidth = registry_mode.dmPelsWidth;
        present_parameters.BackBufferHeight = registry_mode.dmPelsHeight;
        present_parameters.hDeviceWindow = window;
        present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;

        present_parameters.SwapEffect = tests[i].swap_effect;
        present_parameters.Windowed = tests[i].windowed;
        present_parameters.BackBufferCount = tests[i].backbuffer_count;

        hr = IDirect3DDevice9Ex_ResetEx(device, &present_parameters, tests[i].windowed ? NULL : &mode);
        ok(hr == tests[i].hr, "Expected hr %x, got %x, test %u.\n", tests[i].hr, hr, i);

        if (FAILED(hr))
        {
            hr = IDirect3DDevice9Ex_ResetEx(device, &present_parameters_windowed, NULL);
            ok(SUCCEEDED(hr), "Failed to reset device, hr %#x, test %u.\n", hr, i);
        }
        IDirect3DDevice9Ex_Release(device);
    }

    IDirect3D9Ex_Release(d3d9ex);
    DestroyWindow(window);
}
START_TEST(d3d9ex)
{
    DEVMODEW current_mode;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    pDirect3DCreate9Ex = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9Ex");
    if (!pDirect3DCreate9Ex) {
        win_skip("Failed to get address of Direct3DCreate9Ex\n");
        return;
    }

    memset(&current_mode, 0, sizeof(current_mode));
    current_mode.dmSize = sizeof(current_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &current_mode), "Failed to get display mode.\n");
    registry_mode.dmSize = sizeof(registry_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &registry_mode), "Failed to get display mode.\n");
    if (current_mode.dmPelsWidth != registry_mode.dmPelsWidth
            || current_mode.dmPelsHeight != registry_mode.dmPelsHeight)
    {
        skip("Current mode does not match registry mode, skipping test.\n");
        return;
    }

    test_qi_base_to_ex();
    test_qi_ex_to_base();
    test_swapchain_get_displaymode_ex();
    test_get_adapter_luid();
    test_get_adapter_displaymode_ex();
    test_user_memory();
    test_reset();
    test_reset_resources();
    test_vidmem_accounting();
    test_user_memory_getdc();
    test_lost_device();
    test_unsupported_shaders();
    test_wndproc();
    test_wndproc_windowed();
    test_window_style();
    test_swapchain_parameters();
}
