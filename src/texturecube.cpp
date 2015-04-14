
#include "texturecube.hpp"

#include <limits>
#include <array>

#include "trace.hpp"
#include "glformat.hpp"
#include "d3dgl.hpp"
#include "device.hpp"
#include "adapter.hpp"
#include "private_iids.hpp"


namespace
{

const std::array<GLenum,6> D3D2GLCubeFace{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, // D3DCUBEMAP_FACE_POSITIVE_X     = 0,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // D3DCUBEMAP_FACE_NEGATIVE_X     = 1,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // D3DCUBEMAP_FACE_POSITIVE_Y     = 2,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // D3DCUBEMAP_FACE_NEGATIVE_Y     = 3,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // D3DCUBEMAP_FACE_POSITIVE_Z     = 4,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z  // D3DCUBEMAP_FACE_NEGATIVE_Z     = 5,
};

inline int calc_pitch(int w, int bpp)
{
    int ret = w * bpp;
    return (ret+3) & ~3;
}

inline int calc_blocked_pitch(int w, int bpp)
{
    int ret = (w+3)/4 * bpp;
    return (ret+3) & ~3;
}

}

void D3DGLCubeTexture::initGL()
{
    UINT total_size = 0;
    for(auto &surfaces : mSurfaces)
    {
        for(D3DGLCubeSurface *surface : surfaces)
        {
            GLint w = std::max(1u, mDesc.Width>>surface->getLevel());
            GLint h = std::max(1u, mDesc.Height>>surface->getLevel());

            UINT level_size;
            if(mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
               mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
               mDesc.Format == D3DFMT_DXT5)
            {
                level_size  = calc_blocked_pitch(w, mGLFormat->bytesperpixel);
                level_size *= ((h+3)/4);
            }
            else
            {
                level_size  = calc_pitch(w, mGLFormat->bytesperpixel);
                level_size *= h;
            }

            surface->init(total_size, level_size);
            total_size += (level_size+15) & ~15;
        }
    }

    glGenTextures(1, &mTexId);
    glTextureParameteriEXT(mTexId, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, mSurfaces.size()-1);
    for(GLenum face : D3D2GLCubeFace)
        glTextureImage2DEXT(mTexId, face, 0, mGLFormat->internalformat, mDesc.Width, mDesc.Height, 0,
                            mGLFormat->format, mGLFormat->type, nullptr);
    checkGLError();

    // Force allocation of mipmap levels, if any
    if(mSurfaces.size() > 1)
    {
        glGenerateTextureMipmapEXT(mTexId, GL_TEXTURE_CUBE_MAP);
        checkGLError();
    }

    if(mDesc.Pool != D3DPOOL_DEFAULT || (mDesc.Usage&D3DUSAGE_DYNAMIC))
        mSysMem.assign(total_size, 0);

    mUpdateInProgress = 0;
}
class CubeTextureInitCmd : public Command {
    D3DGLCubeTexture *mTarget;

public:
    CubeTextureInitCmd(D3DGLCubeTexture *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->initGL();
        return sizeof(*this);
    }
};


class CubeTextureDeinitCmd : public Command {
    GLuint mTexId;

public:
    CubeTextureDeinitCmd(GLuint texid) : mTexId(texid) { }

    virtual ULONG execute()
    {
        glDeleteTextures(1, &mTexId);
        checkGLError();
        return sizeof(*this);
    }
};


void D3DGLCubeTexture::genMipmapGL()
{
    glGenerateTextureMipmapEXT(mTexId, GL_TEXTURE_CUBE_MAP);
    checkGLError();
}
class CubeTextureGenMipCmd : public Command {
    D3DGLCubeTexture *mTarget;

public:
    CubeTextureGenMipCmd(D3DGLCubeTexture *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->genMipmapGL();
        return sizeof(*this);
    }
};


