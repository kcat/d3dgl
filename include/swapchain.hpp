#ifndef D3DGLSWAPCHAIN_HPP
#define D3DGLSWAPCHAIN_HPP

#include <atomic>
#include <vector>
#include <d3d9.h>

#include "condwait.hpp"


class D3DGLDevice;
class D3DGLRenderTarget;

class D3DGLSwapChain : public IDirect3DSwapChain9 {
    std::atomic<ULONG> mRefCount;
    std::atomic<ULONG> mIfaceCount;

    D3DGLDevice *mParent;

    std::vector<D3DGLRenderTarget*> mBackbuffers;
    D3DPRESENT_PARAMETERS mParams;
    HWND mWindow;
    HDC mDevCtx;
    bool mIsAuto;

    ConditionWaiter mSwapWaiter;
    std::atomic<ULONG> mPendingSwaps;

    void addIface();
    void releaseIface();

public:
    D3DGLSwapChain(D3DGLDevice *parent);
    virtual ~D3DGLSwapChain();

    void swapBuffersGL(size_t backbuffer);

    bool init(const D3DPRESENT_PARAMETERS *params, HWND window, bool isauto=false);

    D3DGLRenderTarget *getBackbuffer() { return mBackbuffers[0]; }
    bool getIsWindowed() const { return mParams.Windowed; }

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

#endif /* D3DGLSWAPCHAIN_HPP */
