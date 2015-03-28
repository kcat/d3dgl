
#include "swapchain.hpp"
#include "device.hpp"
#include "trace.hpp"
#include "private_iids.hpp"


void D3DGLSwapChain::swapBuffersGL()
{
    if(!SwapBuffers(mDevCtx))
        ERR("Failed to swap buffers, error: 0x%lx\n", GetLastError());
}
class SwapchainSwapBuffers : public Command {
    D3DGLSwapChain *mTarget;

public:
    SwapchainSwapBuffers(D3DGLSwapChain *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->swapBuffersGL();
        return sizeof(*this);
    }
};


D3DGLSwapChain::D3DGLSwapChain(D3DGLDevice *parent)
  : mRefCount(0)
  , mIfaceCount(0)
  , mParent(parent)
  , mWindow(nullptr)
  , mDevCtx(nullptr)
  , mIsAuto(false)
{
}

D3DGLSwapChain::~D3DGLSwapChain()
{
    // FIXME: Send a command to destroy GL objects, and wait for it to complete.

    for(auto surface : mBackbuffers)
        delete surface;
    mBackbuffers.clear();

    if(mDevCtx)
        ReleaseDC(mWindow, mDevCtx);
}

bool D3DGLSwapChain::init(const D3DPRESENT_PARAMETERS *params, HWND window, bool isauto)
{
    // Params are already sanitized.
    mParams = *params;
    mWindow = window;
    mIsAuto = isauto;

    mDevCtx = GetDC(mWindow);
    if(!mDevCtx)
    {
        ERR("Failed to get DC for windows, error: 0x%lx\n", GetLastError());
        return false;
    }

    mBackbuffers.push_back(new D3DGLBackbufferSurface(this));

    // FIXME: Send a command to initialize GL objects
    return true;
}

void D3DGLSwapChain::addIface()
{
    if(++mIfaceCount == 1)
        mParent->AddRef();
}

void D3DGLSwapChain::releaseIface()
{
    if(--mIfaceCount == 0)
    {
        if(mIsAuto)
            mParent->Release();
        else
        {
            IDirect3DDevice9 *device = mParent;
            delete this;
            device->Release();
        }
    }
}


HRESULT D3DGLSwapChain::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLSwapChain);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DSwapChain9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLSwapChain::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1) addIface();
    return ret;
}

ULONG D3DGLSwapChain::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) releaseIface();
    return ret;
}


HRESULT D3DGLSwapChain::Present(const RECT *srcRect, const RECT *dstRect, HWND dstWindowOverride, const RGNDATA *dirtyRegion, DWORD flags)
{
    TRACE("iface %p, srcRect %p, dstRect %p, dstWindowOverride %p, dirtyRegion %p, flags 0x%lx\n", this, srcRect, dstRect, dstWindowOverride, dirtyRegion, flags);

    if(srcRect || dstRect)
        ERR("Rectangled present not handled\n");

    if(dstWindowOverride)
    {
        ERR("Destination window override not handled\n");
        return D3D_OK;
    }
    if(dirtyRegion)
        WARN("Dirty region ignored\n");
    if(flags)
        ERR("Ignoring flags 0x%lx\n", flags);

    CommandQueue &queue = mParent->getQueue();
    queue.send<SwapchainSwapBuffers>(this);
    return D3D_OK;
}

HRESULT D3DGLSwapChain::GetFrontBufferData(IDirect3DSurface9 *dstSurface)
{
    FIXME("iface %p, dstSurface %p : stub!\n", this, dstSurface);
    return E_NOTIMPL;
}

