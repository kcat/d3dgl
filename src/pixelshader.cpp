
#include "pixelshader.hpp"

#include "device.hpp"
#include "trace.hpp"
#include "private_iids.hpp"


D3DGLPixelShader::D3DGLPixelShader(D3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
{
    mParent->AddRef();
}

D3DGLPixelShader::~D3DGLPixelShader()
{
    mParent->Release();
}

bool D3DGLPixelShader::init(const DWORD *data)
{
    ERR("Pretending to load %s shader %lu.%lu\n", (((*data>>16)==0xfffe) ? "vertex" :
                                                   ((*data>>16)==0xffff) ? "pixel" : "unknown"),
                                                  (*data>>8)&0xff, *data&0xff);
    return true;
}


HRESULT D3DGLPixelShader::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLPixelShader);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DPixelShader9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLPixelShader::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG D3DGLPixelShader::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLPixelShader::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);

    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLPixelShader::GetFunction(void *data, UINT *size)
{
    TRACE("iface %p, data %p, size %p\n", this, data, size);

    *size = mCode.size();
    if(data)
        memcpy(data, mCode.data(), mCode.size());
    return D3D_OK;
}
