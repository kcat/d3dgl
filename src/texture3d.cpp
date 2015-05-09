
#include "texture3d.hpp"

#include <limits>

#include "trace.hpp"
#include "glformat.hpp"
#include "d3dgl.hpp"
#include "device.hpp"
#include "adapter.hpp"
#include "private_iids.hpp"


void D3DGLTexture3D::initGL()
{
    UINT total_size = 0;
    GLint w = mDesc.Width;
    GLint h = mDesc.Height;
    GLint d = mDesc.Depth;
    for(D3DGLTextureVolume *volume : mVolumes)
    {
        w = std::max(1, w);
        h = std::max(1, h);
        d = std::min(1, d);

        UINT level_size;
        if(mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
           mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
           mDesc.Format == D3DFMT_DXT5 || mDesc.Format == D3DFMT_ATI1 ||
           mDesc.Format == D3DFMT_ATI2)
        {
            level_size  = mGLFormat->calcBlockPitch(w, mGLFormat->bytesperblock);
            level_size *= ((h+3)/4) * d;
        }
        else
        {
            level_size  = mGLFormat->calcPitch(w, mGLFormat->bytesperpixel);
            level_size *= h * d;
        }

        volume->init(total_size, level_size);
        total_size += (level_size+15) & ~15;

        w >>= 1;
        h >>= 1;
        d >>= 1;
    }

    glGenTextures(1, &mTexId);
    glTextureParameteriEXT(mTexId, GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, mVolumes.size()-1);
    glTextureImage3DEXT(mTexId, GL_TEXTURE_3D, 0, mGLFormat->internalformat, mDesc.Width, mDesc.Height, mDesc.Depth,
                        0, mGLFormat->format, mGLFormat->type, nullptr);
    checkGLError();

    // Force allocation of mipmap levels, if any
    if(mVolumes.size() > 1)
    {
        glGenerateTextureMipmapEXT(mTexId, GL_TEXTURE_3D);
        checkGLError();
    }

    if(mDesc.Pool != D3DPOOL_DEFAULT)
        mSysMem.assign(total_size, 0);

    mUpdateInProgress = 0;
}
class Texture3DInitCmd : public Command {
    D3DGLTexture3D *mTarget;

public:
    Texture3DInitCmd(D3DGLTexture3D *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->initGL();
        return sizeof(*this);
    }
};

class Texture3DDeinitCmd : public Command {
    GLuint mTexId;

public:
    Texture3DDeinitCmd(GLuint texid) : mTexId(texid) { }

    virtual ULONG execute()
    {
        glDeleteTextures(1, &mTexId);
        checkGLError();
        return sizeof(*this);
    }
};


void D3DGLTexture3D::genMipmapGL()
{
    glGenerateTextureMipmapEXT(mTexId, GL_TEXTURE_3D);
    checkGLError();
}
class Texture3DGenMipCmd : public Command {
    D3DGLTexture3D *mTarget;

public:
    Texture3DGenMipCmd(D3DGLTexture3D *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->genMipmapGL();
        return sizeof(*this);
    }
};


