
#include "d3dgl.hpp"

#include <string>

#include "glew.h"
#include "trace.hpp"
#include "device.hpp"
#include "adapter.hpp"


#define WARN_AND_RETURN(val, ...) do { \
    WARN(__VA_ARGS__);                 \
    return val;                        \
} while(0)

#ifndef D3DFMT4CC
#define D3DFMT4CC(a,b,c,d)  (a | ((b<<8)&0xff00) | ((b<<16)&0xff0000) | ((d<<24)&0xff000000))
#endif

namespace
{

/* The d3d device ID */
static const GUID IID_D3DDEVICE_D3DUID = GUID{ 0xaeb2cdd4, 0x6e41, 0x43ea, { 0x94,0x1c,0x83,0x61,0xcc,0x76,0x07,0x81 } };


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
    // Only handle one adapter for now.
    mAdapters.push_back(mAdapters.size());

    for(size_t i = 0;i < mAdapters.size();++i)
    {
        if(!mAdapters[i].init())
            return false;
    }
    return true;
}


HRESULT Direct3DGL::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p.\n", this, debugstr_guid(riid), obj);

    if(riid == IID_IDirect3D9 || riid == IID_IUnknown)
    {
        AddRef();
        *obj = static_cast<IDirect3D9*>(this);
        return S_OK;
    }
    if(riid == IID_IDirect3D9Ex)
    {
        FIXME("Application asks for IDirect3D9Ex, but this instance wasn't created with Direct3DCreate9Ex.\n");
        *obj = nullptr;
        return E_NOINTERFACE;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *obj = nullptr;
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

    return mAdapters.size();
}

HRESULT Direct3DGL::GetAdapterIdentifier(UINT adapter, DWORD flags, D3DADAPTER_IDENTIFIER9 *identifier)
{
    FIXME("iface %p, adapter %u, flags 0x%lx, identifier %p semi-stub\n", this, adapter, flags, identifier);

    if(adapter >= mAdapters.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, mAdapters.size());

    snprintf(identifier->Driver, sizeof(identifier->Driver), "%s", "something.dll");
    snprintf(identifier->Description, sizeof(identifier->Description), "%s", mAdapters[adapter].getDescription());
    snprintf(identifier->DeviceName, sizeof(identifier->DeviceName), "%ls", mAdapters[adapter].getDeviceName().c_str());
    identifier->DriverVersion.QuadPart = 0;

    identifier->VendorId = mAdapters[adapter].getVendorId();
    identifier->DeviceId = mAdapters[adapter].getDeviceId();
    identifier->SubSysId = 0;
    identifier->Revision = 0;

    identifier->DeviceIdentifier = IID_D3DDEVICE_D3DUID;

    identifier->WHQLLevel = (flags&D3DENUM_NO_WHQL_LEVEL) ? 0 : 1;

    return D3D_OK;
}


UINT Direct3DGL::GetAdapterModeCount(UINT adapter, D3DFORMAT format)
{
    TRACE("iface %p, adapter %u, format %s : semi-stub\n", this, adapter, d3dfmt_to_str(format));

    if(adapter >= mAdapters.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, mAdapters.size());

    return mAdapters[adapter].getModeCount(format);
}

