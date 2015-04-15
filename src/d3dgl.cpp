
#include "d3dgl.hpp"

#include <string>

#include "glew.h"
#include "trace.hpp"
#include "device.hpp"
#include "adapter.hpp"
#include "private_iids.hpp"


#define WARN_AND_RETURN(val, ...) do { \
    WARN(__VA_ARGS__);                 \
    return val;                        \
} while(0)


namespace
{

void init_adapters(void)
{
    // Only handle one adapter for now.
    gAdapterList.push_back(gAdapterList.size());

    for(size_t i = 0;i < gAdapterList.size();++i)
    {
        if(!gAdapterList[i].init())
        {
            ERR("Pruning adapter %u\n", i);
            gAdapterList.erase(gAdapterList.begin()+i);
            --i;
        }
    }

    if(gAdapterList.empty())
    {
        ERR("No adapters available!\n");
        std::terminate();
    }
}
// Since MinGW doesn't seem to have std::call_once...
void init_adapters_once()
{
    static std::atomic<ULONG> guard(0);
    ULONG ret;
    while((ret=guard.exchange(1)) == 1)
        Sleep(1);
    if(ret == 0)
    {
        try {
            init_adapters();
        }
        catch(...) {
            guard.store(0);
            throw;
        }
    }
    guard.store(2);
}


D3DFORMAT pixelformat_for_depth(DWORD depth)
{
    switch(depth)
    {
        case 8:  return D3DFMT_P8;
        case 15: return D3DFMT_X1R5G5B5;
        case 16: return D3DFMT_R5G6B5;
        case 24: return D3DFMT_X8R8G8B8; /* Robots needs 24bit to be D3DFMT_X8R8G8B8 */
        case 32: return D3DFMT_X8R8G8B8; /* EVE online and the Fur demo need 32bit AdapterDisplayMode to return D3DFMT_X8R8G8B8 */
    }
    return D3DFMT_UNKNOWN;
}

} // namespace


Direct3DGL::Direct3DGL()
  : mRefCount(0)
{
}

Direct3DGL::~Direct3DGL()
{
}

bool Direct3DGL::init()
{
    init_adapters_once();
    return true;
}


HRESULT Direct3DGL::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = nullptr;
    RETURN_IF_IID_TYPE(obj, riid, IDirect3D9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    if(riid == IID_IDirect3D9Ex)
    {
        WARN("Application asks for IDirect3D9Ex, but this instance wasn't created with Direct3DCreate9Ex.\n");
        return E_NOINTERFACE;
    }

    return E_NOINTERFACE;
}

ULONG Direct3DGL::AddRef(void)
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG Direct3DGL::Release(void)
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
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
    TRACE("iface %p\n", this);

    return gAdapterList.size();
}

HRESULT Direct3DGL::GetAdapterIdentifier(UINT adapter, DWORD flags, D3DADAPTER_IDENTIFIER9 *identifier)
{
    WARN("iface %p, adapter %u, flags 0x%lx, identifier %p semi-stub\n", this, adapter, flags, identifier);

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());

    snprintf(identifier->Driver, sizeof(identifier->Driver), "%s", "something.dll");
    snprintf(identifier->Description, sizeof(identifier->Description), "%s", gAdapterList[adapter].getDescription());
    snprintf(identifier->DeviceName, sizeof(identifier->DeviceName), "%ls", gAdapterList[adapter].getDeviceName().c_str());
    identifier->DriverVersion.QuadPart = 0;

    identifier->VendorId = gAdapterList[adapter].getVendorId();
    identifier->DeviceId = gAdapterList[adapter].getDeviceId();
    identifier->SubSysId = 0;
    identifier->Revision = 0;

    identifier->DeviceIdentifier = IID_D3DDEVICE_D3DUID;

    identifier->WHQLLevel = (flags&D3DENUM_NO_WHQL_LEVEL) ? 0 : 1;

    return D3D_OK;
}


UINT Direct3DGL::GetAdapterModeCount(UINT adapter, D3DFORMAT format)
{
    TRACE("iface %p, adapter %u, format %s\n", this, adapter, d3dfmt_to_str(format));

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());

    return gAdapterList[adapter].getModeCount(format);
}

