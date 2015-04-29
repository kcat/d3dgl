#ifndef PLAINSURFACE_HPP
#define PLAINSURFACE_HPP

#include <atomic>
#include <memory>
#include <d3d9.h>

#include "glew.h"


struct GLFormatInfo;
class D3DGLDevice;

class D3DGLPlainSurface : public IDirect3DSurface9 {
    std::atomic<ULONG> mRefCount;

    D3DGLDevice *mParent;

    const GLFormatInfo *mGLFormat;
    D3DSURFACE_DESC mDesc;
    bool mIsCompressed;

    std::shared_ptr<GLubyte> mBufData;

    enum LockType {
        LT_Unlocked,
        LT_ReadOnly,
        LT_Full
    };
    std::atomic<LockType> mLock;
    RECT mLockRegion;

public:
    D3DGLPlainSurface(D3DGLDevice *parent);
    virtual ~D3DGLPlainSurface();

    bool init(const D3DSURFACE_DESC *desc);

    const D3DSURFACE_DESC &getDesc() const { return mDesc; }
    const GLFormatInfo &getFormat() const { return *mGLFormat; }

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

#endif /* PLAINSURFACE_HPP */
