#ifndef VERTEXSHADER_HPP
#define VERTEXSHADER_HPP

#include <atomic>
#include <vector>
#include <map>
#include <d3d9.h>

#include "glew.h"
#include "commandqueue.hpp"


class D3DGLDevice;

class D3DGLVertexShader : public IDirect3DVertexShader9 {
    std::atomic<ULONG> mRefCount;

    D3DGLDevice *mParent;

    std::atomic<ULONG> mPendingUpdates;
    std::atomic<GLuint> mProgram;
    UINT mSamplerMask; // Bitmask of used samplers
    UINT mShadowSamplers; // Bitmask of samplers that have a shadow texture format

    std::vector<DWORD> mCode;

    std::map<USHORT,GLint> mUsageMap;

public:
    D3DGLVertexShader(D3DGLDevice *parent);
    virtual ~D3DGLVertexShader();

    bool init(const DWORD *data);

    GLuint compileShaderGL(UINT shadowsamplers);

    void addPendingUpdate() { ++mPendingUpdates; }
    ULONG getPendingUpdates() { return mPendingUpdates; }

    GLuint getProgram() const { return mProgram; }
    GLint getLocation(BYTE usage, BYTE index) const
    {
        auto idx = mUsageMap.find((usage<<8) | index);
        if(idx == mUsageMap.end()) return -1;
        return idx->second;
    }

    void checkShadowSamplers(UINT mask);

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj) final;
    virtual ULONG WINAPI AddRef() final;
    virtual ULONG WINAPI Release() final;
    /*** IDirect3DVertexShader9 methods ***/
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device) final;
    virtual HRESULT WINAPI GetFunction(void *data, UINT *size) final;
};


class CompileAndSetVShaderCmd : public Command {
    D3DGLVertexShader *mTarget;
    GLuint mPipeline;
    UINT mShadowSamplers;

public:
    CompileAndSetVShaderCmd(D3DGLVertexShader *target, GLuint pipeline, UINT shadowsamplers=0)
      : mTarget(target), mPipeline(pipeline), mShadowSamplers(shadowsamplers) { }

    virtual ULONG execute()
    {
        GLuint program = mTarget->compileShaderGL(mShadowSamplers);
        glUseProgramStages(mPipeline, GL_VERTEX_SHADER_BIT, program);
        checkGLError();
        return sizeof(*this);
    }
};

class SetVShaderCmd : public Command {
    GLuint mPipeline;
    GLuint mProgram;

public:
    SetVShaderCmd(GLuint pipeline, GLuint program)
      : mPipeline(pipeline), mProgram(program) { }

    virtual ULONG execute()
    {
        glUseProgramStages(mPipeline, GL_VERTEX_SHADER_BIT, mProgram);
        checkGLError();
        return sizeof(*this);
    }
};

#endif /* VERTEXSHADER_HPP */
