#ifndef GLFORMAT_HPP
#define GLFORMAT_HPP

#include <map>
#include <d3d9.h>

#include "glew.h"


#ifndef D3DFMT4CC
#define D3DFMT4CC(a,b,c,d)  (a | ((b<<8)&0xff00) | ((b<<16)&0xff0000) | ((d<<24)&0xff000000))
#endif

struct GLFormatInfo {
    GLenum internalformat;
    GLenum format;
    GLenum type;
    int bytesperpixel; /* For compressed formats, bytes per block */
    GLbitfield buffermask;

    GLenum getDepthStencilAttachment() const;
};
extern const std::map<DWORD,GLFormatInfo> gFormatList;

#endif /* GLFORMAT_HPP */
