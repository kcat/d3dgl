#ifndef QUERY_HPP
#define QUERY_HPP

#include <atomic>
#include <d3d9.h>

#include "glew.h"


class D3DGLDevice;

class D3DGLQuery : public IDirect3DQuery9 {
    std::atomic<ULONG> mRefCount;

    D3DGLDevice *mParent;

    D3DQUERYTYPE mType;

    enum State {
        Signaled,
        Building,
        Issued
    };
    State mState;

public:
    D3DGLQuery(D3DGLDevice *parent);
    virtual ~D3DGLQuery();

    bool init(D3DQUERYTYPE type);

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj) final;
    virtual ULONG WINAPI AddRef() final;
    virtual ULONG WINAPI Release() final;
    /*** IDirect3DQuery9 methods ***/
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device) final;
    virtual D3DQUERYTYPE WINAPI GetType() final;
    virtual DWORD WINAPI GetDataSize() final;
    virtual HRESULT WINAPI Issue(DWORD flags) final;
    virtual HRESULT WINAPI GetData(void *data, DWORD size, DWORD flags) final;
};

#endif /* QUERY_HPP */
