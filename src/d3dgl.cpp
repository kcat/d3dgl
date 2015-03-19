
#include "d3dgl.hpp"

#include "trace.hpp"


Direct3DGL::Direct3DGL()
{
}

Direct3DGL::~Direct3DGL()
{
}


HRESULT Direct3DGL::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, out %p.\n", this, (const char*)debugstr_guid(riid), obj);

    if(riid == IID_IDirect3D9 || riid == IID_IUnknown)
    {
        AddRef();
        *obj = static_cast<IDirect3D9*>(this);
        return S_OK;
    }
    if(riid == IID_IDirect3D9Ex)
    {
        WARN("Application asks for IDirect3D9Ex, but this instance wasn't created with Direct3DCreate9Ex.\n");
        *obj = nullptr;
        return E_NOINTERFACE;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", (const char*)debugstr_guid(riid));

    *obj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DGL::AddRef(void)
{
    ULONG ret = ++mRefCount;
    TRACE("New refcount: %lu\n", ret);
    return ret;
}

ULONG Direct3DGL::Release(void)
{
    ULONG ret = --mRefCount;
    TRACE("New refcount: %lu\n", ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT Direct3DGL::RegisterSoftwareDevice(void *initFunction)
{
    FIXME("iface %p, init_function %p stub!\n", this, initFunction);

    return D3D_OK;
}


UINT Direct3DGL::GetAdapterCount(void)
{
    FIXME("iface %p stub!\n", this);
    return 0;
}

HRESULT Direct3DGL::GetAdapterIdentifier(UINT adapter, DWORD flags, D3DADAPTER_IDENTIFIER9 *identifier)
{
    FIXME("iface %p, adapter %u, flags 0x%lx, identifier %p stub!\n", this, adapter, flags, identifier);
    return E_NOTIMPL;
}


UINT Direct3DGL::GetAdapterModeCount(UINT adapter, D3DFORMAT format)
{
    FIXME("iface %p, adapter %u, format 0x%x, stub!\n", this, adapter, format);
    return 0;
}

HRESULT Direct3DGL::EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode, D3DDISPLAYMODE *displayMode)
{
    FIXME("iface %p, adapter %u, format 0x%x, mode %u, displayMode %p stub!\n", this, adapter, format, mode, displayMode);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE *displayMode)
{
    FIXME("iface %p, adapter %u, displayMode %p stub!\n", this, adapter, displayMode);
    return E_NOTIMPL;
}


HRESULT Direct3DGL::CheckDeviceType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, WINBOOL windowed)
{
    FIXME("iface %p, adapter %u, devType 0x%x, displayFormat 0x%x, backBufferFormat 0x%x, windowed %u stub!\n", this, adapter, devType, displayFormat, backBufferFormat, windowed);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDeviceFormat(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, DWORD usage, D3DRESOURCETYPE resType, D3DFORMAT checkFormat)
{
    FIXME("iface %p, adapter %u, devType 0x%x, adapterFormat 0x%x, usage 0x%lx, resType 0x%x, checkFormat 0x%x stub!\n", this, adapter, devType, adapterFormat, usage, resType, checkFormat);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT surfaceFormat, WINBOOL windowed, D3DMULTISAMPLE_TYPE multiSampleType, DWORD *qualityLevels)
{
    FIXME("iface %p, adapter %u, devType 0x%x, surfaceFormat 0x%x, windowed %u, multiSampleType 0x%x, qualityLevels %p stub!\n", this, adapter, devType, surfaceFormat, windowed, multiSampleType, qualityLevels);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat)
{
    FIXME("iface %p, adapter %u, devType 0x%x, adapterFormat 0x%x, renderTargetFormat 0x%x, depthStencilFormat 0x%x stub!\n", this, adapter, devType, adapterFormat, renderTargetFormat, depthStencilFormat);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDeviceFormatConversion(UINT adapter, D3DDEVTYPE devType, D3DFORMAT srcFormat, D3DFORMAT dstFormat)
{
    FIXME("iface %p, adapter %u, devType 0x%x, srcFormat 0x%x, dstFormat 0x%x stub!\n", this, adapter, devType, srcFormat, dstFormat);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::GetDeviceCaps(UINT adapter, D3DDEVTYPE devType, D3DCAPS9 *caps)
{
    FIXME("iface %p, adapter %u, devType 0x%x, caps %p stub!\n", this, adapter, devType, caps);
    return E_NOTIMPL;
}

HMONITOR Direct3DGL::GetAdapterMonitor(UINT adapter)
{
    FIXME("iface %p, adapter %u stub!\n", this, adapter);
    return nullptr;
}


HRESULT Direct3DGL::CreateDevice(UINT adapter, D3DDEVTYPE devType, HWND focusWindow, DWORD behaviorFlags, D3DPRESENT_PARAMETERS *presentParams, IDirect3DDevice9 **iface)
{
    FIXME("iface %p, adapter %u, devType 0x%x, focusWindow %p, behaviorFlags 0x%lx, presentParams %p, iface %p stub!\n", this, adapter, devType, focusWindow, behaviorFlags, presentParams, iface);
    return E_NOTIMPL;
}