void D3DGLCubeTexture::loadTexLevelGL(DWORD level, GLint facenum, const RECT &rect, const GLubyte *dataPtr)
{
    UINT w = std::max(1u, mDesc.Width>>level);
    /*UINT h = std::max(1u, mDesc.Height>>Level);*/

    if(mIsCompressed)
    {
        D3DGLCubeSurface *surface = mSurfaces[level][facenum];
        GLsizei len = -1;
        if(mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
           mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
           mDesc.Format == D3DFMT_DXT5)
        {
            int pitch = calc_blocked_pitch(w, mGLFormat->bytesperpixel);
            int offset = (rect.top/4*pitch) + (rect.left/4*mGLFormat->bytesperpixel);
            len = surface->getDataLength() - offset;
            dataPtr += offset;
        }
        glCompressedTextureSubImage2DEXT(mTexId, D3D2GLCubeFace[facenum], level,
            rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
            mGLFormat->internalformat, len, dataPtr
        );
    }
    else
    {
        int pitch = calc_pitch(w, mGLFormat->bytesperpixel);
        dataPtr += (rect.top*pitch) + (rect.left*mGLFormat->bytesperpixel);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
        glTextureSubImage2DEXT(mTexId, D3D2GLCubeFace[facenum], level,
            rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
            mGLFormat->format, mGLFormat->type, dataPtr
        );
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    if(level == 0 && (mDesc.Usage&D3DUSAGE_AUTOGENMIPMAP) && mSurfaces.size() > 1)
        glGenerateTextureMipmapEXT(mTexId, GL_TEXTURE_CUBE_MAP);
    checkGLError();

    --mUpdateInProgress;
}
class CubeTextureLoadLevelCmd : public Command {
    D3DGLCubeTexture *mTarget;
    DWORD mLevel;
    GLint mFaceNum;
    RECT mRect;
    const GLubyte *mDataPtr;

public:
    CubeTextureLoadLevelCmd(D3DGLCubeTexture *target, DWORD level, GLint facenum, const RECT &rect, const GLubyte *dataPtr)
      : mTarget(target), mLevel(level), mFaceNum(facenum), mRect(rect), mDataPtr(dataPtr)
    { }

    virtual ULONG execute()
    {
        mTarget->loadTexLevelGL(mLevel, mFaceNum, mRect, mDataPtr);
        return sizeof(*this);
    }
};


D3DGLCubeTexture::D3DGLCubeTexture(D3DGLDevice *parent)
  : mRefCount(0)
  , mIfaceCount(0)
  , mParent(parent)
  , mGLFormat(nullptr)
  , mTexId(0)
  , mUpdateInProgress(0)
  , mLodLevel(0)
{
    for(RECT &rect : mDirtyRect)
        rect = RECT{std::numeric_limits<LONG>::max(), std::numeric_limits<LONG>::max(),
                    std::numeric_limits<LONG>::min(), std::numeric_limits<LONG>::min()};
}

D3DGLCubeTexture::~D3DGLCubeTexture()
{
    if(mTexId)
        mParent->getQueue().send<CubeTextureDeinitCmd>(mTexId);
    while(mUpdateInProgress)
        Sleep(1);
    mTexId = 0;

    for(auto &surfaces : mSurfaces)
    {
        for(auto surface : surfaces)
            delete surface;
    }
    mSurfaces.clear();
}

bool D3DGLCubeTexture::init(const D3DSURFACE_DESC *desc, UINT levels)
{
    mDesc = *desc;

    if(mDesc.Width == 0 || mDesc.Height == 0)
    {
        ERR("Width or height of 0: %ux%u\n", mDesc.Width, mDesc.Height);
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
    {
        std::array<D3DGLCubeSurface*,6> surfaces;
        for(UINT j = 0;j < surfaces.size();++j)
            surfaces[j] = new D3DGLCubeSurface(this, i, j);
        mSurfaces.push_back(surfaces);
    }

    mIsCompressed = (mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
                     mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
                     mDesc.Format == D3DFMT_DXT5);
    if(mDesc.Format == D3DFMT_DXT2 || mDesc.Format == D3DFMT_DXT4)
        WARN("Pre-mulitplied alpha textures not supported; loading anyway.");

    mUpdateInProgress = 1;
    mParent->getQueue().sendSync<CubeTextureInitCmd>(this);

    return true;
}

void D3DGLCubeTexture::updateTexture(DWORD level, GLint facenum, const RECT &rect, const GLubyte *dataPtr)
{
    CommandQueue &queue = mParent->getQueue();
    queue.lock();
    ++mUpdateInProgress;
    queue.sendAndUnlock<CubeTextureLoadLevelCmd>(this, level, facenum, rect, dataPtr);
}

void D3DGLCubeTexture::addIface()
{
    ++mIfaceCount;
}

void D3DGLCubeTexture::releaseIface()
{
    if(--mIfaceCount == 0)
        delete this;
}


HRESULT D3DGLCubeTexture::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLCubeTexture);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DCubeTexture9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DBaseTexture9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DResource9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLCubeTexture::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1)
    {
        mParent->AddRef();
        addIface();
    }
    return ret;
}

