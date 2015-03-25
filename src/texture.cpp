
#include "texture.hpp"

#include "d3dgl.hpp"
#include "device.hpp"
#include "adapter.hpp"
#include "trace.hpp"


class D3DGLTextureSurface : public IDirect3DSurface9 {
    std::atomic<ULONG> mRefCount;

    Direct3DGLTexture *mParent;
    UINT mLevel;

    UINT mDataOffset;
    UINT mDataLength;

public:
    D3DGLTextureSurface(Direct3DGLTexture *parent, UINT level);
    virtual ~D3DGLTextureSurface();

    void init(UINT offset, UINT length);

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();
    /*** IDirect3DResource9 methods ***/
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device);
    virtual HRESULT WINAPI SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags);
    virtual HRESULT WINAPI GetPrivateData(REFGUID refguid, void *data, DWORD *size);
    virtual HRESULT WINAPI FreePrivateData(REFGUID refguid);
    virtual DWORD WINAPI SetPriority(DWORD priority);
    virtual DWORD WINAPI GetPriority();
    virtual void WINAPI PreLoad();
    virtual D3DRESOURCETYPE WINAPI GetType();
    /*** IDirect3DSurface9 methods ***/
    virtual HRESULT WINAPI GetContainer(REFIID riid, void **container);
    virtual HRESULT WINAPI GetDesc(D3DSURFACE_DESC *desc);
    virtual HRESULT WINAPI LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags);
    virtual HRESULT WINAPI UnlockRect();
    virtual HRESULT WINAPI GetDC(HDC *hdc);
    virtual HRESULT WINAPI ReleaseDC(HDC hdc);
};


void Direct3DGLTexture::initGL()
{
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &mTexId);
    checkGLError();

    if(!mTexId)
       return;

    glBindTexture(GL_TEXTURE_2D, mTexId);
    //mParent->Samplers[0].dirty = true;

    UINT total_size = 0;
    GLint w = mDesc.Width;
    GLint h = mDesc.Height;
    for(UINT l = 0;l < mSurfaces.size();l++)
    {
        w = std::max(1, w);
        h = std::max(1, h);

        UINT level_size;
        if(mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
           mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
           mDesc.Format == D3DFMT_DXT5)
            level_size = ((w+3)/4) * ((h+3)/4) * mGLFormat->bytesperpixel;
        else
            level_size = w*h * mGLFormat->bytesperpixel;

        mSurfaces[l]->init(total_size, level_size);
        total_size += level_size;

        glTexImage2D(GL_TEXTURE_2D, l, mGLFormat->internalformat, w, h, 0,
                     mGLFormat->format, mGLFormat->type, NULL);
        checkGLError();

        w >>= 1;
        h >>= 1;
    }

    /*if((mDesc.Usage&D3DUSAGE_DYNAMIC))
    {
        glGenBuffers(1, &mPBO);
        checkGLError();

        if(mPBO)
        {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
            glBufferData(GL_PIXEL_PACK_BUFFER, total_size, NULL, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            checkGLError();
        }
    }*/
    if((mDesc.Pool == D3DPOOL_SYSTEMMEM || (mDesc.Usage&D3DUSAGE_DYNAMIC)) && !mPBO)
        mSysMem.resize(total_size);

    if(mDesc.Pool == D3DPOOL_MANAGED)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mSurfaces.size()-1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    checkGLError();

    if((mDesc.Usage&D3DUSAGE_AUTOGENMIPMAP))
    {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        checkGLError();
    }
}
class TextureInitCmd : public Command {
    Direct3DGLTexture *mTarget;

public:
    TextureInitCmd(Direct3DGLTexture *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->initGL();
        return sizeof(*this);
    }
};

