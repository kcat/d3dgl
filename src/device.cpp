
#include "device.hpp"

#include <array>
#include <sstream>
#include <d3d9.h>

#include "glew.h"
#include "wglew.h"
#include "trace.hpp"
#include "glformat.hpp"
#include "d3dgl.hpp"
#include "swapchain.hpp"
#include "rendertarget.hpp"
#include "adapter.hpp"
#include "texture.hpp"
#include "texturecube.hpp"
#include "bufferobject.hpp"
#include "vertexshader.hpp"
#include "pixelshader.hpp"
#include "vertexdeclaration.hpp"
#include "query.hpp"
#include "private_iids.hpp"


namespace
{

template<typename T>
bool fmt_to_glattrs(D3DFORMAT fmt, T inserter)
{
    switch(fmt)
    {
        case D3DFMT_X8R8G8B8:
            *inserter = {WGL_RED_BITS_ARB, 8};
            *inserter = {WGL_GREEN_BITS_ARB, 8};
            *inserter = {WGL_BLUE_BITS_ARB, 8};
            *inserter = {WGL_COLOR_BITS_ARB, 32};
            return true;
        case D3DFMT_A8R8G8B8:
            *inserter = {WGL_RED_BITS_ARB, 8};
            *inserter = {WGL_GREEN_BITS_ARB, 8};
            *inserter = {WGL_BLUE_BITS_ARB, 8};
            *inserter = {WGL_ALPHA_BITS_ARB, 8};
            *inserter = {WGL_COLOR_BITS_ARB, 32};
            return true;
        case D3DFMT_D24S8:
            *inserter = {WGL_DEPTH_BITS_ARB, 24};
            *inserter = {WGL_STENCIL_BITS_ARB, 8};
            return true;
        case D3DFMT_D16:
            *inserter = {WGL_DEPTH_BITS_ARB, 16};
            return true;

        default:
            ERR("Unhandled D3DFORMAT: 0x%x\n", fmt);
            break;
    }
    return false;
}

DWORD float_to_dword(float f)
{
    union {
        float f;
        DWORD d;
    } tmp = { f };
    return tmp.d;
}
float dword_to_float(DWORD d)
{
    union {
        DWORD d;
        float f;
    } tmp = { d };
    return tmp.f;
}
static_assert(sizeof(DWORD)==sizeof(float), "Sizeof DWORD does not match float");

std::array<DWORD,210> GenerateDefaultRSValues()
{
    std::array<DWORD,210> ret{0ul};
    ret[D3DRS_ZENABLE] = D3DZB_TRUE;
    ret[D3DRS_FILLMODE] = D3DFILL_SOLID;
    ret[D3DRS_SHADEMODE] = D3DSHADE_GOURAUD;
    ret[D3DRS_ZWRITEENABLE] = TRUE;
    ret[D3DRS_ALPHATESTENABLE] = FALSE;
    ret[D3DRS_LASTPIXEL] = TRUE;
    ret[D3DRS_SRCBLEND] = D3DBLEND_ONE;
    ret[D3DRS_DESTBLEND] = D3DBLEND_ZERO;
    ret[D3DRS_CULLMODE] = D3DCULL_CCW;
    ret[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
    ret[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
    ret[D3DRS_ALPHAREF] = 0;
    ret[D3DRS_DITHERENABLE] = FALSE;
    ret[D3DRS_ALPHABLENDENABLE] = FALSE;
    ret[D3DRS_FOGENABLE] = FALSE;
    ret[D3DRS_SPECULARENABLE] = FALSE;
    ret[D3DRS_FOGCOLOR] = 0;
    ret[D3DRS_FOGTABLEMODE] = D3DFOG_NONE;
    ret[D3DRS_FOGSTART] = float_to_dword(0.0f);
    ret[D3DRS_FOGEND] = float_to_dword(1.0f);
    ret[D3DRS_FOGDENSITY] = float_to_dword(1.0f);
    ret[D3DRS_RANGEFOGENABLE] = FALSE;
    ret[D3DRS_STENCILENABLE] = FALSE;
    ret[D3DRS_STENCILFAIL] = D3DSTENCILOP_KEEP;
    ret[D3DRS_STENCILZFAIL] = D3DSTENCILOP_KEEP;
    ret[D3DRS_STENCILPASS] = D3DSTENCILOP_KEEP;
    ret[D3DRS_STENCILREF] = 0;
    ret[D3DRS_STENCILMASK] = 0xffffffff;
    ret[D3DRS_STENCILFUNC] = D3DCMP_ALWAYS;
    ret[D3DRS_STENCILWRITEMASK] = 0xffffffff;
    ret[D3DRS_TEXTUREFACTOR] = 0xffffffff;
    ret[D3DRS_WRAP0] = 0;
    ret[D3DRS_WRAP1] = 0;
    ret[D3DRS_WRAP2] = 0;
    ret[D3DRS_WRAP3] = 0;
    ret[D3DRS_WRAP4] = 0;
    ret[D3DRS_WRAP5] = 0;
    ret[D3DRS_WRAP6] = 0;
    ret[D3DRS_WRAP7] = 0;
    ret[D3DRS_CLIPPING] = TRUE;
    ret[D3DRS_LIGHTING] = TRUE;
    ret[D3DRS_AMBIENT] = 0;
    ret[D3DRS_FOGVERTEXMODE] = D3DFOG_NONE;
    ret[D3DRS_COLORVERTEX] = TRUE;
    ret[D3DRS_LOCALVIEWER] = TRUE;
    ret[D3DRS_NORMALIZENORMALS] = FALSE;
    ret[D3DRS_DIFFUSEMATERIALSOURCE] = D3DMCS_COLOR1;
    ret[D3DRS_SPECULARMATERIALSOURCE] = D3DMCS_COLOR2;
    ret[D3DRS_AMBIENTMATERIALSOURCE] = D3DMCS_MATERIAL;
    ret[D3DRS_EMISSIVEMATERIALSOURCE] = D3DMCS_MATERIAL;
    ret[D3DRS_VERTEXBLEND] = D3DVBF_DISABLE;
    ret[D3DRS_CLIPPLANEENABLE] = 0;
    ret[D3DRS_POINTSIZE] = float_to_dword(1.0f);
    ret[D3DRS_POINTSIZE_MIN] = float_to_dword(1.0f);
    ret[D3DRS_POINTSPRITEENABLE] = FALSE;
    ret[D3DRS_POINTSCALEENABLE] = FALSE;
    ret[D3DRS_POINTSCALE_A] = float_to_dword(1.0f);
    ret[D3DRS_POINTSCALE_B] = float_to_dword(0.0f);
    ret[D3DRS_POINTSCALE_C] = float_to_dword(0.0f);
    ret[D3DRS_MULTISAMPLEANTIALIAS] = TRUE;
    ret[D3DRS_MULTISAMPLEMASK] = 0xffffffff;
    ret[D3DRS_PATCHEDGESTYLE] = D3DPATCHEDGE_DISCRETE;
    ret[D3DRS_DEBUGMONITORTOKEN] = 0xbaadcafe;
    ret[D3DRS_POINTSIZE_MAX] = float_to_dword(1.0f);
    ret[D3DRS_INDEXEDVERTEXBLENDENABLE] = FALSE;
    ret[D3DRS_COLORWRITEENABLE] = 0x0000000f;
    ret[D3DRS_TWEENFACTOR] = float_to_dword(0.0f);
    ret[D3DRS_BLENDOP] = D3DBLENDOP_ADD;
    ret[D3DRS_POSITIONDEGREE] = D3DDEGREE_CUBIC;
    ret[D3DRS_NORMALDEGREE] = D3DDEGREE_LINEAR;
    ret[D3DRS_SCISSORTESTENABLE] = FALSE;
    ret[D3DRS_SLOPESCALEDEPTHBIAS] = 0;
    ret[D3DRS_MINTESSELLATIONLEVEL] = float_to_dword(1.0f);
    ret[D3DRS_MAXTESSELLATIONLEVEL] = float_to_dword(1.0f);
    ret[D3DRS_ANTIALIASEDLINEENABLE] = FALSE;
    ret[D3DRS_ADAPTIVETESS_X] = float_to_dword(0.0f);
    ret[D3DRS_ADAPTIVETESS_Y] = float_to_dword(0.0f);
    ret[D3DRS_ADAPTIVETESS_Z] = float_to_dword(1.0f);
    ret[D3DRS_ADAPTIVETESS_W] = float_to_dword(0.0f);
    ret[D3DRS_ENABLEADAPTIVETESSELLATION] = FALSE;
    ret[D3DRS_TWOSIDEDSTENCILMODE] = FALSE;
    ret[D3DRS_CCW_STENCILFAIL] = D3DSTENCILOP_KEEP;
    ret[D3DRS_CCW_STENCILZFAIL] = D3DSTENCILOP_KEEP;
    ret[D3DRS_CCW_STENCILPASS] = D3DSTENCILOP_KEEP;
    ret[D3DRS_CCW_STENCILFUNC] = D3DCMP_ALWAYS;
    ret[D3DRS_COLORWRITEENABLE1] = 0x0000000f;
    ret[D3DRS_COLORWRITEENABLE2] = 0x0000000f;
    ret[D3DRS_COLORWRITEENABLE3] = 0x0000000f;
    ret[D3DRS_BLENDFACTOR] = 0xffffffff;
    ret[D3DRS_SRGBWRITEENABLE] = 0;
    ret[D3DRS_DEPTHBIAS] = 0;
    ret[D3DRS_WRAP8] = 0;
    ret[D3DRS_WRAP9] = 0;
    ret[D3DRS_WRAP10] = 0;
    ret[D3DRS_WRAP11] = 0;
    ret[D3DRS_WRAP12] = 0;
    ret[D3DRS_WRAP13] = 0;
    ret[D3DRS_WRAP14] = 0;
    ret[D3DRS_WRAP15] = 0;
    ret[D3DRS_SEPARATEALPHABLENDENABLE] = FALSE;
    ret[D3DRS_SRCBLENDALPHA] = D3DBLEND_ONE;
    ret[D3DRS_DESTBLENDALPHA] = D3DBLEND_ZERO;
    ret[D3DRS_BLENDOPALPHA] = D3DBLENDOP_ADD;
    return ret;
}
static const std::array<DWORD,210> DefaultRSValues = GenerateDefaultRSValues();

static const std::array<DWORD,14> DefaultSSValues{
    0,
    D3DTADDRESS_WRAP,
    D3DTADDRESS_WRAP,
    D3DTADDRESS_WRAP,
    0,
    D3DTEXF_POINT,
    D3DTEXF_POINT,
    D3DTEXF_NONE,
    0,
    0,
    1,
    0,
    0,
    0
};

std::array<DWORD,33> GenerateDefaultTSSValues()
{
    std::array<DWORD,33> ret{0ul};
    ret[D3DTSS_COLOROP] = D3DTOP_DISABLE;
    ret[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
    ret[D3DTSS_COLORARG2] = D3DTA_CURRENT;
    ret[D3DTSS_ALPHAOP] = D3DTOP_DISABLE;
    ret[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
    ret[D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
    ret[D3DTSS_BUMPENVMAT00] = 0;
    ret[D3DTSS_BUMPENVMAT01] = 0;
    ret[D3DTSS_BUMPENVMAT10] = 0;
    ret[D3DTSS_BUMPENVMAT11] = 0;
    ret[D3DTSS_TEXCOORDINDEX] = 0;
    ret[D3DTSS_BUMPENVLSCALE] = 0;
    ret[D3DTSS_BUMPENVLOFFSET] = 0;
    ret[D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_DISABLE;
    ret[D3DTSS_COLORARG0] = D3DTA_CURRENT;
    ret[D3DTSS_ALPHAARG0] = D3DTA_CURRENT;
    ret[D3DTSS_RESULTARG] = D3DTA_CURRENT;
    ret[D3DTSS_CONSTANT] = 0;
    return ret;
}
static const std::array<DWORD,33> DefaultTSSValues = GenerateDefaultTSSValues();


// A simple table that maps on/off D3D render states to GL states passed to
// glEnable/glDisable.
static const std::map<D3DRENDERSTATETYPE,GLenum> RSStateEnableMap{
    {D3DRS_DITHERENABLE,         GL_DITHER},
    {D3DRS_FOGENABLE,            GL_FOG},
    {D3DRS_COLORVERTEX,          GL_COLOR_MATERIAL},
    {D3DRS_NORMALIZENORMALS,     GL_NORMALIZE},
    {D3DRS_SCISSORTESTENABLE,    GL_SCISSOR_TEST},
    {D3DRS_STENCILENABLE,        GL_STENCIL_TEST},
    {D3DRS_ALPHATESTENABLE,      GL_ALPHA_TEST},
    {D3DRS_ALPHABLENDENABLE,     GL_BLEND},
    {D3DRS_LIGHTING,             GL_LIGHTING},
    {D3DRS_MULTISAMPLEANTIALIAS, GL_MULTISAMPLE},
    {D3DRS_POINTSPRITEENABLE,    GL_POINT_SPRITE},
    {D3DRS_SRGBWRITEENABLE,      GL_FRAMEBUFFER_SRGB},
    // WARNING: ZENABLE is a three-state setting; FALSE, TRUE, and USEW. USEW
    // is unsupportable with OpenGL, and is special-cased in SetRenderState.
    {D3DRS_ZENABLE,              GL_DEPTH_TEST}
};


GLenum GetGLBlendFunc(DWORD mode)
{
    switch(mode)
    {
        case D3DBLEND_ZERO: return GL_ZERO;
        case D3DBLEND_ONE: return GL_ONE;
        case D3DBLEND_SRCCOLOR: return GL_SRC_COLOR;
        case D3DBLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
        case D3DBLEND_SRCALPHA: return GL_SRC_ALPHA;
        case D3DBLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case D3DBLEND_DESTALPHA: return GL_DST_ALPHA;
        case D3DBLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case D3DBLEND_DESTCOLOR: return GL_DST_COLOR;
        case D3DBLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
        case D3DBLEND_SRCALPHASAT: return GL_SRC_ALPHA_SATURATE;
        //case D3DBLEND_BOTHSRCALPHA:
        //case D3DBLEND_BOTHINVSRCALPHA:
        case D3DBLEND_BLENDFACTOR: return GL_CONSTANT_COLOR;
        case D3DBLEND_INVBLENDFACTOR: return GL_ONE_MINUS_CONSTANT_COLOR;
    }
    FIXME("Unhandled D3DBLEND mode: 0x%lx\n", mode);
    return GL_ONE;
}

GLenum GetGLBlendOp(DWORD op)
{
    switch(op)
    {
        case D3DBLENDOP_ADD: return GL_FUNC_ADD;
        case D3DBLENDOP_SUBTRACT: return GL_FUNC_SUBTRACT;
        case D3DBLENDOP_REVSUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
        case D3DBLENDOP_MIN: return GL_MIN;
        case D3DBLENDOP_MAX: return GL_MAX;
    }
    FIXME("Unhandled D3DBLENDOP: 0x%lx\n", op);
    return GL_FUNC_ADD;
}

GLenum GetGLStencilOp(DWORD op)
{
    switch(op)
    {
        case D3DSTENCILOP_KEEP: return GL_KEEP;
        case D3DSTENCILOP_ZERO: return GL_ZERO;
        case D3DSTENCILOP_REPLACE: return GL_REPLACE;
        case D3DSTENCILOP_INCRSAT: return GL_INCR;
        case D3DSTENCILOP_DECRSAT: return GL_DECR;
        case D3DSTENCILOP_INVERT: return GL_INVERT;
        case D3DSTENCILOP_INCR: return GL_INCR_WRAP;
        case D3DSTENCILOP_DECR: return GL_DECR_WRAP;
    }
    FIXME("Unhandled D3DSTENCILOP: 0x%lx\n", op);
    return GL_KEEP;
}

GLenum GetGLCompFunc(DWORD func)
{
    switch(func)
    {
        case D3DCMP_NEVER: return GL_NEVER;
        case D3DCMP_LESS: return GL_LESS;
        case D3DCMP_EQUAL: return GL_EQUAL;
        case D3DCMP_LESSEQUAL: return GL_LEQUAL;
        case D3DCMP_GREATER: return GL_GREATER;
        case D3DCMP_NOTEQUAL: return GL_NOTEQUAL;
        case D3DCMP_GREATEREQUAL: return GL_GEQUAL;
        case D3DCMP_ALWAYS: return GL_ALWAYS;
    }
    FIXME("Unhandled D3DCMPFUNC: 0x%lx\n", func);
    return GL_ALWAYS;
}

GLenum GetGLWrapMode(DWORD mode)
{
    switch(mode)
    {
        case D3DTADDRESS_WRAP: return GL_REPEAT;
        case D3DTADDRESS_MIRROR: return GL_MIRRORED_REPEAT;
        case D3DTADDRESS_CLAMP: return GL_CLAMP_TO_EDGE;
        case D3DTADDRESS_BORDER: return GL_CLAMP_TO_BORDER;
        case D3DTADDRESS_MIRRORONCE: return GL_MIRROR_CLAMP_TO_EDGE;
    }
    FIXME("Unhandled D3DTEXTUREADDRESS: 0x%lx\n", mode);
    return GL_REPEAT;
}

GLenum GetGLFilterMode(DWORD type, DWORD miptype)
{
    if(miptype == D3DTEXF_NONE)
    {
        if(type <= D3DTEXF_POINT) return GL_NEAREST;
        if(type <= D3DTEXF_ANISOTROPIC) return GL_LINEAR;
        FIXME("Unsupported D3DTEXTUREFILTERTYPE: 0x%lx\n", type);
        return GL_LINEAR;
    }
    if(miptype == D3DTEXF_POINT)
    {
        if(type <= D3DTEXF_POINT) return GL_NEAREST_MIPMAP_NEAREST;
        if(type <= D3DTEXF_ANISOTROPIC) return GL_LINEAR_MIPMAP_NEAREST;
        FIXME("Unsupported D3DTEXTUREFILTERTYPE: 0x%lx\n", type);
        return GL_LINEAR_MIPMAP_NEAREST;
    }
    if(miptype != D3DTEXF_LINEAR)
        FIXME("Unsupported mipmap D3DTEXTUREFILTERTYPE: 0x%lx\n", miptype);
    if(type <= D3DTEXF_POINT) return GL_NEAREST_MIPMAP_LINEAR;
    if(type <= D3DTEXF_ANISOTROPIC) return GL_LINEAR_MIPMAP_LINEAR;
    FIXME("Unsupported D3DTEXTUREFILTERTYPE: 0x%lx\n", type);
    return GL_LINEAR_MIPMAP_LINEAR;
}

GLenum GetGLDrawMode(D3DPRIMITIVETYPE type, UINT &count)
{
    switch(type)
    {
        case D3DPT_POINTLIST:
            return GL_POINTS;
        case D3DPT_LINELIST:
            count *= 2;
            return GL_LINES;
        case D3DPT_LINESTRIP:
            count += 1;
            return GL_LINE_STRIP;
        case D3DPT_TRIANGLELIST:
            count *= 3;
            return GL_TRIANGLES;
        case D3DPT_TRIANGLESTRIP:
            count += 2;
            return GL_TRIANGLE_STRIP;
        case D3DPT_TRIANGLEFAN:
            count += 2;
            return GL_TRIANGLE_FAN;
        case D3DPT_FORCE_DWORD:
            break;
    }
    FIXME("Unhandled D3DPRIMITIVETYPE: 0x%x\n", type);
    return GL_POINTS;
}

#define D3DCOLOR_R(color) (((color)>>16)&0xff)
#define D3DCOLOR_G(color) (((color)>> 8)&0xff)
#define D3DCOLOR_B(color) (((color)    )&0xff)
#define D3DCOLOR_A(color) (((color)>>24)&0xff)


class StateEnable : public Command {
    GLenum mState;
    bool mEnable;

public:
    StateEnable(GLenum state, bool enable) : mState(state), mEnable(enable) { }

    virtual ULONG execute()
    {
        if(mEnable)
            glEnable(mState);
        else
            glDisable(mState);
        return sizeof(*this);
    }
};

class MaterialSet : public Command {
    float mShininess;
    float mDiffuse[4];
    float mAmbient[4];
    float mSpecular[4];
    float mEmission[4];

public:
    MaterialSet(const D3DMATERIAL9 &material)
      : mShininess(material.Power)
      , mDiffuse{material.Diffuse.r, material.Diffuse.g, material.Diffuse.b, material.Diffuse.a}
      , mAmbient{material.Ambient.r, material.Ambient.g, material.Ambient.b, material.Ambient.a}
      , mSpecular{material.Specular.r, material.Specular.g, material.Specular.b, material.Specular.a}
      , mEmission{material.Emissive.r, material.Emissive.g, material.Emissive.b, material.Emissive.a}
    { }

    virtual ULONG execute()
    {
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mShininess);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mSpecular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mEmission);
        return sizeof(*this);
    }
};

class ViewportSet : public Command {
    GLint mX, mY;
    GLsizei mWidth, mHeight;
    GLfloat mMinZ, mMaxZ;

public:
    ViewportSet(GLint x, GLint y, GLsizei width, GLsizei height, GLfloat minz, GLfloat maxz)
      : mX(x), mY(y), mWidth(width), mHeight(height), mMinZ(minz), mMaxZ(maxz)
    { }

    virtual ULONG execute()
    {
        glViewport(mX, mY, mWidth, mHeight);
        glDepthRange(mMinZ, mMaxZ);
        return sizeof(*this);
    }
};

class ScissorRectSet : public Command {
    RECT mRect;

public:
    ScissorRectSet(const RECT &rect) : mRect(rect) { }

    virtual ULONG execute()
    {
        glScissor(mRect.left, mRect.top, mRect.right-mRect.left, mRect.bottom-mRect.top);
        return sizeof(*this);
    }
};

class PolygonModeSet : public Command {
    GLenum mMode;

public:
    PolygonModeSet(GLenum mode) : mMode(mode) { }

    virtual ULONG execute()
    {
        glPolygonMode(GL_FRONT_AND_BACK, mMode);
        return sizeof(*this);
    }
};

class CullFaceSet : public Command {
    GLenum mFace;

public:
    CullFaceSet(GLenum face) : mFace(face) { }

    virtual ULONG execute()
    {
        if(!mFace)
            glDisable(GL_CULL_FACE);
        else
        {
            glEnable(GL_CULL_FACE);
            glCullFace(mFace);
        }
        return sizeof(*this);
    }
};

class ColorMaskSet : public Command {
    UINT mEnable;

public:
    ColorMaskSet(UINT enable) : mEnable(enable) { }

    virtual ULONG execute()
    {
        glColorMask(!!(mEnable&D3DCOLORWRITEENABLE_RED),
                    !!(mEnable&D3DCOLORWRITEENABLE_GREEN),
                    !!(mEnable&D3DCOLORWRITEENABLE_BLUE),
                    !!(mEnable&D3DCOLORWRITEENABLE_ALPHA));
        return sizeof(*this);
    }
};

class DepthMaskSet : public Command {
    bool mEnable;

public:
    DepthMaskSet(bool enable) : mEnable(enable) { }

    virtual ULONG execute()
    {
        glDepthMask(mEnable);
        return sizeof(*this);
    }
};

class DepthFuncSet : public Command {
    GLenum mFunc;

public:
    DepthFuncSet(GLenum func) : mFunc(func) { }

    virtual ULONG execute()
    {
        glDepthFunc(mFunc);
        return sizeof(*this);
    }
};

class AlphaFuncSet : public Command {
    GLenum mFunc;
    GLclampf mRef;

public:
    AlphaFuncSet(GLenum func, GLclampf ref) : mFunc(func), mRef(ref) { }

    virtual ULONG execute()
    {
        glAlphaFunc(mFunc, std::min(std::max(mRef, 0.0f), 1.0f));
        return sizeof(*this);
    }
};

class BlendFuncSet : public Command {
    GLenum mSrc, mDst;

public:
    BlendFuncSet(GLenum src, GLenum dst) : mSrc(src), mDst(dst) { }

    virtual ULONG execute()
    {
        glBlendFunc(mSrc, mDst);
        return sizeof(*this);
    }
};

class StencilFuncSet : public Command {
    GLenum mFace;
    GLenum mFunc;
    GLuint mRef;
    GLuint mMask;

public:
    StencilFuncSet(GLenum face, GLenum func, GLuint ref, GLuint mask) : mFace(face), mFunc(func), mRef(ref), mMask(mask) { }

    virtual ULONG execute()
    {
        glStencilFuncSeparate(mFace, mFunc, mRef, mMask);
        return sizeof(*this);
    }
};

class BlendOpSet : public Command {
    GLenum mColorOp;
    GLenum mAlphaOp;

public:
    BlendOpSet(GLenum op) : mColorOp(op), mAlphaOp(op) { }

    virtual ULONG execute()
    {
        glBlendEquationSeparate(mColorOp, mAlphaOp);
        return sizeof(*this);
    }
};

class StencilOpSet : public Command {
    GLenum mFace;
    GLenum mFail;
    GLenum mZFail;
    GLenum mZPass;

public:
    StencilOpSet(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
      : mFace(face), mFail(fail), mZFail(zfail), mZPass(zpass)
    { }

    virtual ULONG execute()
    {
        glStencilOpSeparate(mFace, mFail, mZFail, mZPass);
        return sizeof(*this);
    }
};

class StencilMaskSet : public Command {
    GLuint mMask;

public:
    StencilMaskSet(GLuint mask) : mMask(mask) { }

    virtual ULONG execute()
    {
        glStencilMask(mMask);
        return sizeof(*this);
    }
};

class DepthBiasSet : public Command {
    GLfloat mScale;
    GLfloat mBias;

public:
    DepthBiasSet(GLfloat scale, GLfloat bias) : mScale(scale), mBias(bias) { }

    virtual ULONG execute()
    {
        glPolygonOffset(mScale, mBias);
        return sizeof(*this);
    }
};

class FogValuefSet : public Command {
    GLenum mParam;
    GLfloat mValues[4];

public:
    FogValuefSet(GLenum param, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) : mParam(param), mValues{v0, v1, v2, v3} { }
    FogValuefSet(GLenum param, GLfloat v0) : mParam(param), mValues{v0, v0, v0, v0} { }

    virtual ULONG execute()
    {
        glFogfv(mParam, mValues);
        return sizeof(*this);
    }
};

class SetSamplerParameteri : public Command {
    GLuint mSampler;
    GLenum mParameter;
    GLint mValue;

public:
    SetSamplerParameteri(GLuint sampler, GLenum parameter, GLint value)
      : mSampler(sampler), mParameter(parameter), mValue(value)
    { }

    virtual ULONG execute()
    {
        if((mParameter != GL_TEXTURE_MAX_ANISOTROPY_EXT && mParameter != GL_TEXTURE_SRGB_DECODE_EXT) ||
           (mParameter == GL_TEXTURE_MAX_ANISOTROPY_EXT && GLEW_EXT_texture_filter_anisotropic) ||
           (mParameter == GL_TEXTURE_SRGB_DECODE_EXT && GLEW_EXT_texture_sRGB_decode))
        {
            glSamplerParameteri(mSampler, mParameter, mValue);
            checkGLError();
        }
        return sizeof(*this);
    }
};
class SetSamplerParameter4f : public Command {
    GLuint mSampler;
    GLenum mParameter;
    GLfloat mValues[4];

public:
    SetSamplerParameter4f(GLuint sampler, GLenum parameter, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
      : mSampler(sampler), mParameter(parameter), mValues{v0,v1,v2,v3}
    { }

    virtual ULONG execute()
    {
        glSamplerParameterfv(mSampler, mParameter, mValues);
        checkGLError();
        return sizeof(*this);
    }
};

class ClipPlaneSet : public Command {
    GLint mPlaneIdx;
    GLdouble mPlane[4];

public:
    ClipPlaneSet(GLint planeidx, const Vector4f &plane)
      : mPlaneIdx(planeidx)
      , mPlane{plane.x, plane.y, plane.z, plane.w}
    { }

    virtual ULONG execute()
    {
        glClipPlane(GL_CLIP_PLANE0+mPlaneIdx, mPlane);
        checkGLError();
        return sizeof(*this);
    }
};

template<size_t MAXCOUNT>
class SetBufferValues : public Command {
    GLuint mBuffer;
    GLintptr mStart;
    GLsizeiptr mSize;
    union {
        char mData[MAXCOUNT*4*4];
        float mVec4fs[MAXCOUNT][4];
    };

public:
    SetBufferValues(GLuint buffer, const float *data, GLintptr start, GLsizeiptr count)
      : mBuffer(buffer)
    {
        mStart = start * 4 * 4;
        mSize = count * 4 * 4;
        for(GLsizeiptr i = 0;i < count;++i)
        {
            mVec4fs[i][0] = *(data++);
            mVec4fs[i][1] = *(data++);
            mVec4fs[i][2] = *(data++);
            mVec4fs[i][3] = *(data++);
        }
    }

    virtual ULONG execute()
    {
        glNamedBufferSubDataEXT(mBuffer, mStart, mSize, mData);
        checkGLError();
        return sizeof(*this);
    }
};

class ElementArraySet : public Command {
    GLuint mBufferId;

public:
    ElementArraySet(GLuint bufferid) : mBufferId(bufferid) { }

    virtual ULONG execute()
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufferId);
        checkGLError();
        return sizeof(*this);
    }
};

} // namespace


void D3DGLDevice::setTextureGL(GLuint stage, GLenum type, GLuint binding)
{
    if(stage != mGLState.active_stage)
    {
        mGLState.active_stage = stage;
        glActiveTexture(GL_TEXTURE0 + stage);
    }

    // Stupid nVidia. Their drivers only report 4 fixed-function texture
    // stages regardless of the number of fragment samplers and texture coords
    // provided by the hardware. This means that glEnable/Disable with the
    // various GL_TEXTURE_ types on stages at/above GL_MAX_TEXTURE_UNITS is
    // invalid. This also effectively means only 4 blending stages will work.
    // Fixing this requires manual FFP emulation with shaders.
    if(stage >= getAdapter().getLimits().textures)
        mGLState.sampler_type[stage] = type;
    else if(type != mGLState.sampler_type[stage])
    {
        if(mGLState.sampler_type[stage] != GL_NONE)
            glDisable(mGLState.sampler_type[stage]);
        mGLState.sampler_type[stage] = type;
        if(type != GL_NONE)
            glEnable(type);
    }

    if(binding != mGLState.sampler_binding[stage])
    {
        mGLState.sampler_binding[stage] = binding;
        if(type)
            glBindTexture(type, binding);
        else
            glBindTexture(GL_TEXTURE_2D, 0);
    }
    checkGLError();
}
class SetTextureCmd : public Command {
    D3DGLDevice *mTarget;
    GLuint mStage;
    GLenum mType;
    GLuint mBinding;

public:
    SetTextureCmd(D3DGLDevice *target, GLuint stage, GLenum type, GLuint binding)
      : mTarget(target), mStage(stage), mType(type), mBinding(binding)
    { }

    virtual ULONG execute()
    {
        mTarget->setTextureGL(mStage, mType, mBinding);
        return sizeof(*this);
    }
};


void D3DGLDevice::setClipPlanesGL(UINT planes)
{
    if(planes == mGLState.clip_plane_enabled)
        return;

    UINT num_planes = std::min(mAdapter.getLimits().clipplanes, 32u);
    for(UINT i = 0;i < num_planes;++i)
    {
        UINT m = 1<<i;
        if((planes&m) != (mGLState.clip_plane_enabled&m))
        {
            if((planes&m)) glEnable(GL_CLIP_PLANE0+i);
            else glDisable(GL_CLIP_PLANE0+i);
        }
    }
    mGLState.clip_plane_enabled = planes;
    checkGLError();
}
class ClipPlaneEnableCmd : public Command {
    D3DGLDevice *mTarget;
    UINT mPlanes;

public:
    ClipPlaneEnableCmd(D3DGLDevice *target, UINT planes) : mTarget(target), mPlanes(planes) { }

    virtual ULONG execute()
    {
        mTarget->setClipPlanesGL(mPlanes);
        return sizeof(*this);
    }
};


void D3DGLDevice::setFBAttachmentGL(GLenum attachment, GLenum target, GLuint id, GLint level)
{
    if(target == GL_RENDERBUFFER)
        glNamedFramebufferRenderbufferEXT(mGLState.main_framebuffer, attachment, GL_RENDERBUFFER, id);
    else if(target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
            target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X || target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
            target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y || target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
            target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
        glNamedFramebufferTexture2DEXT(mGLState.main_framebuffer, attachment, target, id, level);
    checkGLError();
}
class SetFBAttachmentCmd : public Command {
    D3DGLDevice *mTarget;
    GLenum mAttachment;
    GLenum mRTarget;
    GLuint mId;
    GLint mLevel;

public:
    SetFBAttachmentCmd(D3DGLDevice *target, GLenum attachment, GLenum rtarget, GLuint id, GLint level)
      : mTarget(target), mAttachment(attachment), mRTarget(rtarget), mId(id), mLevel(level)
    { }

    virtual ULONG execute()
    {
        mTarget->setFBAttachmentGL(mAttachment, mRTarget, mId, mLevel);
        return sizeof(*this);
    }
};


void D3DGLDevice::setVertexArrayStateGL(bool vertex, bool normal, bool color, bool specular, UINT texcoord)
{
#define SET_CLIENT_STATE(f, e) do { \
    if(f) glEnableClientState(e);   \
    else glDisableClientState(e);   \
} while(0)
    if(vertex != mGLState.vertex_array_enabled)
    {
        mGLState.vertex_array_enabled = vertex;
        SET_CLIENT_STATE(vertex, GL_VERTEX_ARRAY);
    }
    if(normal != mGLState.normal_array_enabled)
    {
        mGLState.normal_array_enabled = normal;
        SET_CLIENT_STATE(normal, GL_NORMAL_ARRAY);
    }
    if(color != mGLState.color_array_enabled)
    {
        mGLState.color_array_enabled = color;
        SET_CLIENT_STATE(color, GL_COLOR_ARRAY);
    }
    if(specular != mGLState.specular_array_enabled)
    {
        mGLState.specular_array_enabled = specular;
        SET_CLIENT_STATE(specular, GL_SECONDARY_COLOR_ARRAY);
    }
    if(texcoord != mGLState.texcoord_array_enabled)
    {
        UINT numcoords = std::min<UINT>(mAdapter.getLimits().texture_coords, D3DDP_MAXTEXCOORD);
        for(UINT i = 0;i < numcoords;++i)
        {
            UINT t = 1<<i;
            if((texcoord&t) != (mGLState.texcoord_array_enabled&t))
            {
                glClientActiveTexture(GL_TEXTURE0 + i);
                SET_CLIENT_STATE((texcoord&t), GL_TEXTURE_COORD_ARRAY);
            }
        }
        mGLState.texcoord_array_enabled = texcoord;
    }
#undef SET_CLIENT_STATE
    checkGLError();
}
class SetVertexArrayStateCmd : public Command {
    D3DGLDevice *mTarget;
    bool mVertex, mNormal, mColor, mSpecular;
    UINT mTexcoord;

public:
    SetVertexArrayStateCmd(D3DGLDevice *target, bool vertex, bool normal, bool color, bool specular, UINT texcoord)
      : mTarget(target), mVertex(vertex), mNormal(normal), mColor(color), mSpecular(specular), mTexcoord(texcoord)
    { }

    virtual ULONG execute()
    {
        mTarget->setVertexArrayStateGL(mVertex, mNormal, mColor, mSpecular, mTexcoord);
        return sizeof(*this);
    }
};

void D3DGLDevice::setVertexAttribArrayGL(UINT attribs)
{
    // Early out
    if(attribs == mGLState.attrib_array_enabled)
        return;

    UINT numattribs = mAdapter.getLimits().vertex_attribs;
    for(UINT i = 0;i < numattribs;++i)
    {
        UINT a = 1<<i;
        if((attribs&a) != (mGLState.attrib_array_enabled&a))
        {
            if((attribs&a))
                glEnableVertexAttribArray(i);
            else
            {
                glDisableVertexAttribArray(i);
                glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 1.0f);
            }
        }
    }
    mGLState.attrib_array_enabled = attribs;
    checkGLError();
}
class SetVertexAttribArrayCmd : public Command {
    D3DGLDevice *mTarget;
    UINT mAttribs;

public:
    SetVertexAttribArrayCmd(D3DGLDevice *target, UINT attribs)
      : mTarget(target), mAttribs(attribs)
    { }

    virtual ULONG execute()
    {
        mTarget->setVertexAttribArrayGL(mAttribs);
        return sizeof(*this);
    }
};

void D3DGLDevice::setShaderProgramGL(GLbitfield stages, GLuint program)
{
    glUseProgramStages(mGLState.pipeline, stages, program);
    checkGLError();
}
class SetShaderProgramCmd : public Command {
    D3DGLDevice *mTarget;
    GLbitfield mStages;
    GLuint mProgram;

public:
    SetShaderProgramCmd(D3DGLDevice *target, GLbitfield stages, GLuint program)
      : mTarget(target), mStages(stages), mProgram(program)
    { }

    virtual ULONG execute()
    {
        mTarget->setShaderProgramGL(mStages, mProgram);
        return sizeof(*this);
    }
};


void D3DGLDevice::clearGL(GLbitfield mask, GLuint color, GLfloat depth, GLuint stencil, const RECT &rect)
{
    if(mGLState.current_framebuffer[0] != mGLState.main_framebuffer)
    {
        mGLState.current_framebuffer[0] = mGLState.main_framebuffer;
        mGLState.current_framebuffer[1] = mGLState.main_framebuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, mGLState.main_framebuffer);
    }

    glPushAttrib(mask | GL_SCISSOR_BIT);

    if((mask&GL_COLOR_BUFFER_BIT))
    {
        glClearColor(D3DCOLOR_R(color)/255.0f, D3DCOLOR_G(color)/255.0f,
                     D3DCOLOR_B(color)/255.0f, D3DCOLOR_A(color)/255.0f);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    if((mask&GL_DEPTH_BUFFER_BIT))
    {
        glClearDepth(depth);
        glDepthMask(GL_TRUE);
    }
    if((mask&GL_STENCIL_BUFFER_BIT))
    {
        glClearStencil(stencil);
        glStencilMask(GL_TRUE);
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top);
    glClear(mask);

    glPopAttrib();
    checkGLError();
}
class ClearCmd : public Command {
    D3DGLDevice *mTarget;
    GLbitfield mMask;
    GLuint mColor;
    GLfloat mDepth;
    GLuint mStencil;
    RECT mRect;

public:
    ClearCmd(D3DGLDevice *target, GLbitfield mask, GLuint color, GLfloat depth, GLuint stencil, const RECT &rect)
      : mTarget(target), mMask(mask), mColor(color), mDepth(depth), mStencil(stencil), mRect(rect)
    { }

    virtual ULONG execute()
    {
        mTarget->clearGL(mMask, mColor, mDepth, mStencil, mRect);
        return sizeof(*this);
    }
};

void D3DGLDevice::setVtxDataGL(const D3DGLDevice::GLStreamData *streams, GLuint numstreams, bool ffp)
{
    GLuint binding = 0;
    if(!ffp)
    {
        for(GLuint i = 0;i < numstreams;++i)
        {
            if(binding != streams[i].mBufferId)
            {
                binding = streams[i].mBufferId;
                glBindBuffer(GL_ARRAY_BUFFER, binding);
            }
            glVertexAttribPointer(streams[i].mTarget, streams[i].mGLCount,
                                  streams[i].mGLType, streams[i].mNormalize,
                                  streams[i].mStride, streams[i].mPointer);
            // Setting a high divisor will keep the vertex attribute from
            // incrementing, just like D3D's stride==0 setting.
            if(streams[i].mStride == 0)
                glVertexAttribDivisor(streams[i].mTarget, 65535);
            else
                glVertexAttribDivisor(streams[i].mTarget, streams[i].mDivisor);
        }
    }
    else for(GLuint i = 0;i < numstreams;++i)
    {
        if(binding != streams[i].mBufferId)
        {
            binding = streams[i].mBufferId;
            glBindBuffer(GL_ARRAY_BUFFER, binding);
        }

        if(streams[i].mTarget == GL_VERTEX_ARRAY)
            glVertexPointer(streams[i].mGLCount, streams[i].mGLType, streams[i].mStride, streams[i].mPointer);
        else if(streams[i].mTarget == GL_TEXTURE_COORD_ARRAY)
        {
            UINT numcoords = std::min<UINT>(mAdapter.getLimits().texture_coords, D3DDP_MAXTEXCOORD);
            for(UINT t = 0;t < numcoords;++t)
            {
                if((streams[i].mIndex&(1<<t)))
                {
                    glClientActiveTexture(GL_TEXTURE0 + t);
                    glTexCoordPointer(streams[i].mGLCount, streams[i].mGLType, streams[i].mStride, streams[i].mPointer);
                }
            }
        }
        else if(streams[i].mTarget == GL_NORMAL_ARRAY)
            glNormalPointer(streams[i].mGLType, streams[i].mStride, streams[i].mPointer);
        else if(streams[i].mTarget == GL_COLOR_ARRAY)
            glColorPointer(streams[i].mGLCount, streams[i].mGLType, streams[i].mStride, streams[i].mPointer);
        else if(streams[i].mTarget == GL_SECONDARY_COLOR_ARRAY)
            glSecondaryColorPointer(streams[i].mGLCount, streams[i].mGLType, streams[i].mStride, streams[i].mPointer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGLError();
}
template<bool UseFFP>
class SetVtxDataCmd : public Command {
    D3DGLDevice *mTarget;
    D3DGLDevice::GLStreamData mStreams[16];
    GLuint mNumStreams;

public:
    SetVtxDataCmd(D3DGLDevice *target, const D3DGLDevice::GLStreamData *streams, GLuint numstream)
      : mTarget(target)
    {
        for(GLuint i = 0;i < numstream;++i)
            mStreams[i] = streams[i];
        mNumStreams = numstream;
    }

    virtual ULONG execute()
    {
        mTarget->setVtxDataGL(mStreams, mNumStreams, UseFFP);
        return sizeof(*this);
    }
};

void D3DGLDevice::drawArraysGL(GLenum mode, GLint count, GLsizei numinstances)
{
    if(mGLState.current_framebuffer[0] != mGLState.main_framebuffer)
    {
        mGLState.current_framebuffer[0] = mGLState.main_framebuffer;
        mGLState.current_framebuffer[1] = mGLState.main_framebuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, mGLState.main_framebuffer);
    }
    glDrawArraysInstanced(mode, 0, count, numinstances);
    checkGLError();
}
class DrawGLArraysCmd : public Command {
    D3DGLDevice *mTarget;
    GLenum mMode;
    GLint mCount;
    GLsizei mNumInstances;

public:
    DrawGLArraysCmd(D3DGLDevice *target, GLenum mode, GLint count, GLsizei num_instances)
      : mTarget(target), mMode(mode), mCount(count), mNumInstances(num_instances)
    { }

    virtual ULONG execute()
    {
        mTarget->drawArraysGL(mMode, mCount, mNumInstances);
        return sizeof(*this);
    }
};

void D3DGLDevice::drawElementsGL(GLenum mode, GLint count, GLenum type, GLubyte *pointer, GLsizei numinstances, GLsizei basevtx)
{
    if(mGLState.current_framebuffer[0] != mGLState.main_framebuffer)
    {
        mGLState.current_framebuffer[0] = mGLState.main_framebuffer;
        mGLState.current_framebuffer[1] = mGLState.main_framebuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, mGLState.main_framebuffer);
    }
    glDrawElementsInstancedBaseVertex(mode, count, type, pointer, numinstances, basevtx);
    checkGLError();
}
class DrawGLElementsCmd : public Command {
    D3DGLDevice *mTarget;
    GLenum mMode;
    GLint mCount;
    GLenum mType;
    GLubyte *mPointer;
    GLsizei mNumInstances;
    GLsizei mBaseVtx;

public:
    DrawGLElementsCmd(D3DGLDevice *target, GLenum mode, GLint count, GLenum type, GLubyte *pointer, GLsizei num_instances, GLsizei basevtx)
      : mTarget(target), mMode(mode), mCount(count), mType(type), mPointer(pointer), mNumInstances(num_instances), mBaseVtx(basevtx)
    { }

    virtual ULONG execute()
    {
        mTarget->drawElementsGL(mMode, mCount, mType, mPointer, mNumInstances, mBaseVtx);
        return sizeof(*this);
    }
};


void D3DGLDevice::blitFramebufferGL(GLenum src_target, GLuint src_binding, GLint src_level, const RECT &src_rect, GLenum dst_target, GLuint dst_binding, GLint dst_level, const RECT &dst_rect, GLenum filter)
{
    if(mGLState.current_framebuffer[0] != mGLState.copy_framebuffers[0])
    {
        mGLState.current_framebuffer[0] = mGLState.copy_framebuffers[0];
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mGLState.current_framebuffer[0]);
    }
    if(src_target == GL_RENDERBUFFER)
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, src_binding);
    else if(src_target == GL_TEXTURE_2D || src_target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
            src_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X || src_target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
            src_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y || src_target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
            src_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src_target, src_binding, src_level);
    else
    {
        ERR("Unhandled source target: 0x%x\n", src_target);
        goto done;
    }

    if(!dst_target)
    {
        if(mGLState.current_framebuffer[1] != 0)
        {
            mGLState.current_framebuffer[1] = 0;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }
    }
    else
    {
        if(mGLState.current_framebuffer[1] != mGLState.copy_framebuffers[1])
        {
            mGLState.current_framebuffer[1] = mGLState.copy_framebuffers[1];
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mGLState.current_framebuffer[1]);
        }
        if(dst_target == GL_RENDERBUFFER)
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, dst_binding);
        else if(dst_target == GL_TEXTURE_2D || dst_target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
                dst_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X || dst_target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
                dst_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y || dst_target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
                dst_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst_target, dst_binding, dst_level);
        else
        {
            ERR("Unhandled destination target: 0x%x\n", dst_target);
            goto done;
        }
    }

    glPushAttrib(GL_SCISSOR_BIT);
    glDisable(GL_SCISSOR_TEST);

    glBlitFramebuffer(src_rect.left, src_rect.top, src_rect.right, src_rect.bottom,
                      dst_rect.left, dst_rect.top, dst_rect.right, dst_rect.bottom,
                      GL_COLOR_BUFFER_BIT, filter);

    glPopAttrib();

