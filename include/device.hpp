#ifndef D3DGLDEVICE_HPP
#define D3DGLDEVICE_HPP

#include <d3d9.h>

#include <atomic>
#include <array>

#include "d3dgl.hpp"
#include "misc.hpp"
#include "commandqueue.hpp"


class D3DGLSwapChain;
class D3DGLRenderTarget;

class D3DGLDevice : public IDirect3DDevice9 {
    std::atomic<ULONG> mRefCount;

    ref_ptr<Direct3DGL> mParent;

    const D3DAdapter &mAdapter;

    HGLRC mGLContext;

    CommandQueue mQueue;

    const HWND mWindow;
    const DWORD mFlags;

    D3DGLRenderTarget *mAutoDepthStencil;
    std::array<D3DGLSwapChain*,1> mSwapchains;

    std::array<std::atomic<IDirect3DSurface9*>,D3D_MAX_SIMULTANEOUS_RENDERTARGETS> mRenderTargets;
    std::atomic<IDirect3DSurface9*> mDepthStencil;

    std::array<std::atomic<DWORD>,210> mRenderState;
    D3DMATERIAL9 mMaterial;
    std::atomic<bool> mInScene;

public:
    D3DGLDevice(Direct3DGL *parent, const D3DAdapter &adapter, HWND window, DWORD flags);
    virtual ~D3DGLDevice();

    bool init(D3DPRESENT_PARAMETERS *params);

