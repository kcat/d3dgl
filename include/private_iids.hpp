#ifndef PRIVATE_IIDS_HPP
#define PRIVATE_IIDS_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* The d3d device ID */
DEFINE_GUID(IID_D3DDEVICE_D3DUID, 0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x81);

/* Our implementation IDs */
DEFINE_GUID(IID_D3DGLSwapChain,         0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x82);
DEFINE_GUID(IID_D3DGLRenderTarget,      0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x83);
DEFINE_GUID(IID_D3DGLBackbufferSurface, 0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x84);
DEFINE_GUID(IID_D3DGLTexture,           0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x85);
DEFINE_GUID(IID_D3DGLTextureSurface,    0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x86);
DEFINE_GUID(IID_D3DGLBufferObject,      0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x87);
DEFINE_GUID(IID_D3DGLVertexShader,      0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x88);
DEFINE_GUID(IID_D3DGLPixelShader,       0xaeb2cdd4, 0x6e41, 0x43ea, 0x94,0x1c, 0x83,0x61,0xcc,0x76,0x07,0x89);


#define RETURN_IF_IID_TYPE(obj, riid, TYPE) do { \
    if((riid) == IID_##TYPE)                     \
    {                                            \
        AddRef();                                \
        *(obj) = static_cast<TYPE*>(this);       \
        return D3D_OK;                           \
    }                                            \
} while (0)

#define RETURN_IF_IID_TYPE2(obj, riid, TYPE1, TYPE2) do {        \
    if((riid) == IID_##TYPE2)                                    \
    {                                                            \
        AddRef();                                                \
        *(obj) = static_cast<TYPE2*>(static_cast<TYPE1*>(this)); \
        return D3D_OK;                                           \
    }                                                            \
} while (0)

#endif /* PRIVATE_IIDS_HPP */
