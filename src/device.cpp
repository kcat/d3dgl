
#include "device.hpp"

#include <array>
#include <d3d9.h>

#include "glew.h"
#include "wglew.h"
#include "trace.hpp"
#include "d3dgl.hpp"
#include "swapchain.hpp"
#include "rendertarget.hpp"
#include "adapter.hpp"
#include "texture.hpp"
#include "bufferobject.hpp"
#include "vertexshader.hpp"
#include "pixelshader.hpp"
#include "vertexdeclaration.hpp"
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
        case D3DFMT_D24S8:
            *inserter = {WGL_DEPTH_BITS_ARB, 24};
            *inserter = {WGL_STENCIL_BITS_ARB, 8};
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

GLenum GetGLStencilOp(D3DSTENCILOP op)
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
        case D3DSTENCILOP_FORCE_DWORD: break;
    }
    FIXME("Unhandled D3DSTENCILOP: 0x%x\n", op);
    return GL_KEEP;
}

GLenum GetGLCompFunc(D3DCMPFUNC func)
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
        case D3DCMP_FORCE_DWORD: break;
    }
    FIXME("Unhandled D3DCMPFUNC: 0x%x\n", func);
    return GL_ALWAYS;
}


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

#define BUFFER_VALUE_SEND_MAX (64u)
class SetBufferValues : public Command {
    GLuint mBuffer;
    GLintptr mStart;
    GLsizeiptr mSize;
    union {
        char mData[BUFFER_VALUE_SEND_MAX*4*4];
        float mVec4fs[BUFFER_VALUE_SEND_MAX][4];
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
// Specialization of the above, for the common simple case of updating one vec4f.
class SetBufferValue : public Command {
    GLuint mBuffer;
    GLintptr mStart;
    union {
        char mData[4*4];
        float mVec4f[4];
    };

public:
    SetBufferValue(GLuint buffer, const float *data, GLintptr start)
      : mBuffer(buffer)
    {
        mStart = start * 4 * 4;
        mVec4f[0] = *(data++);
        mVec4f[1] = *(data++);
        mVec4f[2] = *(data++);
        mVec4f[3] = *(data++);
    }

    virtual ULONG execute()
    {
        glNamedBufferSubDataEXT(mBuffer, mStart, 4*4, mData);
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


void D3DGLDevice::resetRenderTargetGL(GLsizei index)
{
    if(mGLState.active_framebuffer != 0)
    {
        mGLState.active_framebuffer = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glFrontFace(GL_CW);
    }
    glNamedFramebufferRenderbufferEXT(mGLState.main_framebuffer, GL_COLOR_ATTACHMENT0+index,
                                      GL_RENDERBUFFER, 0);
    checkGLError();
}
class ResetRenderTargetCmd : public Command {
    D3DGLDevice *mTarget;
    GLsizei mIndex;

public:
    ResetRenderTargetCmd(D3DGLDevice *target, GLsizei index) : mTarget(target), mIndex(index) { }

    virtual ULONG execute()
    {
        mTarget->resetRenderTargetGL(mIndex);
        return sizeof(*this);
    }
};

void D3DGLDevice::setRenderTargetGL(GLsizei index, GLuint id, GLint level, GLenum target)
{
    std::array<GLenum,D3D_MAX_SIMULTANEOUS_RENDERTARGETS> buffers{
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
    };

    if(mGLState.active_framebuffer != mGLState.main_framebuffer)
    {
        mGLState.active_framebuffer = mGLState.main_framebuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, mGLState.main_framebuffer);
        glDrawBuffers(std::min(buffers.size(), mAdapter.getLimits().buffers),
                      buffers.data());
        glFrontFace(GL_CCW);
    }

    if(target == GL_RENDERBUFFER)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, buffers[index], GL_RENDERBUFFER, id);
    else if(target == GL_TEXTURE_2D)
        glFramebufferTexture2D(GL_FRAMEBUFFER, buffers[index], target, id, level);
    checkGLError();
}
class SetRenderTargetCmd : public Command {
    D3DGLDevice *mTarget;
    GLsizei mIndex;
    GLuint mId;
    GLint mLevel;
    GLenum mRTarget;

public:
    SetRenderTargetCmd(D3DGLDevice *target, GLsizei index, GLuint id, GLint level, GLenum rtarget)
      : mTarget(target), mIndex(index), mId(id), mLevel(level), mRTarget(rtarget)
    { }

