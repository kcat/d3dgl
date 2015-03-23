
#include "rendertarget.hpp"

#include "trace.hpp"
#include "device.hpp"


Direct3DGLRenderTarget::Direct3DGLRenderTarget(Direct3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
  , mIsAuto(false)
{
}

Direct3DGLRenderTarget::~Direct3DGLRenderTarget()
{
}

bool Direct3DGLRenderTarget::init(const D3DSURFACE_DESC *desc, bool isauto)
{
    mDesc = *desc;
    mIsAuto = isauto;
    return true;
}


HRESULT Direct3DGLRenderTarget::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    if(riid == IID_IUnknown || riid == IID_IDirect3DSurface9)
    {
        AddRef();
        *obj = static_cast<IDirect3DSurface9*>(this);
        return D3D_OK;
    }
    if(riid == IID_IDirect3DResource9)
    {
        AddRef();
        *obj = static_cast<IDirect3DResource9*>(this);
        return D3D_OK;
    }

    return E_NOINTERFACE;
}

ULONG Direct3DGLRenderTarget::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1) mParent->AddRef();
    return ret;
}

ULONG Direct3DGLRenderTarget::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0)
    {
        mParent->Release();
        if(!mIsAuto)
            delete this;
    }
    return ret;
}


HRESULT Direct3DGLRenderTarget::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT Direct3DGLRenderTarget::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT Direct3DGLRenderTarget::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT Direct3DGLRenderTarget::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD Direct3DGLRenderTarget::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD Direct3DGLRenderTarget::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void Direct3DGLRenderTarget::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE Direct3DGLRenderTarget::GetType()
{
    TRACE("iface %p\n", this);
    return D3DRTYPE_SURFACE;
}


HRESULT Direct3DGLRenderTarget::GetContainer(REFIID riid, void **container)
{
    TRACE("iface %p, riid %s, container %p\n", this, debugstr_guid(riid), container);
    return mParent->QueryInterface(riid, container);
}

HRESULT Direct3DGLRenderTarget::GetDesc(D3DSURFACE_DESC *desc)
{
    TRACE("iface %p, desc %p\n", this, desc);
    *desc = mDesc;
    return D3D_OK;
}

HRESULT Direct3DGLRenderTarget::LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags)
{
    FIXME("iface %p, lockedRect %p, rect %p, flags 0x%lx : stub!\n", this, lockedRect, rect, flags);
    return E_NOTIMPL;
}

HRESULT Direct3DGLRenderTarget::UnlockRect()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLRenderTarget::GetDC(HDC *hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}

HRESULT Direct3DGLRenderTarget::ReleaseDC(HDC hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}
