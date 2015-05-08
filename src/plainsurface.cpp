
#include "plainsurface.hpp"

#include "trace.hpp"
#include "glformat.hpp"
#include "device.hpp"
#include "private_iids.hpp"
#include "allocators.hpp"


D3DGLPlainSurface::D3DGLPlainSurface(D3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
  , mLock(LT_Unlocked)
{

}

D3DGLPlainSurface::~D3DGLPlainSurface()
{
}

bool D3DGLPlainSurface::init(const D3DSURFACE_DESC *desc)
{
    mDesc = *desc;

    auto fmtinfo = gFormatList.find(mDesc.Format);
    if(fmtinfo == gFormatList.end())
    {
        ERR("Failed to find info for format %s\n", d3dfmt_to_str(mDesc.Format));
        return false;
    }
    mGLFormat = &fmtinfo->second;

    mIsCompressed = (mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
                     mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
                     mDesc.Format == D3DFMT_DXT5 || mDesc.Format == D3DFMT_ATI1 ||
                     mDesc.Format == D3DFMT_ATI2);

    UINT data_len;
    if(mIsCompressed)
        data_len = mGLFormat->calcBlockPitch(mDesc.Width, mGLFormat->bytesperblock) * ((mDesc.Height+3)/4);
    else
        data_len = mGLFormat->calcPitch(mDesc.Width, mGLFormat->bytesperpixel) * mDesc.Height;
    data_len = (data_len+15) & ~15;
    mBufData.reset(DataAllocator<GLubyte>()(data_len), DataDeallocator<GLubyte>());

    return true;
}


HRESULT D3DGLPlainSurface::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLPlainSurface);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DSurface9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DResource9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    FIXME("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

ULONG D3DGLPlainSurface::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1) mParent->AddRef();
    return ret;
}

ULONG D3DGLPlainSurface::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0)
    {
        D3DGLDevice *device = mParent;
        delete this;
        device->Release();
    }
    return ret;
}


HRESULT D3DGLPlainSurface::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLPlainSurface::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLPlainSurface::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT D3DGLPlainSurface::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD D3DGLPlainSurface::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD D3DGLPlainSurface::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void D3DGLPlainSurface::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE D3DGLPlainSurface::GetType()
{
    TRACE("iface %p\n", this);
    return D3DRTYPE_SURFACE;
}


HRESULT D3DGLPlainSurface::GetContainer(REFIID riid, void **container)
{
    TRACE("iface %p, riid %s, container %p\n", this, debugstr_guid(riid), container);
    return mParent->QueryInterface(riid, container);
}

HRESULT D3DGLPlainSurface::GetDesc(D3DSURFACE_DESC *desc)
{
    TRACE("iface %p, desc %p\n", this, desc);
    *desc = mDesc;
    return D3D_OK;
}

HRESULT D3DGLPlainSurface::LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags)
{
    TRACE("iface %p, lockedRect %p, rect %p, flags 0x%lx\n", this, lockedRect, rect, flags);

    if(mDesc.Format == D3DFMT_NULL)
    {
        FIXME("Attempting to lock NULL format texture\n");
        return D3DERR_INVALIDCALL;
    }

    DWORD unknown_flags = flags & ~(D3DLOCK_DISCARD|D3DLOCK_READONLY);
    if(unknown_flags) FIXME("Unknown lock flags: 0x%lx\n", unknown_flags);

    RECT full = { 0, 0, (LONG)mDesc.Width, (LONG)mDesc.Height };
    if((flags&D3DLOCK_DISCARD))
    {
        if((flags&D3DLOCK_READONLY))
        {
            WARN("Read-only discard specified\n");
            return D3DERR_INVALIDCALL;
        }
        if(rect)
        {
            WARN("Discardable rect specified\n");
            return D3DERR_INVALIDCALL;
        }
    }
    if(!rect)
        rect = &full;

    {
        LockType lt = ((flags&D3DLOCK_READONLY) ? LT_ReadOnly : LT_Full);
        LockType nolock = LT_Unlocked;
        if(!mLock.compare_exchange_strong(nolock, lt))
        {
            FIXME("Locking already locked surface\n");
            return D3DERR_INVALIDCALL;
        }
    }

    GLubyte *memPtr = mBufData.get();
    mLockRegion = *rect;
    if(mIsCompressed && !(mGLFormat->flags&GLFormatInfo::BadPitch))
    {
        int pitch = mGLFormat->calcBlockPitch(mDesc.Width, mGLFormat->bytesperblock);
        memPtr += (rect->top/4*pitch) + (rect->left/4*mGLFormat->bytesperblock);
        lockedRect->Pitch = pitch;
    }
    else
    {
        int pitch = mGLFormat->calcPitch(mDesc.Width, mGLFormat->bytesperpixel);
        memPtr += (rect->top*pitch) + (rect->left*mGLFormat->bytesperpixel);
        lockedRect->Pitch = pitch;
    }
    lockedRect->pBits = memPtr;

    TRACE("Locked region: pBits=%p, Pitch=%d\n", lockedRect->pBits, lockedRect->Pitch);
    return D3D_OK;
}

HRESULT D3DGLPlainSurface::UnlockRect()
{
    TRACE("iface %p\n", this);

    if(mLock == LT_Unlocked)
    {
        ERR("Attempted to unlock an unlocked surface\n");
        return D3DERR_INVALIDCALL;
    }

    mLock = LT_Unlocked;
    return D3D_OK;
}

HRESULT D3DGLPlainSurface::GetDC(HDC *hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}

HRESULT D3DGLPlainSurface::ReleaseDC(HDC hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}