done:
    checkGLError();
}
class BlitFramebufferCmd : public Command {
    D3DGLDevice *mTarget;
    GLenum mSrcTarget;
    GLuint mSrcBinding;
    GLint mSrcLevel;
    RECT mSrcRect;
    GLenum mDstTarget;
    GLuint mDstBinding;
    GLint mDstLevel;
    RECT mDstRect;
    GLenum mFilter;

public:
    BlitFramebufferCmd(D3DGLDevice *target, GLenum src_target, GLuint src_binding, GLint src_level, const RECT &src_rect, GLenum dst_target, GLuint dst_binding, GLint dst_level, const RECT &dst_rect, GLenum filter)
      : mTarget(target), mSrcTarget(src_target), mSrcBinding(src_binding), mSrcLevel(src_level), mSrcRect(src_rect)
      , mDstTarget(dst_target), mDstBinding(dst_binding), mDstLevel(dst_level), mDstRect(dst_rect), mFilter(filter)
    { }

    virtual ULONG execute()
    {
        mTarget->blitFramebufferGL(mSrcTarget, mSrcBinding, mSrcLevel, mSrcRect,
                                   mDstTarget, mDstBinding, mDstLevel, mDstRect, mFilter);
        return sizeof(*this);
    }
};

void D3DGLDevice::debugProcGL(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar *message) const
{
    std::stringstream sstr;
    sstr<< "<<<<<<<<< OpenGL message <<<<<<<<<" <<std::endl;

    sstr<< "Source: ";
    if(source == GL_DEBUG_SOURCE_API) sstr<< "API" <<std::endl;
    else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM) sstr<< "Window System" <<std::endl;
    else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER) sstr<< "Shader Compiler" <<std::endl;
    else if(source == GL_DEBUG_SOURCE_THIRD_PARTY) sstr<< "Third Party" <<std::endl;
    else if(source == GL_DEBUG_SOURCE_APPLICATION) sstr<< "Application" <<std::endl;
    else if(source == GL_DEBUG_SOURCE_OTHER) sstr<< "Other" <<std::endl;
    else sstr<< "!!Unknown!!" <<std::endl;

    sstr<< "Type: ";
    if(type == GL_DEBUG_TYPE_ERROR) sstr<< "Error" <<std::endl;
    else if(type == GL_DEBUG_TYPE_ERROR) sstr<< "Error" <<std::endl;
    else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) sstr<< "Deprecated Behavior" <<std::endl;
    else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR) sstr<< "Undefined Behavior" <<std::endl;
    else if(type == GL_DEBUG_TYPE_PORTABILITY) sstr<< "Portability" <<std::endl;
    else if(type == GL_DEBUG_TYPE_PERFORMANCE) sstr<< "Performance" <<std::endl;
    else if(type == GL_DEBUG_TYPE_MARKER) sstr<< "Marker" <<std::endl;
    else if(type == GL_DEBUG_TYPE_PUSH_GROUP) sstr<< "Push Group" <<std::endl;
    else if(type == GL_DEBUG_TYPE_POP_GROUP) sstr<< "Pop GroupError" <<std::endl;
    else if(type == GL_DEBUG_TYPE_OTHER) sstr<< "Other" <<std::endl;
    else sstr<< "!!Unknown!!" <<std::endl;

    sstr<< "ID: "<<id <<std::endl;
    sstr<< "Message: "<<message <<std::endl;

    std::string str = sstr.str();
    if(severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        TRACE("%s", str.c_str());
    else if(severity == GL_DEBUG_SEVERITY_LOW)
        WARN("%s", str.c_str());
    else if(severity == GL_DEBUG_SEVERITY_MEDIUM)
        FIXME("%s", str.c_str());
    else if(severity == GL_DEBUG_SEVERITY_HIGH)
        ERR("%s", str.c_str());
    else
        ERR("%s\nSeverity: !!Unknown!!\n", str.c_str());
}


