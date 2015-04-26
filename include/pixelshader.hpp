#ifndef PIXELSHADER_HPP
#define PIXELSHADER_HPP

#include <atomic>
#include <vector>
#include <d3d9.h>

#include "glew.h"
#include "commandqueue.hpp"


class D3DGLDevice;

class D3DGLPixelShader : public IDirect3DPixelShader9 {
    std::atomic<ULONG> mRefCount;

    D3DGLDevice *mParent;

    std::atomic<ULONG> mPendingUpdates;
    std::atomic<GLuint> mProgram;
    UINT mSamplerMask; // Bitmask of used samplers
    UINT mShadowSamplers; // Bitmask of samplers that have a shadow texture format

    std::vector<DWORD> mCode;

public:
    D3DGLPixelShader(D3DGLDevice *parent);
    virtual ~D3DGLPixelShader();

    bool init(const DWORD *data);

    GLuint compileShaderGL(UINT shadowsamplers);

    void addPendingUpdate() { ++mPendingUpdates; }
    ULONG getPendingUpdates() { return mPendingUpdates; }
    GLuint getProgram() const { return mProgram; }

    void checkShadowSamplers(UINT mask);

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj) final;
    virtual ULONG WINAPI AddRef() final;
    virtual ULONG WINAPI Release() final;
    /*** IDirect3DPixelShader9 methods ***/
    virtual HRESULT WINAPI GetDevice(IDirect3DDevice9 **device) final;
    virtual HRESULT WINAPI GetFunction(void *data, UINT *size) final;
};


class CompileAndSetPShaderCmd : public Command {
    D3DGLPixelShader *mTarget;
    GLuint mPipeline;
    UINT mShadowSamplers;

public:
    CompileAndSetPShaderCmd(D3DGLPixelShader *target, GLuint pipeline, UINT shadowsamplers=0)
      : mTarget(target), mPipeline(pipeline), mShadowSamplers(shadowsamplers) { }

    virtual ULONG execute()
    {
        GLuint program = mTarget->compileShaderGL(mShadowSamplers);
        glUseProgramStages(mPipeline, GL_FRAGMENT_SHADER_BIT, program);
        checkGLError();
        return sizeof(*this);
    }
};

class SetPShaderCmd : public Command {
    GLuint mPipeline;
    GLuint mProgram;

public:
    SetPShaderCmd(GLuint pipeline, GLuint program)
      : mPipeline(pipeline), mProgram(program) { }

    virtual ULONG execute()
    {
        glUseProgramStages(mPipeline, GL_FRAGMENT_SHADER_BIT, mProgram);
        checkGLError();
        return sizeof(*this);
    }
};

#endif /* PIXELSHADER_HPP */
