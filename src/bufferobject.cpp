
#include "bufferobject.hpp"

#include "device.hpp"
#include "private_iids.hpp"


void D3DGLBufferObject::initGL(const GLubyte *data)
{
    UINT data_len = (mLength+15) & ~15;

    GLenum usage = (mUsage&D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STREAM_DRAW;

    glGenBuffers(1, &mBufferId);
    glNamedBufferDataEXT(mBufferId, data_len, data, usage);
    checkGLError();

    mUpdateInProgress = 0;
}
class InitBufferObjectCmd : public Command {
    D3DGLBufferObject *mTarget;
    std::shared_ptr<GLubyte> mData;

public:
    InitBufferObjectCmd(D3DGLBufferObject *target, std::shared_ptr<GLubyte> data)
      : mTarget(target), mData(data)
    { }

    virtual ULONG execute()
    {
        mTarget->initGL(mData.get());
        return sizeof(*this);
    }
};

class DestroyBufferCmd : public Command {
    GLuint mBufferId;

public:
    DestroyBufferCmd(GLuint buffer) : mBufferId(buffer) { }

    virtual ULONG execute()
    {
        glDeleteBuffers(1, &mBufferId);
        checkGLError();
        return sizeof(*this);
    }
};

void D3DGLBufferObject::resizeBufferGL(UINT length)
{
    UINT data_len = (length+15) & ~15;
    glNamedBufferDataEXT(mBufferId, data_len, nullptr, GL_STREAM_DRAW);
    checkGLError();
}
class ResizeBufferCmd : public Command {
    D3DGLBufferObject *mTarget;
    UINT mLength;

public:
    ResizeBufferCmd(D3DGLBufferObject *target, UINT length) : mTarget(target), mLength(length) { }

    virtual ULONG execute()
    {
        mTarget->resizeBufferGL(mLength);
        return sizeof(*this);
    }
};

void D3DGLBufferObject::loadBufferDataGL(UINT offset, UINT length, const GLubyte *data, GLbitfield flags)
{
    if(!flags)
        glNamedBufferSubDataEXT(mBufferId, offset, length, &data[offset]);
    else
    {
        void *ptr = glMapNamedBufferRangeEXT(mBufferId, offset, length, flags);
        memcpy(ptr, &data[offset], length);
        glUnmapNamedBufferEXT(mBufferId);
    }
    checkGLError();

    --mUpdateInProgress;
}
class LoadBufferDataCmd : public Command {
    D3DGLBufferObject *mTarget;
    UINT mOffset;
    UINT mLength;
    std::shared_ptr<GLubyte> mData;
    GLbitfield mFlags;

public:
    LoadBufferDataCmd(D3DGLBufferObject *target, UINT offset, UINT length, std::shared_ptr<GLubyte> data, GLbitfield flags)
      : mTarget(target), mOffset(offset), mLength(length), mData(data), mFlags(flags)
    { }

    virtual ULONG execute()
    {
        mTarget->loadBufferDataGL(mOffset, mLength, mData.get(), mFlags);
        return sizeof(*this);
    }
};


D3DGLBufferObject::D3DGLBufferObject(D3DGLDevice *parent)
  : mRefCount(0)
  , mIfaceCount(0)
  , mParent(parent)
  , mLength(0)
  , mUsage(0)
  , mFormat(D3DFMT_UNKNOWN)
  , mFvf(0)
  , mPool(D3DPOOL_DEFAULT)
  , mBufferId(0)
  , mLock(LT_Unlocked)
  , mLockedOffset(0)
  , mLockedLength(0)
  , mUpdateInProgress(0)
{
}

D3DGLBufferObject::~D3DGLBufferObject()
{
    if(mBufferId)
    {
        mParent->getQueue().send<DestroyBufferCmd>(mBufferId);
        while(mUpdateInProgress)
            mParent->getQueue().wakeAndWait();
        mBufferId = 0;
    }
}

bool D3DGLBufferObject::init_common(UINT length, DWORD usage, D3DPOOL pool)
{
    mLength = length;
    mUsage = usage;
    mPool = pool;

    if(mPool == D3DPOOL_SCRATCH)
    {
        FIXME("Buffer objects not allowed in scratch mem\n");
        return false;
    }
    if(mPool == D3DPOOL_MANAGED && (mUsage&D3DUSAGE_DYNAMIC))
    {
        FIXME("Managed dynamic buffers aren't allowed\n");
        return false;
    }

    UINT data_len = (mLength+15) & ~15;
    mBufData.reset(DataAllocator<GLubyte>()(data_len), DataDeallocator<GLubyte>());
    memset(mBufData.get(), 0, data_len);

    mUpdateInProgress = 1;
    mParent->getQueue().sendSync<InitBufferObjectCmd>(this, mBufData);

    return true;
}

bool D3DGLBufferObject::init_vbo(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool)
{
    UINT size = 0;
    if((fvf&D3DFVF_XYZRHW) == D3DFVF_XYZRHW)
        size += sizeof(float)*4;
    else if((fvf&D3DFVF_XYZW) == D3DFVF_XYZW)
        size += sizeof(float)*4;
    else if((fvf&D3DFVF_XYZ) == D3DFVF_XYZ)
        size += sizeof(float)*3;
    if((fvf&D3DFVF_NORMAL))
        size += sizeof(float)*3;
    if((fvf&D3DFVF_DIFFUSE))
        size += sizeof(unsigned char)*4;
    if((fvf&D3DFVF_SPECULAR))
        size += sizeof(unsigned char)*4;
    for(size_t t = 0;t < (fvf&D3DFVF_TEXCOUNT_MASK)>>D3DFVF_TEXCOUNT_SHIFT;++t)
    {
        int num = 0;
        switch((fvf >> (16 + t*2)) & 0x03)
        {
            case D3DFVF_TEXTUREFORMAT1:
                num = 1;
                break;
            case D3DFVF_TEXTUREFORMAT2:
                num = 2;
                break;
            case D3DFVF_TEXTUREFORMAT3:
                num = 3;
                break;
            case D3DFVF_TEXTUREFORMAT4:
                num = 4;
                break;
        }
        size += sizeof(float)*num;
    }
    if(length < size)
    {
        WARN("Specified length is less than FVF size (%u < %u)\n", length, size);
        return false;
    }

    mFormat = D3DFMT_VERTEXDATA;
    mFvf = fvf;
    return init_common(length, usage, pool);
}

bool D3DGLBufferObject::init_ibo(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    if(format != D3DFMT_INDEX16 && format != D3DFMT_INDEX32)
    {
        WARN("Invalid index buffer format: %s\n", d3dfmt_to_str(format));
        return false;
    }

    mFormat = format;
    mFvf = 0;
    return init_common(length, usage, pool);
}

void D3DGLBufferObject::resetBufferData(const GLubyte *data, GLuint length)
{
    ++mUpdateInProgress;
    mParent->getQueue().lock();
    if(length > mLength)
    {
        mLength = length;
        mParent->getQueue().doSend<ResizeBufferCmd>(this, length);
    }
    if(mUpdateInProgress > 1)
    {
        UINT data_len = (mLength+15) & ~15;
        mBufData.reset(DataAllocator<GLubyte>()(data_len), DataDeallocator<GLubyte>());
    }
    memcpy(mBufData.get(), data, length);

    mParent->getQueue().doSend<LoadBufferDataCmd>(this, 0, mLength, mBufData, 0);
    mParent->getQueue().unlock();
}

ULONG D3DGLBufferObject::releaseIface()
{
    ULONG ret = --mIfaceCount;
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLBufferObject::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLBufferObject);
    if(mFormat == D3DFMT_VERTEXDATA)
    {
        RETURN_IF_IID_TYPE2(obj, riid, IDirect3DVertexBuffer9, IDirect3DVertexBuffer9);
        RETURN_IF_IID_TYPE2(obj, riid, IDirect3DVertexBuffer9, IDirect3DResource9);
        RETURN_IF_IID_TYPE2(obj, riid, IDirect3DVertexBuffer9, IUnknown);
    }
    else
    {
        RETURN_IF_IID_TYPE2(obj, riid, IDirect3DIndexBuffer9, IDirect3DIndexBuffer9);
        RETURN_IF_IID_TYPE2(obj, riid, IDirect3DIndexBuffer9, IDirect3DResource9);
        RETURN_IF_IID_TYPE2(obj, riid, IDirect3DIndexBuffer9, IUnknown);
    }

    return E_NOINTERFACE;
}

