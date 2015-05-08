#ifndef GLFORMAT_HPP
#define GLFORMAT_HPP

#include <map>
#include <d3d9.h>

#include "glew.h"


#ifndef D3DFMT4CC
#define D3DFMT4CC(a,b,c,d)  (a | ((b<<8)&0xff00) | ((c<<16)&0xff0000) | ((d<<24)&0xff000000))
#endif

#define D3DFMT_ATI1 D3DFMT4CC('A','T','I','1')
#define D3DFMT_ATI2 D3DFMT4CC('A','T','I','2')
#define D3DFMT_NULL D3DFMT4CC('N','U','L','L')

struct GLFormatInfo {
    GLenum internalformat;
    GLenum format;
    GLenum type;
    int bytesperpixel;
    int bytesperblock; /* Same as bytesperpixel for uncompressed formats */
    GLbitfield buffermask;
    GLbitfield flags;

    enum FlagBits {
        Normal = 0,
        ShadowTexture = 1<<0, /* Expects depth comparison enabled on the sampler */
        BadPitch = 1<<1 /* Calculate normal pitch/offset despite being compressed */
    };

    GLenum getDepthStencilAttachment() const;
    GLuint getDepthBits() const;

    static int calcPitch(int w, int bpp)
    {
        int ret = w * bpp;
        return (ret+3) & ~3;
    }
    static int calcBlockPitch(int w, int bpp)
    {
        int ret = (w+3)/4 * bpp;
        return (ret+3) & ~3;
    }
};
extern const std::map<DWORD,GLFormatInfo> gFormatList;

#endif /* GLFORMAT_HPP */