ULONG D3DGLCubeTexture::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0)
    {
        D3DGLDevice *parent = mParent;
        releaseIface();
        parent->Release();
    }
    return ret;
}


HRESULT D3DGLCubeTexture::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLCubeTexture::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLCubeTexture::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT D3DGLCubeTexture::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD D3DGLCubeTexture::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD D3DGLCubeTexture::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void D3DGLCubeTexture::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE D3DGLCubeTexture::GetType()
{
    TRACE("iface %p\n", this);
    return D3DRTYPE_CUBETEXTURE;
}


DWORD D3DGLCubeTexture::SetLOD(DWORD lod)
{
    TRACE("iface %p, lod %lu\n", this, lod);

    if(mDesc.Pool != D3DPOOL_MANAGED)
        return 0;

    lod = std::min(lod, (DWORD)mSurfaces.size()-1);

    // FIXME: Set GL_TEXTURE_BASE_LEVEL? Or GL_TEXTURE_MIN_LOD?
    lod = mLodLevel.exchange(lod);

    return lod;
}

DWORD D3DGLCubeTexture::GetLOD()
{
    TRACE("iface %p\n", this);
    return mLodLevel.load();
}

DWORD D3DGLCubeTexture::GetLevelCount()
{
    TRACE("iface %p\n", this);
    return mSurfaces.size();
}

HRESULT D3DGLCubeTexture::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE type)
{
    FIXME("iface %p, type 0x%x : stub!\n", this, type);
    return D3D_OK;
}

D3DTEXTUREFILTERTYPE D3DGLCubeTexture::GetAutoGenFilterType()
{
    FIXME("iface %p\n", this);
    return D3DTEXF_LINEAR;
}

void D3DGLCubeTexture::GenerateMipSubLevels()
{
    TRACE("iface %p\n", this);
    mParent->getQueue().send<CubeTextureGenMipCmd>(this);
}


HRESULT D3DGLCubeTexture::GetLevelDesc(UINT level, D3DSURFACE_DESC *desc)
{
    TRACE("iface %p, level %u, desc %p\n", this, level, desc);

    if(level >= mSurfaces.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mSurfaces.size());
        return D3DERR_INVALIDCALL;
    }

    desc->Format = mDesc.Format;
    desc->Usage = mDesc.Usage;
    desc->Pool = mDesc.Pool;
    desc->MultiSampleType = mDesc.MultiSampleType;
    desc->MultiSampleQuality = mDesc.MultiSampleQuality;
    desc->Width = std::max(1u, mDesc.Width>>level);
    desc->Height = std::max(1u, mDesc.Height>>level);
    return D3D_OK;
}

HRESULT D3DGLCubeTexture::GetCubeMapSurface(D3DCUBEMAP_FACES face, UINT level, IDirect3DSurface9 **surface)
{
    TRACE("iface %p, face %u, level %u, surface %p\n", this, face, level, surface);

    if(level >= mSurfaces.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mSurfaces.size());
        return D3DERR_INVALIDCALL;
    }

    *surface = mSurfaces[level][face];
    (*surface)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLCubeTexture::LockRect(D3DCUBEMAP_FACES face, UINT level, D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags)
{
    TRACE("iface %p, face %u, level %u, lockedRect %p, rect %p, flags 0x%lx\n", this, face, level, lockedRect, rect, flags);

    if(level >= mSurfaces.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mSurfaces.size());
        return D3DERR_INVALIDCALL;
    }

    return mSurfaces[level][face]->LockRect(lockedRect, rect, flags);
}

HRESULT D3DGLCubeTexture::UnlockRect(D3DCUBEMAP_FACES face, UINT level)
{
    TRACE("iface %p, face %u, level %u\n", this, face, level);

    if(level >= mSurfaces.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mSurfaces.size());
        return D3DERR_INVALIDCALL;
    }

    return mSurfaces[level][face]->UnlockRect();
}

HRESULT D3DGLCubeTexture::AddDirtyRect(D3DCUBEMAP_FACES face, const RECT *rect)
{
    TRACE("iface %p, face %u, rect %p\n", this, face, rect);
    mDirtyRect[face].left = std::min(mDirtyRect[face].left, rect->left);
    mDirtyRect[face].top = std::min(mDirtyRect[face].top, rect->top);
    mDirtyRect[face].right = std::max(mDirtyRect[face].right, rect->right);
    mDirtyRect[face].bottom = std::max(mDirtyRect[face].bottom, rect->bottom);
    return D3D_OK;
}



