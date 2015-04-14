#ifndef TEXTURECUBE_HPP
#define TEXTURECUBE_HPP

#include <atomic>
#include <vector>
#include <array>
#include <d3d9.h>

#include "glew.h"
#include "misc.hpp"


struct GLFormatInfo;
class D3DGLDevice;
class D3DGLCubeSurface;

class D3DGLCubeTexture : public IDirect3DCubeTexture9 {
    std::atomic<ULONG> mRefCount;
    std::atomic<ULONG> mIfaceCount;

    ref_ptr<D3DGLDevice> mParent;

    const GLFormatInfo *mGLFormat;
    bool mIsCompressed;
    GLuint mTexId;
    std::vector<GLubyte> mSysMem;

    std::array<RECT,6> mDirtyRect;
    std::atomic<ULONG> mUpdateInProgress;

    D3DSURFACE_DESC mDesc;
    std::vector<std::array<D3DGLCubeSurface*,6>> mSurfaces;
    std::atomic<DWORD> mLodLevel;

    void addIface();
    void releaseIface();

    friend class D3DGLCubeSurface;

public:
    D3DGLCubeTexture(D3DGLDevice *parent);
    virtual ~D3DGLCubeTexture();

    bool init(const D3DSURFACE_DESC *desc, UINT levels);
    void updateTexture(DWORD level, GLint facenum, const RECT &rect, const GLubyte *dataPtr);

    const D3DSURFACE_DESC &getDesc() const { return mDesc; }
    GLuint getTextureId() const { return mTexId; }
    const GLFormatInfo &getFormat() const { return *mGLFormat; }

    void initGL();
    void deinitGL();
    void genMipmapGL();
    void loadTexLevelGL(DWORD level, GLint facenum, const RECT &rect, const GLubyte *dataPtr);

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
    /*** IDirect3DBaseTexture9 methods ***/
    virtual DWORD WINAPI SetLOD(DWORD lod) final;
    virtual DWORD WINAPI GetLOD() final;
    virtual DWORD WINAPI GetLevelCount() final;
    virtual HRESULT WINAPI SetAutoGenFilterType(D3DTEXTUREFILTERTYPE type) final;
    virtual D3DTEXTUREFILTERTYPE WINAPI GetAutoGenFilterType() final;
    virtual void WINAPI GenerateMipSubLevels() final;
    /*** IDirect3DCubeTexture9 methods ***/
    virtual HRESULT WINAPI GetLevelDesc(UINT level, D3DSURFACE_DESC *desc) final;
    virtual HRESULT WINAPI GetCubeMapSurface(D3DCUBEMAP_FACES face, UINT level, IDirect3DSurface9 **surface) final;
    virtual HRESULT WINAPI LockRect(D3DCUBEMAP_FACES face, UINT level, D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags) final;
    virtual HRESULT WINAPI UnlockRect(D3DCUBEMAP_FACES face, UINT level) final;
    virtual HRESULT WINAPI AddDirtyRect(D3DCUBEMAP_FACES face, CONST RECT *dirtyRect) final;
};


class D3DGLCubeSurface : public IDirect3DSurface9 {
    std::atomic<ULONG> mRefCount;

    D3DGLCubeTexture *mParent;
    UINT mLevel;
    GLint mFaceNum;

    enum LockType {
        LT_Unlocked,
        LT_ReadOnly,
        LT_Full
    };
    std::atomic<LockType> mLock;
    RECT mLockRegion;

    UINT mDataOffset;
    UINT mDataLength;

    GLubyte *mScratchMem;

public:
    D3DGLCubeSurface(D3DGLCubeTexture *parent, UINT level, GLint facenum);
    virtual ~D3DGLCubeSurface();

    void init(UINT offset, UINT length);
    D3DGLCubeTexture *getParent() { return mParent; }
    const GLFormatInfo &getFormat() const { return mParent->getFormat(); }
    UINT getLevel() const { return mLevel; }
    GLenum getTarget() const;
    UINT getDataLength() const { return mDataLength; }

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

#endif /* TEXTURECUBE_HPP */