ULONG D3DGLBufferObject::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 1)
    {
        addIface();
        mParent->AddRef();
    }
    return ret;
}

ULONG D3DGLBufferObject::Release()
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


HRESULT D3DGLBufferObject::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLBufferObject::SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, refguid %s, data %p, size %lu, flags 0x%lx : stub!\n", this, debugstr_guid(refguid), data, size, flags);
    return E_NOTIMPL;
}

HRESULT D3DGLBufferObject::GetPrivateData(REFGUID refguid, void *data, DWORD *size)
{
    FIXME("iface %p, refguid %s, data %p, size %p : stub!\n", this, debugstr_guid(refguid), data, size);
    return E_NOTIMPL;
}

HRESULT D3DGLBufferObject::FreePrivateData(REFGUID refguid)
{
    FIXME("iface %p, refguid %s : stub!\n", this, debugstr_guid(refguid));
    return E_NOTIMPL;
}

DWORD D3DGLBufferObject::SetPriority(DWORD priority)
{
    FIXME("iface %p, priority %lu : stub!\n", this, priority);
    return 0;
}

DWORD D3DGLBufferObject::GetPriority()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

void D3DGLBufferObject::PreLoad()
{
    FIXME("iface %p : stub!\n", this);
}

D3DRESOURCETYPE D3DGLBufferObject::GetType()
{
    TRACE("iface %p\n", this);
    if(mFormat == D3DFMT_VERTEXDATA)
        return D3DRTYPE_VERTEXBUFFER;
    return D3DRTYPE_INDEXBUFFER;
}


