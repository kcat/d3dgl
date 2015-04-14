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
class D3DGLBufferObject;
class D3DGLVertexShader;
class D3DGLPixelShader;
class D3DGLVertexDeclaration;

#define VSF_BINDING_IDX 0
#define VSI_BINDING_IDX 1
#define VSB_BINDING_IDX 2
#define PSF_BINDING_IDX 3
#define PSI_BINDING_IDX 4
#define PSB_BINDING_IDX 5
#define POSFIXUP_BINDING_IDX 6

union Vector4f {
    float value[4];
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    float *ptr() { return &value[0]; };
    const float *ptr() const { return &value[0]; };
};
static_assert(sizeof(Vector4f)==sizeof(float[4]), "Bad Vector4f size");

class D3DGLDevice : public IDirect3DDevice9 {
public:
    struct GLStreamData {
        D3DGLBufferObject *mBuffer;
        GLubyte *mPointer;
        GLenum mGLType;
        GLint mGLCount;
        GLenum mNormalize;
        GLsizei mStride;
        GLint mTarget;
        GLsizei mIndex;
        GLuint mDivisor;
    };
    struct GLIndexData {
        D3DGLBufferObject *mBuffer;
        GLubyte *mPointer;
        GLenum mMode;
        GLenum mType;
        GLint mCount;
    };

private:
    std::atomic<ULONG> mRefCount;

    ref_ptr<Direct3DGL> mParent;

    const D3DAdapter &mAdapter;

    HGLRC mGLContext;
    struct {
        std::array<GLuint,MAX_COMBINED_SAMPLERS> samplers;
        GLuint pipeline;

        GLuint main_framebuffer;     // Used for offscreen rendering
        GLuint copy_framebuffers[2]; // Used for FBO blits (0=read, 1=draw)

        GLuint vs_uniform_bufferf;
        GLuint ps_uniform_bufferf;
        GLuint pos_fixup_uniform_buffer;

        GLenum active_stage;
        std::array<GLenum,MAX_COMBINED_SAMPLERS> sampler_type;
        std::array<GLuint,MAX_COMBINED_SAMPLERS> sampler_binding;

        bool vertex_array_enabled;
        bool normal_array_enabled;
        bool color_array_enabled;
        bool specular_array_enabled;
        UINT texcoord_array_enabled; // Bitmask, 1<<texture_stage

        UINT attrib_array_enabled; // Bitmask, 1<<attrib

        UINT clip_plane_enabled; // Bitmask, 1<<plane_index
    } mGLState;

    enum FFPSelector {
        UseShaders=0,
        UseFFP=1
    };

    CommandQueue mQueue;

    const HWND mWindow;
    const DWORD mFlags;

    D3DGLRenderTarget *mAutoDepthStencil;
    std::array<D3DGLSwapChain*,1> mSwapchains;

    std::array<std::atomic<IDirect3DSurface9*>,D3D_MAX_SIMULTANEOUS_RENDERTARGETS> mRenderTargets;
    std::atomic<IDirect3DSurface9*> mDepthStencil;

    std::array<std::atomic<IDirect3DBaseTexture9*>,MAX_COMBINED_SAMPLERS> mTextures;

    typedef std::array<std::atomic<DWORD>,33> TexStageStates;
    typedef std::array<std::atomic<DWORD>,14> SamplerStates;

    std::array<TexStageStates,MAX_TEXTURES> mTexStageState;
    std::array<SamplerStates,MAX_COMBINED_SAMPLERS> mSamplerState;
    std::array<std::atomic<DWORD>,210> mRenderState;
    D3DVIEWPORT9 mViewport;
    D3DMATERIAL9 mMaterial;
    std::array<Vector4f,8> mClipPlane;
    std::atomic<bool> mInScene;

    std::array<Vector4f,256> mVSConstantsF;
    std::array<Vector4f,256> mPSConstantsF;

    std::atomic<D3DGLVertexShader*> mVertexShader;
    std::atomic<D3DGLPixelShader*> mPixelShader;

    std::atomic<D3DGLVertexDeclaration*> mVertexDecl;

    struct StreamSource {
        D3DGLBufferObject *mBuffer;
        UINT mOffset;
        UINT mStride;
        UINT mFreq;
        StreamSource() : mBuffer(0), mOffset(0), mStride(0), mFreq(1) { }
    };
    std::array<StreamSource,MAX_STREAMS> mStreams;
    std::atomic<D3DGLBufferObject*> mIndexBuffer;

