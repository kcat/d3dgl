
#include "pixelshader.hpp"

#include <sstream>

#include "mojoshader/mojoshader.h"
#include "device.hpp"
#include "trace.hpp"
#include "private_iids.hpp"


void D3DGLPixelShader::compileShaderGL(const DWORD *data)
{
    TRACE("Parsing %s shader %lu.%lu using profile %s\n",
          (((*data>>16)==0xfffe) ? "vertex" : ((*data>>16)==0xffff) ? "pixel" : "unknown"),
          (*data>>8)&0xff, *data&0xff, MOJOSHADER_PROFILE_GLSL120);

    mShader = MOJOSHADER_parse(MOJOSHADER_PROFILE_GLSL120,
        reinterpret_cast<const unsigned char*>(data), 0, nullptr, 0, nullptr, 0
    );
    if(mShader->error_count > 0)
    {
        std::stringstream sstr;
        for(int i = 0;i < mShader->error_count;++i)
            sstr<< mShader->errors[i].error_position<<":"<<mShader->errors[i].error <<std::endl;
        ERR("Failed to parse shader:\n----\n%s\n----\n", sstr.str().c_str());
        return;
    }

    TRACE("Parsed shader:\n----\n%s\n----\n", mShader->output);
    mProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &mShader->output);
    checkGLError();

    if(!mProgram)
    {
        FIXME("Failed to create shader program\n");
        GLint logLen;
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen);
        if(logLen > 0)
        {
            std::vector<char> log(logLen+1);
            glGetProgramInfoLog(mProgram, logLen, &logLen, log.data());
            FIXME("Compile log:\n----\n%s\n----\n", log.data());
        }
        checkGLError();
        return;
    }

    TRACE("Created fragment shader program 0x%x\n", mProgram);

    GLint logLen = 0;
    glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen);
    if(logLen > 4)
    {
        std::vector<char> log(logLen+1);
        glGetProgramInfoLog(mProgram, logLen, &logLen, log.data());
        WARN("Compile log:\n----\n%s\n----\n", log.data());
    }

    GLuint v4f_idx = glGetUniformBlockIndex(mProgram, "ps_vec4f");
    if(v4f_idx != GL_INVALID_INDEX)
        glUniformBlockBinding(mProgram, v4f_idx, PSF_BINDING_IDX);

    for(int i = 0;i < mShader->sampler_count;++i)
    {
        GLint loc = glGetUniformLocation(mProgram, mShader->samplers[i].name);
        TRACE("Got sampler %s:%d at location %d\n", mShader->samplers[i].name, mShader->samplers[i].index, loc);
        glProgramUniform1i(mProgram, loc, mShader->samplers[i].index);
    }

    checkGLError();
}
class CompilePShaderCmd : public Command {
    D3DGLPixelShader *mTarget;
    const DWORD *mData;

public:
    CompilePShaderCmd(D3DGLPixelShader *target, const DWORD *data) : mTarget(target), mData(data)
    { }

    virtual ULONG execute()
    {
        mTarget->compileShaderGL(mData);
        return sizeof(*this);
    }
};

void D3DGLPixelShader::deinitGL()
{
    glDeleteProgram(mProgram);
    MOJOSHADER_freeParseData(mShader);
}
class DeinitPShaderCmd : public Command {
    D3DGLPixelShader *mTarget;

public:
    DeinitPShaderCmd(D3DGLPixelShader *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->deinitGL();
        return sizeof(*this);
    }
};


D3DGLPixelShader::D3DGLPixelShader(D3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
  , mShader(nullptr)
  , mProgram(0)
{
    mParent->AddRef();
}

D3DGLPixelShader::~D3DGLPixelShader()
{
    mParent->getQueue().sendSync<DeinitPShaderCmd>(this);
    mParent->Release();
}

bool D3DGLPixelShader::init(const DWORD *data)
{
    mParent->getQueue().sendSync<CompilePShaderCmd>(this, data);

    return true;
}


HRESULT D3DGLPixelShader::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLPixelShader);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DPixelShader9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLPixelShader::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG D3DGLPixelShader::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLPixelShader::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);

    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLPixelShader::GetFunction(void *data, UINT *size)
{
    TRACE("iface %p, data %p, size %p\n", this, data, size);

    *size = mCode.size();
    if(data)
        memcpy(data, mCode.data(), mCode.size());
    return D3D_OK;
}
