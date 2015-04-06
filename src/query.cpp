
#include "query.hpp"

#include "device.hpp"
#include "private_iids.hpp"


D3DGLQuery::D3DGLQuery(D3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
  , mState(Signaled)
{
    mParent->AddRef();
}

D3DGLQuery::~D3DGLQuery()
{
    mParent->Release();
}

bool D3DGLQuery::init(D3DQUERYTYPE type)
{
    mType = type;

    if(mType == D3DQUERYTYPE_OCCLUSION)
    {
        ERR("Faking occlusion query\n");
        return true;
    }

    FIXME("Query type %s unsupported\n", d3dquery_to_str(mType));
    return false;
}


HRESULT D3DGLQuery::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLQuery);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DQuery9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLQuery::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG D3DGLQuery::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLQuery::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

D3DQUERYTYPE D3DGLQuery::GetType()
{
    TRACE("iface %p\n", this);
    return mType;
}

DWORD D3DGLQuery::GetDataSize()
{
    TRACE("iface %p\n", this);

    if(mType == D3DQUERYTYPE_OCCLUSION)
        return sizeof(DWORD);

    ERR("Unexpected query type: %s\n", d3dquery_to_str(mType));
    return 0;
}

HRESULT D3DGLQuery::Issue(DWORD flags)
{
    FIXME("iface %p, flags 0x%lx : stub\n", this, flags);

    if((flags&~(D3DISSUE_END|D3DISSUE_BEGIN)))
    {
        FIXME("Unhandled flags: 0x%lx\n", flags);
        return D3DERR_INVALIDCALL;
    }

    if((flags&D3DISSUE_END) && mState == Building)
        mState = Issued;

    if((flags&D3DISSUE_BEGIN))
        mState = Building;

    return D3D_OK;
}

HRESULT D3DGLQuery::GetData(void *data, DWORD size, DWORD flags)
{
    FIXME("iface %p, data %p, size %lu, flags 0x%lx : stub\n", this, data, size, flags);

    union {
        void *pointer;
        DWORD *occlusion_result;
    };
    pointer = data;

    // Fake a delay
    if(mState == Issued)
    {
        mState = Signaled;
        return S_FALSE;
    }
    if(mState == Signaled)
    {
        if(mType == D3DQUERYTYPE_OCCLUSION)
        {
            if(size < sizeof(*occlusion_result))
            {
                WARN("Size %lu too small\n", size);
                return D3DERR_INVALIDCALL;
            }
            *occlusion_result = 1;
            return D3D_OK;
        }

        ERR("Unexpected query type: %s\n", d3dquery_to_str(mType));
        return E_NOTIMPL;
    }

    WARN("Called while building query\n");
    return D3DERR_INVALIDCALL;
}