HRESULT Direct3DGL::EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode, D3DDISPLAYMODE *displayMode)
{
    TRACE("iface %p, adapter %u, format 0x%x, mode %u, displayMode %p\n", this, adapter, format, mode, displayMode);

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());

    return gAdapterList[adapter].getModeInfo(format, mode, displayMode);
}

HRESULT Direct3DGL::GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE *displayMode)
{
    TRACE("iface %p, adapter %u, displayMode %p\n", this, adapter, displayMode);

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());

    DEVMODEW m;
    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);

    EnumDisplaySettingsExW(gAdapterList[adapter].getDeviceName().c_str(), ENUM_CURRENT_SETTINGS, &m, 0);
    displayMode->Width = m.dmPelsWidth;
    displayMode->Height = m.dmPelsHeight;
    displayMode->RefreshRate = 0;
    if((m.dmFields&DM_DISPLAYFREQUENCY))
        displayMode->RefreshRate = m.dmDisplayFrequency;
    displayMode->Format = pixelformat_for_depth(m.dmBitsPerPel);

    return D3D_OK;
}


HRESULT Direct3DGL::CheckDeviceType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, WINBOOL windowed)
{
    FIXME("iface %p, adapter %u, devType 0x%x, displayFormat 0x%x, backBufferFormat 0x%x, windowed %u stub!\n", this, adapter, devType, displayFormat, backBufferFormat, windowed);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDeviceFormat(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, DWORD usage, D3DRESOURCETYPE resType, D3DFORMAT checkFormat)
{
    TRACE("iface %p, adapter %u, devType 0x%x, adapterFormat %s, usage 0x%lx, resType 0x%x, checkFormat %s\n", this, adapter, devType, d3dfmt_to_str(adapterFormat), usage, resType, d3dfmt_to_str(checkFormat));

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    /* Check that there's at least one mode for the given format. */
    if(gAdapterList[adapter].getModeCount(adapterFormat) == 0)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter format %s not supported\n", d3dfmt_to_str(adapterFormat));

    DWORD realusage = gAdapterList[adapter].getUsage(resType, checkFormat);
    if(!realusage) return D3DERR_NOTAVAILABLE;

    if((usage&realusage) != usage)
    {
        DWORD nomipusage = usage & ~D3DUSAGE_AUTOGENMIPMAP;
        if((nomipusage&realusage) != nomipusage)
        {
            WARN("Usage query 0x%lx does not match real usage 0x%lx, for resource 0x%x, %s\n", usage, realusage, resType, d3dfmt_to_str(checkFormat));
            return D3DERR_NOTAVAILABLE;
        }
        return D3DOK_NOAUTOGEN;
    }

    return D3D_OK;
}

HRESULT Direct3DGL::CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT surfaceFormat, WINBOOL windowed, D3DMULTISAMPLE_TYPE multiSampleType, DWORD *qualityLevels)
{
    TRACE("iface %p, adapter %u, devType 0x%x, surfaceFormat 0x%x, windowed %u, multiSampleType 0x%x, qualityLevels %p : semi-stub\n", this, adapter, devType, surfaceFormat, windowed, multiSampleType, qualityLevels);

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    if(multiSampleType == D3DMULTISAMPLE_NONE)
    {
        if(qualityLevels) *qualityLevels = 0;
        return D3D_OK;
    }
    if(multiSampleType == D3DMULTISAMPLE_NONMASKABLE)
    {
        FIXME("D3DMULTISAMPLE_NONMASKABLE not currently handled\n");
        if(qualityLevels) *qualityLevels = 0;
        return D3DERR_NOTAVAILABLE;
    }

    UINT mask = gAdapterList[adapter].getSamples(surfaceFormat);
    UINT type_mask = 1<<(multiSampleType-2);
    if(!(mask&type_mask))
    {
        WARN("Multisample type 0x%x not supported with format %s\n", multiSampleType, d3dfmt_to_str(surfaceFormat));
        if(qualityLevels) *qualityLevels = 0;
        return D3DERR_NOTAVAILABLE;
    }

    if(qualityLevels)
        *qualityLevels = 1;
    return D3D_OK;
}