void D3DGLDevice::initGL(HDC dc, HGLRC glcontext)
{
    if(!wglMakeCurrent(dc, glcontext))
    {
        ERR("Failed to make context current! Error: %lu\n", GetLastError());
        std::terminate();
    }

    if((GLEW_VERSION_4_3 || GLEW_KHR_debug) && DebugEnable && LogLevel != NONE_)
    {
        TRACE("Installing debug handler\n");
        glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(debugProcWrapGL), this);
        if(LogLevel < TRACE_) glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        if(LogLevel < WARN_) glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
        if(LogLevel < FIXME_) glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_FALSE);
        if(LogLevel < ERR_) glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_FALSE);
        checkGLError();
    }

    glGenSamplers(mGLState.samplers.size(), mGLState.samplers.data());
    checkGLError();

    for(size_t i = 0;i < mGLState.samplers.size();++i)
    {
        auto &sampler = mGLState.samplers[i];

        GLint color[4]{0, 0, 0, 0};
        glBindSampler(i, sampler);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glSamplerParameteriv(sampler, GL_TEXTURE_BORDER_COLOR, color);
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        if(GLEW_EXT_texture_filter_anisotropic)
            glSamplerParameteri(sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
        if(GLEW_EXT_texture_sRGB_decode)
            glSamplerParameteri(sampler, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
        checkGLError();
    }

    glGenProgramPipelines(1, &mGLState.pipeline);
    glBindProgramPipeline(mGLState.pipeline);
    checkGLError();

    glGenFramebuffers(1, &mGLState.main_framebuffer);
    glGenFramebuffers(2, mGLState.copy_framebuffers);
    checkGLError();

    {
        float zero[256*4] = {0.0f};
        // Binding index 0 = VS floats, 1 = VS ints, 2 = VS bools
        glGenBuffers(1, &mGLState.vs_uniform_bufferf);
        glBindBuffer(GL_UNIFORM_BUFFER, mGLState.vs_uniform_bufferf);
        glBufferData(GL_UNIFORM_BUFFER, 256*sizeof(Vector4f), zero, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, VSF_BINDING_IDX, mGLState.vs_uniform_bufferf);
        // Binding index 3 = PS floats, 4 = PS ints, 5 = PS bools
        glGenBuffers(1, &mGLState.ps_uniform_bufferf);
        glBindBuffer(GL_UNIFORM_BUFFER, mGLState.ps_uniform_bufferf);
        glBufferData(GL_UNIFORM_BUFFER, 256*sizeof(Vector4f), zero, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, PSF_BINDING_IDX, mGLState.ps_uniform_bufferf);
        // Binding index 6 = projection fixup matrix
        glGenBuffers(1, &mGLState.pos_fixup_uniform_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, mGLState.pos_fixup_uniform_buffer);
        // Maybe STATIC_DRAW? Many things could cause this to change unnecessarily, though.
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Vector4f), zero, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, POSFIXUP_BINDING_IDX, mGLState.pos_fixup_uniform_buffer);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkGLError();

    for(size_t i = 0;i < mGLState.sampler_type.size();++i)
        mGLState.sampler_type[i] = GL_NONE;
    for(size_t i = 0;i < mGLState.sampler_binding.size();++i)
        mGLState.sampler_binding[i] = 0;
    glActiveTexture(GL_TEXTURE0);
    mGLState.active_stage = 0;

    mGLState.vertex_array_enabled = false;
    mGLState.normal_array_enabled = false;
    mGLState.color_array_enabled = false;
    mGLState.specular_array_enabled = false;
    mGLState.texcoord_array_enabled = 0;

    mGLState.attrib_array_enabled = 0;

    {
        std::array<GLenum,D3D_MAX_SIMULTANEOUS_RENDERTARGETS> buffers{
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
        };
        glBindFramebuffer(GL_FRAMEBUFFER, mGLState.main_framebuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffers(std::min(buffers.size(), mAdapter.getLimits().buffers),
                      buffers.data());
        mGLState.current_framebuffer[0] = mGLState.main_framebuffer;
        mGLState.current_framebuffer[1] = mGLState.main_framebuffer;
    }

    glEnable(GL_POLYGON_OFFSET_FILL);

    glFrontFace(GL_CCW);
    checkGLError();
}
class InitGLDeviceCmd : public Command {
    D3DGLDevice *mTarget;
    HDC mDc;
    HGLRC mGLContext;

public:
    InitGLDeviceCmd(D3DGLDevice *target, HDC dc, HGLRC glcontext) : mTarget(target), mDc(dc), mGLContext(glcontext) { }
    virtual ULONG execute()
    {
        mTarget->initGL(mDc, mGLContext);
        return sizeof(*this);
    }
};

void D3DGLDevice::deinitGL()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteBuffers(1, &mGLState.pos_fixup_uniform_buffer);
    glDeleteBuffers(1, &mGLState.ps_uniform_bufferf);
    glDeleteBuffers(1, &mGLState.vs_uniform_bufferf);

    glDeleteFramebuffers(2, mGLState.copy_framebuffers);
    glDeleteFramebuffers(1, &mGLState.main_framebuffer);

    glBindProgramPipeline(0);
    glDeleteProgramPipelines(1, &mGLState.pipeline);

    for(size_t i = 0;i < mGLState.samplers.size();++i)
        glBindSampler(i, 0);
    glDeleteSamplers(mGLState.samplers.size(), mGLState.samplers.data());

    wglMakeCurrent(nullptr, nullptr);
}
class DeinitGLDeviceCmd : public Command {
    D3DGLDevice *mTarget;

public:
    DeinitGLDeviceCmd(D3DGLDevice *target) : mTarget(target) { }
    virtual ULONG execute()
    {
        mTarget->deinitGL();
        return sizeof(*this);
    }
};


