
#include "d3dgldevice.hpp"

#include "trace.hpp"
#include "d3dgl.hpp"


Direct3DGLDevice::Direct3DGLDevice(Direct3DGL *parent)
  : mRefCount(0)
  , mParent(parent)
{
    mParent->AddRef();
}

Direct3DGLDevice::~Direct3DGLDevice()
{
    mParent->Release();
    mParent = nullptr;
}

bool Direct3DGLDevice::init()
{
    return false;
}


HRESULT Direct3DGLDevice::QueryInterface(const IID &riid, void **obj)
{
    TRACE("iface %p, riid %s, out %p.\n", this, (const char*)debugstr_guid(riid), obj);

    if(riid == IID_IDirect3DDevice9 || riid == IID_IUnknown)
    {
        AddRef();
        *obj = static_cast<IDirect3DDevice9*>(this);
        return S_OK;
    }

    if(riid == IID_IDirect3DDevice9Ex)
    {
        WARN("IDirect3D9 instance wasn't created with CreateDirect3D9Ex, returning E_NOINTERFACE.\n");
        *obj = NULL;
        return E_NOINTERFACE;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", (const char*)debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

ULONG Direct3DGLDevice::AddRef(void)
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG Direct3DGLDevice::Release(void)
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT Direct3DGLDevice::TestCooperativeLevel()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

UINT Direct3DGLDevice::GetAvailableTextureMem()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

HRESULT Direct3DGLDevice::EvictManagedResources()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetDirect3D(IDirect3D9 **d3d9)
{
    TRACE("iface %p, d3d9 %p\n", this, d3d9);
    *d3d9 = mParent;
    (*d3d9)->AddRef();
    return D3D_OK;
}

HRESULT Direct3DGLDevice::GetDeviceCaps(D3DCAPS9* pCaps)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

void Direct3DGLDevice::SetCursorPosition(int X,int Y, DWORD Flags)
{
    FIXME("iface %p : stub!\n", this);
}

WINBOOL Direct3DGLDevice::ShowCursor(WINBOOL bShow)
{
    FIXME("iface %p : stub!\n", this);
    return FALSE;
}

HRESULT Direct3DGLDevice::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

UINT Direct3DGLDevice::GetNumberOfSwapChains()
{
    FIXME("iface %p : stub!\n", this);
    return 0;
}

HRESULT Direct3DGLDevice::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetDialogBoxMode(WINBOOL bEnableDialogs)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

void Direct3DGLDevice::SetGammaRamp(UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp)
{
    FIXME("iface %p : stub!\n", this);
}

void Direct3DGLDevice::GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp)
{
    FIXME("iface %p : stub!\n", this);
}

HRESULT Direct3DGLDevice::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::UpdateSurface(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::StretchRect(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::ColorFill(IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::BeginScene()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::EndScene()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetViewport(CONST D3DVIEWPORT9* pViewport)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetViewport(D3DVIEWPORT9* pViewport)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetMaterial(CONST D3DMATERIAL9* pMaterial)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetMaterial(D3DMATERIAL9* pMaterial)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetLight(DWORD Index, CONST D3DLIGHT9*)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetLight(DWORD Index, D3DLIGHT9*)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::LightEnable(DWORD Index, WINBOOL Enable)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetLightEnable(DWORD Index, WINBOOL* pEnable)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetClipPlane(DWORD Index, CONST float* pPlane)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetClipPlane(DWORD Index, float* pPlane)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::BeginStateBlock()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::EndStateBlock(IDirect3DStateBlock9** ppSB)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetClipStatus(D3DCLIPSTATUS9* pClipStatus)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::ValidateDevice(DWORD* pNumPasses)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetCurrentTexturePalette(UINT PaletteNumber)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetCurrentTexturePalette(UINT *PaletteNumber)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetScissorRect(CONST RECT* pRect)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetScissorRect(RECT* pRect)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetSoftwareVertexProcessing(WINBOOL bSoftware)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

WINBOOL Direct3DGLDevice::GetSoftwareVertexProcessing()
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetNPatchMode(float nSegments)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

float Direct3DGLDevice::GetNPatchMode()
{
    FIXME("iface %p : stub!\n", this);
    return 0.0f;
}

HRESULT Direct3DGLDevice::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetFVF(DWORD FVF)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetFVF(DWORD* pFVF)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateVertexShader(CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetVertexShader(IDirect3DVertexShader9* pShader)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetVertexShader(IDirect3DVertexShader9** ppShader)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetVertexShaderConstantB(UINT StartRegister, CONST WINBOOL* pConstantData, UINT  BoolCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetVertexShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetStreamSourceFreq(UINT StreamNumber, UINT Divider)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetStreamSourceFreq(UINT StreamNumber, UINT* Divider)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetIndices(IDirect3DIndexBuffer9* pIndexData)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetIndices(IDirect3DIndexBuffer9** ppIndexData)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreatePixelShader(CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetPixelShader(IDirect3DPixelShader9* pShader)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetPixelShader(IDirect3DPixelShader9** ppShader)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::SetPixelShaderConstantB(UINT StartRegister, CONST WINBOOL* pConstantData, UINT  BoolCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::GetPixelShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::DeletePatch(UINT Handle)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}

HRESULT Direct3DGLDevice::CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)
{
    FIXME("iface %p : stub!\n", this);
    return E_NOTIMPL;
}
