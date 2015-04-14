#ifndef D3DGL_HPP
#define D3DGL_HPP

#include <atomic>
#include <vector>
#include <map>
#include <d3d9.h>

#include "glew.h"


#define MAX_TEXTURES                8
#define MAX_STREAMS                 16
#define MAX_VERTEX_SAMPLERS         4
#define MAX_FRAGMENT_SAMPLERS       16
#define MAX_COMBINED_SAMPLERS       (MAX_FRAGMENT_SAMPLERS + MAX_VERTEX_SAMPLERS)


bool CreateFakeContext(HINSTANCE hInstance, HWND &hWnd, HDC &dc, HGLRC &glrc);


class D3DAdapter;

class Direct3DGL : public IDirect3D9 {
    std::atomic<ULONG> mRefCount;

public:
    Direct3DGL();
    virtual ~Direct3DGL();

    bool init();

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj) final;
    virtual ULONG WINAPI AddRef() final;
    virtual ULONG WINAPI Release() final;
    /*** IDirect3D9 methods ***/
    virtual HRESULT WINAPI RegisterSoftwareDevice(void *initFunction) final;
    virtual UINT WINAPI GetAdapterCount(void) final;
    virtual HRESULT WINAPI GetAdapterIdentifier(UINT adapter, DWORD flags, D3DADAPTER_IDENTIFIER9 *identifier) final;
    virtual UINT WINAPI GetAdapterModeCount(UINT adapter, D3DFORMAT format) final;
    virtual HRESULT WINAPI EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode, D3DDISPLAYMODE *displayMode) final;
    virtual HRESULT WINAPI GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE *displayMode) final;
    virtual HRESULT WINAPI CheckDeviceType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, WINBOOL windowed) final;
    virtual HRESULT WINAPI CheckDeviceFormat(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, DWORD usage, D3DRESOURCETYPE resType, D3DFORMAT checkFormat) final;
    virtual HRESULT WINAPI CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT surfaceFormat, WINBOOL windowed, D3DMULTISAMPLE_TYPE multiSampleType, DWORD *qualityLevels) final;
    virtual HRESULT WINAPI CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat) final;
    virtual HRESULT WINAPI CheckDeviceFormatConversion(UINT adapter, D3DDEVTYPE devType, D3DFORMAT srcFormat, D3DFORMAT dstFormat) final;
    virtual HRESULT WINAPI GetDeviceCaps(UINT adapter, D3DDEVTYPE devType, D3DCAPS9 *caps) final;
    virtual HMONITOR WINAPI GetAdapterMonitor(UINT adapter) final;
    virtual HRESULT WINAPI CreateDevice(UINT adapter, D3DDEVTYPE devType, HWND focusWindow, DWORD behaviorFlags, D3DPRESENT_PARAMETERS *presentParams, struct IDirect3DDevice9 **iface) final;
};

#endif /* D3DGL_HPP */