HRESULT Direct3DGL::CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat)
{
    TRACE("iface %p, adapter %u, devType 0x%x, adapterFormat %s, renderTargetFormat %s, depthStencilFormat %s\n", this, adapter, devType, d3dfmt_to_str(adapterFormat), d3dfmt_to_str(renderTargetFormat), d3dfmt_to_str(depthStencilFormat));

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    /* Check that there's at least one mode for the given format. */
    if(gAdapterList[adapter].getModeCount(adapterFormat) == 0)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter format %s not supported\n", d3dfmt_to_str(adapterFormat));

    /* Currently we just check if the given formats are valid for RENDERTARGET
     * and DEPTHSTENCIL usage as SURFACE resources. */
    DWORD rtusage = gAdapterList[adapter].getUsage(D3DRTYPE_SURFACE, renderTargetFormat);
    DWORD dsusage = gAdapterList[adapter].getUsage(D3DRTYPE_SURFACE, depthStencilFormat);

    HRESULT hr = D3D_OK;
    if(!(rtusage&D3DUSAGE_RENDERTARGET))
    {
        FIXME("%s not usable as a RenderTarget\n", d3dfmt_to_str(renderTargetFormat));
        hr = D3DERR_NOTAVAILABLE;
    }
    if(!(dsusage&D3DUSAGE_DEPTHSTENCIL))
    {
        FIXME("%s not usable as a DepthStencil\n", d3dfmt_to_str(depthStencilFormat));
        hr = D3DERR_NOTAVAILABLE;
    }
    return hr;
}

HRESULT Direct3DGL::CheckDeviceFormatConversion(UINT adapter, D3DDEVTYPE devType, D3DFORMAT srcFormat, D3DFORMAT dstFormat)
{
    FIXME("iface %p, adapter %u, devType 0x%x, srcFormat 0x%x, dstFormat 0x%x stub!\n", this, adapter, devType, srcFormat, dstFormat);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::GetDeviceCaps(UINT adapter, D3DDEVTYPE devType, D3DCAPS9 *caps)
{
    TRACE("iface %p, adapter %u, devType 0x%x, caps %p\n", this, adapter, devType, caps);

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    *caps = gAdapterList[adapter].getCaps();
    return D3D_OK;
}

HMONITOR Direct3DGL::GetAdapterMonitor(UINT adapter)
{
    FIXME("iface %p, adapter %u stub!\n", this, adapter);
    return nullptr;
}


HRESULT Direct3DGL::CreateDevice(UINT adapter, D3DDEVTYPE devType, HWND window, DWORD flags, D3DPRESENT_PARAMETERS *params, IDirect3DDevice9 **iface)
{
    TRACE("iface %p, adapter %u, devType 0x%x, window %p, flags 0x%lx, params %p, iface %p\n", this, adapter, devType, window, flags, params, iface);

    if(adapter >= gAdapterList.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, gAdapterList.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    if(!window && params->Windowed)
        window = params->hDeviceWindow;
    if(!window)
    {
        WARN("No focus window specified\n");
        return D3DERR_INVALIDCALL;
    }

    if(params->BackBufferFormat == D3DFMT_UNKNOWN && params->Windowed)
        params->BackBufferFormat = D3DFMT_X8R8G8B8; // FIXME
    if(params->BackBufferFormat == D3DFMT_UNKNOWN)
    {
        WARN("No format specified\n");
        return D3DERR_INVALIDCALL;
    }

    // FIXME: handle known flags
    //D3DCREATE_FPU_PRESERVE
    //D3DCREATE_MULTITHREADED - we should be thread-safe already
    //D3DCREATE_PUREDEVICE
    //D3DCREATE_SOFTWARE_VERTEXPROCESSING - OpenGL handles vertex processing for us
    //D3DCREATE_HARDWARE_VERTEXPROCESSING -   ^       ^       ^        ^      ^  ^
    //D3DCREATE_MIXED_VERTEXPROCESSING    -   ^       ^       ^        ^      ^  ^
    //D3DCREATE_DISABLE_DRIVER_MANAGEMENT
    //D3DCREATE_ADAPTERGROUP_DEVICE
    FIXME("Unhandled flags: 0x%lx\n", flags);

    D3DGLDevice *device = new D3DGLDevice(this, gAdapterList[adapter], window, flags);
    if(!device->init(params))
    {
        delete device;
        return D3DERR_INVALIDCALL;
    }

    *iface = device;
    (*iface)->AddRef();

    return D3D_OK;
}