D3DGLDevice::D3DGLDevice(Direct3DGL *parent, const D3DAdapter &adapter, HWND window, DWORD flags)
  : mRefCount(0)
  , mParent(parent)
  , mAdapter(adapter)
  , mGLContext(nullptr)
  , mWindow(window)
  , mFlags(flags)
  , mAutoDepthStencil(nullptr)
  , mSwapchains{nullptr}
  , mDepthStencil(nullptr)
  , mInScene(false)
  , mVSConstantsF{0.0f}
  , mPSConstantsF{0.0f}
  , mVertexShader(nullptr)
  , mPixelShader(nullptr)
  , mVertexDecl(nullptr)
  , mIndexBuffer(nullptr)
  , mPrimitiveUserData(nullptr)
  , mDepthBits(0)
{
    for(auto &rt : mRenderTargets) rt = nullptr;
    for(auto &tex : mTextures) tex = nullptr;
    for(size_t i = 0;i < mTexStageState.size();++i)
    {
        auto &tss = mTexStageState[i];
        std::copy(DefaultTSSValues.begin(), DefaultTSSValues.end(), tss.begin());
        tss[D3DTSS_COLOROP] = i ? D3DTOP_DISABLE : D3DTOP_MODULATE;
        tss[D3DTSS_ALPHAOP] = i ? D3DTOP_DISABLE : D3DTOP_SELECTARG1;
        tss[D3DTSS_TEXCOORDINDEX] = i;
    }
    for(auto &ss : mSamplerState)
        std::copy(DefaultSSValues.begin(), DefaultSSValues.end(), ss.begin());
    std::copy(DefaultRSValues.begin(), DefaultRSValues.end(), mRenderState.begin());
    mRenderState[D3DRS_POINTSIZE_MAX] = float_to_dword(mAdapter.getLimits().pointsize_max);

    InitializeCriticalSection(&mSwapCS);
    InitializeConditionVariable(&mSwapCV);

    mParent->AddRef();
}

D3DGLDevice::~D3DGLDevice()
{
    delete mPrimitiveUserData;
    mPrimitiveUserData = nullptr;

    D3DGLVertexDeclaration *vtxdecl = mVertexDecl.exchange(nullptr);
    if(vtxdecl) vtxdecl->releaseIface();

    for(auto &schain : mSwapchains)
    {
        delete schain;
        schain = nullptr;
    }
    delete mAutoDepthStencil;
    mAutoDepthStencil = nullptr;

    if(mQueue.isActive())
    {
        mQueue.send<DeinitGLDeviceCmd>(this);
        mQueue.deinit();
    }
    if(mGLContext)
        wglDeleteContext(mGLContext);
    mGLContext = nullptr;

    DeleteCriticalSection(&mSwapCS);

    mParent->Release();
    mParent = nullptr;
}

bool D3DGLDevice::init(D3DPRESENT_PARAMETERS *params)
{
    if(params->BackBufferCount > 1)
    {
        WARN("Too many backbuffers requested (%u)\n", params->BackBufferCount);
        params->BackBufferCount = 1;
        return false;
    }

    if((params->Flags&D3DPRESENTFLAG_LOCKABLE_BACKBUFFER))
    {
        FIXME("Lockable backbuffer not currently supported\n");
        return false;
    }

    if(!(mAdapter.getUsage(D3DRTYPE_SURFACE, params->BackBufferFormat)&D3DUSAGE_RENDERTARGET))
    {
        WARN("Format %s is not a valid rendertarget format\n", d3dfmt_to_str(params->BackBufferFormat));
        return false;
    }
    if(params->EnableAutoDepthStencil)
    {
        if(!(mAdapter.getUsage(D3DRTYPE_SURFACE, params->AutoDepthStencilFormat)&D3DUSAGE_DEPTHSTENCIL))
        {
            WARN("Format %s is not a valid depthstencil format\n", d3dfmt_to_str(params->AutoDepthStencilFormat));
            return false;
        }
    }

    if(!mQueue.init())
        return false;

    std::vector<std::array<int,2>> glattrs;
    glattrs.reserve(16);
    glattrs.push_back({WGL_DRAW_TO_WINDOW_ARB, GL_TRUE});
    glattrs.push_back({WGL_SUPPORT_OPENGL_ARB, GL_TRUE});
    glattrs.push_back({WGL_DOUBLE_BUFFER_ARB, GL_TRUE});
    glattrs.push_back({WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB});
    if(!fmt_to_glattrs(params->BackBufferFormat, std::back_inserter(glattrs)))
        return false;
    // Got all attrs
    glattrs.push_back({0, 0});

    HWND win = ((params->Windowed && !params->hDeviceWindow) ? mWindow : params->hDeviceWindow);
    HDC hdc = GetDC(win);

    int pixelFormat;
    UINT numFormats;
    if(!wglChoosePixelFormatARB(hdc, &glattrs[0][0], NULL, 1, &pixelFormat, &numFormats))
    {
        ERR("Failed to choose a pixel format\n");
        ReleaseDC(win, hdc);
        return false;
    }
    if(numFormats < 1)
    {
        ERR("No suitable pixel formats found\n");
        ReleaseDC(win, hdc);
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd;
    if(SetPixelFormat(hdc, pixelFormat, &pfd) == 0)
    {
        ERR("Failed to set a pixel format, error %lu\n", GetLastError());
        ReleaseDC(win, hdc);
        return false;
    }

    glattrs.clear();
    // TODO: Switch to CORE profile if we ever do FFP emulation with shaders.
    glattrs.push_back({WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB});
    if(DebugEnable)
        glattrs.push_back({WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB});
    glattrs.push_back({0, 0});
    mGLContext = wglCreateContextAttribsARB(hdc, nullptr, &glattrs[0][0]);
    if(!mGLContext)
    {
        ERR("Failed to create OpenGL context, error %lu\n", GetLastError());
        ReleaseDC(win, hdc);
        return false;
    }

    mQueue.sendSync<InitGLDeviceCmd>(this, hdc, mGLContext);
    ReleaseDC(win, hdc);

    return SUCCEEDED(Reset(params));
}


HRESULT D3DGLDevice::drawVtxDecl(GLenum mode, INT startvtx, UINT minvtx, UINT startidx, UINT count, bool use_indices, bool user_vtxdata)
{
    mQueue.lock();

    D3DGLVertexDeclaration *vtxdecl = mVertexDecl;
    if(!vtxdecl)
    {
        WARN("No vertex declaration set\n");
        mQueue.unlock();
        return D3DERR_INVALIDCALL;
    }

    std::array<GLStreamData,16> streams;
    GLuint cur = 0;

    D3DGLVertexShader *vshader = mVertexShader;
    for(const D3DGLVERTEXELEMENT &elem : vtxdecl->getVtxElements())
    {
        if(cur >= streams.size())
        {
            ERR("Too many vertex elements!\n");
            mQueue.unlock();
            return D3DERR_INVALIDCALL;
        }

        if(user_vtxdata && elem.Stream != 0)
        {
            ERR("Stream %u referenced with user vertex data\n", elem.Stream);
            mQueue.unlock();
            return D3DERR_INVALIDCALL;
        }

        const StreamSource &stream = mStreams[elem.Stream];
        D3DGLBufferObject *buffer = stream.mBuffer;

        GLint offset = elem.Offset + stream.mOffset + stream.mStride*startvtx;
        streams[cur].mBufferId = buffer->getBufferId();
        streams[cur].mPointer = ((GLubyte*)0) + offset;
        streams[cur].mGLCount = elem.mGLCount;
        streams[cur].mGLType = elem.mGLType;
        streams[cur].mNormalize = elem.mNormalize;
        streams[cur].mStride = stream.mStride;
        streams[cur].mDivisor = 0;
        if((stream.mFreq&D3DSTREAMSOURCE_INSTANCEDATA))
            streams[cur].mDivisor = (stream.mFreq&0x3fffffff);

        if(vshader)
        {
            streams[cur].mTarget = vshader->getLocation(elem.Usage, elem.UsageIndex);
            if(streams[cur].mTarget == -1)
            {
                TRACE("Skipping element (usage 0x%02x, index %u, vshader %p)\n",
                      elem.Usage, elem.UsageIndex, vshader);
                continue;
            }
            streams[cur].mIndex = 0;
        }
        else if(elem.Usage == D3DDECLUSAGE_POSITION || elem.Usage == D3DDECLUSAGE_POSITIONT)
        {
            streams[cur].mTarget = GL_VERTEX_ARRAY;
            streams[cur].mIndex = 0;
        }
        else if(elem.Usage == D3DDECLUSAGE_TEXCOORD)
        {
            streams[cur].mTarget = GL_TEXTURE_COORD_ARRAY;
            streams[cur].mIndex = 0;
            // Apply this element data to whatever stages are using this index
            for(UINT i = 0;i < mTexStageState.size();i++)
            {
                if((mTexStageState[i][D3DTSS_TEXCOORDINDEX]&0xFFFF) == elem.UsageIndex)
                    streams[cur].mIndex |= 1<<i;
            }
        }
        else if(elem.Usage == D3DDECLUSAGE_NORMAL)
        {
            streams[cur].mTarget = GL_NORMAL_ARRAY;
            streams[cur].mIndex = 0;
        }
        else if(elem.Usage == D3DDECLUSAGE_COLOR)
        {
            if(elem.UsageIndex == 0)
                streams[cur].mTarget = GL_COLOR_ARRAY;
            else if(elem.UsageIndex == 1)
                streams[cur].mTarget = GL_SECONDARY_COLOR_ARRAY;
            streams[cur].mIndex = 0;
        }
        ++cur;
    }

    if(vshader)
        mQueue.doSend<SetVtxDataCmd<UseShaders>>(this, streams.data(), cur);
    else
        mQueue.doSend<SetVtxDataCmd<UseFFP>>(this, streams.data(), cur);

    if(!use_indices)
        mQueue.sendAndUnlock<DrawGLArraysCmd>(this, mode, count, 1/*num_instances*/);
    else
    {
        GLsizei num_instances = 1;
        if((mStreams[0].mFreq&D3DSTREAMSOURCE_INDEXEDDATA))
            num_instances = (mStreams[0].mFreq&0x3fffffff);

        D3DGLBufferObject *idxbuffer = mIndexBuffer;
        if(!idxbuffer)
        {
            WARN("No index buffer set\n");
            mQueue.unlock();
            return D3DERR_INVALIDCALL;
        }
        GLenum type = GL_NONE;
        GLubyte *pointer = nullptr;
        switch(idxbuffer->getFormat())
        {
            case D3DFMT_INDEX16:
                type = GL_UNSIGNED_SHORT;
                pointer += startidx*2;
                break;
            case D3DFMT_INDEX32:
                type = GL_UNSIGNED_INT;
                pointer += startidx*4;
                break;
            default:
                mQueue.unlock();
                ERR("Unexpected index data format, %s\n", d3dfmt_to_str(idxbuffer->getFormat()));
                return D3DERR_INVALIDCALL;
        }

        mQueue.sendAndUnlock<DrawGLElementsCmd>(this, mode, count, type, pointer, num_instances, minvtx);
    }

    return D3D_OK;
}

void D3DGLDevice::resetProjectionFixup(UINT width, UINT height)
{
    // OpenGL places pixel coords at the pixel's bottom-left, while D3D places
    // it at the center. So we have to translate X and Y by /almost/ 0.5 pixels
    // (almost because of certain triangle edge rules GL is a bit lax on). The
    // offset is doubled since projection ranges from -1...+1 instead of 0...1.
    //
    // We supply the needed X/Y offsets through a (shared) vec4 uniform. The
    // shader is also responsible for flipping Y and fixing Z depth.
    float trans[4] = { 0.99f/width, 0.99f/height, 0.0f, 0.0f };

    mQueue.doSend<SetBufferValues<1>>(mGLState.pos_fixup_uniform_buffer, trans, 0, 1);
}


HRESULT D3DGLDevice::QueryInterface(const IID &riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p.\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLDevice);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DDevice9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    if(riid == IID_IDirect3DDevice9Ex)
    {
        WARN("IDirect3D9 instance wasn't created with CreateDirect3D9Ex, returning E_NOINTERFACE.\n");
        return E_NOINTERFACE;
    }

    return E_NOINTERFACE;
}

ULONG D3DGLDevice::AddRef(void)
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG D3DGLDevice::Release(void)
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLDevice::TestCooperativeLevel()
{
    FIXME("iface %p : stub!\n", this);
    return D3D_OK;
}

UINT D3DGLDevice::GetAvailableTextureMem()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

HRESULT D3DGLDevice::EvictManagedResources()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetDirect3D(IDirect3D9 **d3d9)
{
    TRACE("iface %p, d3d9 %p\n", this, d3d9);
    *d3d9 = mParent;
    (*d3d9)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::GetDeviceCaps(D3DCAPS9 *caps)
{
    TRACE("iface %p, caps %p\n", this, caps);
    *caps = mAdapter.getCaps();
    return D3D_OK;
}

HRESULT D3DGLDevice::GetDisplayMode(UINT swapchain, D3DDISPLAYMODE *mode)
{
    TRACE("iface %p, swapchain %u, mode %p\n", this, swapchain, mode);

    if(swapchain >= mSwapchains.size())
    {
        FIXME("Out of range swapchain (%u >= %u)\n", swapchain, mSwapchains.size());
        return D3DERR_INVALIDCALL;
    }

    return mSwapchains[swapchain]->GetDisplayMode(mode);
}

HRESULT D3DGLDevice::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *params)
{
    TRACE("iface %p, params %p\n", this, params);

    params->AdapterOrdinal = mAdapter.getOrdinal();
    params->DeviceType = D3DDEVTYPE_HAL;
    params->hFocusWindow = mWindow;
    params->BehaviorFlags = mFlags;

    return D3D_OK;
}

HRESULT D3DGLDevice::SetCursorProperties(UINT xoffset, UINT yoffset, IDirect3DSurface9 *image)
{
    FIXME("iface %p, xoffset %u, yoffset %u, image %p : stub!\n", this, xoffset, yoffset, image);
    return E_NOTIMPL;
}

void D3DGLDevice::SetCursorPosition(int x, int y, DWORD flags)
{
    FIXME("iface %p, x %d, y %d, flags 0x%lx : stub!\n", this, x, y, flags);
}

WINBOOL D3DGLDevice::ShowCursor(WINBOOL show)
{
    FIXME("iface %p, show %u : stub!\n", this, show);
    return FALSE;
}