HRESULT Direct3DGL::EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode, D3DDISPLAYMODE *displayMode)
{
    FIXME("iface %p, adapter %u, format 0x%x, mode %u, displayMode %p stub!\n", this, adapter, format, mode, displayMode);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE *displayMode)
{
    TRACE("iface %p, adapter %u, displayMode %p\n", this, adapter, displayMode);

    if(adapter >= mAdapters.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, mAdapters.size());

    DEVMODEW m;
    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);

    EnumDisplaySettingsExW(mAdapters[adapter].getDeviceName().c_str(), ENUM_CURRENT_SETTINGS, &m, 0);
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
    FIXME("iface %p, adapter %u, devType 0x%x, adapterFormat %s, usage 0x%lx, resType 0x%x, checkFormat %s : semi-stub\n", this, adapter, devType, d3dfmt_to_str(adapterFormat), usage, resType, d3dfmt_to_str(checkFormat));

    if(adapter >= mAdapters.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, mAdapters.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    /* Check that there's at least one mode for the given format. */
    if(mAdapters[adapter].getModeCount(adapterFormat) == 0)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter format %s not supported\n", d3dfmt_to_str(adapterFormat));

    HRESULT hr = D3D_OK;
    // Check that the format can be used for the given resource type
    if(resType == D3DRTYPE_SURFACE)
    {
        DWORD InvalidFlags;
        InvalidFlags = (usage&~(D3DUSAGE_RENDERTARGET|D3DUSAGE_DEPTHSTENCIL|D3DUSAGE_DYNAMIC));
        if(InvalidFlags)
        {
            FIXME("Unhandled usage flags specified for surface: 0x%lx\n", InvalidFlags);
            return D3DERR_INVALIDCALL;
        }
    }
    else if(resType == D3DRTYPE_TEXTURE || resType == D3DRTYPE_VOLUMETEXTURE ||
            resType == D3DRTYPE_CUBETEXTURE || resType == D3DRTYPE_VOLUME)
    {
        DWORD InvalidFlags;
        InvalidFlags = (usage&~(D3DUSAGE_RENDERTARGET|D3DUSAGE_DEPTHSTENCIL|
                                D3DUSAGE_AUTOGENMIPMAP|D3DUSAGE_DYNAMIC|
                                D3DUSAGE_QUERY_LEGACYBUMPMAP|
                                D3DUSAGE_QUERY_SRGBREAD|D3DUSAGE_QUERY_SRGBWRITE|
                                D3DUSAGE_QUERY_FILTER|D3DUSAGE_QUERY_VERTEXTEXTURE|
                                D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING|
                                D3DUSAGE_QUERY_WRAPANDMIP));
        if(InvalidFlags)
        {
            FIXME("Unhandled usage flags specified for texture type: 0x%lx\n", InvalidFlags);
            return D3DERR_INVALIDCALL;
        }

        if(resType == D3DRTYPE_VOLUMETEXTURE || resType == D3DRTYPE_VOLUME)
        {
            switch(checkFormat)
            {
                case D3DFMT_D15S1:
                case D3DFMT_D16:
                case D3DFMT_D16_LOCKABLE:
                case D3DFMT_D24X8:
                case D3DFMT_D24X4S4:
                case D3DFMT_D24S8:
                case D3DFMT_D32:
                case D3DFMT_D24FS8:
                case D3DFMT_D32F_LOCKABLE:
                    WARN("3D textures not supported for texture format: %s\n", d3dfmt_to_str(checkFormat));
                    return D3DERR_NOTAVAILABLE;
                default:
                    break;
            }
        }

        switch(checkFormat)
        {
            // NOTE: Although 24-bit RGB is perfectly supported at the API
            // level, hardware is likely hiding the fact that it's using XRGB.
            // D3D9 does *not* allow R8G8B8 textures.
            case D3DFMT_R8G8B8:
            // RGBA3328 and LA4 have no loadable data type in OpenGL
            case D3DFMT_A8R3G3B2:
            case D3DFMT_A4L4:
            // OpenGL has no corresponding types for these depth/stencil formats
            case D3DFMT_D24X4S4:
            case D3DFMT_D24FS8:
            // FIXME: Lockable depth formats are not supported (why?)
            case D3DFMT_D16_LOCKABLE:
            case D3DFMT_D32F_LOCKABLE:
            // Need to check into these...
            case D3DFMT_UYVY:
            case D3DFMT_YUY2:
            case D3DFMT_MULTI2_ARGB8:
            case D3DFMT_G8R8_G8B8:
            case D3DFMT_R8G8_B8G8:
            case D3DFMT_L6V5U5:
            case D3DFMT_Q16W16V16U16:
            case D3DFMT_A2W10V10U10:
            case D3DFMT_CxV8U8:
                WARN("Texture format not supported: %s\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;

            case D3DFMT_A8R8G8B8:
            case D3DFMT_X8R8G8B8:
            case D3DFMT_R5G6B5:
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
            case D3DFMT_A4R4G4B4:
            case D3DFMT_R3G3B2:
            case D3DFMT_A8:
            case D3DFMT_X4R4G4B4:
            case D3DFMT_A8B8G8R8:
            case D3DFMT_X8B8G8R8:
            case D3DFMT_A2R10G10B10:
            case D3DFMT_A2B10G10R10:
            case D3DFMT_A16B16G16R16:
            case D3DFMT_L8:
            case D3DFMT_L16:
            case D3DFMT_A8L8:
            case D3DFMT4CC('A','L','1','6'):
            case D3DFMT_D16:
            case D3DFMT_D32:
            case D3DFMT_D24X8:
            case D3DFMT_D24S8:
            case D3DFMT4CC(' ','R','1','6'):
            case D3DFMT_G16R16:
            case D3DFMT_R16F:
            case D3DFMT_G16R16F:
            case D3DFMT_R32F:
            case D3DFMT_G32R32F:
            case D3DFMT_A16B16G16R16F:
            case D3DFMT_A32B32G32R32F:
                break;

            case D3DFMT_DXT1:
            case D3DFMT_DXT2:
            case D3DFMT_DXT3:
            case D3DFMT_DXT4:
            case D3DFMT_DXT5:
                WARN("Assuming S3_s3tc for texture format: %s\n", d3dfmt_to_str(checkFormat));
                break;

            case D3DFMT_V8U8:
            case D3DFMT_X8L8V8U8:
            case D3DFMT_Q8W8V8U8:
            case D3DFMT_V16U16:
                WARN("Assuming ATI_envmap_bumpmap or NV_register_combiners/NV_texture_shader2 for texture format: %s\n", d3dfmt_to_str(checkFormat));
                break;

            case D3DFMT_A8P8:
            case D3DFMT_P8:
                WARN("Paletted textures not handled for texture format: %s\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;

            default:
                ERR("Format %s is not a recognized texture format\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;
        }
    }
    else if(resType == D3DRTYPE_VERTEXBUFFER)
    {
        DWORD InvalidFlags;
        InvalidFlags = (usage&~(D3DUSAGE_WRITEONLY|D3DUSAGE_SOFTWAREPROCESSING|D3DUSAGE_DYNAMIC));
        if(InvalidFlags)
        {
            ERR("Invalid usage flags specified for vertex buffers: 0x%lx\n", InvalidFlags);
            return D3DERR_INVALIDCALL;
        }

        switch(checkFormat)
        {
            case D3DFMT_VERTEXDATA:
                break;
            default:
                WARN("Format %s is not a vertex buffer format\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;
        }
    }
    else if(resType == D3DRTYPE_INDEXBUFFER)
    {
        DWORD InvalidFlags;
        InvalidFlags = (usage&~(D3DUSAGE_WRITEONLY|D3DUSAGE_SOFTWAREPROCESSING|D3DUSAGE_DYNAMIC));
        if(InvalidFlags)
        {
            ERR("Invalid usage flags specified for index buffers: 0x%lx\n", InvalidFlags);
            return D3DERR_INVALIDCALL;
        }

        switch(checkFormat)
        {
            case D3DFMT_INDEX16:
            case D3DFMT_INDEX32:
                break;
            default:
                WARN("Format %s is not an index format\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;
        }
    }
    else
    {
        ERR("Unexpected resource type: 0x%x\n", resType);
        return D3DERR_INVALIDCALL;
    }

    // Format is valid for the resource type. Now make sure it can be used
    // according to the request.
    if((usage&D3DUSAGE_RENDERTARGET))
    {
        if(!(resType == D3DRTYPE_TEXTURE || resType == D3DRTYPE_VOLUMETEXTURE ||
             resType == D3DRTYPE_CUBETEXTURE || resType == D3DRTYPE_SURFACE))
        {
            ERR("Resource type 0x%x not handled for rendertarget usage\n", resType);
            return D3DERR_INVALIDCALL;
        }

        // FIXME
        switch(checkFormat)
        {
            case D3DFMT_A8R8G8B8:
            case D3DFMT_X8R8G8B8:
            case D3DFMT_R5G6B5:
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
            case D3DFMT_A4R4G4B4:
            case D3DFMT_R3G3B2:
            case D3DFMT_A8:
            case D3DFMT_X4R4G4B4:
            case D3DFMT_A8B8G8R8:
            case D3DFMT_X8B8G8R8:
            case D3DFMT_A2R10G10B10:
            case D3DFMT_A2B10G10R10:
            case D3DFMT_A16B16G16R16:
            case D3DFMT4CC(' ','R','1','6'):
            case D3DFMT_G16R16:
            case D3DFMT_R16F:
            case D3DFMT_G16R16F:
            case D3DFMT_R32F:
            case D3DFMT_G32R32F:
            case D3DFMT_A16B16G16R16F:
            case D3DFMT_A32B32G32R32F:
                FIXME("Assuming format %s is color renderable\n", d3dfmt_to_str(checkFormat));
                break;

            default:
                ERR("Format %s is not a recognized rendertarget format\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;
        }
        /*if(!IsColorFormatRenderable(adapter, chkformat))
        {
            WARN("Format not color renderable (%s)\n", d3dfmt_to_str(chkformat));
            return D3DERR_NOTAVAILABLE;
        }*/
    }
    if((usage&D3DUSAGE_DEPTHSTENCIL))
    {
        if(!(resType == D3DRTYPE_TEXTURE || resType == D3DRTYPE_VOLUMETEXTURE ||
             resType == D3DRTYPE_CUBETEXTURE || resType == D3DRTYPE_SURFACE))
        {
            ERR("Resource type 0x%x not handled for depthstencil usage\n", resType);
            return D3DERR_INVALIDCALL;
        }

        // FIXME
        switch(checkFormat)
        {
            case D3DFMT_D16:
            //case D3DFMT_D16_LOCKABLE:
            case D3DFMT_D24X8:
            case D3DFMT_D24S8:
            case D3DFMT_D32:
            //case D3DFMT_D32F_LOCKABLE:
                FIXME("Assuming format %s is depth-stencil renderable\n", d3dfmt_to_str(checkFormat));
                break;

            default:
                ERR("Format %s is not a recognized depth-stencil format\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;
        }
        /*if(!IsDepthStencilFormatRenderable(adapter, chkformat))
        {
            WARN("Format not depth-stencil renderable (%s)\n", d3dfmt_to_str(chkformat));
            return D3DERR_NOTAVAILABLE;
        }*/
    }
    if((usage&D3DUSAGE_SOFTWAREPROCESSING))
    {
        WARN("Software processing queried; allowing\n");
    }
    if((usage&D3DUSAGE_DYNAMIC))
    {
        WARN("Dynamic usage queried; allowing\n");
    }
    if((usage&D3DUSAGE_WRITEONLY))
    {
        WARN("Write-only usage queried; allowing\n");
    }
    //if((usage&D3DUSAGE_AUTOGENMIPMAP))
    //{
    //}
    if((usage&D3DUSAGE_DMAP))
    {
        // FIXME: displacement map?
        ERR("Displacement map usage requested for format (%s)!\n", d3dfmt_to_str(checkFormat));
    }
    if((usage&D3DUSAGE_QUERY_LEGACYBUMPMAP))
    {
        switch(checkFormat)
        {
            case D3DFMT_V8U8:
            case D3DFMT_Q8W8V8U8:
            case D3DFMT_V16U16:
            case D3DFMT_Q16W16V16U16:
                break;
            default:
                WARN("Format %s is not a bumpmap format\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;
        }
    }
    if((usage&D3DUSAGE_QUERY_SRGBREAD))
    {
        switch(checkFormat)
        {
            case D3DFMT_DXT1:
            case D3DFMT_DXT3:
            case D3DFMT_DXT5:
            case D3DFMT_R8G8B8:
            case D3DFMT_X8R8G8B8:
            case D3DFMT_A8R8G8B8:
            case D3DFMT_X8B8G8R8:
            case D3DFMT_A8B8G8R8:
            case D3DFMT_L8:
            case D3DFMT_A8L8:
                break;

            default:
                WARN("Reading sRGB not supported for format: %s\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;
        }
    }
    if((usage&D3DUSAGE_QUERY_FILTER) || (usage&D3DUSAGE_QUERY_WRAPANDMIP))
    {
        switch(checkFormat)
        {
            case D3DFMT_R16F:
            case D3DFMT_G16R16F:
            case D3DFMT_A16B16G16R16F:
            case D3DFMT_R32F:
            case D3DFMT_G32R32F:
            case D3DFMT_A32B32G32R32F:
                break;

            case D3DFMT_D15S1:
            case D3DFMT_D16:
            case D3DFMT_D16_LOCKABLE:
            case D3DFMT_D24X8:
            case D3DFMT_D24X4S4:
            case D3DFMT_D24S8:
            case D3DFMT_D32:
            case D3DFMT_D24FS8:
            case D3DFMT_D32F_LOCKABLE:
                WARN("No filtering on texture format: %s\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;

            default:
                break;
        }
    }
    if((usage&D3DUSAGE_QUERY_SRGBWRITE))
    {
        // FIXME
        ERR("sRGB writing not current supported\n");
        return D3DERR_NOTAVAILABLE;
    }
    //if((usage&D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING))
    //{
    //}
    if((usage&D3DUSAGE_QUERY_VERTEXTEXTURE))
    {
        if(resType == D3DRTYPE_VOLUME)
        {
            WARN("3D surfaces not supported for vertex texture sampling: %s\n", d3dfmt_to_str(checkFormat));
            return D3DERR_NOTAVAILABLE;
        }
        switch(checkFormat)
        {
            case D3DFMT_R32F:
            case D3DFMT_G32R32F:
            case D3DFMT_A32B32G32R32F:
            case D3DFMT_R16F:
            case D3DFMT_G16R16F:
            case D3DFMT_A16B16G16R16F:
                break;
            default:
                WARN("Unable to sample from texture format: %s\n", d3dfmt_to_str(checkFormat));
                return D3DERR_NOTAVAILABLE;
        }
    }

    return hr;
}

HRESULT Direct3DGL::CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT surfaceFormat, WINBOOL windowed, D3DMULTISAMPLE_TYPE multiSampleType, DWORD *qualityLevels)
{
    FIXME("iface %p, adapter %u, devType 0x%x, surfaceFormat 0x%x, windowed %u, multiSampleType 0x%x, qualityLevels %p stub!\n", this, adapter, devType, surfaceFormat, windowed, multiSampleType, qualityLevels);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat)
{
    FIXME("iface %p, adapter %u, devType 0x%x, adapterFormat %s, renderTargetFormat %s, depthStencilFormat %s : stub!\n", this, adapter, devType, d3dfmt_to_str(adapterFormat), d3dfmt_to_str(renderTargetFormat), d3dfmt_to_str(depthStencilFormat));
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

    if(adapter >= mAdapters.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, mAdapters.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    *caps = mAdapters[adapter].getCaps();
    return D3D_OK;
}

HMONITOR Direct3DGL::GetAdapterMonitor(UINT adapter)
{
    FIXME("iface %p, adapter %u stub!\n", this, adapter);
    return nullptr;
}


HRESULT Direct3DGL::CreateDevice(UINT adapter, D3DDEVTYPE devType, HWND window, DWORD flags, D3DPRESENT_PARAMETERS *params, IDirect3DDevice9 **iface)
{
    FIXME("iface %p, adapter %u, devType 0x%x, window %p, flags 0x%lx, params %p, iface %p stub!\n", this, adapter, devType, window, flags, params, iface);

    if(adapter >= mAdapters.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, mAdapters.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    TRACE("Creating device with parameters:\n"
          "\tBackBufferWidth            = %u\n"
          "\tBackBufferHeight           = %u\n"
          "\tBackBufferFormat           = %s\n"
          "\tBackBufferCount            = %u\n"
          "\tMultiSampleType            = 0x%x\n"
          "\tMultiSampleQuality         = %lu\n"
          "\tSwapEffect                 = 0x%x\n"
          "\thDeviceWindow              = %p\n"
          "\tWindowed                   = %d\n"
          "\tEnableAutoDepthStencil     = %d\n"
          "\tAutoDepthStencilFormat     = %s\n"
          "\tFlags                      = 0x%lx\n"
          "\tFullScreen_RefreshRateInHz = %u\n"
          "\tPresentationInterval       = 0x%x\n",
          params->BackBufferWidth, params->BackBufferHeight, d3dfmt_to_str(params->BackBufferFormat),
          params->BackBufferCount, params->MultiSampleType, params->MultiSampleQuality,
          params->SwapEffect, params->hDeviceWindow, params->Windowed,
          params->EnableAutoDepthStencil, d3dfmt_to_str(params->AutoDepthStencilFormat),
          params->Flags, params->FullScreen_RefreshRateInHz, params->PresentationInterval);

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

    DWORD UnknownFlags = (flags&~(D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED |
                                  D3DCREATE_PUREDEVICE | D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                                  D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MIXED_VERTEXPROCESSING |
                                  D3DCREATE_DISABLE_DRIVER_MANAGEMENT | D3DCREATE_ADAPTERGROUP_DEVICE));
    if(UnknownFlags)
    {
        ERR("Unknown flags specified (0x%lx)!\n", UnknownFlags);
        flags &= ~UnknownFlags;
    }

    // Unless we want to transform and process vertices manually, just pretend
    // the app is getting what it wants. OpenGL will automatically do what it
    // needs to.
    flags &= ~(D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_HARDWARE_VERTEXPROCESSING |
               D3DCREATE_MIXED_VERTEXPROCESSING);

    // FIXME: handle known flags
    UnknownFlags = (flags&(D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED |
                           D3DCREATE_PUREDEVICE | D3DCREATE_DISABLE_DRIVER_MANAGEMENT |
                           D3DCREATE_ADAPTERGROUP_DEVICE));
    if(UnknownFlags)
        ERR("Unhandled flags: 0x%lx\n", UnknownFlags);


    Direct3DGLDevice *device = new Direct3DGLDevice(this, mAdapters[adapter], window, flags);
    if(!device->init(params))
    {
        delete device;
        return D3DERR_INVALIDCALL;
    }

    *iface = device;
    (*iface)->AddRef();

    return D3D_OK;
}
