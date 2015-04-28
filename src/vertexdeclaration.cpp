
#include "vertexdeclaration.hpp"

#include "trace.hpp"
#include "device.hpp"
#include "private_iids.hpp"


namespace
{

static const struct {
    unsigned int component_count;
    unsigned int component_size;
} d3d_dtype_lookup[] = {
    /* D3DDECLTYPE_FLOAT1    */ {1, sizeof(float)},
    /* D3DDECLTYPE_FLOAT2    */ {2, sizeof(float)},
    /* D3DDECLTYPE_FLOAT3    */ {3, sizeof(float)},
    /* D3DDECLTYPE_FLOAT4    */ {4, sizeof(float)},
    /* D3DDECLTYPE_D3DCOLOR  */ {4, sizeof(BYTE)},
    /* D3DDECLTYPE_UBYTE4    */ {4, sizeof(BYTE)},
    /* D3DDECLTYPE_SHORT2    */ {2, sizeof(short int)},
    /* D3DDECLTYPE_SHORT4    */ {4, sizeof(short int)},
    /* D3DDECLTYPE_UBYTE4N   */ {4, sizeof(BYTE)},
    /* D3DDECLTYPE_SHORT2N   */ {2, sizeof(short int)},
    /* D3DDECLTYPE_SHORT4N   */ {4, sizeof(short int)},
    /* D3DDECLTYPE_USHORT2N  */ {2, sizeof(short int)},
    /* D3DDECLTYPE_USHORT4N  */ {4, sizeof(short int)},
    /* D3DDECLTYPE_UDEC3     */ {3, sizeof(short int)},
    /* D3DDECLTYPE_DEC3N     */ {3, sizeof(short int)},
    /* D3DDECLTYPE_FLOAT16_2 */ {2, sizeof(short int)},
    /* D3DDECLTYPE_FLOAT16_4 */ {4, sizeof(short int)}
};

} // namespace


D3DGLVertexDeclaration::D3DGLVertexDeclaration(D3DGLDevice *parent)
  : mRefCount(0)
  , mIfaceCount(0)
  , mParent(parent)
  , mIsAuto(false)
  , mFvf(0)
{
}

D3DGLVertexDeclaration::~D3DGLVertexDeclaration()
{
}