HRESULT D3DGLDevice::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *params, IDirect3DSwapChain9 **schain)
{
    FIXME("iface %p, params %p, schain %p : stub!\n", this, params, schain);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetSwapChain(UINT swapchain, IDirect3DSwapChain9 **schain)
{
    TRACE("iface %p, swapchain %u, schain %p\n", this, swapchain, schain);

    if(swapchain >= mSwapchains.size())
    {
        FIXME("Out of range swapchain (%u >= %u)\n", swapchain, mSwapchains.size());
        return D3DERR_INVALIDCALL;
    }

    *schain = mSwapchains[swapchain];
    (*schain)->AddRef();
    return D3D_OK;
}

UINT D3DGLDevice::GetNumberOfSwapChains()
{
    TRACE("iface %p\n", this);
    return mSwapchains.size();
}

HRESULT D3DGLDevice::Reset(D3DPRESENT_PARAMETERS *params)
{
    TRACE("iface %p, params %p\n", this, params);

    TRACE("Resetting device with parameters:\n"
          "\tBackBufferWidth            = %u\n"
          "\tBackBufferHeight           = %u\n"
          "\tBackBufferFormat           = %s\n"
          "\tBackBufferCount            = %u\n"
          "\tMultiSampleType            = 0x%x\n"
          "\tMultiSampleQuality         = %lu\n"
          "\tSwapEffect                 = 0x%x\n"
          "\thDeviceWindow              = %p\n"
          "\tWindowed                   = %d\n"
          "\tEnableAutoDepthStencil     = %d\n"
          "\tAutoDepthStencilFormat     = %s\n"
          "\tFlags                      = 0x%lx\n"
          "\tFullScreen_RefreshRateInHz = %u\n"
          "\tPresentationInterval       = 0x%x\n",
          params->BackBufferWidth, params->BackBufferHeight, d3dfmt_to_str(params->BackBufferFormat),
          params->BackBufferCount, params->MultiSampleType, params->MultiSampleQuality,
          params->SwapEffect, params->hDeviceWindow, params->Windowed,
          params->EnableAutoDepthStencil, d3dfmt_to_str(params->AutoDepthStencilFormat),
          params->Flags, params->FullScreen_RefreshRateInHz, params->PresentationInterval);

    if(params->BackBufferCount > 1)
    {
        WARN("Too many backbuffers requested (%u)\n", params->BackBufferCount);
        params->BackBufferCount = 1;
        return D3DERR_INVALIDCALL;
    }

    if((params->Flags&D3DPRESENTFLAG_LOCKABLE_BACKBUFFER))
    {
        FIXME("Lockable backbuffer not currently supported\n");
        return D3DERR_INVALIDCALL;
    }

    if(!(mAdapter.getUsage(D3DRTYPE_SURFACE, params->BackBufferFormat)&D3DUSAGE_RENDERTARGET))
    {
        WARN("Format %s is not a valid rendertarget format\n", d3dfmt_to_str(params->BackBufferFormat));
        return D3DERR_INVALIDCALL;
    }

    if(params->EnableAutoDepthStencil)
    {
        if(!(mAdapter.getUsage(D3DRTYPE_SURFACE, params->AutoDepthStencilFormat)&D3DUSAGE_DEPTHSTENCIL))
        {
            WARN("Format %s is not a valid depthstencil format\n", d3dfmt_to_str(params->AutoDepthStencilFormat));
            return D3DERR_INVALIDCALL;
        }
    }

    if(mSwapchains[0])
    {
        IDirect3DSurface9 *oldrt = mSwapchains[0]->getBackbuffer();
        for(auto &rtarget : mRenderTargets)
        {
            if(rtarget && rtarget != oldrt)
            {
                FIXME("Unexpected rendertarget, iface %p\n", rtarget.load());
                return D3DERR_INVALIDCALL;
            }
        }
    }
    if(mDepthStencil && mDepthStencil != mAutoDepthStencil)
    {
        FIXME("Unexpected depthstencil, iface %p\n", mDepthStencil.load());
        return D3DERR_INVALIDCALL;
    }

    // FIXME: Check to make sure no D3DPOOL_DEFAULT resources remain.
    // FIXME: Failure beyond this point causes an unusable device.

    delete mSwapchains[0];
    mSwapchains[0] = nullptr;
    for(auto &rtarget : mRenderTargets)
        rtarget = nullptr;

    delete mAutoDepthStencil;
    mAutoDepthStencil = nullptr;
    mDepthStencil = nullptr;
    mDepthBits = 0;

    HWND win = ((params->Windowed && !params->hDeviceWindow) ? mWindow : params->hDeviceWindow);
    D3DGLSwapChain *schain = new D3DGLSwapChain(this);
    if(!schain->init(params, win, true))
    {
        delete schain;
        return D3DERR_INVALIDCALL;
    }
    mSwapchains[0] = schain;

    // Set the default backbuffer and depth-stencil surface. Note that they do
    // not start with any reference count, which means that their refcounts
    // will underflow if the targets are changed without the app getting a
    // reference to them. This is generally okay, since they won't be deleted
    // until the device is anyway.
    mRenderTargets[0] = schain->getBackbuffer();

    if(params->EnableAutoDepthStencil)
    {
        D3DSURFACE_DESC desc;
        desc.Format = params->AutoDepthStencilFormat;
        desc.Type = D3DRTYPE_SURFACE;
        desc.Usage = D3DUSAGE_DEPTHSTENCIL;
        desc.Pool = D3DPOOL_DEFAULT;
        desc.MultiSampleType = params->MultiSampleType;
        desc.MultiSampleQuality = params->MultiSampleQuality;
        desc.Width = params->BackBufferWidth;
        desc.Height = params->BackBufferHeight;

        mAutoDepthStencil = new D3DGLRenderTarget(this);
        if(!mAutoDepthStencil->init(&desc, true))
            return D3DERR_INVALIDCALL;
        mDepthStencil = mAutoDepthStencil;

        mDepthBits = mAutoDepthStencil->getFormat().getDepthBits();
    }


    mQueue.lock();
    mViewport.X = 0;
    mViewport.Y = 0;
    mViewport.Width = params->BackBufferWidth;
    mViewport.Height = params->BackBufferHeight;
    mViewport.MinZ = 0.0f;
    mViewport.MaxZ = 1.0f;
    resetProjectionFixup(mViewport.Width, mViewport.Height);

    mScissorRect = RECT{0, 0, (LONG)params->BackBufferWidth, (LONG)params->BackBufferHeight};
    mQueue.doSend<ScissorRectSet>(mScissorRect);

    if(mAutoDepthStencil)
        mQueue.doSend<SetFBAttachmentCmd>(this, mAutoDepthStencil->getFormat().getDepthStencilAttachment(),
                                          GL_RENDERBUFFER, mAutoDepthStencil->getId(), 0);
    mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                             schain->getBackbuffer()->getId(), 0);

    return D3D_OK;
}

HRESULT D3DGLDevice::Present(const RECT *srcRect, const RECT *dstRect, HWND dstWindowOverride, const RGNDATA *dirtyRegion)
{
    TRACE("iface %p, srcRect %p, dstRect %p, dstWindowOverride %p, dirtyRegion %p : semi-stub\n",
          this, srcRect, dstRect, dstWindowOverride, dirtyRegion);

    return mSwapchains[0]->Present(srcRect, dstRect, dstWindowOverride, dirtyRegion, 0);
}

HRESULT D3DGLDevice::GetBackBuffer(UINT swapchain, UINT backbuffer, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **bbuffer)
{
    TRACE("iface %p, swapchain %u, backbuffer %u, type 0x%x, bbuffer %p\n", this, swapchain, backbuffer, type, bbuffer);

    if(swapchain >= mSwapchains.size())
    {
        WARN("Out of range swapchain (%u >= %u)\n", swapchain, mSwapchains.size());
        return D3DERR_INVALIDCALL;
    }

    return mSwapchains[swapchain]->GetBackBuffer(backbuffer, type, bbuffer);
}

HRESULT D3DGLDevice::GetRasterStatus(UINT swapchain, D3DRASTER_STATUS *status)
{
    TRACE("iface %p, swapchain %u, status %p\n", this, swapchain, status);

    if(swapchain >= mSwapchains.size())
    {
        WARN("Out of range swapchain (%u >= %u)\n", swapchain, mSwapchains.size());
        return D3DERR_INVALIDCALL;
    }

    return mSwapchains[swapchain]->GetRasterStatus(status);
}

HRESULT D3DGLDevice::SetDialogBoxMode(WINBOOL enable)
{
    FIXME("iface %p, enable %u : stub!\n", this, enable);
    return E_NOTIMPL;
}

void D3DGLDevice::SetGammaRamp(UINT swapchain, DWORD flags, const D3DGAMMARAMP *ramp)
{
    FIXME("iface %p, swapchain %u, flags 0x%lx, ramp %p : stub!\n", this, swapchain, flags, ramp);
}

void D3DGLDevice::GetGammaRamp(UINT swapchain, D3DGAMMARAMP *ramp)
{
    FIXME("iface %p, swapchain %u, ramp %p : stub!\n", this, swapchain, ramp);
}

