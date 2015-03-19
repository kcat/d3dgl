
#include "d3dgl.hpp"

#include "glew.h"
#include "trace.hpp"


#define WARN_AND_RETURN(val, ...) do { \
    WARN(__VA_ARGS__);                 \
    return val;                        \
} while(0)


#define MAX_TEXTURES                8
#define MAX_STREAMS                 16
#define MAX_VERTEX_SAMPLERS         4
#define MAX_FRAGMENT_SAMPLERS       16
#define MAX_COMBINED_SAMPLERS       (MAX_FRAGMENT_SAMPLERS + MAX_VERTEX_SAMPLERS)

#define D3DGL_MAX_CBS 15


class D3DAdapter {
    UINT mOrdinal;
    bool mInited;

    D3DCAPS9 mCaps;

    struct {
        UINT buffers;
        UINT lights;
        UINT textures;
        UINT texture_coords;
        UINT vertex_uniform_blocks;
        UINT geometry_uniform_blocks;
        UINT fragment_uniform_blocks;
        UINT fragment_samplers;
        UINT vertex_samplers;
        UINT combined_samplers;
        UINT general_combiners;
        UINT clipplanes;
        UINT texture_size;
        UINT texture3d_size;
        float pointsize_max;
        float pointsize_min;
        UINT blends;
        UINT anisotropy;
        float shininess;
        UINT samples;
        UINT vertex_attribs;

        UINT glsl_varyings;
        UINT glsl_vs_float_constants;
        UINT glsl_ps_float_constants;

        UINT arb_vs_float_constants;
        UINT arb_vs_native_constants;
        UINT arb_vs_instructions;
        UINT arb_vs_temps;
        UINT arb_ps_float_constants;
        UINT arb_ps_local_constants;
        UINT arb_ps_native_constants;
        UINT arb_ps_instructions;
        UINT arb_ps_temps;
    } mLimits;
    void init_limits();

    void init_caps();

    bool init();

public:
    D3DAdapter(UINT num);

    bool getCaps(D3DCAPS9 *caps);
};

D3DAdapter::D3DAdapter(UINT num)
  : mOrdinal(num)
  , mInited(false)
{
    memset(&mCaps, 0, sizeof(mCaps));
}


