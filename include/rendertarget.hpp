#ifndef RENDERTARGET_HPP
#define RENDERTARGET_HPP

#include <atomic>
#include <d3d9.h>


class Direct3DGLDevice;

class Direct3DGLRenderTarget : public IDirect3DSurface9 {
    std::atomic<ULONG> mRefCount;

    Direct3DGLDevice *mParent;

    D3DSURFACE_DESC mDesc;
    bool mIsAuto;

public:
    Direct3DGLRenderTarget(Direct3DGLDevice *parent);
    virtual ~Direct3DGLRenderTarget();

    bool init(const D3DSURFACE_DESC *desc, bool isauto=false);

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
    /*** IDirect3DSurface9 methods ***/
    virtual HRESULT WINAPI GetContainer(REFIID riid, void **container);
    virtual HRESULT WINAPI GetDesc(D3DSURFACE_DESC *desc);
    virtual HRESULT WINAPI LockRect(D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags);
    virtual HRESULT WINAPI UnlockRect();
    virtual HRESULT WINAPI GetDC(HDC *hdc);
    virtual HRESULT WINAPI ReleaseDC(HDC hdc);
};

#endif /* RENDERTARGET_HPP */
