
#include "glformat.hpp"

#include "trace.hpp"


#define DEPTH_STENCIL_BUFFER_BITS (GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT)

const std::map<DWORD,GLFormatInfo> gFormatList{
    /* FIXME: OpenGL reports GL_SRGB_READ/GL_SRGB_DECODE_ARB as false for
     * GL_RGB(A)8. This is because EXT_texture_sRGB_decode requires an sRGB
     * internalformat to be eligible for toggling sRGB decoding (GL_SRGB8 and
     * others). The problem is that EXT_texture_sRGB_decode is not guaranteed
     * to be available, and we don't want sRGB decoding to be stuck on in that
     * case.
     */
    { D3DFMT_A8R8G8B8, { GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_A8B8G8R8, { GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_X8R8G8B8, { GL_RGB8,  GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_X8B8G8R8, { GL_RGB8,  GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, GL_COLOR_BUFFER_BIT } },

    { D3DFMT_R5G6B5,   { GL_RGB5,        GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,       2, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_A1R5G5B5, { GL_RGB5_A1,     GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, 2, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_X1R5G5B5, { GL_RGB5,        GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, 2, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_A4R4G4B4, { GL_RGBA4,       GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 2, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_X4R4G4B4, { GL_RGB4,        GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 2, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_R3G3B2,   { GL_R3_G3_B2,    GL_BGR,  GL_UNSIGNED_BYTE_2_3_3_REV,    1, GL_COLOR_BUFFER_BIT } },
//  { D3DFMT_A8R3G3B2, { GL_R3_G3_B2_A8, GL_BGRA, GL_UNSIGNED_SHORT_8_3_3_2_REV, 2, GL_COLOR_BUFFER_BIT } },

    { D3DFMT4CC(' ','R','1','6'), { GL_R16,    GL_RED,  GL_UNSIGNED_SHORT, 2, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_G16R16,              { GL_RG16,   GL_RG,   GL_UNSIGNED_SHORT, 4, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_A16B16G16R16,        { GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT, 8, GL_COLOR_BUFFER_BIT } },

    { D3DFMT_A2R10G10B10, { GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, 4, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_A2B10G10R10, { GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, 4, GL_COLOR_BUFFER_BIT } },

    { D3DFMT_A8,                  { GL_ALPHA8,              GL_ALPHA,           GL_UNSIGNED_BYTE,  1, GL_COLOR_BUFFER_BIT        } },
    { D3DFMT_L8,                  { GL_LUMINANCE8,          GL_LUMINANCE,       GL_UNSIGNED_BYTE,  1, GL_COLOR_BUFFER_BIT        } },
    { D3DFMT_A8L8,                { GL_LUMINANCE8_ALPHA8,   GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,  2, GL_COLOR_BUFFER_BIT        } },
//  { D3DFMT_A4L4,                { GL_LUMINANCE4_ALPHA4,   GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE_4_4_REV, 1, GL_COLOR_BUFFER_BIT } },
    { D3DFMT4CC('A','L','1','6'), { GL_LUMINANCE16_ALPHA16, GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT, 4, GL_COLOR_BUFFER_BIT        } },

    { D3DFMT_D16,                 { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,    2, GL_DEPTH_BUFFER_BIT       } },
    { D3DFMT_D24X8,               { GL_DEPTH_COMPONENT24, GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8, 4, GL_DEPTH_BUFFER_BIT       } },
    { D3DFMT_D24S8,               { GL_DEPTH24_STENCIL8,  GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8, 4, DEPTH_STENCIL_BUFFER_BITS } },
    { D3DFMT4CC('I','N','T','Z'), { GL_DEPTH24_STENCIL8,  GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8, 4, DEPTH_STENCIL_BUFFER_BITS } },
    { D3DFMT_D32,                 { GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,      4, GL_DEPTH_BUFFER_BIT       } },

    { D3DFMT_R16F,          { GL_R16F,        GL_RED,  GL_HALF_FLOAT, 2, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_G16R16F,       { GL_RG16F,       GL_RG,   GL_HALF_FLOAT, 4, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_A16B16G16R16F, { GL_RGBA16F_ARB, GL_RGBA, GL_HALF_FLOAT, 8, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_R32F,          { GL_R32F,        GL_RED,  GL_FLOAT,  4, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_G32R32F,       { GL_RG32F,       GL_RG,   GL_FLOAT,  8, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_A32B32G32R32F, { GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT, 16, GL_COLOR_BUFFER_BIT } },

    { D3DFMT_V8U8,     { GL_DSDT8_NV,                 GL_DSDT_NV,         GL_BYTE, 2, GL_COLOR_BUFFER_BIT                          } },
    { D3DFMT_X8L8V8U8, { GL_DSDT8_MAG8_INTENSITY8_NV, GL_DSDT_MAG_VIB_NV, GL_UNSIGNED_INT_8_8_S8_S8_REV_NV, 4, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_Q8W8V8U8, { GL_SIGNED_RGBA8_NV,          GL_RGBA,            GL_BYTE, 4, GL_COLOR_BUFFER_BIT                          } },

    { D3DFMT_DXT1, { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA, GL_UNSIGNED_BYTE,  8, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_DXT3, { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE, 16, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_DXT5, { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE, 16, GL_COLOR_BUFFER_BIT } },
    // NOTE: These are premultiplied-alpha versions of DXT formats. We don't
    // support the premultiplication (yet), but there shouldn't be any other
    // issue other than slightly darkened textures.
    { D3DFMT_DXT2, { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE, 16, GL_COLOR_BUFFER_BIT } },
    { D3DFMT_DXT4, { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE, 16, GL_COLOR_BUFFER_BIT } },
};


GLenum GLFormatInfo::getDepthStencilAttachment() const
{
    if((buffermask&DEPTH_STENCIL_BUFFER_BITS) == DEPTH_STENCIL_BUFFER_BITS)
        return GL_DEPTH_STENCIL_ATTACHMENT;
    if((buffermask&GL_DEPTH_BUFFER_BIT))
        return GL_DEPTH_ATTACHMENT;
    if((buffermask&GL_STENCIL_BUFFER_BIT))
        return GL_STENCIL_ATTACHMENT;

    ERR("Unhandled internal depthstencil format: 0x%04x\n", internalformat);
    return GL_NONE;
}

GLuint GLFormatInfo::getDepthBits() const
{
    if(internalformat == GL_DEPTH_COMPONENT16) return 16;
    if(internalformat == GL_DEPTH_COMPONENT24) return 24;
    if(internalformat == GL_DEPTH24_STENCIL8) return 24;
    if(internalformat == GL_DEPTH_COMPONENT32) return 32;

    ERR("Unhandled internal depthstencil format: 0x%04x\n", internalformat);
    return 1;
}
