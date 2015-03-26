#ifndef TRACE_HPP
#define TRACE_HPP

#include <cstdio>
#include <d3d9.h>
#include <ctype.h>


enum eLogLevel {
    ERR_=0,
    FIXME_=1,
    WARN_=2,
    TRACE_=3
};
extern eLogLevel LogLevel;
extern FILE *LogFile;


#define D3DGL_PRINT(TYPE, MSG, ...) do {                                            \
    fprintf(LogFile, "%04lx:" TYPE ":d3dgl:%s " MSG, GetCurrentThreadId(), __PRETTY_FUNCTION__ , ## __VA_ARGS__); \
    fflush(LogFile);                                                          \
} while(0)

#define TRACE(...) do {                                                       \
    if(LogLevel >= TRACE_)                                                    \
        D3DGL_PRINT("trace", __VA_ARGS__);                                    \
} while(0)

#define WARN(...) do {                                                        \
    if(LogLevel >= WARN_)                                                     \
        D3DGL_PRINT("warn", __VA_ARGS__);                                     \
} while(0)

#define FIXME(...) do {                                                       \
    if(LogLevel >= FIXME_)                                                    \
        D3DGL_PRINT("fixme", __VA_ARGS__);                                    \
} while(0)

#define ERR(...) do {                                                         \
    if(LogLevel >= ERR_)                                                      \
        D3DGL_PRINT("err", __VA_ARGS__);                                      \
} while(0)


//#define checkGLError()
#ifndef checkGLError
#define checkGLError() do {          \
    GLenum err = glGetError();       \
    if(err != GL_NO_ERROR)           \
    {                                \
        ERR("<<<<<<<<< GL error detected @ %s:%u: 0x%04x\n", __FILE__, __LINE__, err);\
        while((err=glGetError()) != GL_NO_ERROR) \
           ERR("<<<<<<< GL error detected: 0x%04x\n", err);\
    }                                \
} while(0)
#endif


class debugstr_guid {
    char mStr[64];

public:
    debugstr_guid(const IID &id)
    {
        snprintf(mStr, sizeof(mStr), "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 id.Data1, id.Data2, id.Data3,
                 id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
                 id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);
    }
    debugstr_guid(const GUID *id)
    {
        if(!id)
            snprintf(mStr, sizeof(mStr), "(null)");
        else
            snprintf(mStr, sizeof(mStr), "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                     id->Data1, id->Data2, id->Data3,
                     id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                     id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    }

    operator const char*() const
    { return mStr; }
};
#define debugstr_guid(x) ((const char*)debugstr_guid(x))

#define TEST(id)  case id: return #id
class d3dfmt_to_str {
    char mText[12];
    D3DFORMAT mFormat;

public:
    d3dfmt_to_str(D3DFORMAT format) : mFormat(format) { }