HRESULT D3DGLBufferObject::Lock(UINT offset, UINT length, void **data, DWORD flags)
{
    TRACE("iface %p, offset %u, length %u, data %p, flags 0x%lx\n", this, offset, length, data, flags);

    if(length == 0)
    {
        if(offset > 0)
        {
            FIXME("Locking whole buffer with offset %u\n", offset);
            return D3DERR_INVALIDCALL;
        }
        length = mLength;
    }

    if(offset >= mLength || length > mLength-offset)
    {
        FIXME("Locking size larger than available (%u + %u > %u)\n", offset, length, mLength);
        return D3DERR_INVALIDCALL;
    }

    DWORD unknown_flags = flags & ~(D3DLOCK_READONLY|D3DLOCK_NOOVERWRITE|D3DLOCK_DISCARD|D3DLOCK_NOSYSLOCK);
    if(unknown_flags) FIXME("Unhandled flags: 0x%lx\n", unknown_flags);

    if((flags&D3DLOCK_READONLY))
    {
        if((flags&D3DLOCK_DISCARD))
        {
            FIXME("Read-only lock requested with DISCARD flag\n");
            return D3DERR_INVALIDCALL;
        }
        if((mUsage&D3DUSAGE_WRITEONLY))
        {
            FIXME("Read-only lock requested for write-only buffer\n");
            return D3DERR_INVALIDCALL;
        }
    }

    {
        LockType lt = ((flags&D3DLOCK_READONLY) ? LT_ReadOnly : LT_Full);
        LockType nolock = LT_Unlocked;
        // Apparently this is allowed? According to MSDN: "When working with
        // vertex buffers, you are allowed to make multiple lock calls;
        // however, you must ensure that the number of lock calls match the
        // number of unlock calls. DrawPrimitive calls will not succeed with
        // any outstanding lock count on any currently set vertex buffer."
        if(!mLock.compare_exchange_strong(nolock, lt))
        {
            FIXME("Locking a locked buffer\n");
            return D3DERR_INVALIDCALL;
        }
    }

    // No need to wait if we're not writing over previous data.
    if((flags&D3DLOCK_DISCARD))
    {
        if(mUpdateInProgress > 0)
        {
            UINT data_len = (mLength+15) & ~15;
            mBufData.reset(DataAllocator<GLubyte>()(data_len), DataDeallocator<GLubyte>());
        }
    }
    else if(!(flags&D3DLOCK_NOOVERWRITE) && !(flags&D3DLOCK_READONLY))
    {
        while(mUpdateInProgress)
            mParent->getQueue().wakeAndWait();
    }

    mLockedOffset = offset;
    mLockedLength = length;
    mLockedFlags  = flags;

    *data = mBufData.get() + mLockedOffset;
    return D3D_OK;
}

HRESULT D3DGLBufferObject::Unlock()
{
    TRACE("iface %p\n", this);

    if(mLock == LT_Unlocked)
    {
        WARN("Unlocking an unlocked buffer\n");
        return D3DERR_INVALIDCALL;
    }

    if(mLock != LT_ReadOnly)
    {
        ++mUpdateInProgress;
        GLbitfield flags = 0;
        if((mLockedFlags&D3DLOCK_DISCARD))
            flags |= GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT;
        else if((mLockedFlags&D3DLOCK_NOOVERWRITE))
            flags |= GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_WRITE_BIT;
        mParent->getQueue().send<LoadBufferDataCmd>(this,
            mLockedOffset, mLockedLength, mBufData, flags
        );
    }

    mLockedOffset = 0;
    mLockedLength = 0;
    mLockedFlags = 0;
    mLock = LT_Unlocked;

    return D3D_OK;
}

HRESULT D3DGLBufferObject::GetDesc(D3DVERTEXBUFFER_DESC *desc)
{
    TRACE("iface %p, desc %p\n", this, desc);

    desc->Format = mFormat;
    desc->Type = D3DRTYPE_VERTEXBUFFER;
    desc->Usage = mUsage;
    desc->Pool = mPool;
    desc->Size = mLength;
    desc->FVF = mFvf;

    return D3D_OK;
}

HRESULT D3DGLBufferObject::GetDesc(D3DINDEXBUFFER_DESC *desc)
{
    TRACE("iface %p, desc %p\n", this, desc);

    desc->Format = mFormat;
    desc->Type = D3DRTYPE_INDEXBUFFER;
    desc->Usage = mUsage;
    desc->Pool = mPool;
    desc->Size = mLength;

    return D3D_OK;
}
