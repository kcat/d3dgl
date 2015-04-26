#ifndef VERTEXSHADER_HPP
#define VERTEXSHADER_HPP

#include <atomic>
#include <vector>
#include <map>
#include <d3d9.h>

#include "glew.h"


struct MOJOSHADER_parseData;
class D3DGLDevice;

class D3DGLVertexShader : public IDirect3DVertexShader9 {
    std::atomic<ULONG> mRefCount;

    D3DGLDevice *mParent;

    GLuint mProgram;

    std::vector<DWORD> mCode;

    std::map<USHORT,GLint> mUsageMap;

public:
    D3DGLVertexShader(D3DGLDevice *parent);
    virtual ~D3DGLVertexShader();

    bool init(const DWORD *data);

    void compileShaderGL(const MOJOSHADER_parseData *shader);

    GLuint getProgram() const { return mProgram; }
    GLint getLocation(BYTE usage, BYTE index) const
    {
        auto idx = mUsageMap.find((usage<<8) | index);
        if(idx == mUsageMap.end()) return -1;
        return idx->second;
    }

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();
    /*** IDirect3DVertexShader9 methods ***/
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device);
    virtual HRESULT WINAPI GetFunction(void *data, UINT *size);
};

#endif /* VERTEXSHADER_HPP */