    operator const char*()
    {
        switch(mFormat)
        {
        TEST(D3DFMT_UNKNOWN);
        TEST(D3DFMT_R8G8B8);
        TEST(D3DFMT_A8R8G8B8);
        TEST(D3DFMT_X8R8G8B8);
        TEST(D3DFMT_R5G6B5);
        TEST(D3DFMT_X1R5G5B5);
        TEST(D3DFMT_A1R5G5B5);
        TEST(D3DFMT_A4R4G4B4);
        TEST(D3DFMT_R3G3B2);
        TEST(D3DFMT_A8);
        TEST(D3DFMT_A8R3G3B2);
        TEST(D3DFMT_X4R4G4B4);
        TEST(D3DFMT_A2B10G10R10);
        TEST(D3DFMT_A8B8G8R8);
        TEST(D3DFMT_X8B8G8R8);
        TEST(D3DFMT_G16R16);
        TEST(D3DFMT_A2R10G10B10);
        TEST(D3DFMT_A16B16G16R16);
        TEST(D3DFMT_A8P8);
        TEST(D3DFMT_P8);
        TEST(D3DFMT_L8);
        TEST(D3DFMT_A8L8);
        TEST(D3DFMT_A4L4);
        TEST(D3DFMT_V8U8);
        TEST(D3DFMT_L6V5U5);
        TEST(D3DFMT_X8L8V8U8);
        TEST(D3DFMT_Q8W8V8U8);
        TEST(D3DFMT_V16U16);
        TEST(D3DFMT_A2W10V10U10);
        TEST(D3DFMT_UYVY);
        TEST(D3DFMT_YUY2);
        TEST(D3DFMT_DXT1);
        TEST(D3DFMT_DXT2);
        TEST(D3DFMT_DXT3);
        TEST(D3DFMT_DXT4);
        TEST(D3DFMT_DXT5);
        TEST(D3DFMT_MULTI2_ARGB8);
        TEST(D3DFMT_G8R8_G8B8);
        TEST(D3DFMT_R8G8_B8G8);
        TEST(D3DFMT_D16_LOCKABLE);
        TEST(D3DFMT_D32);
        TEST(D3DFMT_D15S1);
        TEST(D3DFMT_D24S8);
        TEST(D3DFMT_D24X8);
        TEST(D3DFMT_D24X4S4);
        TEST(D3DFMT_D16);
        TEST(D3DFMT_L16);
        TEST(D3DFMT_D32F_LOCKABLE);
        TEST(D3DFMT_D24FS8);
        TEST(D3DFMT_VERTEXDATA);
        TEST(D3DFMT_INDEX16);
        TEST(D3DFMT_INDEX32);
        TEST(D3DFMT_Q16W16V16U16);
        TEST(D3DFMT_R16F);
        TEST(D3DFMT_G16R16F);
        TEST(D3DFMT_A16B16G16R16F);
        TEST(D3DFMT_R32F);
        TEST(D3DFMT_G32R32F);
        TEST(D3DFMT_A32B32G32R32F);
        TEST(D3DFMT_CxV8U8);
        case D3DFMT_FORCE_DWORD:
            break;
        }

        if(isprint(mFormat&0xff) && isprint((mFormat>>8)&0xff) && isprint((mFormat>>16)&0xff) && isprint((mFormat>>24)&0xff))
            snprintf(mText, sizeof(mText), "'%c%c%c%c'", (mFormat&0xff), ((mFormat>>8)&0xff), ((mFormat>>16)&0xff), ((mFormat>>24)&0xff));
        else
            snprintf(mText, sizeof(mText), "0x%x", mFormat);
        return mText;
    }
};
#undef TEST
#define d3dfmt_to_str(x) ((const char*)d3dfmt_to_str(x))

#define TEST(x) case x: return #x
class d3drs_to_str {
    char mBuffer[12];
    D3DRENDERSTATETYPE mType;

public:
    d3drs_to_str(D3DRENDERSTATETYPE type) : mType(type) { }