    D3DGLBufferObject *mPrimitiveUserData;

    // Sends buffer values to update proj_fixup_uniform_buffer. Caller is
    // responsible for holding the mQueue lock.
    void resetProjectionFixup(UINT width, UINT height);

    // HACK: This should be GLAPIENTRY, but under Wine the callback is passed
    // as-is to the host. Windows expects GLAPIENTRY to be stdcall, while Linux
    // expects GLAPIENTRY to be cdecl, so the function is called improperly by
    // the host GL.
    static void __cdecl debugProcWrapGL(GLenum source, GLenum type, GLuint id, GLenum severity,
                                        GLsizei length, const GLchar *message, const GLvoid *user)
    { static_cast<const D3DGLDevice*>(user)->debugProcGL(source, type, id, severity, length, message); }
    void GLAPIENTRY debugProcGL(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar *message) const;

    HRESULT drawVtxDecl(GLenum mode, INT startvtx, UINT minvtx, UINT startidx, UINT count, bool use_indices,
                        bool user_vtxdata);

public:
    D3DGLDevice(Direct3DGL *parent, const D3DAdapter &adapter, HWND window, DWORD flags);
    virtual ~D3DGLDevice();

    bool init(D3DPRESENT_PARAMETERS *params);

    const D3DAdapter &getAdapter() const { return mAdapter; }
    CommandQueue &getQueue() { return mQueue; }

