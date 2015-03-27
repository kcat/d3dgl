
#include "bufferobject.hpp"

#include "device.hpp"


D3DGLBufferObject::D3DGLBufferObject(D3DGLDevice *parent, BufferType type)
  : mRefCount(0)
  , mParent(parent)
  , mLength(0)
  , mUsage(0)
  , mFormat(D3DFMT_UNKNOWN)
  , mFvf(0)
  , mPool(D3DPOOL_DEFAULT)
  , mPendingDraws(0)
  , mUserPtr(nullptr)
  , mLocked(false)
  , mLockedOffset(0)
  , mLockedLength(0)
  , mTarget(type)
{
    mParent->AddRef();
}

D3DGLBufferObject::~D3DGLBufferObject()
{
    mParent->Release();
}

bool D3DGLBufferObject::init_common(UINT length, DWORD usage, D3DPOOL pool)
{
    mLength = length;
    mUsage = usage;
    mPool = pool;

    if(mPool == D3DPOOL_SCRATCH)
    {
        WARN("Buffer objects not allowed in scratch mem\n");
        return false;
    }
    if(mPool == D3DPOOL_MANAGED && (mUsage&D3DUSAGE_DYNAMIC))
    {
        WARN("Managed dynamic buffers aren't allowed\n");
        return false;
    }

    mSysMem.resize(mLength);
    mUserPtr = mSysMem.data();

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


HRESULT D3DGLBufferObject::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
#define RETURN_IF_IID_TYPE(obj, riid, TYPE1, TYPE2) do {         \
    if((riid) == IID_##TYPE2)                                    \
    {                                                            \
        AddRef();                                                \
        *(obj) = static_cast<TYPE2*>(static_cast<TYPE1*>(this)); \
        return D3D_OK;                                           \
    }                                                            \
} while (0)
    if(mFormat == D3DFMT_VERTEXDATA)
    {
        RETURN_IF_IID_TYPE(obj, riid, IDirect3DVertexBuffer9, IDirect3DVertexBuffer9);
        RETURN_IF_IID_TYPE(obj, riid, IDirect3DVertexBuffer9, IDirect3DResource9);
        RETURN_IF_IID_TYPE(obj, riid, IDirect3DVertexBuffer9, IUnknown);
    }
    else
    {
        RETURN_IF_IID_TYPE(obj, riid, IDirect3DIndexBuffer9, IDirect3DIndexBuffer9);
        RETURN_IF_IID_TYPE(obj, riid, IDirect3DIndexBuffer9, IDirect3DResource9);
        RETURN_IF_IID_TYPE(obj, riid, IDirect3DIndexBuffer9, IUnknown);
    }
#undef RETURN_IF_IID_TYPE

    return E_NOINTERFACE;
}

ULONG D3DGLBufferObject::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG D3DGLBufferObject::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
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
            WARN("Locking whole buffer with offset %u\n", offset);
            return D3DERR_INVALIDCALL;
        }
        length = mLength;
    }

    if(offset >= mLength || length > mLength-offset)
    {
        WARN("Locking size larger than available (%u + %u > %u)\n", offset, length, mLength);
        return D3DERR_INVALIDCALL;
    }

    if((flags&D3DLOCK_READONLY))
    {
        if((mUsage&D3DUSAGE_WRITEONLY))
        {
            WARN("Read-only lock requested for write-only buffer\n");
            return D3DERR_INVALIDCALL;
        }
    }

    if(mLocked.exchange(true))
    {
        WARN("Locking a locked buffer\n");
        return D3DERR_INVALIDCALL;
    }

    while(mPendingDraws > 0)
        Sleep(1);

    mLockedOffset = offset;
    mLockedLength = length;

    *data = mUserPtr + mLockedOffset;
    return D3D_OK;
}

HRESULT D3DGLBufferObject::Unlock()
{
    TRACE("iface %p\n", this);

    if(mLocked == false)
    {
        WARN("Unlocking an unlocked buffer\n");
        return D3DERR_INVALIDCALL;
    }

    mLockedOffset = 0;
    mLockedLength = 0;
    mLocked = false;

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