    operator const char*()
    {
        switch(mType)
        {
        TEST(D3DRS_ZENABLE);
        TEST(D3DRS_FILLMODE);
        TEST(D3DRS_SHADEMODE);
        TEST(D3DRS_ZWRITEENABLE);
        TEST(D3DRS_ALPHATESTENABLE);
        TEST(D3DRS_LASTPIXEL);
        TEST(D3DRS_SRCBLEND);
        TEST(D3DRS_DESTBLEND);
        TEST(D3DRS_CULLMODE);
        TEST(D3DRS_ZFUNC);
        TEST(D3DRS_ALPHAREF);
        TEST(D3DRS_ALPHAFUNC);
        TEST(D3DRS_DITHERENABLE);
        TEST(D3DRS_ALPHABLENDENABLE);
        TEST(D3DRS_FOGENABLE);
        TEST(D3DRS_SPECULARENABLE);
        TEST(D3DRS_FOGCOLOR);
        TEST(D3DRS_FOGTABLEMODE);
        TEST(D3DRS_FOGSTART);
        TEST(D3DRS_FOGEND);
        TEST(D3DRS_FOGDENSITY);
        TEST(D3DRS_RANGEFOGENABLE);
        TEST(D3DRS_STENCILENABLE);
        TEST(D3DRS_STENCILFAIL);
        TEST(D3DRS_STENCILZFAIL);
        TEST(D3DRS_STENCILPASS);
        TEST(D3DRS_STENCILFUNC);
        TEST(D3DRS_STENCILREF);
        TEST(D3DRS_STENCILMASK);
        TEST(D3DRS_STENCILWRITEMASK);
        TEST(D3DRS_TEXTUREFACTOR);
        TEST(D3DRS_WRAP0);
        TEST(D3DRS_WRAP1);
        TEST(D3DRS_WRAP2);
        TEST(D3DRS_WRAP3);
        TEST(D3DRS_WRAP4);
        TEST(D3DRS_WRAP5);
        TEST(D3DRS_WRAP6);
        TEST(D3DRS_WRAP7);
        TEST(D3DRS_CLIPPING);
        TEST(D3DRS_LIGHTING);
        TEST(D3DRS_AMBIENT);
        TEST(D3DRS_FOGVERTEXMODE);
        TEST(D3DRS_COLORVERTEX);
        TEST(D3DRS_LOCALVIEWER);
        TEST(D3DRS_NORMALIZENORMALS);
        TEST(D3DRS_DIFFUSEMATERIALSOURCE);
        TEST(D3DRS_SPECULARMATERIALSOURCE);
        TEST(D3DRS_AMBIENTMATERIALSOURCE);
        TEST(D3DRS_EMISSIVEMATERIALSOURCE);
        TEST(D3DRS_VERTEXBLEND);
        TEST(D3DRS_CLIPPLANEENABLE);
        TEST(D3DRS_POINTSIZE);
        TEST(D3DRS_POINTSIZE_MIN);
        TEST(D3DRS_POINTSPRITEENABLE);
        TEST(D3DRS_POINTSCALEENABLE);
        TEST(D3DRS_POINTSCALE_A);
        TEST(D3DRS_POINTSCALE_B);
        TEST(D3DRS_POINTSCALE_C);
        TEST(D3DRS_MULTISAMPLEANTIALIAS);
        TEST(D3DRS_MULTISAMPLEMASK);
        TEST(D3DRS_PATCHEDGESTYLE);
        TEST(D3DRS_DEBUGMONITORTOKEN);
        TEST(D3DRS_POINTSIZE_MAX);
        TEST(D3DRS_INDEXEDVERTEXBLENDENABLE);
        TEST(D3DRS_COLORWRITEENABLE);
        TEST(D3DRS_TWEENFACTOR);
        TEST(D3DRS_BLENDOP);
        TEST(D3DRS_POSITIONDEGREE);
        TEST(D3DRS_NORMALDEGREE);
        TEST(D3DRS_SCISSORTESTENABLE);
        TEST(D3DRS_SLOPESCALEDEPTHBIAS);
        TEST(D3DRS_ANTIALIASEDLINEENABLE);
        TEST(D3DRS_MINTESSELLATIONLEVEL);
        TEST(D3DRS_MAXTESSELLATIONLEVEL);
        TEST(D3DRS_ADAPTIVETESS_X);
        TEST(D3DRS_ADAPTIVETESS_Y);
        TEST(D3DRS_ADAPTIVETESS_Z);
        TEST(D3DRS_ADAPTIVETESS_W);
        TEST(D3DRS_ENABLEADAPTIVETESSELLATION);
        TEST(D3DRS_TWOSIDEDSTENCILMODE);
        TEST(D3DRS_CCW_STENCILFAIL);
        TEST(D3DRS_CCW_STENCILZFAIL);
        TEST(D3DRS_CCW_STENCILPASS);
        TEST(D3DRS_CCW_STENCILFUNC);
        TEST(D3DRS_COLORWRITEENABLE1);
        TEST(D3DRS_COLORWRITEENABLE2);
        TEST(D3DRS_COLORWRITEENABLE3);
        TEST(D3DRS_BLENDFACTOR);
        TEST(D3DRS_SRGBWRITEENABLE);
        TEST(D3DRS_DEPTHBIAS);
        TEST(D3DRS_WRAP8);
        TEST(D3DRS_WRAP9);
        TEST(D3DRS_WRAP10);
        TEST(D3DRS_WRAP11);
        TEST(D3DRS_WRAP12);
        TEST(D3DRS_WRAP13);
        TEST(D3DRS_WRAP14);
        TEST(D3DRS_WRAP15);
        TEST(D3DRS_SEPARATEALPHABLENDENABLE);
        TEST(D3DRS_SRCBLENDALPHA);
        TEST(D3DRS_DESTBLENDALPHA);
        TEST(D3DRS_BLENDOPALPHA);
        case D3DRS_FORCE_DWORD:
            break;
        }
        snprintf(mBuffer, sizeof(mBuffer), "0x%x", mType);
        return mBuffer;
    }
};
#undef TEST
#define d3drs_to_str(x) ((const char*)d3drs_to_str(x))

#define TEST(x) case x: return #x
class d3dtss_to_str {
    char mBuffer[12];
    D3DTEXTURESTAGESTATETYPE mType;

public:
    d3dtss_to_str(D3DTEXTURESTAGESTATETYPE type) : mType(type) { }

