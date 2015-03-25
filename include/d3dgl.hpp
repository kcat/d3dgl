#ifndef D3DGL_HPP
#define D3DGL_HPP

#include <atomic>
#include <vector>
#include <map>
#include <d3d9.h>

#include "glew.h"


bool CreateFakeContext(HINSTANCE hInstance, HWND &hWnd, HDC &dc, HGLRC &glrc);


#ifndef D3DFMT4CC
#define D3DFMT4CC(a,b,c,d)  (a | ((b<<8)&0xff00) | ((b<<16)&0xff0000) | ((d<<24)&0xff000000))
#endif

struct GLFormatInfo {
    GLenum internalformat;
    GLenum format;
    GLenum type;
    int bytesperpixel; /* For compressed formats, bytes per block */
    bool color;
};
extern const std::map<DWORD,GLFormatInfo> gFormatList;


class D3DAdapter;

class Direct3DGL : public IDirect3D9 {
    std::atomic<ULONG> mRefCount;

public:
    Direct3DGL();
    virtual ~Direct3DGL();

    bool init();

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj);
    virtual ULONG WINAPI AddRef(void);
    virtual ULONG WINAPI Release(void);

    /*** IDirect3D9 methods ***/
    virtual HRESULT WINAPI RegisterSoftwareDevice(void *initFunction);
    virtual UINT WINAPI GetAdapterCount(void);
    virtual HRESULT WINAPI GetAdapterIdentifier(UINT adapter, DWORD flags, D3DADAPTER_IDENTIFIER9 *identifier);
    virtual UINT WINAPI GetAdapterModeCount(UINT adapter, D3DFORMAT format);
    virtual HRESULT WINAPI EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode, D3DDISPLAYMODE *displayMode);
    virtual HRESULT WINAPI GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE *displayMode);
    virtual HRESULT WINAPI CheckDeviceType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, WINBOOL windowed);
    virtual HRESULT WINAPI CheckDeviceFormat(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, DWORD usage, D3DRESOURCETYPE resType, D3DFORMAT checkFormat);
    virtual HRESULT WINAPI CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT surfaceFormat, WINBOOL windowed, D3DMULTISAMPLE_TYPE multiSampleType, DWORD *qualityLevels);
    virtual HRESULT WINAPI CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat);
    virtual HRESULT WINAPI CheckDeviceFormatConversion(UINT adapter, D3DDEVTYPE devType, D3DFORMAT srcFormat, D3DFORMAT dstFormat);
    virtual HRESULT WINAPI GetDeviceCaps(UINT adapter, D3DDEVTYPE devType, D3DCAPS9 *caps);
    virtual HMONITOR WINAPI GetAdapterMonitor(UINT adapter);
    virtual HRESULT WINAPI CreateDevice(UINT adapter, D3DDEVTYPE devType, HWND focusWindow, DWORD behaviorFlags, D3DPRESENT_PARAMETERS *presentParams, struct IDirect3DDevice9 **iface);
};

#endif /* D3DGL_HPP */