    const D3DAdapter &getAdapter() const { return mAdapter; }
    CommandQueue &getQueue() { return mQueue; }

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();
    /*** IDirect3DDevice9 methods ***/
    virtual HRESULT WINAPI TestCooperativeLevel();
    virtual UINT WINAPI GetAvailableTextureMem();
    virtual HRESULT WINAPI EvictManagedResources();
    virtual HRESULT WINAPI GetDirect3D(IDirect3D9** ppD3D9);
    virtual HRESULT WINAPI GetDeviceCaps(D3DCAPS9* pCaps);
    virtual HRESULT WINAPI GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode);
    virtual HRESULT WINAPI GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters);
    virtual HRESULT WINAPI SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap);
    virtual void WINAPI SetCursorPosition(int X,int Y, DWORD Flags);
    virtual WINBOOL WINAPI ShowCursor(WINBOOL bShow);
    virtual HRESULT WINAPI CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain);
    virtual HRESULT WINAPI GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain);
    virtual UINT WINAPI GetNumberOfSwapChains();
    virtual HRESULT WINAPI Reset(D3DPRESENT_PARAMETERS* pPresentationParameters);
    virtual HRESULT WINAPI Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
    virtual HRESULT WINAPI GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer);
    virtual HRESULT WINAPI GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus);
    virtual HRESULT WINAPI SetDialogBoxMode(WINBOOL bEnableDialogs);
    virtual void WINAPI SetGammaRamp(UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp);
    virtual void WINAPI GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp);
    virtual HRESULT WINAPI CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle);
    virtual HRESULT WINAPI CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle);
    virtual HRESULT WINAPI CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle);
    virtual HRESULT WINAPI CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle);
    virtual HRESULT WINAPI CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle);
    virtual HRESULT WINAPI CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
    virtual HRESULT WINAPI CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
    virtual HRESULT WINAPI UpdateSurface(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint);
    virtual HRESULT WINAPI UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture);
    virtual HRESULT WINAPI GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface);
    virtual HRESULT WINAPI GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface);
    virtual HRESULT WINAPI StretchRect(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);
    virtual HRESULT WINAPI ColorFill(IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color);
    virtual HRESULT WINAPI CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
    virtual HRESULT WINAPI SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget);
    virtual HRESULT WINAPI GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget);
    virtual HRESULT WINAPI SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil);
    virtual HRESULT WINAPI GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface);
    virtual HRESULT WINAPI BeginScene();
    virtual HRESULT WINAPI EndScene();
    virtual HRESULT WINAPI Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
    virtual HRESULT WINAPI SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
    virtual HRESULT WINAPI GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
    virtual HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
    virtual HRESULT WINAPI SetViewport(CONST D3DVIEWPORT9* pViewport);
    virtual HRESULT WINAPI GetViewport(D3DVIEWPORT9* pViewport);
    virtual HRESULT WINAPI SetMaterial(const D3DMATERIAL9* material);
    virtual HRESULT WINAPI GetMaterial(D3DMATERIAL9* material);
    virtual HRESULT WINAPI SetLight(DWORD Index, CONST D3DLIGHT9*);
    virtual HRESULT WINAPI GetLight(DWORD Index, D3DLIGHT9*);
    virtual HRESULT WINAPI LightEnable(DWORD Index, WINBOOL Enable);
    virtual HRESULT WINAPI GetLightEnable(DWORD Index, WINBOOL* pEnable);
    virtual HRESULT WINAPI SetClipPlane(DWORD Index, CONST float* pPlane);
    virtual HRESULT WINAPI GetClipPlane(DWORD Index, float* pPlane);
    virtual HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE State, DWORD Value);
    virtual HRESULT WINAPI GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue);
    virtual HRESULT WINAPI CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB);
    virtual HRESULT WINAPI BeginStateBlock();
    virtual HRESULT WINAPI EndStateBlock(IDirect3DStateBlock9** ppSB);
    virtual HRESULT WINAPI SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus);
    virtual HRESULT WINAPI GetClipStatus(D3DCLIPSTATUS9* pClipStatus);
    virtual HRESULT WINAPI GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture);
    virtual HRESULT WINAPI SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture);
    virtual HRESULT WINAPI GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
    virtual HRESULT WINAPI SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
    virtual HRESULT WINAPI GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue);
    virtual HRESULT WINAPI SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
    virtual HRESULT WINAPI ValidateDevice(DWORD* pNumPasses);
    virtual HRESULT WINAPI SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries);
    virtual HRESULT WINAPI GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries);
    virtual HRESULT WINAPI SetCurrentTexturePalette(UINT PaletteNumber);
    virtual HRESULT WINAPI GetCurrentTexturePalette(UINT *PaletteNumber);
    virtual HRESULT WINAPI SetScissorRect(CONST RECT* pRect);
    virtual HRESULT WINAPI GetScissorRect(RECT* pRect);
    virtual HRESULT WINAPI SetSoftwareVertexProcessing(WINBOOL bSoftware);
    virtual WINBOOL WINAPI GetSoftwareVertexProcessing();
    virtual HRESULT WINAPI SetNPatchMode(float nSegments);
    virtual float WINAPI GetNPatchMode();
    virtual HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
    virtual HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
    virtual HRESULT WINAPI DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
    virtual HRESULT WINAPI DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
    virtual HRESULT WINAPI ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags);
    virtual HRESULT WINAPI CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl);
    virtual HRESULT WINAPI SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl);
    virtual HRESULT WINAPI GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl);
    virtual HRESULT WINAPI SetFVF(DWORD FVF);
    virtual HRESULT WINAPI GetFVF(DWORD* pFVF);
    virtual HRESULT WINAPI CreateVertexShader(CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader);
    virtual HRESULT WINAPI SetVertexShader(IDirect3DVertexShader9* pShader);
    virtual HRESULT WINAPI GetVertexShader(IDirect3DVertexShader9** ppShader);
    virtual HRESULT WINAPI SetVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount);
    virtual HRESULT WINAPI GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount);
    virtual HRESULT WINAPI SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount);
    virtual HRESULT WINAPI GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount);
    virtual HRESULT WINAPI SetVertexShaderConstantB(UINT StartRegister, CONST WINBOOL* pConstantData, UINT  BoolCount);
    virtual HRESULT WINAPI GetVertexShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount);
    virtual HRESULT WINAPI SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride);
    virtual HRESULT WINAPI GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride);
    virtual HRESULT WINAPI SetStreamSourceFreq(UINT StreamNumber, UINT Divider);
    virtual HRESULT WINAPI GetStreamSourceFreq(UINT StreamNumber, UINT* Divider);
    virtual HRESULT WINAPI SetIndices(IDirect3DIndexBuffer9* pIndexData);
    virtual HRESULT WINAPI GetIndices(IDirect3DIndexBuffer9** ppIndexData);
    virtual HRESULT WINAPI CreatePixelShader(CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader);
    virtual HRESULT WINAPI SetPixelShader(IDirect3DPixelShader9* pShader);
    virtual HRESULT WINAPI GetPixelShader(IDirect3DPixelShader9** ppShader);
    virtual HRESULT WINAPI SetPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount);
    virtual HRESULT WINAPI GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount);
    virtual HRESULT WINAPI SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount);
    virtual HRESULT WINAPI GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount);
    virtual HRESULT WINAPI SetPixelShaderConstantB(UINT StartRegister, CONST WINBOOL* pConstantData, UINT  BoolCount);
    virtual HRESULT WINAPI GetPixelShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount);
    virtual HRESULT WINAPI DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo);
    virtual HRESULT WINAPI DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo);
    virtual HRESULT WINAPI DeletePatch(UINT Handle);
    virtual HRESULT WINAPI CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery);
};

#endif /* D3DGLDEVICE_HPP */