    operator const char*()
    {
        switch(mType)
        {
        TEST(D3DTSS_COLOROP);
        TEST(D3DTSS_COLORARG1);
        TEST(D3DTSS_COLORARG2);
        TEST(D3DTSS_ALPHAOP);
        TEST(D3DTSS_ALPHAARG1);
        TEST(D3DTSS_ALPHAARG2);
        TEST(D3DTSS_BUMPENVMAT00);
        TEST(D3DTSS_BUMPENVMAT01);
        TEST(D3DTSS_BUMPENVMAT10);
        TEST(D3DTSS_BUMPENVMAT11);
        TEST(D3DTSS_TEXCOORDINDEX);
        TEST(D3DTSS_BUMPENVLSCALE);
        TEST(D3DTSS_BUMPENVLOFFSET);
        TEST(D3DTSS_TEXTURETRANSFORMFLAGS);
        TEST(D3DTSS_COLORARG0);
        TEST(D3DTSS_ALPHAARG0);
        TEST(D3DTSS_RESULTARG);
        TEST(D3DTSS_CONSTANT);
        case D3DTSS_FORCE_DWORD:
            break;
        }
        snprintf(mBuffer, sizeof(mBuffer), "0x%x", mType);
        return mBuffer;
    }
};
#undef TEST
#define d3dtss_to_str(x) ((const char*)d3dtss_to_str(x))

#define TEST(x) case x: return #x
class d3dsamp_to_str {
    char mBuffer[12];
    D3DSAMPLERSTATETYPE mType;

public:
    d3dsamp_to_str(D3DSAMPLERSTATETYPE type) : mType(type) { }

    operator const char*()
    {
        switch(mType)
        {
        TEST(D3DSAMP_ADDRESSU);
        TEST(D3DSAMP_ADDRESSV);
        TEST(D3DSAMP_ADDRESSW);
        TEST(D3DSAMP_BORDERCOLOR);
        TEST(D3DSAMP_MAGFILTER);
        TEST(D3DSAMP_MINFILTER);
        TEST(D3DSAMP_MIPFILTER);
        TEST(D3DSAMP_MIPMAPLODBIAS);
        TEST(D3DSAMP_MAXMIPLEVEL);
        TEST(D3DSAMP_MAXANISOTROPY);
        TEST(D3DSAMP_SRGBTEXTURE);
        TEST(D3DSAMP_ELEMENTINDEX);
        TEST(D3DSAMP_DMAPOFFSET);
        case D3DSAMP_FORCE_DWORD:
            break;
        }
        snprintf(mBuffer, sizeof(mBuffer), "0x%x", mType);
        return mBuffer;
    }
};
#undef TEST
#define d3dsamp_to_str(x) ((const char*)d3dsamp_to_str(x))

#endif /* TRACE_HPP */