HRESULT D3DGLDevice::CreateTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9 **texture, HANDLE *handle)
{
    TRACE("iface %p, width %u, height %u, levels %u, usage 0x%lx, format %s, pool 0x%x, texture %p, handle %p\n", this, width, height, levels, usage, d3dfmt_to_str(format), pool, texture, handle);

    if(handle)
    {
        WARN("Non-NULL handle specified\n");
        return D3DERR_INVALIDCALL;
    }
    if(!texture)
    {
        WARN("NULL texture storage specified\n");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = D3D_OK;
    DWORD realusage = mAdapter.getUsage(D3DRTYPE_TEXTURE, format);
    if((usage&realusage) != usage)
    {
        usage &= ~D3DUSAGE_AUTOGENMIPMAP;
        if((usage&realusage) != usage)
        {
            ERR("Invalid usage flags, 0x%lx / 0x%lx\n", usage, realusage);
            return D3DERR_INVALIDCALL;
        }
        WARN("AUTOGENMIPMAP requested, but unavailable (usage: 0x%lx)\n", realusage);
        hr = D3DOK_NOAUTOGEN;
    }

    D3DSURFACE_DESC desc;
    desc.Format = format;
    desc.Type = D3DRTYPE_TEXTURE;
    desc.Usage = usage;
    desc.Pool = pool;
    desc.MultiSampleType = D3DMULTISAMPLE_NONE;
    desc.MultiSampleQuality = 0;
    desc.Width = width;
    desc.Height = height;

    D3DGLTexture *tex = new D3DGLTexture(this);
    if(!tex->init(&desc, levels))
    {
        delete tex;
        return D3DERR_INVALIDCALL;
    }

    *texture = tex;
    (*texture)->AddRef();

    return hr;
}

HRESULT D3DGLDevice::CreateVolumeTexture(UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture9 **texture, HANDLE *handle)
{
    FIXME("iface %p, width %u, height %u, depth %u, levels %u, usage 0x%lx, format %s, pool 0x%x, texture %p, handle %p : stub!\n", this, width, height, depth, levels, usage, d3dfmt_to_str(format), pool, texture, handle);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::CreateCubeTexture(UINT edgeLength, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture9 **texture, HANDLE *handle)
{
    TRACE("iface %p, edgeLength %u, levels %u, usage 0x%lx, format %s, pool 0x%x, texture %p, handle %p\n", this, edgeLength, levels, usage, d3dfmt_to_str(format), pool, texture, handle);

    if(handle)
    {
        WARN("Non-NULL handle specified\n");
        return D3DERR_INVALIDCALL;
    }
    if(!texture)
    {
        WARN("NULL texture storage specified\n");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = D3D_OK;
    DWORD realusage = mAdapter.getUsage(D3DRTYPE_CUBETEXTURE, format);
    if((usage&realusage) != usage)
    {
        usage &= ~D3DUSAGE_AUTOGENMIPMAP;
        if((usage&realusage) != usage)
        {
            ERR("Invalid usage flags, 0x%lx / 0x%lx\n", usage, realusage);
            return D3DERR_INVALIDCALL;
        }
        WARN("AUTOGENMIPMAP requested, but unavailable (usage: 0x%lx)\n", realusage);
        hr = D3DOK_NOAUTOGEN;
    }

    D3DSURFACE_DESC desc;
    desc.Format = format;
    desc.Type = D3DRTYPE_TEXTURE;
    desc.Usage = usage;
    desc.Pool = pool;
    desc.MultiSampleType = D3DMULTISAMPLE_NONE;
    desc.MultiSampleQuality = 0;
    desc.Width = edgeLength;
    desc.Height = edgeLength;

    D3DGLCubeTexture *tex = new D3DGLCubeTexture(this);
    if(!tex->init(&desc, levels))
    {
        delete tex;
        return D3DERR_INVALIDCALL;
    }

    *texture = tex;
    (*texture)->AddRef();

    return hr;
}

HRESULT D3DGLDevice::CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9 **vbuffer, HANDLE *handle)
{
    TRACE("iface %p, length %u, usage 0x%lx, fvf 0x%lx, pool 0x%x, vbuffer %p, handle %p\n", this, length, usage, fvf, pool, vbuffer, handle);

    if(handle)
    {
        WARN("Non-NULL handle specified\n");
        return D3DERR_INVALIDCALL;
    }
    if(!vbuffer)
    {
        WARN("NULL vertex buffer storage specified\n");
        return D3DERR_INVALIDCALL;
    }

    D3DGLBufferObject *vbuf = new D3DGLBufferObject(this);
    if(!vbuf->init_vbo(length, usage, fvf, pool))
    {
        delete vbuf;
        return D3DERR_INVALIDCALL;
    }

    *vbuffer = vbuf;
    (*vbuffer)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9 **ibuffer, HANDLE *handle)
{
    TRACE("iface %p, length %u, usage 0x%lx, format %s, pool 0x%x, vbuffer %p, handle %p\n", this, length, usage, d3dfmt_to_str(format), pool, ibuffer, handle);

    if(handle)
    {
        WARN("Non-NULL handle specified\n");
        return D3DERR_INVALIDCALL;
    }
    if(!ibuffer)
    {
        WARN("NULL index buffer storage specified\n");
        return D3DERR_INVALIDCALL;
    }

    D3DGLBufferObject *ibuf = new D3DGLBufferObject(this);
    if(!ibuf->init_ibo(length, usage, format, pool))
    {
        delete ibuf;
        return D3DERR_INVALIDCALL;
    }

    *ibuffer = ibuf;
    (*ibuffer)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::CreateRenderTarget(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisampleQuality, WINBOOL lockable, IDirect3DSurface9 **surface, HANDLE *handle)
{
    TRACE("iface %p, width %u, height %u, format %s, multisample 0x%x, multisampleQuality %lu, lockable %d, surface %p, handle %p\n", this, width, height, d3dfmt_to_str(format), multisample, multisampleQuality, lockable, surface, handle);

    if(handle)
    {
        WARN("Non-NULL handle specified\n");
        return D3DERR_INVALIDCALL;
    }
    if(!surface)
    {
        WARN("NULL surface storage specified\n");
        return D3DERR_INVALIDCALL;
    }
    if(lockable)
    {
        FIXME("Lockable rendertargets not implemented\n");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = D3D_OK;
    DWORD realusage = mAdapter.getUsage(D3DRTYPE_SURFACE, format);
    if(!(D3DUSAGE_RENDERTARGET&realusage))
    {
        WARN("Format %s is missing RENDERTARGET usage\n", d3dfmt_to_str(format));
        return D3DERR_INVALIDCALL;
    }

    D3DSURFACE_DESC desc;
    desc.Format = format;
    desc.Type = D3DRTYPE_SURFACE;
    desc.Usage = D3DUSAGE_RENDERTARGET;
    desc.Pool = D3DPOOL_DEFAULT;
    desc.MultiSampleType = multisample;
    desc.MultiSampleQuality = multisampleQuality;
    desc.Width = width;
    desc.Height = height;

    D3DGLRenderTarget *rtarget = new D3DGLRenderTarget(this);
    if(!rtarget->init(&desc))
    {
        delete rtarget;
        return D3DERR_INVALIDCALL;
    }

    *surface = rtarget;
    (*surface)->AddRef();

    return hr;
}

HRESULT D3DGLDevice::CreateDepthStencilSurface(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisampleQuality, WINBOOL discard, IDirect3DSurface9 **surface, HANDLE *handle)
{
    TRACE("iface %p, width %u, height %u, format %s, multisample 0x%x, multisampleQuality %lu, discard %u, surface %p, handle %p\n", this, width, height, d3dfmt_to_str(format), multisample, multisampleQuality, discard, surface, handle);

    if(handle)
    {
        WARN("Non-NULL handle specified\n");
        return D3DERR_INVALIDCALL;
    }
    if(!surface)
    {
        WARN("NULL surface storage specified\n");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = D3D_OK;
    DWORD realusage = mAdapter.getUsage(D3DRTYPE_SURFACE, format);
    if(!(D3DUSAGE_DEPTHSTENCIL&realusage))
    {
        WARN("Format %s is missing DEPTHSTENCIL usage\n", d3dfmt_to_str(format));
        return D3DERR_INVALIDCALL;
    }

    D3DSURFACE_DESC desc;
    desc.Format = format;
    desc.Type = D3DRTYPE_SURFACE;
    desc.Usage = D3DUSAGE_DEPTHSTENCIL;
    desc.Pool = D3DPOOL_DEFAULT;
    desc.MultiSampleType = multisample;
    desc.MultiSampleQuality = multisampleQuality;
    desc.Width = width;
    desc.Height = height;

    D3DGLRenderTarget *rtarget = new D3DGLRenderTarget(this);
    if(!rtarget->init(&desc))
    {
        delete rtarget;
        return D3DERR_INVALIDCALL;
    }

    *surface = rtarget;
    (*surface)->AddRef();

    return hr;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
HRESULT D3DGLDevice::UpdateSurface(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::StretchRect(IDirect3DSurface9 *srcSurface, const RECT *srcRect, IDirect3DSurface9 *dstSurface, const RECT *dstRect, D3DTEXTUREFILTERTYPE filter)
{
    TRACE("iface %p, srcSurface %p, srcRect %p, dstSurface %p, dstRect %p, filter 0x%x : semi-stub\n", this, srcSurface, srcRect, dstSurface, dstRect, filter);

    GLenum src_target = GL_NONE, dst_target = GL_NONE;
    GLuint src_binding = 0, dst_binding = 0;
    GLint src_level = 0, dst_level = 0;
    RECT src_rect, dst_rect;
    GLbitfield src_mask, dst_mask;

    // FIXME: This doesn't handle depth or stencil blits
    union {
        void *pointer;
        D3DGLTextureSurface *tex2dsurface;
        D3DGLRenderTarget *surface;
        D3DGLCubeSurface *cubesurface;
    };

    // Get source surface info
    if(SUCCEEDED(srcSurface->QueryInterface(IID_D3DGLTextureSurface, &pointer)))
    {
        src_target = GL_TEXTURE_2D;
        src_binding = tex2dsurface->getParent()->getTextureId();
        src_level = tex2dsurface->getLevel();
        src_mask = tex2dsurface->getFormat().buffermask;
        tex2dsurface->Release();
    }
    else if(SUCCEEDED(srcSurface->QueryInterface(IID_D3DGLRenderTarget, &pointer)))
    {
        src_target = GL_RENDERBUFFER;
        src_binding = surface->getId();
        src_level = 0;
        src_mask = surface->getFormat().buffermask;
        surface->Release();
    }
    else if(SUCCEEDED(srcSurface->QueryInterface(IID_D3DGLCubeSurface, &pointer)))
    {
        src_target = cubesurface->getTarget();
        src_binding = cubesurface->getParent()->getTextureId();
        src_level = cubesurface->getLevel();
        src_mask = cubesurface->getFormat().buffermask;
        cubesurface->Release();
    }
    else
    {
        FIXME("Unhandled source surface: %p\n", srcSurface);
        return D3DERR_INVALIDCALL;
    }
    if(srcRect)
    {
        src_rect.left = srcRect->left;
        src_rect.top = srcRect->top;
        src_rect.right = srcRect->right;
        src_rect.bottom = srcRect->bottom;
    }
    else
    {
        D3DSURFACE_DESC desc;
        srcSurface->GetDesc(&desc);
        src_rect.left = 0;
        src_rect.top = 0;
        src_rect.right = desc.Width;
        src_rect.bottom = desc.Height;
    }

    // Get destination surface info
    if(SUCCEEDED(dstSurface->QueryInterface(IID_D3DGLTextureSurface, &pointer)))
    {
        dst_target = GL_TEXTURE_2D;
        dst_binding = tex2dsurface->getParent()->getTextureId();
        dst_level = tex2dsurface->getLevel();
        dst_mask = tex2dsurface->getFormat().buffermask;
        tex2dsurface->Release();
    }
    else if(SUCCEEDED(dstSurface->QueryInterface(IID_D3DGLRenderTarget, &pointer)))
    {
        dst_target = GL_RENDERBUFFER;
        dst_binding = surface->getId();
        dst_level = 0;
        dst_mask = surface->getFormat().buffermask;
        surface->Release();
    }
    else if(SUCCEEDED(dstSurface->QueryInterface(IID_D3DGLCubeSurface, &pointer)))
    {
        dst_target = cubesurface->getTarget();
        dst_binding = cubesurface->getParent()->getTextureId();
        dst_level = cubesurface->getLevel();
        dst_mask = cubesurface->getFormat().buffermask;
        cubesurface->Release();
    }
    else
    {
        FIXME("Unhandled destination surface: %p\n", srcSurface);
        return D3DERR_INVALIDCALL;
    }
    if(dstRect)
    {
        dst_rect.left = dstRect->left;
        dst_rect.top = dstRect->top;
        dst_rect.right = dstRect->right;
        dst_rect.bottom = dstRect->bottom;
    }
    else
    {
        D3DSURFACE_DESC desc;
        dstSurface->GetDesc(&desc);
        dst_rect.left = 0;
        dst_rect.top = 0;
        dst_rect.right = desc.Width;
        dst_rect.bottom = desc.Height;
    }

    if(!(src_mask&dst_mask))
    {
        FIXME("Mismatched format buffer masks: 0x%x & 0x%x\n", src_mask, dst_mask);
        return D3DERR_INVALIDCALL;
    }

    if(!(src_mask&GL_COLOR_BUFFER_BIT))
    {
        if(filter != D3DTEXF_POINT)
        {
            FIXME("Invalid filter 0x%x for non-color blit\n", filter);
            return D3DERR_INVALIDCALL;
        }
        FIXME("Unhandled non-color blit\n");
        return D3DERR_INVALIDCALL;
    }

    mQueue.send<BlitFramebufferCmd>(this, src_target, src_binding, src_level, src_rect,
                                    dst_target, dst_binding, dst_level, dst_rect,
                                    GetGLFilterMode(filter, D3DTEXF_NONE));

    return D3D_OK;
}

HRESULT D3DGLDevice::ColorFill(IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetRenderTarget(DWORD index, IDirect3DSurface9 *rtarget)
{
    TRACE("iface %p, index %lu, rendertarget %p\n", this, index, rtarget);

    if(index >= mRenderTargets.size() || index >= mAdapter.getLimits().buffers)
    {
        WARN("Index out of range: %lu >= %u\n", index, std::min(mRenderTargets.size(), mAdapter.getLimits().buffers));
        return D3DERR_INVALIDCALL;
    }

    if(!rtarget)
    {
        if(index == 0)
        {
            WARN("Cannot set render target #0 to null\n");
            return D3DERR_INVALIDCALL;
        }

        mQueue.lock();
        rtarget = mRenderTargets[index].exchange(rtarget);
        mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, GL_COLOR_ATTACHMENT0+index,
                                                 GL_RENDERBUFFER, 0, 0);
        if(rtarget) rtarget->Release();
        return D3D_OK;
    }

    union {
        void *pointer;
        D3DGLTextureSurface *tex2dsurface;
        D3DGLRenderTarget *surface;
        D3DGLCubeSurface *cubesurface;
    };
    if(SUCCEEDED(rtarget->QueryInterface(IID_D3DGLTextureSurface, &pointer)))
    {
        D3DGLTexture *tex2d = tex2dsurface->getParent();
        if(!(tex2d->getDesc().Usage&D3DUSAGE_RENDERTARGET))
        {
            tex2dsurface->Release();
            WARN("Surface %p missing RENDERTARGET usage (depth stencil?)\n", tex2dsurface);
            return D3DERR_INVALIDCALL;
        }

        mQueue.lock();
        rtarget = mRenderTargets[index].exchange(tex2dsurface);
        mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, GL_COLOR_ATTACHMENT0+index,
            GL_TEXTURE_2D, tex2d->getTextureId(), tex2dsurface->getLevel()
        );
    }
    else if(SUCCEEDED(rtarget->QueryInterface(IID_D3DGLRenderTarget, &pointer)))
    {
        if(!(surface->getDesc().Usage&D3DUSAGE_RENDERTARGET))
        {
            surface->Release();
            WARN("Surface %p missing RENDERTARGET usage (depth stencil?)\n", surface);
            return D3DERR_INVALIDCALL;
        }

        mQueue.lock();
        rtarget = mRenderTargets[index].exchange(surface);
        mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, GL_COLOR_ATTACHMENT0+index,
            GL_RENDERBUFFER, surface->getId(), 0
        );
    }
    else if(SUCCEEDED(rtarget->QueryInterface(IID_D3DGLCubeSurface, &pointer)))
    {
        D3DGLCubeTexture *cubetex = cubesurface->getParent();
        if(!(cubetex->getDesc().Usage&D3DUSAGE_RENDERTARGET))
        {
            cubesurface->Release();
            WARN("Surface %p missing RENDERTARGET usage (depth stencil?)\n", cubesurface);
            return D3DERR_INVALIDCALL;
        }

        mQueue.lock();
        rtarget = mRenderTargets[index].exchange(cubesurface);
        mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, GL_COLOR_ATTACHMENT0+index,
            cubesurface->getTarget(), cubetex->getTextureId(), 0
        );
    }
    else
    {
        ERR("Unknown surface %p\n", rtarget);
        rtarget->AddRef();
        rtarget = mRenderTargets[index].exchange(rtarget);
    }
    if(rtarget) rtarget->Release();

    return D3D_OK;
}

HRESULT D3DGLDevice::GetRenderTarget(DWORD index, IDirect3DSurface9 **rendertarget)
{
    TRACE("iface %p, index %lu, rendertarget %p\n", this, index, rendertarget);

    if(index >= mRenderTargets.size() || index >= mAdapter.getLimits().buffers)
    {
        WARN("Index out of range: %lu >= %u\n", index, std::min(mRenderTargets.size(), mAdapter.getLimits().buffers));
        return D3DERR_INVALIDCALL;
    }

    *rendertarget = mRenderTargets[index].load();
    if(*rendertarget) (*rendertarget)->AddRef();

    return D3D_OK;
}

HRESULT D3DGLDevice::SetDepthStencilSurface(IDirect3DSurface9 *depthstencil)
{
    TRACE("iface %p, depthstencil %p\n", this, depthstencil);

    if(!depthstencil)
    {
        mQueue.lock();
        depthstencil = mDepthStencil.exchange(depthstencil);
        mDepthBits = 0;
        mQueue.doSend<DepthBiasSet>(
            dword_to_float(mRenderState[D3DRS_SLOPESCALEDEPTHBIAS]),
            dword_to_float(mRenderState[D3DRS_DEPTHBIAS]) * (float)((1u<<mDepthBits) - 1u)
        );
        mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, GL_DEPTH_STENCIL_ATTACHMENT,
                                                 GL_RENDERBUFFER, 0, 0);
        if(depthstencil) depthstencil->Release();

        return D3D_OK;
    }

    union {
        void *pointer;
        D3DGLTextureSurface *tex2dsurface;
        D3DGLRenderTarget *surface;
        D3DGLCubeSurface *cubesurface;
    };
    if(SUCCEEDED(depthstencil->QueryInterface(IID_D3DGLTextureSurface, &pointer)))
    {
        D3DGLTexture *tex2d = tex2dsurface->getParent();
        if(!(tex2d->getDesc().Usage&D3DUSAGE_DEPTHSTENCIL))
        {
            tex2dsurface->Release();
            WARN("Texture %p missing DEPTHSTENCIL usage (render target?)\n", tex2dsurface);
            return D3DERR_INVALIDCALL;
        }

        GLenum attachment = tex2dsurface->getFormat().getDepthStencilAttachment();

        mQueue.lock();
        depthstencil = mDepthStencil.exchange(tex2dsurface);
        GLuint depthbits = tex2dsurface->getFormat().getDepthBits();
        if(depthbits != mDepthBits)
        {
            // If the previous depth buffer had a different bit depth, the
            // depth bias needs to be updated for the new depth (D3D specifies
            // the bias in normalized 0...1 units, and OpenGL uses unnormalized
            // units that are "the smallest value that is guaranteed to produce
            // a resolvable offset").
            mDepthBits = depthbits;
            mQueue.doSend<DepthBiasSet>(
                dword_to_float(mRenderState[D3DRS_SLOPESCALEDEPTHBIAS]),
                dword_to_float(mRenderState[D3DRS_DEPTHBIAS]) * (float)((1u<<mDepthBits) - 1u)
            );
        }
        if(attachment == GL_DEPTH_ATTACHMENT)
        {
            // If the previous attachment was GL_DEPTH_STENCIL_ATTACHMENT, and
            // this is GL_DEPTH_ATTACHMENT, the previous attachment would
            // remain as the stencil buffer.
            mQueue.doSend<SetFBAttachmentCmd>(this, GL_STENCIL_ATTACHMENT,
                                              GL_RENDERBUFFER, 0, 0);
        }
        mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, attachment,
            GL_TEXTURE_2D, tex2d->getTextureId(), tex2dsurface->getLevel()
        );
    }
    else if(SUCCEEDED(depthstencil->QueryInterface(IID_D3DGLRenderTarget, &pointer)))
    {
        if(!(surface->getDesc().Usage&D3DUSAGE_DEPTHSTENCIL))
        {
            surface->Release();
            WARN("Surface %p missing DEPTHSTENCIL usage (render target?)\n", surface);
            return D3DERR_INVALIDCALL;
        }

        GLenum attachment = surface->getFormat().getDepthStencilAttachment();

        mQueue.lock();
        depthstencil = mDepthStencil.exchange(surface);
        GLuint depthbits = surface->getFormat().getDepthBits();
        if(depthbits != mDepthBits)
        {
            mDepthBits = depthbits;
            mQueue.doSend<DepthBiasSet>(
                dword_to_float(mRenderState[D3DRS_SLOPESCALEDEPTHBIAS]),
                dword_to_float(mRenderState[D3DRS_DEPTHBIAS]) * (float)((1u<<mDepthBits) - 1u)
            );
        }
        if(attachment == GL_DEPTH_ATTACHMENT)
            mQueue.doSend<SetFBAttachmentCmd>(this, GL_STENCIL_ATTACHMENT,
                                              GL_RENDERBUFFER, 0, 0);
        mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, attachment,
            GL_RENDERBUFFER, surface->getId(), 0
        );
    }
    else if(SUCCEEDED(depthstencil->QueryInterface(IID_D3DGLCubeSurface, &pointer)))
    {
        D3DGLCubeTexture *cubetex = cubesurface->getParent();
        if(!(cubetex->getDesc().Usage&D3DUSAGE_DEPTHSTENCIL))
        {
            cubesurface->Release();
            WARN("Surface %p missing DETHSTENCIL usage (render target?)\n", cubesurface);
            return D3DERR_INVALIDCALL;
        }

        GLenum attachment = cubesurface->getFormat().getDepthStencilAttachment();

        mQueue.lock();
        depthstencil = mDepthStencil.exchange(cubesurface);
        GLuint depthbits = cubesurface->getFormat().getDepthBits();
        if(depthbits != mDepthBits)
        {
            mDepthBits = depthbits;
            mQueue.doSend<DepthBiasSet>(
                dword_to_float(mRenderState[D3DRS_SLOPESCALEDEPTHBIAS]),
                dword_to_float(mRenderState[D3DRS_DEPTHBIAS]) * (float)((1u<<mDepthBits) - 1u)
            );
        }
        if(attachment == GL_DEPTH_ATTACHMENT)
            mQueue.doSend<SetFBAttachmentCmd>(this, GL_STENCIL_ATTACHMENT,
                                              GL_RENDERBUFFER, 0, 0);
        mQueue.sendAndUnlock<SetFBAttachmentCmd>(this, attachment,
            cubesurface->getTarget(), cubetex->getTextureId(), cubesurface->getLevel()
        );
    }
    else
    {
        ERR("Unknown surface %p\n", depthstencil);
        depthstencil->AddRef();
        depthstencil = mDepthStencil.exchange(depthstencil);
    }
    if(depthstencil) depthstencil->Release();

    return D3D_OK;
}

HRESULT D3DGLDevice::GetDepthStencilSurface(IDirect3DSurface9 **depthstencil)
{
    TRACE("iface %p, depthstencil %p\n", this, depthstencil);
    *depthstencil = mDepthStencil.load();
    if(*depthstencil)
        (*depthstencil)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::BeginScene()
{
    TRACE("iface %p : semi-stub\n", this);

    if(mInScene.exchange(true))
    {
        ERR("Already in scene\n");
        return D3DERR_INVALIDCALL;
    }

    // Wait for the last swap to complete before starting the next scene
    EnterCriticalSection(&mSwapCS);
    while(mSwapchains[0]->getPendingSwaps() > 0)
        SleepConditionVariableCS(&mSwapCV, &mSwapCS, INFINITE);
    LeaveCriticalSection(&mSwapCS);

    // TODO: Prepare any GL state? Depends on what's allowed or not to be
    // called within a 'scene'.

    return D3D_OK;
}

HRESULT D3DGLDevice::EndScene()
{
    TRACE("iface %p : semi-stub\n", this);

    if(!mInScene.exchange(false))
    {
        ERR("Not in scene\n");
        return D3DERR_INVALIDCALL;
    }
    // TODO: Flush GL?

    return D3D_OK;
}

HRESULT D3DGLDevice::Clear(DWORD count, const D3DRECT *rects, DWORD flags, D3DCOLOR color, float depth, DWORD stencil)
{
    TRACE("iface %p, count %lu, rects %p, flags 0x%lx, color 0x%08lx, depth %f, stencil 0x%lx\n", this, count, rects, flags, color, depth, stencil);

    GLbitfield mask = 0;
    if((flags&D3DCLEAR_TARGET)) mask |= GL_COLOR_BUFFER_BIT;
    if((flags&D3DCLEAR_ZBUFFER)) mask |= GL_DEPTH_BUFFER_BIT;
    if((flags&D3DCLEAR_STENCIL)) mask |= GL_STENCIL_BUFFER_BIT;

    mQueue.lock();
    if(count == 0)
    {
        RECT main_rect;
        main_rect.left = mViewport.X;
        main_rect.top = mViewport.Y;
        main_rect.right = main_rect.left + mViewport.Width;
        main_rect.bottom = main_rect.top + mViewport.Height;
        mQueue.sendAndUnlock<ClearCmd>(this, mask, color, depth, stencil, main_rect);
    }
    else
    {
        mQueue.unlock();
        FIXME("Rectangles not yet handled\n");
    }

    return D3D_OK;
}

HRESULT D3DGLDevice::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetViewport(const D3DVIEWPORT9 *viewport)
{
    TRACE("iface %p, viewport %p\n", this, viewport);

    if(viewport->MinZ < 0.0f || viewport->MinZ > 1.0f ||
       viewport->MaxZ < 0.0f || viewport->MaxZ > 1.0f)
    {
        WARN("Invalid viewport Z range (%f -> %f)\n", viewport->MinZ, viewport->MaxZ);
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    mViewport = *viewport;
    resetProjectionFixup(mViewport.Width, mViewport.Height);
    mQueue.sendAndUnlock<ViewportSet>(mViewport.X, mViewport.Y,
        std::min(mViewport.Width, 0x7ffffffful), std::min(mViewport.Height, 0x7ffffffful),
        mViewport.MinZ, mViewport.MaxZ
    );

    return D3D_OK;
}

HRESULT D3DGLDevice::GetViewport(D3DVIEWPORT9 *viewport)
{
    TRACE("iface %p, viewport %p\n", this, viewport);
    mQueue.lock();
    *viewport = mViewport;
    mQueue.unlock();
    return D3D_OK;
}

HRESULT D3DGLDevice::SetMaterial(const D3DMATERIAL9 *material)
{
    TRACE("iface %p, material %p\n", this, material);
    mQueue.lock();
    mMaterial = *material;
    mQueue.sendAndUnlock<MaterialSet>(mMaterial);
    return D3D_OK;
}

HRESULT D3DGLDevice::GetMaterial(D3DMATERIAL9 *material)
{
    TRACE("iface %p, material %p\n", this, material);
    mQueue.lock();
    *material = mMaterial;
    mQueue.unlock();
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetLight(DWORD Index, CONST D3DLIGHT9*)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetLight(DWORD Index, D3DLIGHT9*)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::LightEnable(DWORD Index, WINBOOL Enable)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetLightEnable(DWORD Index, WINBOOL* pEnable)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetClipPlane(DWORD index, const float *plane)
{
    TRACE("iface %p, index %lu, plane %p\n", this, index, plane);

    if(index >= mClipPlane.size() || index >= mAdapter.getLimits().clipplanes)
    {
        WARN("Clip plane index out of range (%lu >= %u)\n", index,
             std::min(mAdapter.getLimits().clipplanes, mClipPlane.size()));
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    memcpy(mClipPlane[index].ptr(), plane, sizeof(mClipPlane[index]));
    // FIXME: Clip plane needs to be set using the view matrix when no vertex
    // shader is set.
    mQueue.sendAndUnlock<ClipPlaneSet>(index, mClipPlane[index]);

    return D3D_OK;
}

HRESULT D3DGLDevice::GetClipPlane(DWORD index, float *plane)
{
    TRACE("iface %p, index %lu, plane %p\n", this, index, plane);

    if(index >= mClipPlane.size() || index >= mAdapter.getLimits().clipplanes)
    {
        WARN("Clip plane index out of range (%lu >= %u)\n", index,
             std::min(mAdapter.getLimits().clipplanes, mClipPlane.size()));
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    memcpy(plane, mClipPlane[index].ptr(), sizeof(mClipPlane[index]));
    mQueue.unlock();

    return D3D_OK;
}

HRESULT D3DGLDevice::SetRenderState(D3DRENDERSTATETYPE state, DWORD value)
{
    TRACE("iface %p, state %s, value 0x%lx\n", this, d3drs_to_str(state), value);

    auto glstate = RSStateEnableMap.find(state);
    if(glstate != RSStateEnableMap.end())
    {
        if(state == D3DRS_ZENABLE && value == D3DZB_USEW)
        {
            FIXME("W-buffer not handled\n");
            return D3DERR_INVALIDCALL;
        }
        mQueue.lock();
        mRenderState[state] = value;
        mQueue.sendAndUnlock<StateEnable>(glstate->second, value!=0);
    }
    else switch(state)
    {
        case D3DRS_FILLMODE:
        {
            GLenum mode = GL_FILL;
            if(value == D3DFILL_POINT)
                mode = GL_POINT;
            else if(value == D3DFILL_WIREFRAME)
                mode = GL_LINE;
            else if(value != D3DFILL_SOLID)
                WARN("Invalid fill mode: 0x%lx\n", value);

            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<PolygonModeSet>(mode);
            break;
        }

        case D3DRS_CULLMODE:
        {
            GLenum face = GL_NONE;
            if(value == D3DCULL_CW)
                face = GL_FRONT;
            else if(value == D3DCULL_CCW)
                face = GL_BACK;
            else if(value != D3DCULL_NONE)
                WARN("Unhandled cull mode: 0x%lx\n", value);

            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<CullFaceSet>(face);
            break;
        }

        case D3DRS_COLORWRITEENABLE:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<ColorMaskSet>(value);
            break;

        case D3DRS_ZWRITEENABLE:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<DepthMaskSet>(value);
            break;

        case D3DRS_ZFUNC:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<DepthFuncSet>(GetGLCompFunc(value));
            break;

        case D3DRS_SLOPESCALEDEPTHBIAS:
        case D3DRS_DEPTHBIAS:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<DepthBiasSet>(
                dword_to_float(mRenderState[D3DRS_SLOPESCALEDEPTHBIAS]),
                dword_to_float(mRenderState[D3DRS_DEPTHBIAS]) * (float)((1u<<mDepthBits) - 1u)
            );
            break;

        case D3DRS_ALPHAFUNC:
        case D3DRS_ALPHAREF:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<AlphaFuncSet>(GetGLCompFunc(mRenderState[D3DRS_ALPHAFUNC]),
                                               mRenderState[D3DRS_ALPHAREF] / 255.0f);
            break;

        // FIXME: Handle D3DRS_SEPARATEALPHABLENDENABLE
        case D3DRS_SRCBLEND:
        case D3DRS_DESTBLEND:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<BlendFuncSet>(GetGLBlendFunc(mRenderState[D3DRS_SRCBLEND]),
                                               GetGLBlendFunc(mRenderState[D3DRS_DESTBLEND]));
            break;
        case D3DRS_BLENDOP:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<BlendOpSet>(GetGLBlendOp(value));
            break;

        case D3DRS_CLIPPLANEENABLE:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<ClipPlaneEnableCmd>(this, value);
            break;

        case D3DRS_STENCILWRITEMASK:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<StencilMaskSet>(value);
            break;

        case D3DRS_STENCILFUNC:
        case D3DRS_STENCILREF:
        case D3DRS_STENCILMASK:
            {
                mQueue.lock();
                GLenum face = mRenderState[D3DRS_TWOSIDEDSTENCILMODE] ? GL_FRONT : GL_FRONT_AND_BACK;
                mRenderState[state] = value;
                mQueue.sendAndUnlock<StencilFuncSet>(face,
                    GetGLCompFunc(mRenderState[D3DRS_STENCILFUNC]),
                    mRenderState[D3DRS_STENCILREF].load(),
                    mRenderState[D3DRS_STENCILMASK].load()
                );
            }
            break;

        case D3DRS_STENCILFAIL:
        case D3DRS_STENCILZFAIL:
        case D3DRS_STENCILPASS:
            {
                mQueue.lock();
                GLenum face = mRenderState[D3DRS_TWOSIDEDSTENCILMODE] ? GL_FRONT : GL_FRONT_AND_BACK;
                mRenderState[state] = value;
                mQueue.sendAndUnlock<StencilOpSet>(face,
                    GetGLStencilOp(mRenderState[D3DRS_STENCILFAIL]),
                    GetGLStencilOp(mRenderState[D3DRS_STENCILZFAIL]),
                    GetGLStencilOp(mRenderState[D3DRS_STENCILPASS])
                );
            }
            break;

        // FIXME: These probably shouldn't set OpenGL state while
        // D3DRS_TWOSIDEDSTENCILMODE is false.
        case D3DRS_CCW_STENCILFUNC:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<StencilFuncSet>(GL_BACK,
                GetGLCompFunc(mRenderState[D3DRS_CCW_STENCILFUNC]),
                mRenderState[D3DRS_STENCILREF].load(),
                mRenderState[D3DRS_STENCILMASK].load()
            );
            break;

        case D3DRS_CCW_STENCILFAIL:
        case D3DRS_CCW_STENCILZFAIL:
        case D3DRS_CCW_STENCILPASS:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<StencilOpSet>(GL_BACK,
                GetGLStencilOp(mRenderState[D3DRS_CCW_STENCILFAIL]),
                GetGLStencilOp(mRenderState[D3DRS_CCW_STENCILZFAIL]),
                GetGLStencilOp(mRenderState[D3DRS_CCW_STENCILPASS])
            );
            break;

        // FIXME: This should probably set the GL_BACK stencil func/ops from
        // CCW state when enabled, or set GL_FRONT_AND_BACK from CW state when
        // disabled.
        case D3DRS_TWOSIDEDSTENCILMODE:
            mRenderState[state] = value;
            break;

        case D3DRS_FOGCOLOR:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<FogValuefSet>(GL_FOG_COLOR,
                D3DCOLOR_R(value)/255.0f, D3DCOLOR_G(value)/255.0f,
                D3DCOLOR_B(value)/255.0f, D3DCOLOR_A(value)/255.0f
            );
            break;

        default:
            FIXME("Unhandled state %s, value 0x%lx\n", d3drs_to_str(state), value);
            if(state < mRenderState.size())
                mRenderState[state] = value;
            break;
    }

    return D3D_OK;
}

HRESULT D3DGLDevice::GetRenderState(D3DRENDERSTATETYPE state, DWORD *value)
{
    TRACE("iface %p, state %s, value %p\n", this, d3drs_to_str(state), value);

    if(state >= mRenderState.size())
    {
        WARN("State out of range (%u >= %u)\n", state, mRenderState.size());
        return D3DERR_INVALIDCALL;
    }

    *value = mRenderState[state];
    return D3D_OK;
}

HRESULT D3DGLDevice::CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::BeginStateBlock()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::EndStateBlock(IDirect3DStateBlock9** ppSB)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetClipStatus(D3DCLIPSTATUS9* pClipStatus)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetTexture(DWORD stage, IDirect3DBaseTexture9 **texture)
{
    TRACE("iface %p, stage %lu, texture %p\n", this, stage, texture);

    if(stage >= D3DVERTEXTEXTURESAMPLER0 && stage <= D3DVERTEXTEXTURESAMPLER3)
        stage = stage - D3DVERTEXTEXTURESAMPLER0 + MAX_FRAGMENT_SAMPLERS;

    if(stage >= mTextures.size())
    {
        WARN("Texture stage out of range (%lu >= %u)\n", stage, mTextures.size());
        return D3DERR_INVALIDCALL;
    }

    *texture = mTextures[stage];
    if(*texture) (*texture)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::SetTexture(DWORD stage, IDirect3DBaseTexture9 *texture)
{
    TRACE("iface %p, stage %lu, texture %p\n", this, stage, texture);

    if(stage >= D3DVERTEXTEXTURESAMPLER0 && stage <= D3DVERTEXTEXTURESAMPLER3)
        stage = stage - D3DVERTEXTEXTURESAMPLER0 + MAX_FRAGMENT_SAMPLERS;

    if(stage >= mTextures.size())
    {
        WARN("Texture stage out of range (%lu >= %u)\n", stage, mTextures.size());
        return D3DERR_INVALIDCALL;
    }

    if(!texture)
    {
        mQueue.lock();
        texture = mTextures[stage].exchange(texture);
        mQueue.sendAndUnlock<SetTextureCmd>(this, stage, GL_NONE, 0);
        if(texture) texture->Release();
    }
    else
    {
        GLenum type = GL_NONE;
        GLuint binding = 0;
        union {
            void *pointer;
            D3DGLTexture *tex2d;
            D3DGLCubeTexture *cubetex;
        };
        if(SUCCEEDED(texture->QueryInterface(IID_D3DGLTexture, &pointer)))
        {
            type = GL_TEXTURE_2D;
            binding = tex2d->getTextureId();
        }
        else if(SUCCEEDED(texture->QueryInterface(IID_D3DGLCubeTexture, &pointer)))
        {
            type = GL_TEXTURE_CUBE_MAP;
            binding = cubetex->getTextureId();
        }
        else
        {
            ERR("Unhandled texture type from iface %p\n", texture);
            texture->AddRef();
        }
        mQueue.lock();
        // Texture being set already has an added reference
        texture = mTextures[stage].exchange(texture);
        mQueue.sendAndUnlock<SetTextureCmd>(this, stage, type, binding);
        if(texture) texture->Release();
    }

    return D3D_OK;
}

HRESULT D3DGLDevice::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD *value)
{
    TRACE("iface %p, stage %lu, type %s, value %p\n", this, stage, d3dtss_to_str(type), value);

    if(stage >= mTexStageState.size() || stage >= mAdapter.getLimits().fragment_samplers)
    {
        WARN("Texture stage out of range (%lu >= %u)\n", stage, std::min(mTexStageState.size(), mAdapter.getLimits().fragment_samplers));
        return D3DERR_INVALIDCALL;
    }
    if(type >= mTexStageState[stage].size())
    {
        WARN("Type out of range (%u >= %u)\n", type, mTexStageState[stage].size());
        return D3DERR_INVALIDCALL;
    }

    *value = mTexStageState[stage][type];
    return D3D_OK;
}

HRESULT D3DGLDevice::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
    FIXME("iface %p, stage %lu, type %s, value 0x%lx : stub!\n", this, stage, d3dtss_to_str(type), value);

    if(stage >= mTexStageState.size() || stage >= mAdapter.getLimits().fragment_samplers)
    {
        WARN("Texture stage out of range (%lu >= %u)\n", stage, std::min(mTexStageState.size(), mAdapter.getLimits().fragment_samplers));
        return D3DERR_INVALIDCALL;
    }

    if(type < mTexStageState[stage].size())
        mTexStageState[stage][type] = value;
    return D3D_OK;
}

HRESULT D3DGLDevice::GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD *value)
{
    TRACE("iface %p, sampler %lu, type %s, value %p\n", this, sampler, d3dsamp_to_str(type), value);

    if(sampler >= D3DVERTEXTEXTURESAMPLER0 && sampler <= D3DVERTEXTEXTURESAMPLER3)
        sampler = sampler - D3DVERTEXTEXTURESAMPLER0 + MAX_FRAGMENT_SAMPLERS;

    if(sampler >= mSamplerState.size())
    {
        WARN("Sampler index out of range (%lu >= %u)\n", sampler, mSamplerState.size());
        return D3DERR_INVALIDCALL;
    }
    if(type >= mSamplerState[sampler].size())
    {
        WARN("Type out of range (%u >= %u)\n", type, mSamplerState[sampler].size());
        return D3DERR_INVALIDCALL;
    }

    *value = mSamplerState[sampler][type];
    return D3D_OK;
}

HRESULT D3DGLDevice::SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value)
{
    TRACE("iface %p, sampler %lu, type %s, value 0x%lx\n", this, sampler, d3dsamp_to_str(type), value);

    if(sampler >= D3DVERTEXTEXTURESAMPLER0 && sampler <= D3DVERTEXTEXTURESAMPLER3)
        sampler = sampler - D3DVERTEXTEXTURESAMPLER0 + MAX_FRAGMENT_SAMPLERS;

    if(sampler >= mSamplerState.size())
    {
        WARN("Sampler index out of range (%lu >= %u)\n", sampler, mSamplerState.size());
        return D3DERR_INVALIDCALL;
    }
    if(type >= mSamplerState[sampler].size())
    {
        WARN("Sampler type out of range (%u >= %u)\n", type, mSamplerState[sampler].size());
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    DWORD oldvalue = mSamplerState[sampler][type].exchange(value);
    switch(type)
    {
        case D3DSAMP_ADDRESSU:
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_WRAP_S, GetGLWrapMode(value)
            );
            break;
        case D3DSAMP_ADDRESSV:
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_WRAP_T, GetGLWrapMode(value)
            );
            break;
        case D3DSAMP_ADDRESSW:
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_WRAP_R, GetGLWrapMode(value)
            );
            break;
        case D3DSAMP_BORDERCOLOR:
            mQueue.sendAndUnlock<SetSamplerParameter4f>(mGLState.samplers[sampler],
                GL_TEXTURE_BORDER_COLOR, D3DCOLOR_R(value)/255.0f, D3DCOLOR_G(value)/255.0f,
                D3DCOLOR_B(value)/255.0f, D3DCOLOR_A(value)/255.0f
            );
            break;
        case D3DSAMP_MAGFILTER:
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_MAG_FILTER, GetGLFilterMode(value, D3DTEXF_NONE)
            );
            break;
        case D3DSAMP_MINFILTER:
            if((oldvalue == D3DTEXF_ANISOTROPIC) != (value == D3DTEXF_ANISOTROPIC))
                mQueue.doSend<SetSamplerParameteri>(mGLState.samplers[sampler],
                    GL_TEXTURE_MAX_ANISOTROPY_EXT, (value == D3DTEXF_ANISOTROPIC) ?
                                                   mSamplerState[sampler][D3DSAMP_MAXANISOTROPY].load() :
                                                   1ul
                );
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_MIN_FILTER, GetGLFilterMode(value,
                    mSamplerState[sampler][D3DSAMP_MIPFILTER]
                )
            );
            break;
        case D3DSAMP_MIPFILTER:
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_MIN_FILTER, GetGLFilterMode(
                    mSamplerState[sampler][D3DSAMP_MINFILTER], value
                )
            );
            break;
        case D3DSAMP_MIPMAPLODBIAS:
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_LOD_BIAS, value
            );
            break;
        case D3DSAMP_MAXMIPLEVEL:
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_MAX_LOD, value
            );
            break;
        case D3DSAMP_MAXANISOTROPY:
            if(mSamplerState[sampler][D3DSAMP_MIPFILTER] == D3DTEXF_ANISOTROPIC)
                mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                    GL_TEXTURE_MAX_ANISOTROPY_EXT, value
                );
            else
                mQueue.unlock();
            break;
        case D3DSAMP_SRGBTEXTURE:
            mQueue.sendAndUnlock<SetSamplerParameteri>(mGLState.samplers[sampler],
                GL_TEXTURE_SRGB_DECODE_EXT, value ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT
            );
            break;
        case D3DSAMP_ELEMENTINDEX:
        case D3DSAMP_DMAPOFFSET:
        default:
            FIXME("Unhandled sampler state: %s\n", d3dsamp_to_str(type));
            mQueue.unlock();
            break;
    }

    return D3D_OK;
}