void Direct3DGLTexture::deinitGL()
{
    glDeleteTextures(1, &mTexId);
    glDeleteBuffers(1, &mPBO);
    checkGLError();
}
class TextureDeinitCmd : public Command {
    Direct3DGLTexture *mTarget;
    HANDLE mFinished;

public:
    TextureDeinitCmd(Direct3DGLTexture *target, HANDLE finished) : mTarget(target), mFinished(finished) { }

    virtual ULONG execute()
    {
        mTarget->deinitGL();
        SetEvent(mFinished);
        return sizeof(*this);
    }
};

void Direct3DGLTexture::setLodGL(DWORD lod)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, lod);
    checkGLError();

    //mParent->Samplers[0].dirty = true;
}
class TextureSetLODCmd : public Command {
    Direct3DGLTexture *mTarget;
    DWORD mLodLevel;

public:
    TextureSetLODCmd(Direct3DGLTexture *target, DWORD lod)
      : mTarget(target), mLodLevel(lod)
    { }

    virtual ULONG execute()
    {
        mTarget->setLodGL(mLodLevel);
        return sizeof(*this);
    }
};

void Direct3DGLTexture::genMipmapGL()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexId);
    glGenerateMipmap(GL_TEXTURE_2D);
    checkGLError();

    //mParent->Samplers[0].dirty = true;
}
class TextureGenMipCmd : public Command {
    Direct3DGLTexture *mTarget;

public:
    TextureGenMipCmd(Direct3DGLTexture *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->genMipmapGL();
        return sizeof(*this);
    }
};


Direct3DGLTexture::Direct3DGLTexture(Direct3DGLDevice *parent)
  : mRefCount(0)
  , mIfaceCount(0)
  , mParent(parent)
  , mGLFormat(nullptr)
  , mTexId(0)
  , mPBO(0)
  , mLodLevel(0)
{
    mParent->AddRef();
}

Direct3DGLTexture::~Direct3DGLTexture()
{
    HANDLE finished = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    mParent->getQueue().send<TextureDeinitCmd>(this, finished);
    WaitForSingleObject(finished, INFINITE);
    CloseHandle(finished);

    for(auto &surface : mSurfaces)
        delete surface;
    mSurfaces.clear();

    mParent->Release();
    mParent = nullptr;
}


bool Direct3DGLTexture::init(const D3DSURFACE_DESC *desc, UINT levels)
{
    mDesc = *desc;

    if(mDesc.Width == 0 || mDesc.Height == 0)
    {
        ERR("Width of height of 0: %ux%u\n", mDesc.Width, mDesc.Height);
        return false;
    }

    auto fmtinfo = gFormatList.find(mDesc.Format);
    if(fmtinfo == gFormatList.end())
    {
        ERR("Failed to find info for format %s\n", d3dfmt_to_str(mDesc.Format));
        return false;
    }
    mGLFormat = &fmtinfo->second;

    if((mDesc.Usage&D3DUSAGE_RENDERTARGET))
    {
        if(mDesc.Pool != D3DPOOL_DEFAULT)
        {
            WARN("RenderTarget not allowed in non-default pool\n");
            return false;
        }
    }
    else if((mDesc.Usage&D3DUSAGE_DEPTHSTENCIL))
    {
        if(mDesc.Pool != D3DPOOL_DEFAULT)
        {
            WARN("DepthStencil target not allowed in non-default pool\n");
            return false;
        }
    }

    if((mDesc.Usage&D3DUSAGE_AUTOGENMIPMAP))
    {
        if(mDesc.Pool == D3DPOOL_SYSTEMMEM)
        {
            WARN("AutoGenMipMap not allowed in systemmem\n");
            return false;
        }
        if(mDesc.Pool == D3DPOOL_MANAGED)
        {
            if(levels > 1)
            {
                WARN("Cannot AutoGenMipMap managed textures\n");
                return false;
            }
            levels = 1;
        }
    }

    UINT maxLevels = 0;
    UINT m = std::max(mDesc.Width, mDesc.Height);
    while(m > 0)
    {
        maxLevels++;
        m >>= 1;
    }
    TRACE("Calculated max mipmap levels: %u\n", maxLevels);

    if(!levels || levels > maxLevels)
        levels = maxLevels;
    for(UINT i = 0;i < levels;++i)
        mSurfaces.push_back(new D3DGLTextureSurface(this, i));

    mIsCompressed = (mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
                     mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
                     mDesc.Format == D3DFMT_DXT5);
    if(mDesc.Format == D3DFMT_DXT2 || mDesc.Format == D3DFMT_DXT4)
        WARN("Pre-mulitplied alpha textures not supported; loading anyway.");

    mParent->getQueue().send<TextureInitCmd>(this);

    return true;
}