void D3DGLTexture3D::loadTexLevelGL(DWORD level, const D3DBOX &box, const GLubyte *dataPtr)
{
    UINT w = std::max(1u, mDesc.Width>>level);
    UINT h = std::max(1u, mDesc.Height>>level);
    /*UINT d = std::max(1u, mDesc.Depth>>level);*/

    D3DGLTextureVolume *volume = mVolumes[level];
    if(mIsCompressed)
    {
        GLsizei len = -1;
        if(mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
           mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
           mDesc.Format == D3DFMT_DXT5 || mDesc.Format == D3DFMT_ATI1 ||
           mDesc.Format == D3DFMT_ATI2)
        {
            int pitch = mGLFormat->calcBlockPitch(w, mGLFormat->bytesperblock);
            int slice = pitch * ((h+3)/4);
            int offset = box.Front*slice + (box.Top/4*pitch) + (box.Left/4*mGLFormat->bytesperblock);
            len = volume->getDataLength() - offset;
            dataPtr += offset;
        }
        glCompressedTextureSubImage3DEXT(mTexId, GL_TEXTURE_3D, level,
            box.Left, box.Top, box.Front, box.Right-box.Left, box.Bottom-box.Top, box.Back-box.Front,
            mGLFormat->internalformat, len, dataPtr
        );
    }
    else
    {
        int pitch = mGLFormat->calcPitch(w, mGLFormat->bytesperpixel);
        int slice = pitch * h;
        dataPtr += box.Front*slice + (box.Top*pitch) + (box.Left*mGLFormat->bytesperpixel);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, h);
        glTextureSubImage3DEXT(mTexId, GL_TEXTURE_3D, level,
            box.Left, box.Top, box.Front, box.Right-box.Left, box.Bottom-box.Top, box.Back-box.Front,
            mGLFormat->format, mGLFormat->type, dataPtr
        );
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    }

    if(level == 0 && (mDesc.Usage&D3DUSAGE_AUTOGENMIPMAP) && mVolumes.size() > 1)
        glGenerateTextureMipmapEXT(mTexId, GL_TEXTURE_3D);
    checkGLError();

    --mUpdateInProgress;
}
class Texture3DLoadLevelCmd : public Command {
    D3DGLTexture3D *mTarget;
    DWORD mLevel;
    D3DBOX mBox;
    const GLubyte *mDataPtr;

public:
    Texture3DLoadLevelCmd(D3DGLTexture3D *target, DWORD level, const D3DBOX &box, const GLubyte *dataPtr)
      : mTarget(target), mLevel(level), mBox(box), mDataPtr(dataPtr)
    { }

    virtual ULONG execute()
    {
        mTarget->loadTexLevelGL(mLevel, mBox, mDataPtr);
        return sizeof(*this);
    }
};


D3DGLTexture3D::D3DGLTexture3D(D3DGLDevice *parent)
  : mRefCount(0)
  , mIfaceCount(0)
  , mParent(parent)
  , mGLFormat(nullptr)
  , mTexId(0)
  , mDirtyBox({std::numeric_limits<UINT>::max(), std::numeric_limits<UINT>::max(),
               std::numeric_limits<UINT>::min(), std::numeric_limits<UINT>::min(),
               std::numeric_limits<UINT>::min(), std::numeric_limits<UINT>::max()})
  , mUpdateInProgress(0)
  , mLodLevel(0)
{
}

D3DGLTexture3D::~D3DGLTexture3D()
{
    if(mTexId)
    {
        mParent->getQueue().send<Texture3DDeinitCmd>(mTexId);
        while(mUpdateInProgress)
            mParent->getQueue().wakeAndSleep();
        mTexId = 0;
    }

    for(auto volume : mVolumes)
        delete volume;
    mVolumes.clear();
}


