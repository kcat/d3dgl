
#include "d3dglswapchain.hpp"
#include "d3dgldevice.hpp"
#include "trace.hpp"


Direct3DGLSwapChain::Direct3DGLSwapChain(Direct3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
  , mWindow(nullptr)
{
}

Direct3DGLSwapChain::~Direct3DGLSwapChain()
{
    // FIXME: Send a command to destroy GL objects, and wait for it to complete.
}

bool Direct3DGLSwapChain::init(const D3DPRESENT_PARAMETERS *params, HWND window)
{
    // Params are already sanitized.
    mParams = *params;
    mWindow = window;

    // FIXME: Send a command to initialize GL objects
    return true;
}

void Direct3DGLSwapChain::checkDelete()
{
    if(mRefCount > 0 || mParent->isMasterSwapchain(this))
        return;

    delete this;
}


HRESULT Direct3DGLSwapChain::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p.\n", this, (const char*)debugstr_guid(riid), obj);

    *obj = NULL;
    if(riid == IID_IUnknown || riid == IID_IDirect3DSwapChain9)
    {
        AddRef();
        *obj = static_cast<IDirect3DSwapChain9*>(this);
        return D3D_OK;
    }

    return E_NOINTERFACE;
}

ULONG Direct3DGLSwapChain::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1) mParent->AddRef();
    return ret;
}

ULONG Direct3DGLSwapChain::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0)
    {
        mParent->Release();
        checkDelete();
    }
    return ret;
}


HRESULT Direct3DGLSwapChain::Present(const RECT *srcRect, const RECT *dstRect, HWND dstWindowOverride, const RGNDATA *dirtyRegion, DWORD flags)
{
    FIXME("iface %p, srcRect %p, dstRect %p, dstWindowOverride %p, dirtyRegion %p, flags 0x%lx : stub!\n", this, srcRect, dstRect, dstWindowOverride, dirtyRegion, flags);
    return E_NOTIMPL;
}

HRESULT Direct3DGLSwapChain::GetFrontBufferData(IDirect3DSurface9 *dstSurface)
{
    FIXME("iface %p, dstSurface %p : stub!\n", this, dstSurface);
    return E_NOTIMPL;
}

HRESULT Direct3DGLSwapChain::GetBackBuffer(UINT backbuffer, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **out)
{
    FIXME("iface %p, backbuffer %u, type 0x%x, out %p : stub!\n", this, backbuffer, type, out);
    return E_NOTIMPL;
}

HRESULT Direct3DGLSwapChain::GetRasterStatus(D3DRASTER_STATUS *status)
{
    FIXME("iface %p, status %p : stub!\n", this, status);
    return E_NOTIMPL;
}

HRESULT Direct3DGLSwapChain::GetDisplayMode(D3DDISPLAYMODE *mode)
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

HRESULT Direct3DGLSwapChain::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT Direct3DGLSwapChain::GetPresentParameters(D3DPRESENT_PARAMETERS *params)
{
    TRACE("iface %p, params %p\n", this, params);
    *params = mParams;
    return D3D_OK;
}