HRESULT D3DGLDevice::ValidateDevice(DWORD* pNumPasses)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetCurrentTexturePalette(UINT PaletteNumber)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetCurrentTexturePalette(UINT *PaletteNumber)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetScissorRect(const RECT *rect)
{
    TRACE("iface %p, rect %p\n", this, rect);

    if(rect->right < rect->left || rect->bottom < rect->top)
    {
        FIXME("Scissor rectangle has inverted region: %ld,%ld x %ld,%ld\n", rect->left, rect->top, rect->right, rect->bottom);
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    mScissorRect = *rect;
    mQueue.sendAndUnlock<ScissorRectSet>(*rect);

    return D3D_OK;
}

HRESULT D3DGLDevice::GetScissorRect(RECT *rect)
{
    TRACE("iface %p, rect %p\n", this, rect);

    mQueue.lock();
    *rect = mScissorRect;
    mQueue.unlock();

    return D3D_OK;
}

HRESULT D3DGLDevice::SetSoftwareVertexProcessing(WINBOOL bSoftware)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

WINBOOL D3DGLDevice::GetSoftwareVertexProcessing()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetNPatchMode(float nSegments)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

float D3DGLDevice::GetNPatchMode()
{
    FIXME("iface %p : stub!\n", this);
    return 0.0f;
}

HRESULT D3DGLDevice::DrawPrimitive(D3DPRIMITIVETYPE type, UINT startVtx, UINT count)
{
    TRACE("iface %p, type 0x%x, startVtx %u, count %u\n", this, type, startVtx, count);

    GLenum mode = GetGLDrawMode(type, count);
    return drawVtxDecl(mode, startVtx, 0, 0, count, false, false);
}

HRESULT D3DGLDevice::DrawIndexedPrimitive(D3DPRIMITIVETYPE type, INT startVtx, UINT minVtx, UINT numVtx, UINT startIdx, UINT count)
{
    TRACE("iface %p, type 0x%x, startVtx %d, minVtx %u, numVtx %u, startIdx %u, count %u\n", this, type, startVtx, minVtx, numVtx, startIdx, count);

    if(type == D3DPT_POINTLIST)
    {
        WARN("Pointlist not allowed for indexed rendering\n");
        return D3DERR_INVALIDCALL;
    }

    GLenum mode = GetGLDrawMode(type, count);
    return drawVtxDecl(mode, startVtx, minVtx, startIdx, count, true, false);
}

HRESULT D3DGLDevice::DrawPrimitiveUP(D3DPRIMITIVETYPE type, UINT count, const void *vtxData, UINT vtxStride)
{
    TRACE("iface %p, type 0x%x, count %u, vtxData %p, vtxStride %u\n", this, type, count, vtxData, vtxStride);

    GLenum mode = GetGLDrawMode(type, count);

    if(!mPrimitiveUserData)
    {
        mPrimitiveUserData = new D3DGLBufferObject(this);
        if(!mPrimitiveUserData->init_vbo(vtxStride*count, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT))
        {
            ERR("Failed to initialize vertex data storage\n");
            return D3DERR_INVALIDCALL;
        }
    }
    mPrimitiveUserData->resetBufferData(reinterpret_cast<const GLubyte*>(vtxData),
                                        vtxStride*count);

    if(mStreams[0].mBuffer)
        mStreams[0].mBuffer->Release();
    mStreams[0].mBuffer = mPrimitiveUserData;
    mStreams[0].mOffset = 0;
    mStreams[0].mStride = vtxStride;

    HRESULT hr = drawVtxDecl(mode, 0, 0, 0, count, false, true);

    mStreams[0].mBuffer = nullptr;
    mStreams[0].mStride = 0;

    return hr;
}

HRESULT D3DGLDevice::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::CreateVertexDeclaration(const D3DVERTEXELEMENT9 *elems, IDirect3DVertexDeclaration9 **decl)
{
    TRACE("iface %p, elems %p, decl %p\n", this, elems, decl);

    D3DGLVertexDeclaration *vtxdecl = new D3DGLVertexDeclaration(this);
    if(!vtxdecl->init(elems))
    {
        delete vtxdecl;
        return D3DERR_INVALIDCALL;
    }

    *decl = vtxdecl;
    (*decl)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::SetVertexDeclaration(IDirect3DVertexDeclaration9 *decl)
{
    TRACE("iface %p, decl %p\n", this, decl);

    D3DGLVertexDeclaration *vtxdecl = nullptr;
    if(decl)
    {
        HRESULT hr;
        hr = decl->QueryInterface(IID_D3DGLVertexDeclaration, (void**)&vtxdecl);
        if(FAILED(hr)) return D3DERR_INVALIDCALL;
        vtxdecl->addIface();
        vtxdecl->Release();
    }

    mQueue.lock();
    if(!vtxdecl)
    {
        vtxdecl = mVertexDecl.exchange(vtxdecl);
        // FIXME: Set according to FVF
        mQueue.unlock();
    }
    else if(D3DGLVertexShader *vshader = mVertexShader)
    {
        UINT attribs = 0;
        for(const D3DGLVERTEXELEMENT &elem : vtxdecl->getVtxElements())
        {
            GLint loc = vshader->getLocation(elem.Usage, elem.UsageIndex);
            if(loc >= 0) attribs |= 1<<loc;
        }
        vtxdecl = mVertexDecl.exchange(vtxdecl);
        mQueue.sendAndUnlock<SetVertexAttribArrayCmd>(this, attribs);
    }
    else
    {
        bool pos = vtxdecl->hasPos() || vtxdecl->hasPosT();
        bool normal = vtxdecl->hasNormal();
        bool color = vtxdecl->hasColor();
        bool specular = vtxdecl->hasSpecular();
        UINT texcoord = vtxdecl->hasTexCoord();
        vtxdecl = mVertexDecl.exchange(vtxdecl);
        mQueue.sendAndUnlock<SetVertexArrayStateCmd>(this, pos, normal, color, specular, texcoord);
    }
    if(vtxdecl) vtxdecl->releaseIface();

    return D3D_OK;
}

HRESULT D3DGLDevice::GetVertexDeclaration(IDirect3DVertexDeclaration9 **decl)
{
    TRACE("iface %p, decl %p\n", this, decl);

    *decl = mVertexDecl.load();
    if(*decl) (*decl)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::SetFVF(DWORD FVF)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetFVF(DWORD* pFVF)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::CreateVertexShader(const DWORD *function, IDirect3DVertexShader9 **shader)
{
    TRACE("iface %p, function %p, shader %p\n", this, function, shader);

    D3DGLVertexShader *vtxshader = new D3DGLVertexShader(this);
    if(!vtxshader->init(function))
    {
        delete vtxshader;
        return D3DERR_INVALIDCALL;
    }

    *shader = vtxshader;
    (*shader)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::SetVertexShader(IDirect3DVertexShader9 *shader)
{
    TRACE("iface %p, shader %p\n", this, shader);

    D3DGLVertexShader *vshader = nullptr;
    if(shader)
    {
        HRESULT hr;
        hr = shader->QueryInterface(IID_D3DGLVertexShader, (void**)&vshader);
        if(FAILED(hr)) return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    // FIXME: Forcing attributes to specific locations given its usage:index
    // setup means we won't have to reset attribute array state (though we
    // still do when enabling/disabling the FFP).
    D3DGLVertexShader *oldshader = mVertexShader.exchange(vshader);
    if(vshader)
    {
        if(!oldshader)
        {
            // Need to disable fixed-function client state
            mQueue.doSend<SetVertexArrayStateCmd>(this, false, false, false, false, 0);
        }

        UINT attribs = 0;
        if(D3DGLVertexDeclaration *vtxdecl = mVertexDecl)
        {
            for(const D3DGLVERTEXELEMENT &elem : vtxdecl->getVtxElements())
            {
                GLint loc = vshader->getLocation(elem.Usage, elem.UsageIndex);
                if(loc >= 0) attribs |= 1<<loc;
            }
        }
        mQueue.doSend<SetVertexAttribArrayCmd>(this, attribs);
        // TODO: The old shader's local constants should be reloaded with the
        // appropriate global values, and the new shader's local constants
        // should be filled with what the shader defined.
        mQueue.sendAndUnlock<SetShaderProgramCmd>(this, GL_VERTEX_SHADER_BIT, vshader->getProgram());
    }
    else if(oldshader)
    {
        mQueue.doSend<SetVertexAttribArrayCmd>(this, 0);
        if(D3DGLVertexDeclaration *vtxdecl = mVertexDecl)
        {
            bool pos = vtxdecl->hasPos() || vtxdecl->hasPosT();
            bool normal = vtxdecl->hasNormal();
            bool color = vtxdecl->hasColor();
            bool specular = vtxdecl->hasSpecular();
            UINT texcoord = vtxdecl->hasTexCoord();
            vtxdecl = mVertexDecl.exchange(vtxdecl);
            mQueue.doSend<SetVertexArrayStateCmd>(this, pos, normal, color, specular, texcoord);
        }
        mQueue.sendAndUnlock<SetShaderProgramCmd>(this, GL_VERTEX_SHADER_BIT, 0u);
    }
    else
        mQueue.unlock();
    if(oldshader) oldshader->Release();

    return D3D_OK;
}

HRESULT D3DGLDevice::GetVertexShader(IDirect3DVertexShader9 **shader)
{
    TRACE("iface %p, shader %p\n", this, shader);

    *shader = mVertexShader.load();
    if(*shader) (*shader)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::SetVertexShaderConstantF(UINT start, const float *values, UINT count)
{
    TRACE("iface %p, start %u, values %p, count %u\n", this, start, values, count);

    if(start >= mVSConstantsF.size() || count > mVSConstantsF.size()-start)
    {
        WARN("Invalid constants range (%u + %u > %u)\n", start, count, mVSConstantsF.size());
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    memcpy(mVSConstantsF[start].ptr(), values, count*sizeof(Vector4f));
    {
    set_more:
        if(count == 1)
            mQueue.sendAndUnlock<SetBufferValues<1>>(mGLState.vs_uniform_bufferf, values, start, 1);
        else if(count <= 4)
            mQueue.sendAndUnlock<SetBufferValues<4>>(mGLState.vs_uniform_bufferf, values, start, count);
        else if(count <= 32)
            mQueue.sendAndUnlock<SetBufferValues<32>>(mGLState.vs_uniform_bufferf, values, start, count);
        else if(count <= 64)
            mQueue.sendAndUnlock<SetBufferValues<64>>(mGLState.vs_uniform_bufferf, values, start, count);
        else if(count <= 128)
            mQueue.sendAndUnlock<SetBufferValues<128>>(mGLState.vs_uniform_bufferf, values, start, count);
        else
        {
            mQueue.doSend<SetBufferValues<128>>(mGLState.vs_uniform_bufferf, values, start, 128);
            values += 128*4;
            start += 128;
            count -= 128;
            goto set_more;
        }
    }

    return D3D_OK;
}

HRESULT D3DGLDevice::GetVertexShaderConstantF(UINT start, float *values, UINT count)
{
    TRACE("iface %p, start %u, values %p, count %u\n", this, start, values, count);

    if(start >= mVSConstantsF.size() || count > mVSConstantsF.size()-start)
    {
        WARN("Invalid constants range (%u + %u > %u)\n", start, count, mVSConstantsF.size());
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    memcpy(values, mVSConstantsF[start].ptr(), count*sizeof(Vector4f));
    mQueue.unlock();

    return D3D_OK;
}

HRESULT D3DGLDevice::SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetVertexShaderConstantB(UINT StartRegister, CONST WINBOOL* pConstantData, UINT  BoolCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetVertexShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetStreamSource(UINT index, IDirect3DVertexBuffer9 *stream, UINT offset, UINT stride)
{
    TRACE("iface %p, index %u, stream %p, offset %u, stride %u\n", this, index, stream, offset, stride);

    if(index >= mStreams.size())
    {
        WARN("Stream index out of range (%u >= %u)\n", index, mStreams.size());
        return D3DERR_INVALIDCALL;
    }

    D3DGLBufferObject *buffer = nullptr;
    if(stream)
    {
        HRESULT hr;
        hr = stream->QueryInterface(IID_D3DGLBufferObject, (void**)&buffer);
        if(FAILED(hr)) return D3DERR_INVALIDCALL;
    }

    if(mStreams[index].mBuffer)
        mStreams[index].mBuffer->Release();
    mStreams[index].mBuffer = buffer;
    mStreams[index].mOffset = offset;
    mStreams[index].mStride = stride;

    return D3D_OK;
}

HRESULT D3DGLDevice::GetStreamSource(UINT index, IDirect3DVertexBuffer9 **stream, UINT *offset, UINT *stride)
{
    TRACE("iface %p, index %u, stream %p, offset %p, stride %p\n", this, index, stream, offset, stride);

    if(index >= mStreams.size())
    {
        WARN("Stream index out of range (%u >= %u)\n", index, mStreams.size());
        return D3DERR_INVALIDCALL;
    }

    *stream = mStreams[index].mBuffer;
    if(*stream) (*stream)->AddRef();
    *offset = mStreams[index].mOffset;
    *stride = mStreams[index].mStride;

    return D3D_OK;
}

HRESULT D3DGLDevice::SetStreamSourceFreq(UINT index, UINT divisor)
{
    TRACE("iface %p, index %u, divisor 0x%x\n", this, index, divisor);

    if(index >= mStreams.size())
    {
        WARN("Stream index out of range (%u >= %u)\n", index, mStreams.size());
        return D3DERR_INVALIDCALL;
    }

    if((divisor&D3DSTREAMSOURCE_INDEXEDDATA) && (divisor&D3DSTREAMSOURCE_INSTANCEDATA))
    {
        WARN("Indexed and instance data both specified\n");
        return D3DERR_INVALIDCALL;
    }

    if(index == 0 && (divisor&D3DSTREAMSOURCE_INSTANCEDATA))
    {
        WARN("Instance data not allowed on stream 0\n");
        return D3DERR_INVALIDCALL;
    }

    if(!(divisor&(D3DSTREAMSOURCE_INDEXEDDATA|D3DSTREAMSOURCE_INSTANCEDATA)) && divisor != 1)
        FIXME("Unexpected divisor value: 0x%x\n", divisor);

    mStreams[index].mFreq = divisor;
    return D3D_OK;
}

HRESULT D3DGLDevice::GetStreamSourceFreq(UINT index, UINT *divisor)
{
    TRACE("iface %p, index %u, divisor %p\n", this, index, divisor);

    if(index >= mStreams.size())
    {
        WARN("Stream index out of range (%u >= %u)\n", index, mStreams.size());
        return D3DERR_INVALIDCALL;
    }

    *divisor = mStreams[index].mFreq;
    return D3D_OK;
}

HRESULT D3DGLDevice::SetIndices(IDirect3DIndexBuffer9 *index)
{
    TRACE("iface %p, index %p\n", this, index);

    D3DGLBufferObject *buffer = nullptr;
    if(index)
    {
        HRESULT hr;
        hr = index->QueryInterface(IID_D3DGLBufferObject, (void**)&buffer);
        if(FAILED(hr)) return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    D3DGLBufferObject *oldbuffer = mIndexBuffer.exchange(buffer);
    mQueue.sendAndUnlock<ElementArraySet>(buffer ? buffer->getBufferId() : 0);
    if(oldbuffer) oldbuffer->Release();

    return D3D_OK;
}

HRESULT D3DGLDevice::GetIndices(IDirect3DIndexBuffer9 **index)
{
    TRACE("iface %p, index %p\n", this, index);

    *index = mIndexBuffer.load();
    if(*index) (*index)->AddRef();

    return D3D_OK;
}

HRESULT D3DGLDevice::CreatePixelShader(const DWORD *function, IDirect3DPixelShader9 **shader)
{
    TRACE("iface %p, function %p, shader %p\n", this, function, shader);

    D3DGLPixelShader *fragshader = new D3DGLPixelShader(this);
    if(!fragshader->init(function))
    {
        delete fragshader;
        return D3DERR_INVALIDCALL;
    }

    *shader = fragshader;
    (*shader)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::SetPixelShader(IDirect3DPixelShader9 *shader)
{
    TRACE("iface %p, shader %p\n", this, shader);

    D3DGLPixelShader *pshader = nullptr;
    if(shader)
    {
        HRESULT hr;
        hr = shader->QueryInterface(IID_D3DGLPixelShader, (void**)&pshader);
        if(FAILED(hr)) return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    D3DGLPixelShader *oldshader = mPixelShader.exchange(pshader);
    if(pshader)
    {
        // TODO: The old shader's local constants should be reloaded with the
        // appropriate global values, and the new shader's local constants
        // should be filled with what the shader defined.
        mQueue.sendAndUnlock<SetShaderProgramCmd>(this, GL_FRAGMENT_SHADER_BIT, pshader->getProgram());
    }
    else if(oldshader)
        mQueue.sendAndUnlock<SetShaderProgramCmd>(this, GL_FRAGMENT_SHADER_BIT, 0u);
    else
        mQueue.unlock();
    if(oldshader) oldshader->Release();

    return D3D_OK;
}

HRESULT D3DGLDevice::GetPixelShader(IDirect3DPixelShader9 **shader)
{
    TRACE("iface %p, shader %p\n", this, shader);

    *shader = mPixelShader.load();
    if(*shader) (*shader)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLDevice::SetPixelShaderConstantF(UINT start, const float *values, UINT count)
{
    TRACE("iface %p, start %u, values %p, count %u\n", this, start, values, count);

    if(start >= mPSConstantsF.size() || count > mPSConstantsF.size()-start)
    {
        WARN("Invalid constants range (%u + %u > %u)\n", start, count, mPSConstantsF.size());
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    memcpy(mPSConstantsF[start].ptr(), values, count*sizeof(Vector4f));
    {
    set_more:
        if(count == 1)
            mQueue.sendAndUnlock<SetBufferValues<1>>(mGLState.ps_uniform_bufferf, values, start, 1);
        else if(count <= 4)
            mQueue.sendAndUnlock<SetBufferValues<4>>(mGLState.ps_uniform_bufferf, values, start, count);
        else if(count <= 32)
            mQueue.sendAndUnlock<SetBufferValues<32>>(mGLState.ps_uniform_bufferf, values, start, count);
        else if(count <= 64)
            mQueue.sendAndUnlock<SetBufferValues<64>>(mGLState.ps_uniform_bufferf, values, start, count);
        else if(count <= 128)
            mQueue.sendAndUnlock<SetBufferValues<128>>(mGLState.ps_uniform_bufferf, values, start, count);
        else
        {
            mQueue.doSend<SetBufferValues<128>>(mGLState.ps_uniform_bufferf, values, start, 128);
            values += 128*4;
            start += 128;
            count -= 128;
            goto set_more;
        }
    }

    return D3D_OK;
}

HRESULT D3DGLDevice::GetPixelShaderConstantF(UINT start, float *values, UINT count)
{
    TRACE("iface %p, start %u, values %p, count %u\n", this, start, values, count);

    if(start >= mPSConstantsF.size() || count > mPSConstantsF.size()-start)
    {
        WARN("Invalid constants range (%u + %u > %u)\n", start, count, mPSConstantsF.size());
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    memcpy(values, mPSConstantsF[start].ptr(), count*sizeof(Vector4f));
    mQueue.unlock();

    return D3D_OK;
}

HRESULT D3DGLDevice::SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::SetPixelShaderConstantB(UINT StartRegister, CONST WINBOOL* pConstantData, UINT  BoolCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetPixelShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::DeletePatch(UINT Handle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9 **query)
{
    TRACE("iface %p, type %s, query %p\n", this, d3dquery_to_str(type), query);

    D3DGLQuery *_query = new D3DGLQuery(this);
    if(!_query->init(type))
    {
        delete _query;
        return D3DERR_INVALIDCALL;
    }

    *query = _query;
    (*query)->AddRef();
    return D3D_OK;
}
