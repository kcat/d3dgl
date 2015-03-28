#ifndef BUFFEROBJECT_HPP
#define BUFFEROBJECT_HPP

#include <atomic>
#include <vector>
#include <d3d9.h>

#include "glew.h"
#include "misc.hpp"


class D3DGLDevice;

class D3DGLBufferObject : public IDirect3DVertexBuffer9, public IDirect3DIndexBuffer9 {
    std::atomic<ULONG> mRefCount;

    ref_ptr<D3DGLDevice> mParent;

    UINT mLength;
    DWORD mUsage;
    D3DFORMAT mFormat;
    DWORD mFvf;
    D3DPOOL mPool;

    std::atomic<ULONG> mPendingDraws;

    // TODO: Use VBOs/IBOs when possible. Requires GL_ARB_buffer_storage for
    // persistent buffer mapping, or some tricky mapping gymnastics to have it
    // unmapped for drawing but mapped when locked (waiting for an async map
    // request will kill performance).
    std::vector<GLubyte> mSysMem;

    GLubyte *mUserPtr;
    std::atomic<bool> mLocked;
    UINT mLockedOffset;
    UINT mLockedLength;

    const GLenum mTarget; // For VBO/IBO

    bool init_common(UINT length, DWORD usage, D3DPOOL pool);

public:
    enum BufferType { VBO=GL_ARRAY_BUFFER, IBO=GL_ELEMENT_ARRAY_BUFFER };

    D3DGLBufferObject(D3DGLDevice *parent, BufferType type);
    virtual ~D3DGLBufferObject();

    bool init_vbo(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool);
    bool init_ibo(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool);

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();
    /*** IDirect3DResource9 methods ***/
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device);
    virtual HRESULT WINAPI SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags);
    virtual HRESULT WINAPI GetPrivateData(REFGUID refguid, void *data, DWORD *size);
    virtual HRESULT WINAPI FreePrivateData(REFGUID refguid);
    virtual DWORD WINAPI SetPriority(DWORD priority);
    virtual DWORD WINAPI GetPriority();
    virtual void WINAPI PreLoad();
    virtual D3DRESOURCETYPE WINAPI GetType();
    /*** IDirect3DVertexBuffer9/IDirect3DIndexBuffer9 methods ***/
    virtual HRESULT WINAPI Lock(UINT offset, UINT length, void **data, DWORD flags);
    virtual HRESULT WINAPI Unlock();
    /*** IDirect3DVertexBuffer9 methods ***/
    virtual HRESULT WINAPI GetDesc(D3DVERTEXBUFFER_DESC *desc);
    /*** IDirect3DIndexBuffer9 methods ***/
    virtual HRESULT WINAPI GetDesc(D3DINDEXBUFFER_DESC *desc);
};

#endif /* BUFFEROBJECT_HPP */
