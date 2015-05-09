#ifndef TEXTURE3D_HPP
#define TEXTURE3D_HPP

#include <atomic>
#include <vector>
#include <d3d9.h>

#include "glew.h"
#include "allocators.hpp"


struct GLFormatInfo;
class D3DGLDevice;
class D3DGLTextureVolume;

class D3DGLTexture3D : public IDirect3DVolumeTexture9 {
    std::atomic<ULONG> mRefCount;
    std::atomic<ULONG> mIfaceCount;

    D3DGLDevice *mParent;

    const GLFormatInfo *mGLFormat;
    bool mIsCompressed;
    GLuint mTexId;
    std::vector<GLubyte,AlignedAllocator<GLubyte>> mSysMem;

    D3DBOX mDirtyBox;
    std::atomic<ULONG> mUpdateInProgress;

    D3DVOLUME_DESC mDesc;
    std::vector<D3DGLTextureVolume*> mVolumes;
    std::atomic<DWORD> mLodLevel;

    void addIface();
    void releaseIface();

    friend class D3DGLTextureVolume;

public:
    D3DGLTexture3D(D3DGLDevice *parent);
    virtual ~D3DGLTexture3D();

    bool init(const D3DVOLUME_DESC *desc, UINT levels);
    void updateTexture(DWORD level, const D3DBOX &box, const GLubyte *dataPtr);

    const D3DVOLUME_DESC &getDesc() const { return mDesc; }
    GLuint getTextureId() const { return mTexId; }
    const GLFormatInfo &getFormat() const { return *mGLFormat; }

    void initGL();
    void deinitGL();
    void genMipmapGL();
    void loadTexLevelGL(DWORD level, const D3DBOX &box, const GLubyte *dataPtr);

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
    /*** IDirect3DVolumeTexture9 methods ***/
    virtual HRESULT WINAPI GetLevelDesc(UINT level, D3DVOLUME_DESC *desc) final;
    virtual HRESULT WINAPI GetVolumeLevel(UINT level, IDirect3DVolume9 **volume) final;
    virtual HRESULT WINAPI LockBox(UINT level, D3DLOCKED_BOX *lockedbox, const D3DBOX *box, DWORD flags) final;
    virtual HRESULT WINAPI UnlockBox(UINT level) final;
    virtual HRESULT WINAPI AddDirtyBox(const D3DBOX *box) final;
};


class D3DGLTextureVolume : public IDirect3DVolume9 {
    D3DGLTexture3D *mParent;
    UINT mLevel;

    enum LockType {
        LT_Unlocked,
        LT_ReadOnly,
        LT_Full
    };
    std::atomic<LockType> mLock;
    D3DBOX mLockRegion;

    UINT mDataOffset;
    UINT mDataLength;

public:
    D3DGLTextureVolume(D3DGLTexture3D *parent, UINT level);
    virtual ~D3DGLTextureVolume();

    void init(UINT offset, UINT length);
    D3DGLTexture3D *getParent() { return mParent; }
    const GLFormatInfo &getFormat() const { return mParent->getFormat(); }
    UINT getLevel() const { return mLevel; }
    UINT getDataLength() const { return mDataLength; }

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj) final;
    virtual ULONG WINAPI AddRef() final;
    virtual ULONG WINAPI Release() final;
    /*** IDirect3DVolume9 methods ***/
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device) final;
    virtual HRESULT WINAPI SetPrivateData(REFGUID refguid, const void *data, DWORD size, DWORD flags) final;
    virtual HRESULT WINAPI GetPrivateData(REFGUID refguid, void *data, DWORD *size) final;
    virtual HRESULT WINAPI FreePrivateData(REFGUID refguid) final;
    virtual HRESULT WINAPI GetContainer(REFIID riid, void **container) final;
    virtual HRESULT WINAPI GetDesc(D3DVOLUME_DESC *desc) final;
    virtual HRESULT WINAPI LockBox(D3DLOCKED_BOX *lockedbox, const D3DBOX *box, DWORD flags) final;
    virtual HRESULT WINAPI UnlockBox() final;
};

#endif /* TEXTURE3D_HPP */