void Direct3DGLTexture::addIface()
{
    ++mIfaceCount;
}

void Direct3DGLTexture::releaseIface()
{
    if(--mIfaceCount == 0)
        delete this;
}


HRESULT Direct3DGLTexture::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
#define RETURN_IF_IID_TYPE(obj, riid, TYPE) do { \
    if((riid) == IID_##TYPE)                     \
    {                                            \
        AddRef();                                \
        *(obj) = static_cast<TYPE*>(this);       \
        return D3D_OK;                           \
    }                                            \
} while (0)
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DTexture9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DBaseTexture9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DResource9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);
#undef RETURN_IF_IID_TYPE

    return E_NOINTERFACE;
}

ULONG Direct3DGLTexture::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1) addIface();
    return ret;
}

ULONG Direct3DGLTexture::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) releaseIface();
    return ret;
}


HRESULT Direct3DGLTexture::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT Direct3DGLTexture::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT Direct3DGLTexture::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT Direct3DGLTexture::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD Direct3DGLTexture::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD Direct3DGLTexture::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void Direct3DGLTexture::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE Direct3DGLTexture::GetType()
{
    TRACE("iface %p\n", this);
    return D3DRTYPE_TEXTURE;
}


DWORD Direct3DGLTexture::SetLOD(DWORD lod)
{
    TRACE("iface %p, lod %lu\n", this, lod);

    if(mDesc.Pool != D3DPOOL_MANAGED)
        return 0;

    lod = std::min(lod, (DWORD)mSurfaces.size()-1);

    CommandQueue &queue = mParent->getQueue();
    queue.lock();
    if(mLodLevel.exchange(lod) == lod)
        queue.unlock();
    else
    {
        mLodLevel = lod;
        queue.sendAndUnlock<TextureSetLODCmd>(this, lod);
    }

    return lod;
}

DWORD Direct3DGLTexture::GetLOD()
{
    TRACE("iface %p\n", this);
    return mLodLevel.load();
}

DWORD Direct3DGLTexture::GetLevelCount()
{
    TRACE("iface %p\n", this);
    return mSurfaces.size();
}

HRESULT Direct3DGLTexture::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE type)
{
    FIXME("iface %p, type 0x%x : stub!\n", this, type);
    return D3D_OK;
}

D3DTEXTUREFILTERTYPE Direct3DGLTexture::GetAutoGenFilterType()
{
    FIXME("iface %p\n", this);
    return D3DTEXF_GAUSSIANQUAD;
}

void Direct3DGLTexture::GenerateMipSubLevels()
{
    TRACE("iface %p\n", this);
    mParent->getQueue().send<TextureGenMipCmd>(this);
}


HRESULT Direct3DGLTexture::GetLevelDesc(UINT level, D3DSURFACE_DESC *desc)
{
    TRACE("iface %p, level %u, desc %p\n", this, level, desc);

    if(level >= mSurfaces.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mSurfaces.size());
        return D3DERR_INVALIDCALL;
    }

    return mSurfaces[level]->GetDesc(desc);
}

HRESULT Direct3DGLTexture::GetSurfaceLevel(UINT level, IDirect3DSurface9 **surface)
{
    TRACE("iface %p, level %u, surface %p\n", this, level, surface);

    if(level >= mSurfaces.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mSurfaces.size());
        return D3DERR_INVALIDCALL;
    }

    *surface = mSurfaces[level];
    (*surface)->AddRef();
    return D3D_OK;
}