HRESULT D3DGLSwapChain::GetBackBuffer(UINT backbuffer, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **out)
{
    TRACE("iface %p, backbuffer %u, type 0x%x, out %p\n", this, backbuffer, type, out);

    if(backbuffer >= mBackbuffers.size())
    {
        WARN("Backbuffer out of range (%u >= %u)\n", backbuffer, mBackbuffers.size());
        return D3DERR_INVALIDCALL;
    }

    if(type != D3DBACKBUFFER_TYPE_MONO)
    {
        WARN("Can't get backbuffer type 0x%x\n", type);
        return D3DERR_INVALIDCALL;
    }

    *out = mBackbuffers[backbuffer];
    (*out)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLSwapChain::GetRasterStatus(D3DRASTER_STATUS *status)
{
    FIXME("iface %p, status %p : stub!\n", this, status);
    return E_NOTIMPL;
}

HRESULT D3DGLSwapChain::GetDisplayMode(D3DDISPLAYMODE *mode)
{
    TRACE("iface %p, mode %p\n", this, mode);

    RECT winRect;
    if(!GetClientRect(mWindow, &winRect))
    {
        ERR("Failed to get client rect for window %p, error: %lu\n", mWindow, GetLastError());
        return D3DERR_INVALIDCALL;
    }
    mode->Width = winRect.right - winRect.left;
    mode->Height = winRect.bottom - winRect.top;
    mode->RefreshRate = (mParams.Windowed ? 0 : mParams.FullScreen_RefreshRateInHz);
    mode->Format = mParams.BackBufferFormat;

    return D3D_OK;
}

HRESULT D3DGLSwapChain::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLSwapChain::GetPresentParameters(D3DPRESENT_PARAMETERS *params)
{
    TRACE("iface %p, params %p\n", this, params);
    *params = mParams;
    return D3D_OK;
}


D3DGLBackbufferSurface::D3DGLBackbufferSurface(D3DGLSwapChain *parent)
  : mRefCount(0)
  , mParent(parent)
{
}

D3DGLBackbufferSurface::~D3DGLBackbufferSurface()
{
}


HRESULT D3DGLBackbufferSurface::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLBackbufferSurface);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DSurface9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DResource9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLBackbufferSurface::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1) mParent->addIface();
    return ret;
}

ULONG D3DGLBackbufferSurface::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) mParent->releaseIface();
    return ret;
}


HRESULT D3DGLBackbufferSurface::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    return mParent->GetDevice(device);
}

HRESULT D3DGLBackbufferSurface::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLBackbufferSurface::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT D3DGLBackbufferSurface::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD D3DGLBackbufferSurface::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD D3DGLBackbufferSurface::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void D3DGLBackbufferSurface::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE D3DGLBackbufferSurface::GetType()
{
    TRACE("iface %p\n", this);
    return D3DRTYPE_SURFACE;
}


HRESULT D3DGLBackbufferSurface::GetContainer(REFIID riid, void **container)
{
    TRACE("iface %p, riid %s, container %p\n", this, debugstr_guid(riid), container);
    return mParent->QueryInterface(riid, container);
}

HRESULT D3DGLBackbufferSurface::GetDesc(D3DSURFACE_DESC *desc)
{
    TRACE("iface %p, desc %p\n", this, desc);

    desc->Format = mParent->mParams.BackBufferFormat;
    desc->Type = D3DRTYPE_SURFACE;
    desc->Usage = D3DUSAGE_RENDERTARGET;
    desc->Pool = D3DPOOL_DEFAULT;
    desc->MultiSampleType = mParent->mParams.MultiSampleType;
    desc->MultiSampleQuality = mParent->mParams.MultiSampleQuality;
    desc->Width = mParent->mParams.BackBufferWidth;
    desc->Height = mParent->mParams.BackBufferHeight;

    return D3D_OK;
}

HRESULT D3DGLBackbufferSurface::LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags)
{
    FIXME("iface %p, lockedRect %p, rect %p, flags 0x%lx : stub!\n", this, lockedRect, rect, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLBackbufferSurface::UnlockRect()
{
    FIXME("iface %p\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLBackbufferSurface::GetDC(HDC *hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}

HRESULT D3DGLBackbufferSurface::ReleaseDC(HDC hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}