void D3DAdapter::init_limits()
{
    GLfloat gl_floatv[2];
    GLint gl_max;

    mLimits.blends = 1;
    mLimits.buffers = 1;
    mLimits.textures = 1;
    mLimits.texture_coords = 1;
    mLimits.vertex_uniform_blocks = 0;
    mLimits.geometry_uniform_blocks = 0;
    mLimits.fragment_uniform_blocks = 0;
    mLimits.fragment_samplers = 1;
    mLimits.vertex_samplers = 0;
    mLimits.combined_samplers = mLimits.fragment_samplers + mLimits.vertex_samplers;
    mLimits.vertex_attribs = 16;
    mLimits.glsl_vs_float_constants = 0;
    mLimits.glsl_ps_float_constants = 0;
    mLimits.arb_vs_float_constants = 0;
    mLimits.arb_vs_native_constants = 0;
    mLimits.arb_vs_instructions = 0;
    mLimits.arb_vs_temps = 0;
    mLimits.arb_ps_float_constants = 0;
    mLimits.arb_ps_local_constants = 0;
    mLimits.arb_ps_instructions = 0;
    mLimits.arb_ps_temps = 0;

    glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
    mLimits.clipplanes = std::min(D3DMAXUSERCLIPPLANES, gl_max);
    TRACE("Clip plane support - max planes %d.\n", gl_max);

    glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    mLimits.lights = gl_max;
    TRACE("Light support - max lights %d.\n", gl_max);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max);
    mLimits.texture_size = gl_max;
    TRACE("Maximum texture size support - max texture size %d.\n", gl_max);

    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, gl_floatv);
    mLimits.pointsize_min = gl_floatv[0];
    mLimits.pointsize_max = gl_floatv[1];
    TRACE("Maximum point size support - max point size %f.\n", gl_floatv[1]);

    if(GLEW_ARB_map_buffer_alignment)
    {
        glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &gl_max);
        TRACE("Minimum buffer map alignment: %d.\n", gl_max);
    }
    else
    {
        WARN("Driver doesn't guarantee a minimum buffer map alignment.\n");
    }
    if(GLEW_NV_register_combiners)
    {
        glGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &gl_max);
        mLimits.general_combiners = gl_max;
        TRACE("Max general combiners: %d.\n", gl_max);
    }

    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &gl_max);
    mLimits.buffers = gl_max;
    TRACE("Max draw buffers: %u.\n", gl_max);

    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &gl_max);
    mLimits.textures = std::min(MAX_TEXTURES, gl_max);
    TRACE("Max textures: %d.\n", mLimits.textures);

    {
        GLint tmp;
        glGetIntegerv(GL_MAX_TEXTURE_COORDS, &gl_max);
        mLimits.texture_coords = std::min(MAX_TEXTURES, gl_max);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &tmp);
        mLimits.fragment_samplers = std::min(MAX_FRAGMENT_SAMPLERS, tmp);
    }
    TRACE("Max texture coords: %d.\n", mLimits.texture_coords);
    TRACE("Max fragment samplers: %d.\n", mLimits.fragment_samplers);

    {
        GLint tmp;
        glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &tmp);
        mLimits.vertex_samplers = tmp;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &tmp);
        mLimits.combined_samplers = tmp;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &tmp);
        mLimits.vertex_attribs = tmp;

        /* Loading GLSL sampler uniforms is much simpler if we can assume that the sampler setup
         * is known at shader link time. In a vertex shader + pixel shader combination this isn't
         * an issue because then the sampler setup only depends on the two shaders. If a pixel
         * shader is used with fixed function vertex processing we're fine too because fixed function
         * vertex processing doesn't use any samplers. If fixed function fragment processing is
         * used we have to make sure that all vertex sampler setups are valid together with all
         * possible fixed function fragment processing setups. This is true if vsamplers + MAX_TEXTURES
         * <= max_samplers. This is true on all d3d9 cards that support vtf(gf 6 and gf7 cards).
         * dx9 radeon cards do not support vertex texture fetch. DX10 cards have 128 samplers, and
         * dx9 is limited to 8 fixed function texture stages and 4 vertex samplers. DX10 does not have
         * a fixed function pipeline anymore.
         *
         * So this is just a check to check that our assumption holds true. If not, write a warning
         * and reduce the number of vertex samplers or probably disable vertex texture fetch.
         */
        if(mLimits.vertex_samplers && mLimits.combined_samplers < 12
            && MAX_TEXTURES + mLimits.vertex_samplers > mLimits.combined_samplers)
        {
            FIXME("OpenGL implementation supports %u vertex samplers and %u total samplers.\n",
                  mLimits.vertex_samplers, mLimits.combined_samplers);
            FIXME("Expected vertex samplers + MAX_TEXTURES(=8) > combined_samplers.\n");
            if(mLimits.combined_samplers > MAX_TEXTURES)
                mLimits.vertex_samplers = mLimits.combined_samplers - MAX_TEXTURES;
            else
                mLimits.vertex_samplers = 0;
        }
    }
    TRACE("Max vertex samplers: %u.\n", mLimits.vertex_samplers);
    TRACE("Max combined samplers: %u.\n", mLimits.combined_samplers);

    if(GLEW_ARB_vertex_blend)
    {
        glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max);
        mLimits.blends = gl_max;
        TRACE("Max blends: %u.\n", mLimits.blends);
    }

    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &gl_max);
    mLimits.texture3d_size = gl_max;
    TRACE("Max texture3D size: %d.\n", mLimits.texture3d_size);

    if(GLEW_EXT_texture_filter_anisotropic)
    {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max);
        mLimits.anisotropy = gl_max;
        TRACE("Max anisotropy: %d.\n", mLimits.anisotropy);
    }
    if(GLEW_ARB_fragment_program)
    {
        glGetProgramiv(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max);
        mLimits.arb_ps_float_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM float constants: %d.\n", mLimits.arb_ps_float_constants);
        glGetProgramiv(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max);
        mLimits.arb_ps_native_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native float constants: %d.\n",
                mLimits.arb_ps_native_constants);
        glGetProgramiv(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max);
        mLimits.arb_ps_temps = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native temporaries: %d.\n", mLimits.arb_ps_temps);
        glGetProgramiv(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max);
        mLimits.arb_ps_instructions = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native instructions: %d.\n", mLimits.arb_ps_instructions);
        glGetProgramiv(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &gl_max);
        mLimits.arb_ps_local_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM local parameters: %d.\n", mLimits.arb_ps_instructions);
    }
    if(GLEW_ARB_vertex_program)
    {
        glGetProgramiv(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max);
        mLimits.arb_vs_float_constants = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM float constants: %d.\n", mLimits.arb_vs_float_constants);
        glGetProgramiv(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max);
        mLimits.arb_vs_native_constants = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native float constants: %d.\n", mLimits.arb_vs_native_constants);
        glGetProgramiv(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max);
        mLimits.arb_vs_temps = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native temporaries: %d.\n", mLimits.arb_vs_temps);
        glGetProgramiv(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max);
        mLimits.arb_vs_instructions = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native instructions: %d.\n", mLimits.arb_vs_instructions);
    }

    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &gl_max);
    mLimits.glsl_vs_float_constants = gl_max / 4;
    TRACE("Max ARB_VERTEX_SHADER float constants: %u.\n", mLimits.glsl_vs_float_constants);

    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &gl_max);
    mLimits.vertex_uniform_blocks = std::min(gl_max, D3DGL_MAX_CBS);
    TRACE("Max vertex uniform blocks: %u (%d).\n", mLimits.vertex_uniform_blocks, gl_max);

    glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &gl_max);
    mLimits.geometry_uniform_blocks = std::min(gl_max, D3DGL_MAX_CBS);
    TRACE("Max geometry uniform blocks: %u (%d).\n", mLimits.geometry_uniform_blocks, gl_max);

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &gl_max);
    mLimits.glsl_ps_float_constants = gl_max / 4;
    TRACE("Max ARB_FRAGMENT_SHADER float constants: %u.\n", mLimits.glsl_ps_float_constants);
    glGetIntegerv(GL_MAX_VARYING_FLOATS, &gl_max);
    mLimits.glsl_varyings = gl_max;
    TRACE("Max GLSL varyings: %u (%u 4 component varyings).\n", gl_max, gl_max / 4);

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &gl_max);
    mLimits.fragment_uniform_blocks = std::min(gl_max, D3DGL_MAX_CBS);
    TRACE("Max fragment uniform blocks: %u (%d).\n", mLimits.fragment_uniform_blocks, gl_max);

    glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &gl_max);
    TRACE("Max combined uniform blocks: %d.\n", gl_max);
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &gl_max);
    TRACE("Max uniform buffer bindings: %d.\n", gl_max);

    if(GLEW_NV_light_max_exponent)
        glGetFloatv(GL_MAX_SHININESS_NV, &mLimits.shininess);
    else
        mLimits.shininess = 128.0f;

    glGetIntegerv(GL_MAX_SAMPLES, &gl_max);
    mLimits.samples = gl_max;
}

