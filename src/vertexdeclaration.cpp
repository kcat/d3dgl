
#include "vertexdeclaration.hpp"

#include "trace.hpp"
#include "device.hpp"
#include "private_iids.hpp"


D3DGLVertexDeclaration::D3DGLVertexDeclaration(D3DGLDevice *parent)
  : mRefCount(0)
  , mIfaceCount(0)
  , mParent(parent)
{
}

D3DGLVertexDeclaration::~D3DGLVertexDeclaration()
{
}

bool D3DGLVertexDeclaration::init(const D3DVERTEXELEMENT9 *elems)
{
    bool haspos=false, haspost=false;
    size_t size = 0;
    while(!isEnd(elems[size]))
    {
        if(elems[size].Usage == D3DDECLUSAGE_POSITION)
            haspos = true;
        else if(elems[size].Usage == D3DDECLUSAGE_POSITIONT)
            haspost = true;
        else if(elems[size].Usage == D3DDECLUSAGE_NORMAL) {
        }
        else if(elems[size].Usage == D3DDECLUSAGE_BINORMAL) {
        }
        else if(elems[size].Usage == D3DDECLUSAGE_TANGENT) {
        }
        else if(elems[size].Usage == D3DDECLUSAGE_COLOR) {
        }
        else if(elems[size].Usage == D3DDECLUSAGE_TEXCOORD) {
        }
        else if(elems[size].Usage == D3DDECLUSAGE_BLENDWEIGHT || elems[size].Usage == D3DDECLUSAGE_BLENDINDICES) {
        }
        else
            FIXME("Unhandled element usage type: 0x%x\n", elems[size].Usage);
        ++size;
    }

    if(haspos && haspost)
    {
        FIXME("Position and PositionT specified in declaration\n");
        return false;
    }

    mElements.resize(size);
    for(D3DGLVERTEXELEMENT &elem : mElements)
    {
        elem.Stream = elems->Stream;
        elem.Offset = elems->Offset;
        elem.Type   = elems->Type;
        elem.Method = elems->Method;
        elem.Usage  = elems->Usage;
        elem.UsageIndex = elems->UsageIndex;
        ++elems;

        switch(elem.Type)
        {
            case D3DDECLTYPE_FLOAT1:
                elem.mGLType = GL_FLOAT;
                elem.mGLCount = 1;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_FLOAT2:
                elem.mGLType = GL_FLOAT;
                elem.mGLCount = 2;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_FLOAT3:
                elem.mGLType = GL_FLOAT;
                elem.mGLCount = 3;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_FLOAT4:
                elem.mGLType = GL_FLOAT;
                elem.mGLCount = 4;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_D3DCOLOR:
                elem.mGLType = GL_UNSIGNED_BYTE;
                elem.mGLCount = GL_BGRA;
                elem.mNormalize = GL_TRUE;
                break;
            case D3DDECLTYPE_UBYTE4N:
                elem.mGLType = GL_UNSIGNED_BYTE;
                elem.mGLCount = 4;
                elem.mNormalize = GL_TRUE;
                break;
            case D3DDECLTYPE_UBYTE4:
                elem.mGLType = GL_UNSIGNED_BYTE;
                elem.mGLCount = 4;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_FLOAT16_2:
                elem.mGLType = GL_HALF_FLOAT_ARB;
                elem.mGLCount = 2;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_FLOAT16_4:
                elem.mGLType = GL_HALF_FLOAT_ARB;
                elem.mGLCount = 4;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_SHORT2N:
                elem.mGLType = GL_SHORT;
                elem.mGLCount = 2;
                elem.mNormalize = GL_TRUE;
                break;
            case D3DDECLTYPE_SHORT4N:
                elem.mGLType = GL_SHORT;
                elem.mGLCount = 4;
                elem.mNormalize = GL_TRUE;
                break;
            case D3DDECLTYPE_SHORT2:
                elem.mGLType = GL_SHORT;
                elem.mGLCount = 2;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_SHORT4:
                elem.mGLType = GL_SHORT;
                elem.mGLCount = 4;
                elem.mNormalize = GL_FALSE;
                break;
            case D3DDECLTYPE_USHORT2N:
                elem.mGLType = GL_UNSIGNED_SHORT;
                elem.mGLCount = 2;
                elem.mNormalize = GL_TRUE;
                break;
            case D3DDECLTYPE_USHORT4N:
                elem.mGLType = GL_UNSIGNED_SHORT;
                elem.mGLCount = 4;
                elem.mNormalize = GL_TRUE;
                break;

            /* FIXME: Handle these */
#if 0
            case D3DDECLTYPE_DEC3N:
            case D3DDECLTYPE_UDEC3:
#endif
            default:
                ERR("Unhandled element type: 0x%x\n", (UINT)elem.Type);
                return false;
        }

        TRACE("Index: %u, Offset: %u, Type: 0x%x, Method: 0x%x, Usage: 0x%x, UsageIndex: %u\n",
              (UINT)elem.Stream, (UINT)elem.Offset, (UINT)elem.Type,
              (UINT)elem.Method, (UINT)elem.Usage, (UINT)elem.UsageIndex);
    }

    // FIXME: If the declaration resembles an FVF, an FVF code should reflect it
    //mFvf = 0;

    return true;
}

ULONG D3DGLVertexDeclaration::releaseIface()
{
    ULONG ret = --mIfaceCount;
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLVertexDeclaration::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLVertexDeclaration);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DVertexDeclaration9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLVertexDeclaration::AddRef()
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

ULONG D3DGLVertexDeclaration::Release()
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


HRESULT D3DGLVertexDeclaration::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLVertexDeclaration::GetDeclaration(D3DVERTEXELEMENT9 *elems, UINT *count)
{
    TRACE("iface %p, elems %p, count %p\n", this, elems, count);

    *count = mElements.size()+1;
    if(elems)
    {
        for(const auto &elem : mElements)
            *(elems++) = elem;
        *elems = (D3DVERTEXELEMENT9)D3DDECL_END();
    }

    return D3D_OK;
}
