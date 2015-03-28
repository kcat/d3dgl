#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <atomic>
#include <vector>
#include <d3d9.h>

#include "glew.h"
#include "misc.hpp"


struct GLFormatInfo;
class D3DGLDevice;
class D3DGLTextureSurface;

class D3DGLTexture : public IDirect3DTexture9 {
    std::atomic<ULONG> mRefCount;
    std::atomic<ULONG> mIfaceCount;

    ref_ptr<D3DGLDevice> mParent;

    const GLFormatInfo *mGLFormat;
    bool mIsCompressed;
    GLuint mTexId;
    GLuint mPBO;
    std::vector<GLubyte> mSysMem;

    GLubyte *mUserPtr;
    RECT mDirtyRect;
    std::atomic<ULONG> mUpdateInProgress;

    D3DSURFACE_DESC mDesc;
    std::vector<D3DGLTextureSurface*> mSurfaces;
    std::atomic<DWORD> mLodLevel;

    void addIface();
    void releaseIface();

    friend class D3DGLTextureSurface;

public:
    D3DGLTexture(D3DGLDevice *parent);
    virtual ~D3DGLTexture();

    bool init(const D3DSURFACE_DESC *desc, UINT levels);
    void updateTexture(DWORD level, const RECT &rect, GLubyte *dataPtr, bool deletePtr);

    void initGL();
    void deinitGL();
    void setLodGL(DWORD lod);
    void genMipmapGL();
    void loadTexLevelGL(DWORD level, const RECT &rect, GLubyte *dataPtr, bool deletePtr);

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
    /*** IDirect3DBaseTexture9 methods ***/
    virtual DWORD WINAPI SetLOD(DWORD lod);
    virtual DWORD WINAPI GetLOD();
    virtual DWORD WINAPI GetLevelCount();
    virtual HRESULT WINAPI SetAutoGenFilterType(D3DTEXTUREFILTERTYPE type);
    virtual D3DTEXTUREFILTERTYPE WINAPI GetAutoGenFilterType();
    virtual void WINAPI GenerateMipSubLevels();
    /*** IDirect3DTexture9 methods ***/
    virtual HRESULT WINAPI GetLevelDesc(UINT level, D3DSURFACE_DESC *desc);
    virtual HRESULT WINAPI GetSurfaceLevel(UINT level, IDirect3DSurface9 **surface);
    virtual HRESULT WINAPI LockRect(UINT level, D3DLOCKED_RECT *lockedRect, const RECT *rect, DWORD flags);
    virtual HRESULT WINAPI UnlockRect(UINT level);
    virtual HRESULT WINAPI AddDirtyRect(const RECT *rect);
};


class D3DGLTextureSurface : public IDirect3DSurface9 {
    std::atomic<ULONG> mRefCount;

    D3DGLTexture *mParent;
    UINT mLevel;

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
    D3DGLTextureSurface(D3DGLTexture *parent, UINT level);
    virtual ~D3DGLTextureSurface();

    void init(UINT offset, UINT length);
    UINT getDataLength() const { return mDataLength; }

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

#endif /* TEXTURE_HPP */