void D3DAdapter::init_caps()
{
    mCaps.DeviceType = D3DDEVTYPE_HAL;
    mCaps.AdapterOrdinal = mOrdinal;

    mCaps.Caps  = 0;
    mCaps.Caps2 = D3DCAPS2_CANRENDERWINDOWED |
                  D3DCAPS2_FULLSCREENGAMMA |
                  D3DCAPS2_DYNAMICTEXTURES;
    if(GLEW_SGIS_generate_mipmap)
        mCaps.Caps2 |= D3DCAPS2_CANAUTOGENMIPMAP;

    mCaps.Caps3 = D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD |
                  D3DCAPS3_COPY_TO_VIDMEM                   |
                  D3DCAPS3_COPY_TO_SYSTEMMEM;

    mCaps.PresentationIntervals = D3DPRESENT_INTERVAL_IMMEDIATE  |
                                  D3DPRESENT_INTERVAL_ONE;

    mCaps.CursorCaps = D3DCURSORCAPS_COLOR            |
                       D3DCURSORCAPS_LOWRES;

    mCaps.DevCaps = D3DDEVCAPS_EXECUTESYSTEMMEMORY |
                    D3DDEVCAPS_TLVERTEXSYSTEMMEMORY|
                    D3DDEVCAPS_TLVERTEXVIDEOMEMORY |
                    D3DDEVCAPS_DRAWPRIMTLVERTEX    |
                    D3DDEVCAPS_HWTRANSFORMANDLIGHT |
                    D3DDEVCAPS_EXECUTEVIDEOMEMORY  |
                    D3DDEVCAPS_PUREDEVICE          |
                    D3DDEVCAPS_HWRASTERIZATION     |
                    D3DDEVCAPS_TEXTUREVIDEOMEMORY  |
                    D3DDEVCAPS_TEXTURESYSTEMMEMORY |
                    D3DDEVCAPS_CANRENDERAFTERFLIP  |
                    D3DDEVCAPS_DRAWPRIMITIVES2     |
                    D3DDEVCAPS_DRAWPRIMITIVES2EX;

    mCaps.PrimitiveMiscCaps = D3DPMISCCAPS_CULLNONE              |
                              D3DPMISCCAPS_CULLCCW               |
                              D3DPMISCCAPS_CULLCW                |
                              D3DPMISCCAPS_COLORWRITEENABLE      |
                              D3DPMISCCAPS_CLIPTLVERTS           |
                              D3DPMISCCAPS_CLIPPLANESCALEDPOINTS |
                              D3DPMISCCAPS_MASKZ                 |
                              D3DPMISCCAPS_BLENDOP               |
                              D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING |
                              D3DPMISCCAPS_SEPARATEALPHABLEND    |
                              D3DPMISCCAPS_INDEPENDENTWRITEMASKS;
                              /* TODO:
                              D3DPMISCCAPS_NULLREFERENCE
                              D3DPMISCCAPS_FOGANDSPECULARALPHA
                              D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
                              D3DPMISCCAPS_FOGVERTEXCLAMPED */

    mCaps.RasterCaps = D3DPRASTERCAPS_DITHER    |
                       D3DPRASTERCAPS_PAT       |
                       D3DPRASTERCAPS_WFOG      |
                       D3DPRASTERCAPS_ZFOG      |
                       D3DPRASTERCAPS_FOGVERTEX |
                       D3DPRASTERCAPS_FOGTABLE  |
                       D3DPRASTERCAPS_ZTEST     |
                       D3DPRASTERCAPS_SCISSORTEST   |
                       D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS |
                       D3DPRASTERCAPS_DEPTHBIAS;

    if(GLEW_EXT_texture_filter_anisotropic)
        mCaps.RasterCaps |= D3DPRASTERCAPS_ANISOTROPY    |
                            D3DPRASTERCAPS_ZBIAS         |
                            D3DPRASTERCAPS_MIPMAPLODBIAS;

    mCaps.ZCmpCaps = D3DPCMPCAPS_ALWAYS       |
                     D3DPCMPCAPS_EQUAL        |
                     D3DPCMPCAPS_GREATER      |
                     D3DPCMPCAPS_GREATEREQUAL |
                     D3DPCMPCAPS_LESS         |
                     D3DPCMPCAPS_LESSEQUAL    |
                     D3DPCMPCAPS_NEVER        |
                     D3DPCMPCAPS_NOTEQUAL;

    /* D3DPBLENDCAPS_BOTHINVSRCALPHA and D3DPBLENDCAPS_BOTHSRCALPHA
     * are legacy settings for srcblend only. */
    mCaps.SrcBlendCaps  =  D3DPBLENDCAPS_BOTHINVSRCALPHA |
                           D3DPBLENDCAPS_BOTHSRCALPHA    |
                           D3DPBLENDCAPS_DESTALPHA       |
                           D3DPBLENDCAPS_DESTCOLOR       |
                           D3DPBLENDCAPS_INVDESTALPHA    |
                           D3DPBLENDCAPS_INVDESTCOLOR    |
                           D3DPBLENDCAPS_INVSRCALPHA     |
                           D3DPBLENDCAPS_INVSRCCOLOR     |
                           D3DPBLENDCAPS_ONE             |
                           D3DPBLENDCAPS_SRCALPHA        |
                           D3DPBLENDCAPS_SRCALPHASAT     |
                           D3DPBLENDCAPS_SRCCOLOR        |
                           D3DPBLENDCAPS_ZERO;

    mCaps.DestBlendCaps = D3DPBLENDCAPS_DESTALPHA       |
                          D3DPBLENDCAPS_DESTCOLOR       |
                          D3DPBLENDCAPS_INVDESTALPHA    |
                          D3DPBLENDCAPS_INVDESTCOLOR    |
                          D3DPBLENDCAPS_INVSRCALPHA     |
                          D3DPBLENDCAPS_INVSRCCOLOR     |
                          D3DPBLENDCAPS_ONE             |
                          D3DPBLENDCAPS_SRCALPHA        |
                          D3DPBLENDCAPS_SRCCOLOR        |
                          D3DPBLENDCAPS_ZERO            |
                          D3DPBLENDCAPS_SRCALPHASAT     |
                          D3DPBLENDCAPS_BLENDFACTOR     |
                          D3DPBLENDCAPS_BLENDFACTOR;

    mCaps.AlphaCmpCaps = D3DPCMPCAPS_ALWAYS       |
                         D3DPCMPCAPS_EQUAL        |
                         D3DPCMPCAPS_GREATER      |
                         D3DPCMPCAPS_GREATEREQUAL |
                         D3DPCMPCAPS_LESS         |
                         D3DPCMPCAPS_LESSEQUAL    |
                         D3DPCMPCAPS_NEVER        |
                         D3DPCMPCAPS_NOTEQUAL;

    mCaps.ShadeCaps = D3DPSHADECAPS_SPECULARGOURAUDRGB |
                      D3DPSHADECAPS_COLORGOURAUDRGB    |
                      D3DPSHADECAPS_ALPHAGOURAUDBLEND  |
                      D3DPSHADECAPS_FOGGOURAUD;

    mCaps.TextureCaps = D3DPTEXTURECAPS_ALPHA              |
                        D3DPTEXTURECAPS_ALPHAPALETTE       |
                        D3DPTEXTURECAPS_MIPMAP             |
                        D3DPTEXTURECAPS_PROJECTED          |
                        D3DPTEXTURECAPS_PERSPECTIVE        |
                        D3DPTEXTURECAPS_VOLUMEMAP          |
                        D3DPTEXTURECAPS_MIPVOLUMEMAP       |
                        D3DPTEXTURECAPS_CUBEMAP            |
                        D3DPTEXTURECAPS_MIPCUBEMAP         |
                        D3DPTEXTURECAPS_POW2 |
                        D3DPTEXTURECAPS_NONPOW2CONDITIONAL |
                        D3DPTEXTURECAPS_VOLUMEMAP_POW2 |
                        D3DPTEXTURECAPS_CUBEMAP_POW2;

    mCaps.TextureFilterCaps = D3DPTFILTERCAPS_MAGFLINEAR       |
                              D3DPTFILTERCAPS_MAGFPOINT        |
                              D3DPTFILTERCAPS_MINFLINEAR       |
                              D3DPTFILTERCAPS_MINFPOINT        |
                              D3DPTFILTERCAPS_MIPFLINEAR       |
                              D3DPTFILTERCAPS_MIPFPOINT;
    if(GLEW_EXT_texture_filter_anisotropic)
        mCaps.TextureFilterCaps |= D3DPTFILTERCAPS_MAGFANISOTROPIC |
                                   D3DPTFILTERCAPS_MINFANISOTROPIC;

    mCaps.CubeTextureFilterCaps = D3DPTFILTERCAPS_MAGFLINEAR       |
                                  D3DPTFILTERCAPS_MAGFPOINT        |
                                  D3DPTFILTERCAPS_MINFLINEAR       |
                                  D3DPTFILTERCAPS_MINFPOINT        |
                                  D3DPTFILTERCAPS_MIPFLINEAR       |
                                  D3DPTFILTERCAPS_MIPFPOINT;

    if(GLEW_EXT_texture_filter_anisotropic)
        mCaps.CubeTextureFilterCaps |= D3DPTFILTERCAPS_MAGFANISOTROPIC |
                                       D3DPTFILTERCAPS_MINFANISOTROPIC;

    mCaps.VolumeTextureFilterCaps = D3DPTFILTERCAPS_MAGFLINEAR       |
                                    D3DPTFILTERCAPS_MAGFPOINT        |
                                    D3DPTFILTERCAPS_MINFLINEAR       |
                                    D3DPTFILTERCAPS_MINFPOINT        |
                                    D3DPTFILTERCAPS_MIPFLINEAR       |
                                    D3DPTFILTERCAPS_MIPFPOINT;

    mCaps.TextureAddressCaps = D3DPTADDRESSCAPS_INDEPENDENTUV |
                               D3DPTADDRESSCAPS_CLAMP         |
                               D3DPTADDRESSCAPS_WRAP;
    mCaps.TextureAddressCaps |= D3DPTADDRESSCAPS_BORDER;
    mCaps.TextureAddressCaps |= D3DPTADDRESSCAPS_MIRROR;
    mCaps.TextureAddressCaps |= D3DPTADDRESSCAPS_MIRRORONCE;

    mCaps.VolumeTextureAddressCaps = D3DPTADDRESSCAPS_INDEPENDENTUV |
                                     D3DPTADDRESSCAPS_CLAMP  |
                                     D3DPTADDRESSCAPS_WRAP;
    mCaps.VolumeTextureAddressCaps |= D3DPTADDRESSCAPS_BORDER;
    mCaps.VolumeTextureAddressCaps |= D3DPTADDRESSCAPS_MIRROR;
    mCaps.VolumeTextureAddressCaps |= D3DPTADDRESSCAPS_MIRRORONCE;

    mCaps.LineCaps = D3DLINECAPS_TEXTURE       |
                     D3DLINECAPS_ZTEST         |
                     D3DLINECAPS_BLEND         |
                     D3DLINECAPS_ALPHACMP      |
                     D3DLINECAPS_FOG;
    /* D3DLINECAPS_ANTIALIAS is not supported on Windows, and dx and gl seem to have a different
     * idea how generating the smoothing alpha values works; the result is different
     */

    mCaps.MaxTextureWidth = mLimits.texture_size;
    mCaps.MaxTextureHeight = mLimits.texture_size;

    mCaps.MaxVolumeExtent = mLimits.texture3d_size;

    mCaps.MaxTextureRepeat = 32768;
    mCaps.MaxTextureAspectRatio = mLimits.texture_size;
    mCaps.MaxVertexW = 1.0f;

    mCaps.GuardBandLeft = 0.0f;
    mCaps.GuardBandTop = 0.0f;
    mCaps.GuardBandRight = 0.0f;
    mCaps.GuardBandBottom = 0.0f;

    mCaps.ExtentsAdjust = 0.0f;

    mCaps.StencilCaps = D3DSTENCILCAPS_DECRSAT |
                        D3DSTENCILCAPS_INCRSAT |
                        D3DSTENCILCAPS_INVERT  |
                        D3DSTENCILCAPS_KEEP    |
                        D3DSTENCILCAPS_REPLACE |
                        D3DSTENCILCAPS_ZERO;
    if(GLEW_EXT_stencil_wrap)
        mCaps.StencilCaps |= D3DSTENCILCAPS_DECR  |
                             D3DSTENCILCAPS_INCR;
    mCaps.StencilCaps |= D3DSTENCILCAPS_TWOSIDED;

    mCaps.MaxAnisotropy = mLimits.anisotropy;
    mCaps.MaxPointSize = mLimits.pointsize_max;

    mCaps.MaxPrimitiveCount = 0x555555; /* Taken from an AMD Radeon HD 5700 (Evergreen) GPU. */
    mCaps.MaxVertexIndex    = 0xffffff; /* Taken from an AMD Radeon HD 5700 (Evergreen) GPU. */
    mCaps.MaxStreams        = MAX_STREAMS;
    mCaps.MaxStreamStride   = 1024;

    mCaps.DevCaps2 = D3DDEVCAPS2_STREAMOFFSET                       |
                     D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET |
                     D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES;
    mCaps.MaxNpatchTessellationLevel = 0;
    mCaps.MasterAdapterOrdinal       = 0;
    mCaps.AdapterOrdinalInGroup      = 0;
    mCaps.NumberOfAdaptersInGroup    = 1;

    mCaps.NumSimultaneousRTs = std::min<UINT>(D3D_MAX_SIMULTANEOUS_RENDERTARGETS, mLimits.buffers);

    mCaps.StretchRectFilterCaps = D3DPTFILTERCAPS_MINFPOINT  |
                                  D3DPTFILTERCAPS_MAGFPOINT  |
                                  D3DPTFILTERCAPS_MINFLINEAR |
                                  D3DPTFILTERCAPS_MAGFLINEAR;
    mCaps.VertexTextureFilterCaps = 0;

    /* Add shader misc caps. Only some of them belong to the shader parts of the pipeline */
    mCaps.PrimitiveMiscCaps |= D3DPMISCCAPS_TSSARGTEMP |
                               D3DPMISCCAPS_PERSTAGECONSTANT;

    /* We require GL 3.3, which is more than capable of handling SM3.0 with GLSL. So just set that. */
    mCaps.VertexShaderVersion = D3DVS_VERSION(3,0);
    mCaps.MaxVertexShaderConst = std::min(256u, mLimits.glsl_vs_float_constants);

    mCaps.PixelShaderVersion = D3DPS_VERSION(3,0);
    mCaps.PixelShader1xMaxValue = 1024.0f;

    mCaps.TextureOpCaps = D3DTEXOPCAPS_DISABLE |
                          D3DTEXOPCAPS_SELECTARG1 |
                          D3DTEXOPCAPS_SELECTARG2 |
                          D3DTEXOPCAPS_MODULATE4X |
                          D3DTEXOPCAPS_MODULATE2X |
                          D3DTEXOPCAPS_MODULATE |
                          D3DTEXOPCAPS_ADDSIGNED2X |
                          D3DTEXOPCAPS_ADDSIGNED |
                          D3DTEXOPCAPS_ADD |
                          D3DTEXOPCAPS_SUBTRACT |
                          D3DTEXOPCAPS_ADDSMOOTH |
                          D3DTEXOPCAPS_BLENDCURRENTALPHA |
                          D3DTEXOPCAPS_BLENDFACTORALPHA |
                          D3DTEXOPCAPS_BLENDTEXTUREALPHA |
                          D3DTEXOPCAPS_BLENDDIFFUSEALPHA |
                          D3DTEXOPCAPS_BLENDTEXTUREALPHAPM |
                          D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR |
                          D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |
                          D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
                          D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |
                          D3DTEXOPCAPS_DOTPRODUCT3 |
                          D3DTEXOPCAPS_MULTIPLYADD |
                          D3DTEXOPCAPS_LERP |
                          D3DTEXOPCAPS_BUMPENVMAP |
                          D3DTEXOPCAPS_BUMPENVMAPLUMINANCE;
    mCaps.MaxTextureBlendStages   = 8;
    mCaps.MaxSimultaneousTextures = std::min(mLimits.fragment_samplers, 8u);

    mCaps.MaxUserClipPlanes         = mLimits.clipplanes;
    mCaps.MaxActiveLights           = mLimits.lights;
    mCaps.MaxVertexBlendMatrices    = 1;
    mCaps.MaxVertexBlendMatrixIndex = 0;
    mCaps.VertexProcessingCaps      = D3DVTXPCAPS_TEXGEN |
                                      D3DVTXPCAPS_MATERIALSOURCE7 |
                                      D3DVTXPCAPS_DIRECTIONALLIGHTS |
                                      D3DVTXPCAPS_POSITIONALLIGHTS |
                                      D3DVTXPCAPS_LOCALVIEWER |
                                      D3DVTXPCAPS_TEXGEN_SPHEREMAP;
    mCaps.FVFCaps                   = D3DFVFCAPS_PSIZE | 8; /* 8 texture coordinates. */
    mCaps.RasterCaps               |= D3DPRASTERCAPS_FOGRANGE;

    /* The following caps are shader specific, but they are things we cannot detect, or which
     * are the same among all shader models. So to avoid code duplication set the shader version
     * specific, but otherwise constant caps here
     */
    if(mCaps.VertexShaderVersion >= D3DVS_VERSION(3,0))
    {
        /* Where possible set the caps based on OpenGL extensions and if they
         * aren't set (in case of software rendering) use the VS 3.0 from
         * MSDN or else if there's OpenGL spec use a hardcoded value minimum
         * VS3.0 value. */
        mCaps.VS20Caps.Caps = D3DVS20CAPS_PREDICATION;
        /* VS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        mCaps.VS20Caps.DynamicFlowControlDepth = D3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH;
        mCaps.VS20Caps.NumTemps = std::max(32u, mLimits.arb_vs_temps);
        /* level of nesting in loops / if-statements; VS 3.0 requires MAX (4) */
        mCaps.VS20Caps.StaticFlowControlDepth = D3DVS20_MAX_STATICFLOWCONTROLDEPTH;

        mCaps.MaxVShaderInstructionsExecuted    = 65535; /* VS 3.0 needs at least 65535, some cards even use 2^32-1 */
        mCaps.MaxVertexShader30InstructionSlots = std::max(512u, mLimits.arb_vs_instructions);
        mCaps.VertexTextureFilterCaps = D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MAGFPOINT;
    }
    else if(mCaps.VertexShaderVersion >= D3DVS_VERSION(2,0))
    {
        mCaps.VS20Caps.Caps = 0;
        mCaps.VS20Caps.DynamicFlowControlDepth = D3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH;
        mCaps.VS20Caps.NumTemps = std::max(12u, mLimits.arb_vs_temps);
        mCaps.VS20Caps.StaticFlowControlDepth = 1;

        mCaps.MaxVShaderInstructionsExecuted    = 65535;
        mCaps.MaxVertexShader30InstructionSlots = 0;
    }
    else /* VS 1.x */
    {
        mCaps.VS20Caps.Caps = 0;
        mCaps.VS20Caps.DynamicFlowControlDepth = 0;
        mCaps.VS20Caps.NumTemps = 0;
        mCaps.VS20Caps.StaticFlowControlDepth = 0;

        mCaps.MaxVShaderInstructionsExecuted    = 0;
        mCaps.MaxVertexShader30InstructionSlots = 0;
    }

    if(mCaps.PixelShaderVersion >= D3DPS_VERSION(3,0))
    {
        /* Where possible set the caps based on OpenGL extensions and if they
         * aren't set (in case of software rendering) use the PS 3.0 from
         * MSDN or else if there's OpenGL spec use a hardcoded value minimum
         * PS 3.0 value. */

        /* Caps is more or less undocumented on MSDN but it appears to be
         * used for PS20Caps based on results from R9600/FX5900/Geforce6800
         * cards from Windows */
        mCaps.PS20Caps.Caps = D3DPS20CAPS_ARBITRARYSWIZZLE     |
                              D3DPS20CAPS_GRADIENTINSTRUCTIONS |
                              D3DPS20CAPS_PREDICATION          |
                              D3DPS20CAPS_NODEPENDENTREADLIMIT |
                              D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT;
        /* PS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        mCaps.PS20Caps.DynamicFlowControlDepth = D3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH;
        mCaps.PS20Caps.NumTemps = std::max(32u, mLimits.arb_ps_temps);
        /* PS 3.0 requires MAX_STATICFLOWCONTROLDEPTH (4) */
        mCaps.PS20Caps.StaticFlowControlDepth = D3DPS20_MAX_STATICFLOWCONTROLDEPTH;
        /* PS 3.0 requires MAX_NUMINSTRUCTIONSLOTS (512) */
        mCaps.PS20Caps.NumInstructionSlots = D3DPS20_MAX_NUMINSTRUCTIONSLOTS;

        mCaps.MaxPShaderInstructionsExecuted   = 65535;
        mCaps.MaxPixelShader30InstructionSlots = std::max<UINT>(D3DMIN30SHADERINSTRUCTIONS,
                                                                mLimits.arb_ps_instructions);
    }
    else if(mCaps.PixelShaderVersion >= D3DPS_VERSION(2,0))
    {
        /* Below we assume PS2.0 specs, not extended 2.0a(GeforceFX)/2.0b(Radeon R3xx) ones */
        mCaps.PS20Caps.Caps = 0;
        mCaps.PS20Caps.DynamicFlowControlDepth = 0; /* D3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH = 0 */
        mCaps.PS20Caps.NumTemps = std::max(12u, mLimits.arb_ps_temps);
        mCaps.PS20Caps.StaticFlowControlDepth = D3DPS20_MIN_STATICFLOWCONTROLDEPTH; /* Minimum: 1 */
        /* Minimum number (64 ALU + 32 Texture), a GeforceFX uses 512 */
        mCaps.PS20Caps.NumInstructionSlots = D3DPS20_MIN_NUMINSTRUCTIONSLOTS;

        mCaps.MaxPShaderInstructionsExecuted   = 512; /* Minimum value, a GeforceFX uses 1024 */
        mCaps.MaxPixelShader30InstructionSlots = 0;
    }
    else /* PS 1.x */
    {
        mCaps.PS20Caps.Caps = 0;
        mCaps.PS20Caps.DynamicFlowControlDepth = 0;
        mCaps.PS20Caps.NumInstructionSlots = 0;
        mCaps.PS20Caps.StaticFlowControlDepth = 0;
        mCaps.PS20Caps.NumInstructionSlots = 0;

        mCaps.MaxPShaderInstructionsExecuted   = 0;
        mCaps.MaxPixelShader30InstructionSlots = 0;
    }

    if(mCaps.VertexShaderVersion >= D3DVS_VERSION(2,0))
    {
        /* OpenGL supports all the formats below, perhaps not always
         * without conversion, but it supports them.
         * Further GLSL doesn't seem to have an official unsigned type so
         * don't advertise it yet as I'm not sure how we handle it.
         * We might need to add some clamping in the shader engine to
         * support it.
         * TODO: D3DDTCAPS_USHORT2N, D3DDTCAPS_USHORT4N, D3DDTCAPS_UDEC3, D3DDTCAPS_DEC3N */
        mCaps.DeclTypes = D3DDTCAPS_UBYTE4    |
                          D3DDTCAPS_UBYTE4N   |
                          D3DDTCAPS_SHORT2N   |
                          D3DDTCAPS_SHORT4N;
        if(GLEW_ARB_half_float_vertex)
            mCaps.DeclTypes |= D3DDTCAPS_FLOAT16_2 |
                               D3DDTCAPS_FLOAT16_4;
    }
    else
        mCaps.DeclTypes = 0;
}