    virtual ULONG execute()
    {
        mTarget->setRenderTargetGL(mIndex, mId, mLevel, mRTarget);
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
                glDisableVertexAttribArray(i);
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


void D3DGLDevice::drawGL(const D3DGLDevice::GLIndexData& idxdata, const D3DGLDevice::GLStreamData* streams, GLuint numstreams, bool ffp)
{
    if(!ffp)
    {
        for(GLuint i = 0;i < numstreams;++i)
            glVertexAttribPointer(streams[i].mTarget, streams[i].mGLCount,
                                  streams[i].mGLType, streams[i].mNormalize,
                                  streams[i].mStride, streams[i].mPointer);
    }
    else for(GLuint i = 0;i < numstreams;++i)
    {
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

    glDrawElements(idxdata.mMode, idxdata.mCount, idxdata.mType, idxdata.mPointer);

    idxdata.mBuffer->finishedDraw();
    for(GLuint i = 0;i < numstreams;++i)
        streams[i].mBuffer->finishedDraw();

    checkGLError();
}
template<bool UseFFP>
class DrawGLCmd : public Command {
    D3DGLDevice *mTarget;
    D3DGLDevice::GLIndexData mIdxData;
    D3DGLDevice::GLStreamData mStreams[16];
    GLuint mNumStreams;

public:
    DrawGLCmd(D3DGLDevice *target, const D3DGLDevice::GLIndexData &idxdata, const D3DGLDevice::GLStreamData *streams, GLuint numstream)
      : mTarget(target), mIdxData(idxdata)
    {
        for(GLuint i = 0;i < numstream;++i)
            mStreams[i] = streams[i];
        mNumStreams = numstream;
    }

    virtual ULONG execute()
    {
        mTarget->drawGL(mIdxData, mStreams, mNumStreams, UseFFP);
        return sizeof(*this);
    }
};


void D3DGLDevice::initGL()
{
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
        float ident[4][4] = {
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        };
        glGenBuffers(1, &mGLState.proj_fixup_uniform_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, mGLState.proj_fixup_uniform_buffer);
        // Maybe STATIC_DRAW? Many things could cause this to change unnecessarily, though.
        glBufferData(GL_UNIFORM_BUFFER, 4*sizeof(Vector4f), ident, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, PROJECTION_BINDING_IDX, mGLState.proj_fixup_uniform_buffer);
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

    glFrontFace(GL_CW);
    checkGLError();
}
class InitGLDeviceCmd : public Command {
    D3DGLDevice *mTarget;

public:
    InitGLDeviceCmd(D3DGLDevice *target) : mTarget(target) { }
    virtual ULONG execute()
    {
        mTarget->initGL();
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
  , mBackbufferIsMain(true)
  , mDepthStencil(nullptr)
  , mInScene(false)
  , mVSConstantsF{0.0f}
  , mPSConstantsF{0.0f}
  , mVertexShader(nullptr)
  , mPixelShader(nullptr)
  , mVertexDecl(nullptr)
  , mIndexBuffer(nullptr)
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
}

D3DGLDevice::~D3DGLDevice()
{
    for(auto &schain : mSwapchains)
    {
        delete schain;
        schain = nullptr;
    }
    delete mAutoDepthStencil;
    mAutoDepthStencil = nullptr;

    mQueue.deinit();
    if(mGLContext)
        wglDeleteContext(mGLContext);
    mGLContext = nullptr;

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

    std::vector<std::array<int,2>> glattrs;
    glattrs.reserve(16);
    glattrs.push_back({WGL_DRAW_TO_WINDOW_ARB, GL_TRUE});
    glattrs.push_back({WGL_SUPPORT_OPENGL_ARB, GL_TRUE});
    glattrs.push_back({WGL_DOUBLE_BUFFER_ARB, GL_TRUE});
    glattrs.push_back({WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB});
    if(!fmt_to_glattrs(params->BackBufferFormat, std::back_inserter(glattrs)))
        return false;
    if(params->EnableAutoDepthStencil)
    {
        if(!(mAdapter.getUsage(D3DRTYPE_SURFACE, params->AutoDepthStencilFormat)&D3DUSAGE_DEPTHSTENCIL))
        {
            WARN("Format %s is not a valid depthstencil format\n", d3dfmt_to_str(params->AutoDepthStencilFormat));
            return false;
        }

        if(!fmt_to_glattrs(params->AutoDepthStencilFormat, std::back_inserter(glattrs)))
            return false;
    }
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

    mGLContext = wglCreateContextAttribsARB(hdc, nullptr, nullptr);
    if(!mGLContext)
    {
        ERR("Failed to create OpenGL context, error %lu\n", GetLastError());
        ReleaseDC(win, hdc);
        return false;
    }
    ReleaseDC(win, hdc);

    if(!mQueue.init(win, mGLContext))
        return false;

    D3DGLSwapChain *schain = new D3DGLSwapChain(this);
    if(!schain->init(params, win, true))
    {
        delete schain;
        return false;
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
            return false;
        mDepthStencil = mAutoDepthStencil;
    }

    mViewport.X = 0;
    mViewport.Y = 0;
    mViewport.Width = params->BackBufferWidth;
    mViewport.Height = params->BackBufferHeight;
    mViewport.MinZ = 0.0f;
    mViewport.MaxZ = 1.0f;


    mQueue.sendSync<InitGLDeviceCmd>(this);
    float result[4][4];
    calculateProjectionFixup(params->BackBufferWidth, params->BackBufferHeight, false, result);
    mQueue.send<SetBufferValues>(mGLState.proj_fixup_uniform_buffer, &result[0][0], 0, 4);

    return true;
}


HRESULT D3DGLDevice::drawVtxDecl(D3DPRIMITIVETYPE type, INT startvtx, UINT startidx, UINT count)
{
    GLenum mode;

    if(type == D3DPT_POINTLIST)
        mode = GL_POINTS;
    else if(type == D3DPT_LINELIST)
    {
        mode = GL_LINES;
        count *= 2;
    }
    else if(type == D3DPT_LINESTRIP)
    {
        mode = GL_LINE_STRIP;
        count += 1;
    }
    else if(type == D3DPT_TRIANGLELIST)
    {
        mode = GL_TRIANGLES;
        count *= 3;
    }
    else if(type == D3DPT_TRIANGLESTRIP)
    {
        mode = GL_TRIANGLE_STRIP;
        count += 2;
    }
    else if(type == D3DPT_TRIANGLEFAN)
    {
        mode = GL_TRIANGLE_FAN;
        count += 2;
    }
    else
    {
        WARN("Invalid primitive type: 0x%x\n", type);
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    if(!mVertexDecl)
    {
        WARN("No vertex declaration set\n");
        mQueue.unlock();
        return D3DERR_INVALIDCALL;
    }

    if(!mIndexBuffer)
    {
        WARN("No index buffer set\n");
        mQueue.unlock();
        return D3DERR_INVALIDCALL;
    }

    GLStreamData streams[16];
    GLuint cur = 0;

    D3DGLVertexShader *vshader = mVertexShader;
    const std::vector<D3DGLVERTEXELEMENT> &elements = mVertexDecl.load()->getVtxElements();
    for(const D3DGLVERTEXELEMENT &elem : elements)
    {
        if(cur >= 16)
        {
            ERR("Too many vertex elements!\n");
            mQueue.unlock();
            return D3DERR_INVALIDCALL;
        }

        const StreamSource &stream = mStreams[elem.Stream];
        D3DGLBufferObject *buffer = stream.mBuffer;

        GLint offset = elem.Offset + stream.mOffset + stream.mStride*startvtx;
        streams[cur].mBuffer = buffer;
        streams[cur].mPointer = buffer->getDataPtr() + offset;
        streams[cur].mGLCount = elem.mGLCount;
        streams[cur].mGLType = elem.mGLType;
        streams[cur].mNormalize = elem.mNormalize;
        streams[cur].mStride = stream.mStride;

        if(vshader)
        {
            streams[cur].mTarget = vshader->getLocation(elem.Usage, elem.UsageIndex);
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

    GLIndexData idxdata;
    idxdata.mBuffer = mIndexBuffer;
    idxdata.mMode = mode;
    idxdata.mCount = count;
    if(idxdata.mBuffer->getFormat() == D3DFMT_INDEX16)
    {
        idxdata.mType = GL_UNSIGNED_SHORT;
        idxdata.mPointer = idxdata.mBuffer->getDataPtr() + startidx*2;
    }
    else if(idxdata.mBuffer->getFormat() == D3DFMT_INDEX32)
    {
        idxdata.mType = GL_UNSIGNED_INT;
        idxdata.mPointer = idxdata.mBuffer->getDataPtr() + startidx*4;
    }
    else
    {
        // Should not happen!
        idxdata.mType = GL_UNSIGNED_BYTE;
        idxdata.mPointer = idxdata.mBuffer->getDataPtr();
    }

    idxdata.mBuffer->queueDraw();
    for(GLuint i = 0;i < cur;++i)
        streams[i].mBuffer->queueDraw();
    if(vshader)
        mQueue.sendAndUnlock<DrawGLCmd<UseShaders>>(this, idxdata, streams, cur);
    else
        mQueue.sendAndUnlock<DrawGLCmd<UseFFP>>(this, idxdata, streams, cur);

    return D3D_OK;
}

void D3DGLDevice::calculateProjectionFixup(UINT width, UINT height, bool flip, float result[4][4])
{
    // OpenGL places pixel coords at the pixel's bottom-left, while D3D places
    // it at the center. So we have to translate X and Y by /almost/ 0.5 pixels
    // (almost because of certain triangle edge rules GL is a bit lax on). The
    // offset is doubled since it ranges from -1...+1 instead of 0...1.
    //
    // Additionally, OpenGL uses -1...+1 for the Z clip space while D3D uses
    // 0...1, so Z needs to be scaled and offset as well.
    //
    // Finally, offscreen rendering needs to flip the entire scene upside down
    // so the 'top' of the image ends up at texture coord y=0, rather than
    // height-1.
    //
    // This is all accomplished with these two matrices. The matrices are
    // combined so that the scaling applies first, followed by the translation.
    float trans[4][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.99f/width, 0.99f/height, -1.0f, 1.0f }
    };
    float scale[4][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 2.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
    if(flip)
        scale[1][1] *= -1.0f;

    for(int r = 0;r < 4;++r)
    {
        for(int c = 0;c < 4;++c)
        {
            float sum = 0.0f;
            for(int i = 0;i < 4;++i)
                sum += scale[r][i]*trans[i][c];

            result[r][c] = sum;
        }
    }
}


HRESULT D3DGLDevice::QueryInterface(const IID &riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p.\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DDevice9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    if(riid == IID_IDirect3DDevice9Ex)
    {
        WARN("IDirect3D9 instance wasn't created with CreateDirect3D9Ex, returning E_NOINTERFACE.\n");
        *obj = NULL;
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
    FIXME("iface %p, params %p : stub!\n", this, params);

    FIXME("Resetting device with parameters:\n"
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

    // FIXME: When the window/backbuffer gets properly resized, do this:
    //float result[4][4];
    //calculateProjectionFixup(params->BackBufferWidth, params->BackBufferHeight, false, result);
    //mQueue.send<SetBufferValues>(mGLState.proj_fixup_uniform_buffer, &result[0][0], 0, 4);

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
    FIXME("iface %p, edgeLength %u, levels %u, usage 0x%lx, format %s, pool 0x%x, texture %p, handle %p : stub!\n", this, edgeLength, levels, usage, d3dfmt_to_str(format), pool, texture, handle);
    return E_NOTIMPL;
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

    D3DGLBufferObject *vbuf = new D3DGLBufferObject(this, D3DGLBufferObject::VBO);
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

    D3DGLBufferObject *ibuf = new D3DGLBufferObject(this, D3DGLBufferObject::IBO);
    if(!ibuf->init_ibo(length, usage, format, pool))
    {
        delete ibuf;
        return D3DERR_INVALIDCALL;
    }

    *ibuffer = ibuf;
    (*ibuffer)->AddRef();
    return D3D_OK;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
HRESULT D3DGLDevice::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

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

HRESULT D3DGLDevice::StretchRect(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
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
    FIXME("iface %p, index %lu, rendertarget %p : semi-stub\n", this, index, rtarget);

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
        if(mBackbufferIsMain)
            mQueue.sendAndUnlock<ResetRenderTargetCmd>(this, index);
        else
            mQueue.sendAndUnlock<SetRenderTargetCmd>(this, index, GL_RENDERBUFFER, 0, 0);
        if(rtarget) rtarget->Release();
        return D3D_OK;
    }

    union {
        void *pointer;
        D3DGLTexture *tex2d;
        D3DGLSwapChain *schain;
    };
    rtarget->AddRef();
    if(SUCCEEDED(rtarget->GetContainer(IID_D3DGLTexture, &pointer)))
    {
        GLint level = tex2d->getLevelFromSurface(rtarget);

        mQueue.lock();
        rtarget = mRenderTargets[index].exchange(rtarget);
        if(mBackbufferIsMain)
        {
            float result[4][4];
            calculateProjectionFixup(mViewport.Width, mViewport.Height, true, result);
            mQueue.doSend<SetBufferValues>(mGLState.proj_fixup_uniform_buffer, &result[0][0], 0, 4);
        }
        mBackbufferIsMain = false;
        mQueue.sendAndUnlock<SetRenderTargetCmd>(this,
            index, GL_TEXTURE_2D, tex2d->getTextureId(), level
        );
        tex2d->Release();
    }
    else if(SUCCEEDED(rtarget->GetContainer(IID_D3DGLSwapChain, &pointer)))
    {
        mQueue.lock();
        rtarget = mRenderTargets[index].exchange(rtarget);
        if(index != 0)
            mQueue.sendAndUnlock<SetRenderTargetCmd>(this, index, GL_RENDERBUFFER, 0, 0);
        else
        {
            if(!mBackbufferIsMain)
            {
                float result[4][4];
                calculateProjectionFixup(mViewport.Width, mViewport.Height, false, result);
                mQueue.doSend<SetBufferValues>(mGLState.proj_fixup_uniform_buffer, &result[0][0], 0, 4);
            }
            mBackbufferIsMain = true;
            mQueue.sendAndUnlock<ResetRenderTargetCmd>(this, index);
        }
        schain->Release();
    }
    else
    {
        ERR("Unknown surface %p\n", rtarget);
        rtarget = mRenderTargets[index].exchange(rtarget);
    }
    if(rtarget) rtarget->Release();

    return D3D_OK;
}

HRESULT D3DGLDevice::GetRenderTarget(DWORD index, IDirect3DSurface9 **rendertarget)
{
    FIXME("iface %p, index %lu, rendertarget %p\n", this, index, rendertarget);

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
    FIXME("iface %p, depthstencil %p : stub!\n", this, depthstencil);
    if(depthstencil) depthstencil->AddRef();
    depthstencil = mDepthStencil.exchange(depthstencil);
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

HRESULT D3DGLDevice::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
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

    HRESULT hr;
    D3DGLBackbufferSurface *surface;
    mQueue.lock();
    mViewport = *viewport;

    // For on-screen rendering the viewport needs to be realigned vertically,
    // since 0,0 is the window's lower-left in OpenGL. So it needs to set
    // Y = WindowHeight - (vp->Y + vp->Height).
    // The render target being a D3DGLBackbufferSurface indicates on-screen
    // remdering, which we can then immediately query for its size. If it's
    // not, it's an off-screen target.
    hr = mRenderTargets[0].load()->QueryInterface(IID_D3DGLBackbufferSurface, (void**)&surface);
    if(FAILED(hr))
    {
        float result[4][4];
        calculateProjectionFixup(mViewport.Width, mViewport.Height, true, result);
        mQueue.doSend<SetBufferValues>(mGLState.proj_fixup_uniform_buffer, &result[0][0], 0, 4);
        mQueue.sendAndUnlock<ViewportSet>(mViewport.X, mViewport.Y,
                                          std::min(mViewport.Width, 0x7ffffffful),
                                          std::min(mViewport.Height, 0x7ffffffful),
                                          mViewport.MinZ, mViewport.MaxZ);
    }
    else
    {
        D3DSURFACE_DESC desc;
        surface->GetDesc(&desc);

        float result[4][4];
        calculateProjectionFixup(mViewport.Width, mViewport.Height, false, result);
        mQueue.doSend<SetBufferValues>(mGLState.proj_fixup_uniform_buffer, &result[0][0], 0, 4);
        mQueue.sendAndUnlock<ViewportSet>(mViewport.X,
                                          desc.Height - (mViewport.Y+mViewport.Height),
                                          std::min(mViewport.Width, 0x7ffffffful),
                                          std::min(mViewport.Height, 0x7ffffffful),
                                          mViewport.MinZ, mViewport.MaxZ);
        surface->Release();
    }

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

HRESULT D3DGLDevice::SetClipPlane(DWORD Index, CONST float* pPlane)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetClipPlane(DWORD Index, float* pPlane)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
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
            mQueue.sendAndUnlock<ColorMaskSet>(mRenderState[D3DRS_COLORWRITEENABLE].load());
            break;

        case D3DRS_ZWRITEENABLE:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<DepthMaskSet>(mRenderState[D3DRS_ZWRITEENABLE].load());
            break;

        case D3DRS_ZFUNC:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<DepthFuncSet>(GetGLCompFunc((D3DCMPFUNC)mRenderState[D3DRS_ZFUNC].load()));
            break;

        case D3DRS_ALPHAFUNC:
        case D3DRS_ALPHAREF:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<AlphaFuncSet>(GetGLCompFunc((D3DCMPFUNC)mRenderState[D3DRS_ALPHAFUNC].load()),
                                               mRenderState[D3DRS_ALPHAREF] / 255.0f);
            break;

        case D3DRS_SRCBLEND:
        case D3DRS_DESTBLEND:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<BlendFuncSet>(GetGLBlendFunc(mRenderState[D3DRS_SRCBLEND]),
                                               GetGLBlendFunc(mRenderState[D3DRS_DESTBLEND]));
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
                    GetGLCompFunc((D3DCMPFUNC)mRenderState[D3DRS_STENCILFUNC].load()),
                    mRenderState[D3DRS_STENCILREF].load(), mRenderState[D3DRS_STENCILMASK].load()
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
                    GetGLStencilOp((D3DSTENCILOP)mRenderState[D3DRS_STENCILFAIL].load()),
                    GetGLStencilOp((D3DSTENCILOP)mRenderState[D3DRS_STENCILZFAIL].load()),
                    GetGLStencilOp((D3DSTENCILOP)mRenderState[D3DRS_STENCILPASS].load())
                );
            }
            break;

        // FIXME: These probably shouldn't set OpenGL state while
        // D3DRS_TWOSIDEDSTENCILMODE is false.
        case D3DRS_CCW_STENCILFUNC:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<StencilFuncSet>(GL_BACK,
                GetGLCompFunc((D3DCMPFUNC)mRenderState[D3DRS_CCW_STENCILFUNC].load()),
                mRenderState[D3DRS_STENCILREF].load(), mRenderState[D3DRS_STENCILMASK].load()
            );
            break;

        case D3DRS_CCW_STENCILFAIL:
        case D3DRS_CCW_STENCILZFAIL:
        case D3DRS_CCW_STENCILPASS:
            mQueue.lock();
            mRenderState[state] = value;
            mQueue.sendAndUnlock<StencilOpSet>(GL_BACK,
                GetGLStencilOp((D3DSTENCILOP)mRenderState[D3DRS_CCW_STENCILFAIL].load()),
                GetGLStencilOp((D3DSTENCILOP)mRenderState[D3DRS_CCW_STENCILZFAIL].load()),
                GetGLStencilOp((D3DSTENCILOP)mRenderState[D3DRS_CCW_STENCILPASS].load())
            );
            break;

        // FIXME: This should probably set the GL_BACK stencil func/ops from
        // CCW state when enabled, or set GL_FRONT_AND_BACK from CW state when
        // disabled.
        case D3DRS_TWOSIDEDSTENCILMODE:
            mRenderState[state] = value;
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
        HRESULT hr;
        GLenum type = GL_NONE;
        GLuint binding = 0;
        D3DGLTexture *tex2d;
        hr = texture->QueryInterface(IID_D3DGLTexture, (void**)&tex2d);
        if(SUCCEEDED(hr))
        {
            type = GL_TEXTURE_2D;
            binding = tex2d->getTextureId();
        }
        else
        {
            ERR("Unhandled texture type from iface %p\n", texture);
            if(texture) texture->AddRef();
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
    FIXME("iface %p, sampler %lu, type %s, value 0x%lx : stub!\n", this, sampler, d3dsamp_to_str(type), value);

    if(sampler >= D3DVERTEXTEXTURESAMPLER0 && sampler <= D3DVERTEXTEXTURESAMPLER3)
        sampler = sampler - D3DVERTEXTEXTURESAMPLER0 + MAX_FRAGMENT_SAMPLERS;

    if(sampler >= mSamplerState.size())
    {
        WARN("Sampler index out of range (%lu >= %u)\n", sampler, mSamplerState.size());
        return D3DERR_INVALIDCALL;
    }

    if(type < mSamplerState[sampler].size())
        mSamplerState[sampler][type] = value;

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

HRESULT D3DGLDevice::SetScissorRect(CONST RECT* pRect)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::GetScissorRect(RECT* pRect)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
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

HRESULT D3DGLDevice::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT D3DGLDevice::DrawIndexedPrimitive(D3DPRIMITIVETYPE type, INT startVtx, UINT minVtx, UINT numVtx, UINT startIdx, UINT count)
{
    TRACE("iface %p, type 0x%x, startVtx %d, minVtx %u, numVtx %u, startIdx %u, count %u\n", this, type, startVtx, minVtx, numVtx, startIdx, count);

    if(type == D3DPT_POINTLIST)
    {
        WARN("Pointlist not allowed for indexed rendering\n");
        return D3DERR_INVALIDCALL;
    }

    return drawVtxDecl(type, startVtx, startIdx, count);
}

HRESULT D3DGLDevice::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
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
    FIXME("iface %p, elems %p, decl %p\n", this, elems, decl);

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
        // NOTE: We'll always have vertices
        bool normal = vtxdecl->hasNormal();
        bool color = vtxdecl->hasColor();
        bool specular = vtxdecl->hasSpecular();
        UINT texcoord = vtxdecl->hasTexCoord();
        vtxdecl = mVertexDecl.exchange(vtxdecl);
        mQueue.sendAndUnlock<SetVertexArrayStateCmd>(this, true, normal, color, specular, texcoord);
    }
    if(vtxdecl) vtxdecl->Release();

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
    WARN("iface %p, shader %p : semi-stub\n", this, shader);

    D3DGLVertexShader *vshader = nullptr;
    if(shader)
    {
        HRESULT hr;
        hr = shader->QueryInterface(IID_D3DGLVertexShader, (void**)&vshader);
        if(FAILED(hr)) return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
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
        mQueue.sendAndUnlock<SetShaderProgramCmd>(this, GL_VERTEX_SHADER_BIT, vshader->getProgram());
    }
    else if(oldshader)
    {
        mQueue.doSend<SetVertexAttribArrayCmd>(this, 0);
        if(D3DGLVertexDeclaration *vtxdecl = mVertexDecl)
        {
            // NOTE: We'll always have vertices
            bool normal = vtxdecl->hasNormal();
            bool color = vtxdecl->hasColor();
            bool specular = vtxdecl->hasSpecular();
            UINT texcoord = vtxdecl->hasTexCoord();
            vtxdecl = mVertexDecl.exchange(vtxdecl);
            mQueue.doSend<SetVertexArrayStateCmd>(this, true, normal, color, specular, texcoord);
        }
        mQueue.sendAndUnlock<SetShaderProgramCmd>(this, GL_VERTEX_SHADER_BIT, 0u);
    }
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
    WARN("iface %p, start %u, values %p, count %u : semi-stub!\n", this, start, values, count);

    if(start >= mVSConstantsF.size() || count > mVSConstantsF.size()-start)
    {
        WARN("Invalid constants range (%u + %u > %u)\n", start, count, mVSConstantsF.size());
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    memcpy(mVSConstantsF[start].ptr(), values, count*sizeof(Vector4f));
    if(count == 1)
        mQueue.sendAndUnlock<SetBufferValue>(mGLState.vs_uniform_bufferf, values, start);
    else
    {
        mQueue.sendAndUnlock<SetBufferValues>(mGLState.vs_uniform_bufferf,
            values, start, std::min(count, BUFFER_VALUE_SEND_MAX)
        );
        UINT i = std::min(count, BUFFER_VALUE_SEND_MAX);
        while(i < count)
        {
            UINT todo = std::min(count-i, BUFFER_VALUE_SEND_MAX);
            mQueue.send<SetBufferValues>(mGLState.vs_uniform_bufferf,
                &values[i*4], (start+i), todo
            );
            i += todo;
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
    TRACE("iface %p, index %u, divisor %u\n", this, index, divisor);

    if(index >= mStreams.size())
    {
        WARN("Stream index out of range (%u >= %u)\n", index, mStreams.size());
        return D3DERR_INVALIDCALL;
    }

    if(index == 0 && (divisor&D3DSTREAMSOURCE_INSTANCEDATA))
    {
        WARN("Instance data not allowed on stream 0\n");
        return D3DERR_INVALIDCALL;
    }

    if(!(divisor&D3DSTREAMSOURCE_INDEXEDDATA) && divisor != 1)
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

    buffer = mIndexBuffer.exchange(buffer);
    if(buffer) buffer->Release();

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
    WARN("iface %p, shader %p : semi-stub\n", this, shader);

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
        mQueue.sendAndUnlock<SetShaderProgramCmd>(this, GL_FRAGMENT_SHADER_BIT, pshader->getProgram());
    else if(oldshader)
        mQueue.sendAndUnlock<SetShaderProgramCmd>(this, GL_FRAGMENT_SHADER_BIT, 0u);
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
    WARN("iface %p, start %u, values %p, count %u : semi-stub!\n", this, start, values, count);

    if(start >= mPSConstantsF.size() || count > mPSConstantsF.size()-start)
    {
        WARN("Invalid constants range (%u + %u > %u)\n", start, count, mPSConstantsF.size());
        return D3DERR_INVALIDCALL;
    }

    mQueue.lock();
    memcpy(mPSConstantsF[start].ptr(), values, count*sizeof(Vector4f));
    if(count == 1)
        mQueue.sendAndUnlock<SetBufferValue>(mGLState.ps_uniform_bufferf, values, start);
    else
    {
        mQueue.sendAndUnlock<SetBufferValues>(mGLState.ps_uniform_bufferf,
            values, start, std::min(count, BUFFER_VALUE_SEND_MAX)
        );
        UINT i = std::min(count, BUFFER_VALUE_SEND_MAX);
        while(i < count)
        {
            UINT todo = std::min(count-i, BUFFER_VALUE_SEND_MAX);
            mQueue.send<SetBufferValues>(mGLState.ps_uniform_bufferf,
                &values[i*4], (start+i), todo
            );
            i += todo;
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
    FIXME("iface %p, type %s, query %p : stub!\n", this, d3dquery_to_str(type), query);
    return E_NOTIMPL;
}
