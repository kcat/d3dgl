
#include "swapchain.hpp"

#include "trace.hpp"
#include "device.hpp"
#include "rendertarget.hpp"
#include "private_iids.hpp"


void D3DGLSwapChain::swapBuffersGL(size_t backbuffer)
{
    // Flip the destination since we rendered upside down.
    RECT src_rect = { 0, 0, (INT)mParams.BackBufferWidth, (INT)mParams.BackBufferHeight };
    RECT dst_rect = { 0, (INT)mParams.BackBufferHeight-1, (INT)mParams.BackBufferWidth, 0-1 };
    mParent->blitFramebufferGL(GL_RENDERBUFFER, mBackbuffers[backbuffer]->getId(), 0, src_rect,
                               GL_NONE, 0, 0, dst_rect, GL_NEAREST);

    if(!SwapBuffers(mDevCtx))
        ERR("Failed to swap buffers, error: 0x%lx\n", GetLastError());
    --mPendingSwaps;
}
class SwapchainSwapBuffers : public Command {
    D3DGLSwapChain *mTarget;
    size_t mBackbuffer;

public:
    SwapchainSwapBuffers(D3DGLSwapChain *target, size_t backbuffer) : mTarget(target), mBackbuffer(backbuffer) { }

    virtual ULONG execute()
    {
        mTarget->swapBuffersGL(mBackbuffer);
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
  , mPendingSwaps(0)
{
}

D3DGLSwapChain::~D3DGLSwapChain()
{
    for(auto surface : mBackbuffers)
        delete surface;
    mBackbuffers.clear();

    if(mDevCtx)
        ReleaseDC(mWindow, mDevCtx);
    mDevCtx = nullptr;
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

    if(!mParams.Windowed)
    {
        LONG orig_style = GetWindowLongW(mWindow, GWL_STYLE);
        LONG orig_exstyle = GetWindowLongW(mWindow, GWL_EXSTYLE);

        LONG new_style = orig_style | WS_POPUP | WS_SYSMENU;
        new_style &= ~(WS_CAPTION | WS_THICKFRAME);
        LONG new_exstyle = orig_exstyle & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE);

        SetWindowLongW(mWindow, GWL_STYLE, new_style);
        SetWindowLongW(mWindow, GWL_EXSTYLE, new_exstyle);
        SetWindowPos(mWindow, HWND_TOPMOST, 0, 0, mParams.BackBufferWidth, mParams.BackBufferHeight,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }
    else
    {
        SetWindowPos(mWindow, HWND_TOPMOST, 0, 0, mParams.BackBufferWidth, mParams.BackBufferHeight,
                     SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }

    D3DSURFACE_DESC desc;
    desc.Format = mParams.BackBufferFormat;
    desc.Type = D3DRTYPE_SURFACE;
    desc.Usage = D3DUSAGE_RENDERTARGET;
    desc.Pool = D3DPOOL_DEFAULT;
    desc.MultiSampleType = mParams.MultiSampleType;
    desc.MultiSampleQuality = mParams.MultiSampleQuality;
    desc.Width = mParams.BackBufferWidth;
    desc.Height = mParams.BackBufferHeight;

    // Enforce at least one backbuffer
    mParams.BackBufferCount = std::max(mParams.BackBufferCount, 1u);
    for(UINT i = 0;i < mParams.BackBufferCount;++i)
    {
        mBackbuffers.push_back(new D3DGLRenderTarget(mParent));
        if(!mBackbuffers.back()->init(&desc, true))
            return false;
    }

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
        FIXME("Rectangled present not handled\n");

    if(dstWindowOverride)
    {
        FIXME("Destination window override not handled\n");
        return D3D_OK;
    }
    if(dirtyRegion)
        WARN("Dirty region ignored\n");
    if(flags)
        FIXME("Ignoring flags 0x%lx\n", flags);

    ++mPendingSwaps;
    mParent->getQueue().send<SwapchainSwapBuffers>(this, 0);
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
