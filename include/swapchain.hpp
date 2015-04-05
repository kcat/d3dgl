#ifndef D3DGLSWAPCHAIN_HPP
#define D3DGLSWAPCHAIN_HPP

#include <atomic>
#include <vector>
#include <d3d9.h>


class D3DGLDevice;
class D3DGLBackbufferSurface;

class D3DGLSwapChain : public IDirect3DSwapChain9 {
    std::atomic<ULONG> mRefCount;
    std::atomic<ULONG> mIfaceCount;

    D3DGLDevice *mParent;

    std::vector<D3DGLBackbufferSurface*> mBackbuffers;
    D3DPRESENT_PARAMETERS mParams;
    HWND mWindow;
    HDC mDevCtx;
    bool mIsAuto;

    std::atomic<ULONG> mPendingSwaps;

    void addIface();
    void releaseIface();

    friend class D3DGLBackbufferSurface;

public:
    D3DGLSwapChain(D3DGLDevice *parent);
    virtual ~D3DGLSwapChain();

    void swapBuffersGL();

    bool init(const D3DPRESENT_PARAMETERS *params, HWND window, bool isauto=false);

    D3DGLBackbufferSurface *getBackbuffer() { return mBackbuffers[0]; }
    ULONG getPendingSwaps() const { return mPendingSwaps; }

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj) final;
    virtual ULONG WINAPI AddRef() final;
    virtual ULONG WINAPI Release() final;
    /*** IDirect3DSwapChain9 methods ***/
    virtual HRESULT WINAPI Present(CONST RECT *srcRect, CONST RECT *dstRect, HWND dstWindowOverride, CONST RGNDATA *dirtyRegion, DWORD flags) final;
    virtual HRESULT WINAPI GetFrontBufferData(IDirect3DSurface9 *dstSurface) final;
    virtual HRESULT WINAPI GetBackBuffer(UINT backbuffer, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **out) final;
    virtual HRESULT WINAPI GetRasterStatus(D3DRASTER_STATUS *status) final;
    virtual HRESULT WINAPI GetDisplayMode(D3DDISPLAYMODE *mode) final;
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device) final;
    virtual HRESULT WINAPI GetPresentParameters(D3DPRESENT_PARAMETERS *params) final;
};


class D3DGLBackbufferSurface : public IDirect3DSurface9 {
    std::atomic<ULONG> mRefCount;

    D3DGLSwapChain *mParent;

public:
    D3DGLBackbufferSurface(D3DGLSwapChain *parent);
    virtual ~D3DGLBackbufferSurface();

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

#endif /* D3DGLSWAPCHAIN_HPP */