bool D3DGLTexture3D::init(const D3DVOLUME_DESC *desc, UINT levels)
{
    mDesc = *desc;

    if(mDesc.Width == 0 || mDesc.Height == 0 || mDesc.Depth == 0)
    {
        ERR("Width, height, or depth of 0: %ux%ux%u\n", mDesc.Width, mDesc.Height, mDesc.Depth);
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

    mIsCompressed = (mDesc.Format == D3DFMT_DXT1 || mDesc.Format == D3DFMT_DXT2 ||
                     mDesc.Format == D3DFMT_DXT3 || mDesc.Format == D3DFMT_DXT4 ||
                     mDesc.Format == D3DFMT_DXT5 || mDesc.Format == D3DFMT_ATI1 ||
                     mDesc.Format == D3DFMT_ATI2);
    if(mDesc.Format == D3DFMT_DXT2 || mDesc.Format == D3DFMT_DXT4)
        WARN("Pre-mulitplied alpha textures not supported; loading anyway.");

    UINT maxLevels = 0;
    UINT m = std::max(mDesc.Width, std::max(mDesc.Height, mDesc.Depth));
    while(m > 0)
    {
        maxLevels++;
        m >>= 1;
    }
    TRACE("Calculated max mipmap levels: %u (requested: %u)\n", maxLevels, levels);

    if(!levels || levels > maxLevels)
        levels = maxLevels;
    for(UINT i = 0;i < levels;++i)
        mVolumes.push_back(new D3DGLTextureVolume(this, i));

    if(mDesc.Format != D3DFMT_NULL)
    {
        mUpdateInProgress = 1;
        mParent->getQueue().sendSync<Texture3DInitCmd>(this);
    }

    return true;
}

void D3DGLTexture3D::updateTexture(DWORD level, const D3DBOX &box, const GLubyte *dataPtr)
{
    CommandQueue &queue = mParent->getQueue();
    queue.lock();
    ++mUpdateInProgress;
    queue.doSend<Texture3DLoadLevelCmd>(this, level, box, dataPtr);
    queue.unlock();
}

void D3DGLTexture3D::addIface()
{
    ++mIfaceCount;
}

void D3DGLTexture3D::releaseIface()
{
    if(--mIfaceCount == 0)
        delete this;
}


HRESULT D3DGLTexture3D::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLTexture3D);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DVolumeTexture9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DBaseTexture9);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DResource9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    TRACE("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

ULONG D3DGLTexture3D::AddRef()
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

ULONG D3DGLTexture3D::Release()
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


HRESULT D3DGLTexture3D::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLTexture3D::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLTexture3D::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT D3DGLTexture3D::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD D3DGLTexture3D::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD D3DGLTexture3D::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void D3DGLTexture3D::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE D3DGLTexture3D::GetType()
{
    TRACE("iface %p\n", this);
    return D3DRTYPE_VOLUMETEXTURE;
}


DWORD D3DGLTexture3D::SetLOD(DWORD lod)
{
    TRACE("iface %p, lod %lu\n", this, lod);

    if(mDesc.Pool != D3DPOOL_MANAGED || mDesc.Format == D3DFMT_NULL)
        return 0;

    lod = std::min(lod, (DWORD)mVolumes.size()-1);

    // FIXME: Set GL_TEXTURE_BASE_LEVEL? Or GL_TEXTURE_MIN_LOD?
    lod = mLodLevel.exchange(lod);

    return lod;
}

DWORD D3DGLTexture3D::GetLOD()
{
    TRACE("iface %p\n", this);
    return mLodLevel.load();
}

DWORD D3DGLTexture3D::GetLevelCount()
{
    TRACE("iface %p\n", this);
    return mVolumes.size();
}

HRESULT D3DGLTexture3D::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE type)
{
    FIXME("iface %p, type 0x%x : stub!\n", this, type);
    return D3D_OK;
}

D3DTEXTUREFILTERTYPE D3DGLTexture3D::GetAutoGenFilterType()
{
    FIXME("iface %p\n", this);
    return D3DTEXF_LINEAR;
}

void D3DGLTexture3D::GenerateMipSubLevels()
{
    TRACE("iface %p\n", this);
    if(mDesc.Format != D3DFMT_NULL)
        mParent->getQueue().send<Texture3DGenMipCmd>(this);
}


HRESULT D3DGLTexture3D::GetLevelDesc(UINT level, D3DVOLUME_DESC *desc)
{
    TRACE("iface %p, level %u, desc %p\n", this, level, desc);

    if(level >= mVolumes.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mVolumes.size());
        return D3DERR_INVALIDCALL;
    }

    return mVolumes[level]->GetDesc(desc);
}