bool D3DAdapter::init()
{
    TRACE("Initializing adapter %u info\n", mOrdinal);

    HWND hWnd; HDC hDc; HGLRC hGlrc;
    if(!CreateFakeContext(GetModuleHandleW(nullptr), hWnd, hDc, hGlrc))
        return false;

    bool retval = false;
    wglMakeCurrent(hDc, hGlrc);

    if(!GLEW_VERSION_3_3)
    {
        ERR("Required GL 3.3 not supported!");
        goto done;
    }

    init_limits();
    init_caps();

    retval = true;
done:
    wglMakeCurrent(hDc, nullptr);
    wglDeleteContext(hGlrc);
    DestroyWindow(hWnd);

    return retval;
}

bool D3DAdapter::getCaps(D3DCAPS9 *caps)
{
    if(!mInited)
    {
        if(!init())
            return false;
        mInited = true;
    }

    *caps = mCaps;
    return true;
}



Direct3DGL::Direct3DGL()
  : mRefCount(0)
{
    mAdapters.push_back(0);
}

Direct3DGL::~Direct3DGL()
{
}


HRESULT Direct3DGL::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, out %p.\n", this, (const char*)debugstr_guid(riid), obj);

    if(riid == IID_IDirect3D9 || riid == IID_IUnknown)
    {
        AddRef();
        *obj = static_cast<IDirect3D9*>(this);
        return S_OK;
    }
    if(riid == IID_IDirect3D9Ex)
    {
        FIXME("Application asks for IDirect3D9Ex, but this instance wasn't created with Direct3DCreate9Ex.\n");
        *obj = nullptr;
        return E_NOINTERFACE;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", (const char*)debugstr_guid(riid));

    *obj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DGL::AddRef(void)
{
    ULONG ret = ++mRefCount;
    TRACE("New refcount: %lu\n", ret);
    return ret;
}

ULONG Direct3DGL::Release(void)
{
    ULONG ret = --mRefCount;
    TRACE("New refcount: %lu\n", ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT Direct3DGL::RegisterSoftwareDevice(void *initFunction)
{
    FIXME("iface %p, init_function %p stub!\n", this, initFunction);

    return D3D_OK;
}


UINT Direct3DGL::GetAdapterCount(void)
{
    FIXME("iface %p stub!\n", this);

    std::vector<D3DAdapter>().swap(mAdapters);
    mAdapters.push_back(0);

    return mAdapters.size();
}

HRESULT Direct3DGL::GetAdapterIdentifier(UINT adapter, DWORD flags, D3DADAPTER_IDENTIFIER9 *identifier)
{
    FIXME("iface %p, adapter %u, flags 0x%lx, identifier %p stub!\n", this, adapter, flags, identifier);

    if(adapter >= mAdapters.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, mAdapters.size());

    // ME NEXT!
    return E_NOTIMPL;
}


UINT Direct3DGL::GetAdapterModeCount(UINT adapter, D3DFORMAT format)
{
    FIXME("iface %p, adapter %u, format 0x%x, stub!\n", this, adapter, format);
    return 0;
}

HRESULT Direct3DGL::EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode, D3DDISPLAYMODE *displayMode)
{
    FIXME("iface %p, adapter %u, format 0x%x, mode %u, displayMode %p stub!\n", this, adapter, format, mode, displayMode);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE *displayMode)
{
    FIXME("iface %p, adapter %u, displayMode %p stub!\n", this, adapter, displayMode);
    return E_NOTIMPL;
}


HRESULT Direct3DGL::CheckDeviceType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, WINBOOL windowed)
{
    FIXME("iface %p, adapter %u, devType 0x%x, displayFormat 0x%x, backBufferFormat 0x%x, windowed %u stub!\n", this, adapter, devType, displayFormat, backBufferFormat, windowed);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDeviceFormat(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, DWORD usage, D3DRESOURCETYPE resType, D3DFORMAT checkFormat)
{
    FIXME("iface %p, adapter %u, devType 0x%x, adapterFormat 0x%x, usage 0x%lx, resType 0x%x, checkFormat 0x%x stub!\n", this, adapter, devType, adapterFormat, usage, resType, checkFormat);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT surfaceFormat, WINBOOL windowed, D3DMULTISAMPLE_TYPE multiSampleType, DWORD *qualityLevels)
{
    FIXME("iface %p, adapter %u, devType 0x%x, surfaceFormat 0x%x, windowed %u, multiSampleType 0x%x, qualityLevels %p stub!\n", this, adapter, devType, surfaceFormat, windowed, multiSampleType, qualityLevels);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat)
{
    FIXME("iface %p, adapter %u, devType 0x%x, adapterFormat 0x%x, renderTargetFormat 0x%x, depthStencilFormat 0x%x stub!\n", this, adapter, devType, adapterFormat, renderTargetFormat, depthStencilFormat);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::CheckDeviceFormatConversion(UINT adapter, D3DDEVTYPE devType, D3DFORMAT srcFormat, D3DFORMAT dstFormat)
{
    FIXME("iface %p, adapter %u, devType 0x%x, srcFormat 0x%x, dstFormat 0x%x stub!\n", this, adapter, devType, srcFormat, dstFormat);
    return E_NOTIMPL;
}

HRESULT Direct3DGL::GetDeviceCaps(UINT adapter, D3DDEVTYPE devType, D3DCAPS9 *caps)
{
    FIXME("iface %p, adapter %u, devType 0x%x, caps %p stub!\n", this, adapter, devType, caps);

    if(adapter >= mAdapters.size())
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Adapter %u out of range (count=%u)\n", adapter, mAdapters.size());
    if(devType != D3DDEVTYPE_HAL)
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Non-HAL type 0x%x not supported\n", devType);

    if(!mAdapters[adapter].getCaps(caps))
        WARN_AND_RETURN(D3DERR_INVALIDCALL, "Failed to get caps for adapter %u\n", adapter);

    return D3D_OK;
}

HMONITOR Direct3DGL::GetAdapterMonitor(UINT adapter)
{
    FIXME("iface %p, adapter %u stub!\n", this, adapter);
    return nullptr;
}


HRESULT Direct3DGL::CreateDevice(UINT adapter, D3DDEVTYPE devType, HWND focusWindow, DWORD behaviorFlags, D3DPRESENT_PARAMETERS *presentParams, IDirect3DDevice9 **iface)
{
    FIXME("iface %p, adapter %u, devType 0x%x, focusWindow %p, behaviorFlags 0x%lx, presentParams %p, iface %p stub!\n", this, adapter, devType, focusWindow, behaviorFlags, presentParams, iface);
    return E_NOTIMPL;
}
