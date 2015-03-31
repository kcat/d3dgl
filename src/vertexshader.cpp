
#include "vertexshader.hpp"

#include "device.hpp"
#include "trace.hpp"
#include "private_iids.hpp"


D3DGLVertexShader::D3DGLVertexShader(D3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
{
    mParent->AddRef();
}

D3DGLVertexShader::~D3DGLVertexShader()
{
    mParent->Release();
}

bool D3DGLVertexShader::init(const DWORD *data)
{
    ERR("Failing\n");
    return false;
}


HRESULT D3DGLVertexShader::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLVertexShader);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DVertexShader9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLVertexShader::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG D3DGLVertexShader::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLVertexShader::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);

    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLVertexShader::GetFunction(void *data, UINT *size)
{
    TRACE("iface %p, data %p, size %p\n", this, data, size);

    *size = mCode.size();
    if(data)
        memcpy(data, mCode.data(), mCode.size());
    return D3D_OK;
}