HRESULT D3DGLVertexDeclaration::init(DWORD fvf, bool isauto)
{
    bool has_pos = (fvf&D3DFVF_POSITION_MASK) != 0;
    bool has_blend = (fvf&D3DFVF_XYZB5) > D3DFVF_XYZRHW;
    bool has_blend_idx = has_blend &&
        ((fvf&D3DFVF_XYZB5) == D3DFVF_XYZB5 ||
         (fvf&D3DFVF_LASTBETA_D3DCOLOR) ||
         (fvf&D3DFVF_LASTBETA_UBYTE4));
    bool has_normal = (fvf&D3DFVF_NORMAL) != 0;
    bool has_psize = (fvf&D3DFVF_PSIZE) != 0;

    bool has_diffuse = (fvf&D3DFVF_DIFFUSE) != 0;
    bool has_specular = (fvf&D3DFVF_SPECULAR) !=0;

    DWORD num_textures = (fvf&D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    DWORD texcoords = (fvf&0xFFFF0000) >> 16;
    DWORD num_blends = 1 + (((fvf&D3DFVF_XYZB5) - D3DFVF_XYZB1) >> 1);
    if (has_blend_idx) num_blends--;

    std::vector<D3DVERTEXELEMENT9> elems;
    elems.reserve(has_pos + (has_blend && num_blends > 0) + has_blend_idx + has_normal +
                  has_psize + has_diffuse + has_specular + num_textures + 1);

    if(has_pos)
    {
        if(!has_blend && (fvf&D3DFVF_XYZRHW))
            elems.push_back({0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0});
        else
        {
            if(!has_blend && (fvf&D3DFVF_XYZW) == D3DFVF_XYZW)
                elems.push_back({0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0});
            else
                elems.push_back({0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0});
        }
    }
    if(has_blend && num_blends > 0)
    {
        BYTE type = D3DDECLTYPE_D3DCOLOR;
        if((fvf&D3DFVF_XYZB5) != D3DFVF_XYZB2 || !(fvf&D3DFVF_LASTBETA_D3DCOLOR))
        {
            switch(num_blends)
            {
                case 1: type = D3DDECLTYPE_FLOAT1; break;
                case 2: type = D3DDECLTYPE_FLOAT2; break;
                case 3: type = D3DDECLTYPE_FLOAT3; break;
                case 4: type = D3DDECLTYPE_FLOAT4; break;
                default:
                    ERR("Unexpected amount of blend values: %lu\n", num_blends);
            }
        }
        elems.push_back({0, 0, type, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0});
    }
    if(has_blend_idx)
    {
        if((fvf&D3DFVF_LASTBETA_UBYTE4) || ((fvf&D3DFVF_XYZB5) == D3DFVF_XYZB2 &&
                                            (fvf&D3DFVF_LASTBETA_D3DCOLOR)))
            elems.push_back({0, 0, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0});
        else if((fvf&D3DFVF_LASTBETA_D3DCOLOR))
            elems.push_back({0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0});
        else
            elems.push_back({0, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0});
    }
    if(has_normal) elems.push_back({0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0});
    if(has_psize) elems.push_back({0, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0});
    if(has_diffuse) elems.push_back({0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0});
    if(has_specular) elems.push_back({0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1});
    for(DWORD i = 0;i < num_textures;++i)
    {
        BYTE type = D3DDECLTYPE_FLOAT1;
        unsigned int numcoords = (texcoords>>(i*2)) & 0x03;
        switch(numcoords)
        {
            case D3DFVF_TEXTUREFORMAT1: type = D3DDECLTYPE_FLOAT1; break;
            case D3DFVF_TEXTUREFORMAT2: type = D3DDECLTYPE_FLOAT2; break;
            case D3DFVF_TEXTUREFORMAT3: type = D3DDECLTYPE_FLOAT3; break;
            case D3DFVF_TEXTUREFORMAT4: type = D3DDECLTYPE_FLOAT4; break;
        }
        elems.push_back({0, 0, type, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, (BYTE)i});
    }

    DWORD offset = 0;
    for(auto &elem : elems)
    {
        elem.Offset = offset;
        offset += d3d_dtype_lookup[elem.Type].component_count *
                  d3d_dtype_lookup[elem.Type].component_size;
    }

    elems.push_back(D3DDECL_END());
    return init(elems.data(), isauto);
}

HRESULT D3DGLVertexDeclaration::init(const D3DVERTEXELEMENT9 *elems, bool isauto)
{
    mIsAuto = isauto;

    bool haspos=false, haspost=false;
    size_t size = 0;
    while(!isEnd(elems[size]))
    {
        if((elems[size].Offset&3) != 0)
        {
            FIXME("Invalid element offset (%u, expected alignment of 4)\n", elems[size].Offset);
            return E_FAIL;
        }

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
        return E_FAIL;
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
            case D3DDECLTYPE_DEC3N:
                FIXME("D3DDECLTYPE_DEC3N does not ignore alpha value like it should\n");
                elem.mGLType = GL_INT_2_10_10_10_REV;
                elem.mGLCount = 4;
                elem.mNormalize = GL_TRUE;
                break;
            case D3DDECLTYPE_UDEC3:
                FIXME("D3DDECLTYPE_UDEC3 does not ignore alpha value like it should\n");
                elem.mGLType = GL_UNSIGNED_INT_2_10_10_10_REV;
                elem.mGLCount = 4;
                elem.mNormalize = GL_FALSE;
                break;

            default:
                FIXME("Unhandled element type: 0x%x\n", (UINT)elem.Type);
                return E_FAIL;
        }

        TRACE("Index: %u, Offset: %u, Type: 0x%x, Method: 0x%x, Usage: 0x%x, UsageIndex: %u\n",
              (UINT)elem.Stream, (UINT)elem.Offset, (UINT)elem.Type,
              (UINT)elem.Method, (UINT)elem.Usage, (UINT)elem.UsageIndex);
    }

    // FIXME: If the declaration resembles an FVF, an FVF code should reflect it
    //if(!mFvf)
    //{
    //}

    return D3D_OK;
}

ULONG D3DGLVertexDeclaration::releaseIface()
{
    ULONG ret = --mIfaceCount;
    if(ret == 0 && !mIsAuto)
        delete this;
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