HRESULT D3DGLTexture3D::GetVolumeLevel(UINT level, IDirect3DVolume9 **volume)
{
    TRACE("iface %p, level %u, surface %p\n", this, level, volume);

    if(level >= mVolumes.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mVolumes.size());
        return D3DERR_INVALIDCALL;
    }

    *volume = mVolumes[level];
    (*volume)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLTexture3D::LockBox(UINT level, D3DLOCKED_BOX *lockedbox, const D3DBOX *box, DWORD flags)
{
    TRACE("iface %p, level %u, lockedbox %p, box %p, flags 0x%lx\n", this, level, lockedbox, box, flags);

    if(level >= mVolumes.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mVolumes.size());
        return D3DERR_INVALIDCALL;
    }

    return mVolumes[level]->LockBox(lockedbox, box, flags);
}

HRESULT D3DGLTexture3D::UnlockBox(UINT level)
{
    TRACE("iface %p, level %u\n", this, level);

    if(level >= mVolumes.size())
    {
        WARN("Level out of range (%u >= %u)\n", level, mVolumes.size());
        return D3DERR_INVALIDCALL;
    }

    return mVolumes[level]->UnlockBox();
}

HRESULT D3DGLTexture3D::AddDirtyBox(const D3DBOX *box)
{
    TRACE("iface %p, box %p\n", this, box);
    mDirtyBox.Left = std::min(mDirtyBox.Left, box->Left);
    mDirtyBox.Top = std::min(mDirtyBox.Top, box->Top);
    mDirtyBox.Front = std::min(mDirtyBox.Front, box->Front);
    mDirtyBox.Right = std::max(mDirtyBox.Right, box->Right);
    mDirtyBox.Bottom = std::max(mDirtyBox.Bottom, box->Bottom);
    mDirtyBox.Back = std::max(mDirtyBox.Back, box->Back);
    return D3D_OK;
}



D3DGLTextureVolume::D3DGLTextureVolume(D3DGLTexture3D *parent, UINT level)
  : mParent(parent)
  , mLevel(level)
  , mLock(LT_Unlocked)
  , mDataOffset(0)
  , mDataLength(0)
{
}

D3DGLTextureVolume::~D3DGLTextureVolume()
{
}

void D3DGLTextureVolume::init(UINT offset, UINT length)
{
    mDataOffset = offset;
    mDataLength = length;
}


HRESULT D3DGLTextureVolume::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLTextureVolume);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DVolume9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    TRACE("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

ULONG D3DGLTextureVolume::AddRef()
{
    TRACE("iface %p\n", this);
    return mParent->AddRef();
}

ULONG D3DGLTextureVolume::Release()
{
    TRACE("iface %p\n", this);
    return mParent->Release();
}


HRESULT D3DGLTextureVolume::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    return mParent->GetDevice(device);
}

HRESULT D3DGLTextureVolume::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLTextureVolume::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT D3DGLTextureVolume::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

HRESULT D3DGLTextureVolume::GetContainer(REFIID riid, void **container)
{
    TRACE("iface %p, riid %s, container %p\n", this, debugstr_guid(riid), container);
    return mParent->QueryInterface(riid, container);
}

HRESULT D3DGLTextureVolume::GetDesc(D3DVOLUME_DESC *desc)
{
    TRACE("iface %p, desc %p\n", this, desc);

    desc->Format = mParent->mDesc.Format;
    desc->Usage = mParent->mDesc.Usage;
    desc->Pool = mParent->mDesc.Pool;
    desc->Width = std::max(1u, mParent->mDesc.Width>>mLevel);
    desc->Height = std::max(1u, mParent->mDesc.Height>>mLevel);
    desc->Depth = std::max(1u, mParent->mDesc.Depth>>mLevel);
    return D3D_OK;
}