D3DGLCubeSurface::D3DGLCubeSurface(D3DGLCubeTexture *parent, UINT level, GLint facenum)
  : mRefCount(0)
  , mParent(parent)
  , mLevel(level)
  , mFaceNum(facenum)
  , mLock(LT_Unlocked)
{
}

D3DGLCubeSurface::~D3DGLCubeSurface()
{
}

void D3DGLCubeSurface::init(UINT offset, UINT length)
{
    mDataOffset = offset;
    mDataLength = length;
}

GLenum D3DGLCubeSurface::getTarget() const
{
    return D3D2GLCubeFace[mFaceNum];
}


HRESULT D3DGLCubeSurface::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLCubeSurface);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DSurface9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DResource9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLCubeSurface::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1) mParent->addIface();
    return ret;
}

ULONG D3DGLCubeSurface::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) mParent->releaseIface();
    return ret;
}


HRESULT D3DGLCubeSurface::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    return mParent->GetDevice(device);
}

HRESULT D3DGLCubeSurface::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLCubeSurface::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT D3DGLCubeSurface::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD D3DGLCubeSurface::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD D3DGLCubeSurface::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void D3DGLCubeSurface::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE D3DGLCubeSurface::GetType()
{
    TRACE("iface %p\n", this);
    return D3DRTYPE_SURFACE;
}


HRESULT D3DGLCubeSurface::GetContainer(REFIID riid, void **container)
{
    TRACE("iface %p, riid %s, container %p\n", this, debugstr_guid(riid), container);
    return mParent->QueryInterface(riid, container);
}

HRESULT D3DGLCubeSurface::GetDesc(D3DSURFACE_DESC *desc)
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

HRESULT D3DGLCubeSurface::LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags)
{
    TRACE("iface %p, lockedRect %p, rect %p, flags 0x%lx\n", this, lockedRect, rect, flags);

    if(mParent->mDesc.Pool == D3DPOOL_DEFAULT && !(mParent->mDesc.Usage&D3DUSAGE_DYNAMIC))
    {
        WARN("Cannot lock non-dynamic textures in default pool\n");
        return D3DERR_INVALIDCALL;
    }

    UINT w = std::max(1u, mParent->mDesc.Width>>mLevel);
    UINT h = std::max(1u, mParent->mDesc.Height>>mLevel);
    RECT full = { 0, 0, (LONG)w, (LONG)h };
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
            ERR("Texture surface %u already locked!\n", mLevel);
            return D3DERR_INVALIDCALL;
        }
    }

    while(mParent->mUpdateInProgress)
        Sleep(1);

    GLubyte *memPtr = &mParent->mSysMem[mDataOffset];
    mLockRegion = *rect;
    if(mParent->mIsCompressed)
    {
        int pitch = calc_blocked_pitch(w, mParent->mGLFormat->bytesperpixel);
        memPtr += (rect->top/4*pitch) + (rect->left/4*mParent->mGLFormat->bytesperpixel);
        lockedRect->Pitch = pitch;
    }
    else
    {
        int pitch = calc_pitch(w, mParent->mGLFormat->bytesperpixel);
        memPtr += (rect->top*pitch) + (rect->left*mParent->mGLFormat->bytesperpixel);
        lockedRect->Pitch = pitch;
    }
    lockedRect->pBits = memPtr;

    if(!(flags&(D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY)))
    {
        RECT dirty = { rect->left<<mLevel, rect->top<<mLevel,
                       rect->right<<mLevel, rect->bottom<<mLevel };
        mParent->AddDirtyRect((D3DCUBEMAP_FACES)mFaceNum, &dirty);
    }

    return D3D_OK;
}

HRESULT D3DGLCubeSurface::UnlockRect()
{
    TRACE("iface %p\n", this);

    if(mLock == LT_Unlocked)
    {
        ERR("Attempted to unlock an unlocked surface\n");
        return D3DERR_INVALIDCALL;
    }

    if(mLock != LT_ReadOnly)
        mParent->updateTexture(mLevel, mFaceNum, mLockRegion, &mParent->mSysMem[mDataOffset]);

    mLock = LT_Unlocked;
    return D3D_OK;
}

HRESULT D3DGLCubeSurface::GetDC(HDC *hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}

HRESULT D3DGLCubeSurface::ReleaseDC(HDC hdc)
{
    FIXME("iface %p, hdc %p : stub!\n", this, hdc);
    return E_NOTIMPL;
}