HRESULT Direct3DGLTexture::LockRect(UINT level, D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags)
{
    TRACE("iface %p, level %u, lockedRect %p, rect %p, flags 0x%lx\n", this, level, lockedRect, rect, flags);

    if(level >= mSurfaces.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mSurfaces.size());
        return D3DERR_INVALIDCALL;
    }

    return mSurfaces[level]->LockRect(lockedRect, rect, flags);
}

HRESULT Direct3DGLTexture::UnlockRect(UINT level)
{
    TRACE("iface %p, level %u\n", this, level);

    if(level >= mSurfaces.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mSurfaces.size());
        return D3DERR_INVALIDCALL;
    }

    return mSurfaces[level]->UnlockRect();
}

HRESULT Direct3DGLTexture::AddDirtyRect(const RECT *rect)
{
    FIXME("iface %p, rect %p : stub!\n", this, rect);
    return E_NOTIMPL;
}



D3DGLTextureSurface::D3DGLTextureSurface(Direct3DGLTexture *parent, UINT level)
  : mParent(parent)
  , mLevel(level)
{
}

D3DGLTextureSurface::~D3DGLTextureSurface()
{
}

void D3DGLTextureSurface::init(UINT offset, UINT length)
{
    mDataOffset = offset;
    mDataLength = length;
}


HRESULT D3DGLTextureSurface::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
#define RETURN_IF_IID_TYPE(obj, riid, TYPE) do { \
    if((riid) == IID_##TYPE)                     \
    {                                            \
        AddRef();                                \
        *(obj) = static_cast<TYPE*>(this);       \
        return D3D_OK;                           \
    }                                            \
} while (0)
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DSurface9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DResource9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);
#undef RETURN_IF_IID_TYPE

    return E_NOINTERFACE;
}

ULONG D3DGLTextureSurface::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1) mParent->addIface();
    return ret;
}

ULONG D3DGLTextureSurface::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) mParent->releaseIface();
    return ret;
}


HRESULT D3DGLTextureSurface::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    return mParent->GetDevice(device);
}

HRESULT D3DGLTextureSurface::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLTextureSurface::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT D3DGLTextureSurface::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD D3DGLTextureSurface::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD D3DGLTextureSurface::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void D3DGLTextureSurface::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE D3DGLTextureSurface::GetType()
{
    TRACE("iface %p\n", this);
    return D3DRTYPE_SURFACE;
}


HRESULT D3DGLTextureSurface::GetContainer(REFIID riid, void **container)
{
    TRACE("iface %p, riid %s, container %p\n", this, debugstr_guid(riid), container);
    return mParent->QueryInterface(riid, container);
}

HRESULT D3DGLTextureSurface::GetDesc(D3DSURFACE_DESC *desc)
{
    TRACE("iface %p, desc %p\n", this, desc);

    desc->Format = mParent->mDesc.Format;
    desc->Usage = mParent->mDesc.Usage;
    desc->Pool = mParent->mDesc.Pool;
    desc->MultiSampleType = mParent->mDesc.MultiSampleType;
    desc->MultiSampleQuality = mParent->mDesc.MultiSampleQuality;
    desc->Width = std::max(1u, mParent->mDesc.Width>>mLevel);
    desc->Height = std::max(1u, mParent->mDesc.Height>>mLevel);
    return D3D_OK;
}

HRESULT D3DGLTextureSurface::LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags)
{
    FIXME("iface %p, lockedRect %p, rect %p, flags 0x%lx : stub!\n", this, lockedRect, rect, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLTextureSurface::UnlockRect()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLTextureSurface::GetDC(HDC *hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}

HRESULT D3DGLTextureSurface::ReleaseDC(HDC hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}