HRESULT D3DGLTextureVolume::LockBox(D3DLOCKED_BOX *lockedbox, const D3DBOX *box, DWORD flags)
{
    TRACE("iface %p, lockedbox %p, box %p, flags 0x%lx\n", this, lockedbox, box, flags);

    if(mParent->mDesc.Format == D3DFMT_NULL)
    {
        FIXME("Attempting to lock NULL format texture\n");
        return D3DERR_INVALIDCALL;
    }

    if(mParent->mDesc.Pool == D3DPOOL_DEFAULT)
    {
        FIXME("Trying to lock texture in default pool\n");
        return D3DERR_INVALIDCALL;
    }

    DWORD unknown_flags = flags & ~(D3DLOCK_DISCARD|D3DLOCK_NOOVERWRITE|D3DLOCK_READONLY|D3DLOCK_NO_DIRTY_UPDATE);
    if(unknown_flags) FIXME("Unknown lock flags: 0x%lx\n", unknown_flags);

    UINT w = std::max(1u, mParent->mDesc.Width>>mLevel);
    UINT h = std::max(1u, mParent->mDesc.Height>>mLevel);
    UINT d = std::max(1u, mParent->mDesc.Depth>>mLevel);
    D3DBOX full = { 0, 0, 0, w, h, d };
    if((flags&D3DLOCK_DISCARD))
    {
        if((flags&D3DLOCK_READONLY))
        {
            WARN("Read-only discard specified\n");
            return D3DERR_INVALIDCALL;
        }
        if(box)
        {
            WARN("Discardable box specified\n");
            return D3DERR_INVALIDCALL;
        }
    }
    if(!box)
        box = &full;

    {
        LockType lt = ((flags&D3DLOCK_READONLY) ? LT_ReadOnly : LT_Full);
        LockType nolock = LT_Unlocked;
        if(!mLock.compare_exchange_strong(nolock, lt))
        {
            WARN("Texture surface %u already locked!\n", mLevel);
            return D3DERR_INVALIDCALL;
        }
    }

    // No need to wait if we're not writing over previous data.
    if(!(flags&D3DLOCK_NOOVERWRITE) && !(flags&D3DLOCK_READONLY))
    {
        while(mParent->mUpdateInProgress)
            mParent->mParent->getQueue().wakeAndSleep();
    }

    GLubyte *memPtr = &mParent->mSysMem[mDataOffset];
    mLockRegion = *box;
    if(mParent->mIsCompressed && !(mParent->mGLFormat->flags&GLFormatInfo::BadPitch))
    {
        int pitch = GLFormatInfo::calcBlockPitch(w, mParent->mGLFormat->bytesperblock);
        int slice = ((h+3)/4) * pitch;
        memPtr += box->Front*slice + (box->Top/4*pitch) + (box->Left/4*mParent->mGLFormat->bytesperblock);
        lockedbox->RowPitch = pitch;
        lockedbox->SlicePitch = slice;
    }
    else
    {
        int pitch = GLFormatInfo::calcPitch(w, mParent->mGLFormat->bytesperpixel);
        int slice = h * pitch;
        memPtr += box->Front*slice + (box->Top*pitch) + (box->Left*mParent->mGLFormat->bytesperpixel);
        lockedbox->RowPitch = pitch;
        lockedbox->SlicePitch = slice;
    }
    lockedbox->pBits = memPtr;

    if(!(flags&(D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY)))
    {
        D3DBOX dirty = { box->Left<<mLevel, box->Top<<mLevel,
                         box->Right<<mLevel, box->Bottom<<mLevel,
                         box->Front<<mLevel, box->Back<<mLevel };
        mParent->AddDirtyBox(&dirty);
    }

    TRACE("Locked region: pBits=%p, RowPitch=%d, SlicePitch=%d\n", lockedbox->pBits, lockedbox->RowPitch, lockedbox->SlicePitch);
    return D3D_OK;
}

HRESULT D3DGLTextureVolume::UnlockBox()
{
    TRACE("iface %p\n", this);

    if(mLock == LT_Unlocked)
    {
        ERR("Attempted to unlock an unlocked surface\n");
        return D3DERR_INVALIDCALL;
    }

    if(mLock != LT_ReadOnly)
        mParent->updateTexture(mLevel, mLockRegion, &mParent->mSysMem[mDataOffset]);

    mLock = LT_Unlocked;
    return D3D_OK;
}