    void initGL(HDC dc, HGLRC glcontext);
    void deinitGL();
    void clearGL(GLbitfield mask, GLuint color, GLfloat depth, GLuint stencil, const RECT &rect);
    void setTextureGL(GLuint stage, GLenum type, GLuint binding);
    void setClipPlanesGL(UINT planes);
    void setFBAttachmentGL(GLenum attachment, GLenum target, GLuint id, GLint level);
    void setVertexArrayStateGL(bool vertex, bool normal, bool color, bool specular, UINT texcoord);
    void setVertexAttribArrayGL(UINT attribs);
    void setShaderProgramGL(GLbitfield stages, GLuint program);
    void setVtxDataGL(const D3DGLDevice::GLStreamData *streams, GLuint numstreams, bool ffp);
    void blitFramebufferGL(GLenum src_target, GLuint src_binding, GLint src_level, const RECT &src_rect,
                           GLenum dst_target, GLuint dst_binding, GLint dst_level, const RECT &dst_rect,
                           GLenum filter);

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj) final;
    virtual ULONG WINAPI AddRef() final;
    virtual ULONG WINAPI Release() final;
    /*** IDirect3DDevice9 methods ***/
    virtual HRESULT WINAPI TestCooperativeLevel() final;
    virtual UINT WINAPI GetAvailableTextureMem() final;
    virtual HRESULT WINAPI EvictManagedResources() final;
    virtual HRESULT WINAPI GetDirect3D(IDirect3D9 **d3d9) final;
    virtual HRESULT WINAPI GetDeviceCaps(D3DCAPS9 *caps) final;
    virtual HRESULT WINAPI GetDisplayMode(UINT swapchain, D3DDISPLAYMODE *mode) final;
    virtual HRESULT WINAPI GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *params) final;
    virtual HRESULT WINAPI SetCursorProperties(UINT xoffset, UINT yoffset, IDirect3DSurface9 *cursor) final;
    virtual void WINAPI SetCursorPosition(int x, int y, DWORD flags) final;
    virtual WINBOOL WINAPI ShowCursor(WINBOOL show) final;
    virtual HRESULT WINAPI CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* params, IDirect3DSwapChain9 **schain) final;
    virtual HRESULT WINAPI GetSwapChain(UINT swapchain, IDirect3DSwapChain9 **schain) final;
    virtual UINT WINAPI GetNumberOfSwapChains() final;
    virtual HRESULT WINAPI Reset(D3DPRESENT_PARAMETERS *params) final;
    virtual HRESULT WINAPI Present(const RECT *srcRect, const RECT *dstRect, HWND dstWindowOverride, const RGNDATA *dirtyRegion) final;
    virtual HRESULT WINAPI GetBackBuffer(UINT swapchain, UINT backbuffer, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **bbuffer) final;
    virtual HRESULT WINAPI GetRasterStatus(UINT swapchain, D3DRASTER_STATUS *status) final;
    virtual HRESULT WINAPI SetDialogBoxMode(WINBOOL enable) final;
    virtual void WINAPI SetGammaRamp(UINT swapchain, DWORD Flags, const D3DGAMMARAMP *ramp) final;
    virtual void WINAPI GetGammaRamp(UINT swapchain, D3DGAMMARAMP *ramp) final;
    virtual HRESULT WINAPI CreateTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9 **texture, HANDLE *handle) final;
    virtual HRESULT WINAPI CreateVolumeTexture(UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture9 **texture, HANDLE *handle) final;
    virtual HRESULT WINAPI CreateCubeTexture(UINT edgeLength, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture9 **texture, HANDLE *handle) final;
    virtual HRESULT WINAPI CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9 **vbuffer, HANDLE *handle) final;
    virtual HRESULT WINAPI CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9 **ibuffer, HANDLE *handle) final;
    virtual HRESULT WINAPI CreateRenderTarget(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisampleQuality, WINBOOL lockable, IDirect3DSurface9 **surface, HANDLE *handle) final;
    virtual HRESULT WINAPI CreateDepthStencilSurface(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisampleQuality, WINBOOL discard, IDirect3DSurface9 **surface, HANDLE *handle) final;
    virtual HRESULT WINAPI UpdateSurface(IDirect3DSurface9* pSourceSurface, const RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, const POINT* pDestPoint) final;
    virtual HRESULT WINAPI UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) final;
    virtual HRESULT WINAPI GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) final;
    virtual HRESULT WINAPI GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface) final;
    virtual HRESULT WINAPI StretchRect(IDirect3DSurface9* pSourceSurface, const RECT* pSourceRect, IDirect3DSurface9* pDestSurface, const RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) final;
    virtual HRESULT WINAPI ColorFill(IDirect3DSurface9* pSurface, const RECT* pRect, D3DCOLOR color) final;
    virtual HRESULT WINAPI CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) final;
    virtual HRESULT WINAPI SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) final;
    virtual HRESULT WINAPI GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) final;
    virtual HRESULT WINAPI SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil) final;
    virtual HRESULT WINAPI GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) final;
    virtual HRESULT WINAPI BeginScene() final;
    virtual HRESULT WINAPI EndScene() final;
    virtual HRESULT WINAPI Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) final;
    virtual HRESULT WINAPI SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) final;
    virtual HRESULT WINAPI GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) final;
    virtual HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE, const D3DMATRIX*) final;
    virtual HRESULT WINAPI SetViewport(const D3DVIEWPORT9 *viewport) final;
    virtual HRESULT WINAPI GetViewport(D3DVIEWPORT9 *viewport) final;
    virtual HRESULT WINAPI SetMaterial(const D3DMATERIAL9* material) final;
    virtual HRESULT WINAPI GetMaterial(D3DMATERIAL9* material) final;
    virtual HRESULT WINAPI SetLight(DWORD Index, const D3DLIGHT9*) final;
    virtual HRESULT WINAPI GetLight(DWORD Index, D3DLIGHT9*) final;
    virtual HRESULT WINAPI LightEnable(DWORD Index, WINBOOL Enable) final;
    virtual HRESULT WINAPI GetLightEnable(DWORD Index, WINBOOL* pEnable) final;
    virtual HRESULT WINAPI SetClipPlane(DWORD Index, const float* pPlane) final;
    virtual HRESULT WINAPI GetClipPlane(DWORD Index, float* pPlane) final;
    virtual HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) final;
    virtual HRESULT WINAPI GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) final;
    virtual HRESULT WINAPI CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) final;
    virtual HRESULT WINAPI BeginStateBlock() final;
    virtual HRESULT WINAPI EndStateBlock(IDirect3DStateBlock9** ppSB) final;
    virtual HRESULT WINAPI SetClipStatus(const D3DCLIPSTATUS9* pClipStatus) final;
    virtual HRESULT WINAPI GetClipStatus(D3DCLIPSTATUS9* pClipStatus) final;
    virtual HRESULT WINAPI GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) final;
    virtual HRESULT WINAPI SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) final;
    virtual HRESULT WINAPI GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) final;
    virtual HRESULT WINAPI SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) final;
    virtual HRESULT WINAPI GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) final;
    virtual HRESULT WINAPI SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) final;
    virtual HRESULT WINAPI ValidateDevice(DWORD* pNumPasses) final;
    virtual HRESULT WINAPI SetPaletteEntries(UINT PaletteNumber, const PALETTEENTRY* pEntries) final;
    virtual HRESULT WINAPI GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries) final;
    virtual HRESULT WINAPI SetCurrentTexturePalette(UINT PaletteNumber) final;
    virtual HRESULT WINAPI GetCurrentTexturePalette(UINT *PaletteNumber) final;
    virtual HRESULT WINAPI SetScissorRect(const RECT* pRect) final;
    virtual HRESULT WINAPI GetScissorRect(RECT* pRect) final;
    virtual HRESULT WINAPI SetSoftwareVertexProcessing(WINBOOL bSoftware) final;
    virtual WINBOOL WINAPI GetSoftwareVertexProcessing() final;
    virtual HRESULT WINAPI SetNPatchMode(float nSegments) final;
    virtual float WINAPI GetNPatchMode() final;
    virtual HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) final;
    virtual HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) final;
    virtual HRESULT WINAPI DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) final;
    virtual HRESULT WINAPI DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) final;
    virtual HRESULT WINAPI ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) final;
    virtual HRESULT WINAPI CreateVertexDeclaration(const D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) final;
    virtual HRESULT WINAPI SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) final;
    virtual HRESULT WINAPI GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) final;
    virtual HRESULT WINAPI SetFVF(DWORD FVF) final;
    virtual HRESULT WINAPI GetFVF(DWORD* pFVF) final;
    virtual HRESULT WINAPI CreateVertexShader(const DWORD* pFunction, IDirect3DVertexShader9** ppShader) final;
    virtual HRESULT WINAPI SetVertexShader(IDirect3DVertexShader9* pShader) final;
    virtual HRESULT WINAPI GetVertexShader(IDirect3DVertexShader9** ppShader) final;
    virtual HRESULT WINAPI SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) final;
    virtual HRESULT WINAPI GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) final;
    virtual HRESULT WINAPI SetVertexShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount) final;
    virtual HRESULT WINAPI GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) final;
    virtual HRESULT WINAPI SetVertexShaderConstantB(UINT StartRegister, const WINBOOL* pConstantData, UINT  BoolCount) final;
    virtual HRESULT WINAPI GetVertexShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount) final;
    virtual HRESULT WINAPI SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) final;
    virtual HRESULT WINAPI GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride) final;
    virtual HRESULT WINAPI SetStreamSourceFreq(UINT StreamNumber, UINT Divider) final;
    virtual HRESULT WINAPI GetStreamSourceFreq(UINT StreamNumber, UINT* Divider) final;
    virtual HRESULT WINAPI SetIndices(IDirect3DIndexBuffer9* pIndexData) final;
    virtual HRESULT WINAPI GetIndices(IDirect3DIndexBuffer9** ppIndexData) final;
    virtual HRESULT WINAPI CreatePixelShader(const DWORD* pFunction, IDirect3DPixelShader9** ppShader) final;
    virtual HRESULT WINAPI SetPixelShader(IDirect3DPixelShader9* pShader) final;
    virtual HRESULT WINAPI GetPixelShader(IDirect3DPixelShader9** ppShader) final;
    virtual HRESULT WINAPI SetPixelShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) final;
    virtual HRESULT WINAPI GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) final;
    virtual HRESULT WINAPI SetPixelShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount) final;
    virtual HRESULT WINAPI GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) final;
    virtual HRESULT WINAPI SetPixelShaderConstantB(UINT StartRegister, const WINBOOL* pConstantData, UINT  BoolCount) final;
    virtual HRESULT WINAPI GetPixelShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount) final;
    virtual HRESULT WINAPI DrawRectPatch(UINT handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo) final;
    virtual HRESULT WINAPI DrawTriPatch(UINT handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo) final;
    virtual HRESULT WINAPI DeletePatch(UINT handle) final;
    virtual HRESULT WINAPI CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9 **query) final;
};

#endif /* D3DGLDEVICE_HPP */
