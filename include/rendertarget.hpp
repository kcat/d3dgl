#ifndef RENDERTARGET_HPP
#define RENDERTARGET_HPP

#include <atomic>
#include <d3d9.h>

#include "glew.h"


struct GLFormatInfo;
class D3DGLDevice;

class D3DGLRenderTarget : public IDirect3DSurface9 {
    std::atomic<ULONG> mRefCount;

    D3DGLDevice *mParent;

    const GLFormatInfo *mGLFormat;
    D3DSURFACE_DESC mDesc;
    bool mIsAuto;

    GLuint mId;

public:
    D3DGLRenderTarget(D3DGLDevice *parent);
    virtual ~D3DGLRenderTarget();

    bool init(const D3DSURFACE_DESC *desc, bool isauto=false);

    void initGL();

    const D3DSURFACE_DESC &getDesc() const { return mDesc; }
    GLuint getId() const { return mId; }
    GLenum getDepthStencilAttachment() const;

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
    /*** IDirect3DSurface9 methods ***/
    virtual HRESULT WINAPI GetContainer(REFIID riid, void **container) final;
    virtual HRESULT WINAPI GetDesc(D3DSURFACE_DESC *desc) final;
    virtual HRESULT WINAPI LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags) final;
    virtual HRESULT WINAPI UnlockRect() final;
    virtual HRESULT WINAPI GetDC(HDC *hdc) final;
    virtual HRESULT WINAPI ReleaseDC(HDC hdc) final;
};

#endif /* RENDERTARGET_HPP */
