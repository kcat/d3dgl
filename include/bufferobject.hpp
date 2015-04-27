#ifndef BUFFEROBJECT_HPP
#define BUFFEROBJECT_HPP

#include <atomic>
#include <vector>
#include <d3d9.h>

#include "glew.h"
#include "allocators.hpp"


class D3DGLDevice;

class D3DGLBufferObject : public IDirect3DVertexBuffer9, public IDirect3DIndexBuffer9 {
    std::atomic<ULONG> mRefCount;

    D3DGLDevice *mParent;

    UINT mLength;
    DWORD mUsage;
    D3DFORMAT mFormat;
    DWORD mFvf;
    D3DPOOL mPool;

    GLuint mBufferId;
    std::vector<GLubyte,AlignedAllocator<GLubyte>> mSysMem;

    enum LockType {
        LT_Unlocked,
        LT_ReadOnly,
        LT_Full
    };
    std::atomic<LockType> mLock;
    UINT mLockedOffset;
    UINT mLockedLength;

    std::atomic<ULONG> mUpdateInProgress;

    bool init_common(UINT length, DWORD usage, D3DPOOL pool);

public:
    D3DGLBufferObject(D3DGLDevice *parent);
    virtual ~D3DGLBufferObject();

    bool init_vbo(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool);
    bool init_ibo(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool);

    GLuint getBufferId() const { return mBufferId; }

    void resetBufferData(const GLubyte *data, GLuint length);

    void initGL();
    void loadBufferDataGL(UINT offset, UINT length);
    void resizeBufferGL();

    D3DFORMAT getFormat() const { return mFormat; }

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj) final;
    virtual ULONG WINAPI AddRef() final;
    virtual ULONG WINAPI Release() final;
    /*** IDirect3DResource9 methods ***/
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device) final;
    virtual HRESULT WINAPI SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags) final;
    virtual HRESULT WINAPI GetPrivateData(REFGUID refguid, void *data, DWORD *size) final;
    virtual HRESULT WINAPI FreePrivateData(REFGUID refguid) final;
    virtual DWORD WINAPI SetPriority(DWORD priority) final;
    virtual DWORD WINAPI GetPriority() final;
    virtual void WINAPI PreLoad() final;
    virtual D3DRESOURCETYPE WINAPI GetType() final;
    /*** IDirect3DVertexBuffer9/IDirect3DIndexBuffer9 methods ***/
    virtual HRESULT WINAPI Lock(UINT offset, UINT length, void **data, DWORD flags) final;
    virtual HRESULT WINAPI Unlock() final;
    /*** IDirect3DVertexBuffer9 methods ***/
    virtual HRESULT WINAPI GetDesc(D3DVERTEXBUFFER_DESC *desc) final;
    /*** IDirect3DIndexBuffer9 methods ***/
    virtual HRESULT WINAPI GetDesc(D3DINDEXBUFFER_DESC *desc) final;
};

#endif /* BUFFEROBJECT_HPP */
