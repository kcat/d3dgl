
#include "pixelshader.hpp"

#include <sstream>

#include "mojoshader/mojoshader.h"
#include "device.hpp"
#include "trace.hpp"
#include "private_iids.hpp"


void D3DGLPixelShader::compileShaderGL(const MOJOSHADER_parseData *shader)
{
    mProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &shader->output);
    checkGLError();

    if(!mProgram)
    {
        FIXME("Failed to create shader program\n");
        return;
    }

    TRACE("Created fragment shader program 0x%x\n", mProgram);

    GLint logLen = 0;
    GLint status = GL_FALSE;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen+1);
        glGetProgramInfoLog(mProgram, logLen, &logLen, log.data());
        FIXME("Shader not linked:\n----\n%s\n----\nShader text:\n----\n%s\n----\n",
              log.data(), shader->output);

        glDeleteProgram(mProgram);
        mProgram = 0;

        checkGLError();
        return;
    }

    glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen);
    if(logLen > 4)
    {
        std::vector<char> log(logLen+1);
        glGetProgramInfoLog(mProgram, logLen, &logLen, log.data());
        WARN("Compile warning log:\n----\n%s\n----\nShader text:\n----\n%s\n----\n",
             log.data(), shader->output);
    }

    GLuint v4f_idx = glGetUniformBlockIndex(mProgram, "ps_vec4");
    if(v4f_idx != GL_INVALID_INDEX)
        glUniformBlockBinding(mProgram, v4f_idx, PSF_BINDING_IDX);

    for(int i = 0;i < shader->sampler_count;++i)
    {
        GLint loc = glGetUniformLocation(mProgram, shader->samplers[i].name);
        TRACE("Got sampler %s:%d at location %d\n", shader->samplers[i].name, shader->samplers[i].index, loc);
        glProgramUniform1i(mProgram, loc, shader->samplers[i].index);
    }

    checkGLError();
}
class CompilePShaderCmd : public Command {
    D3DGLPixelShader *mTarget;
    const MOJOSHADER_parseData *mShader;

public:
    CompilePShaderCmd(D3DGLPixelShader *target, const MOJOSHADER_parseData *shader)
      : mTarget(target), mShader(shader)
    { }

    virtual ULONG execute()
    {
        mTarget->compileShaderGL(mShader);
        return sizeof(*this);
    }
};

class DeinitPShaderCmd : public Command {
    GLuint mProgram;

public:
    DeinitPShaderCmd(GLuint program) : mProgram(program) { }

    virtual ULONG execute()
    {
        glDeleteProgram(mProgram);
        return sizeof(*this);
    }
};


D3DGLPixelShader::D3DGLPixelShader(D3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
  , mProgram(0)
{
    mParent->AddRef();
}

D3DGLPixelShader::~D3DGLPixelShader()
{
    if(mProgram)
        mParent->getQueue().send<DeinitPShaderCmd>(mProgram);
    mProgram = 0;
    mParent->Release();
}

bool D3DGLPixelShader::init(const DWORD *data)
{
    if(*data>>16 != 0xffff)
    {
        WARN("Shader is not a pixel shader (0x%04lx, expected 0xffff)\n", *data>>16);
        return false;
    }

    TRACE("Parsing %s shader %lu.%lu using profile %s\n",
          (((*data>>16)==0xfffe) ? "vertex" : ((*data>>16)==0xffff) ? "pixel" : "unknown"),
          (*data>>8)&0xff, *data&0xff, MOJOSHADER_PROFILE_GLSL120);

    const MOJOSHADER_parseData *shader = MOJOSHADER_parse(MOJOSHADER_PROFILE_GLSL330,
        reinterpret_cast<const unsigned char*>(data), 0, nullptr, 0, nullptr, 0
    );
    if(shader->error_count > 0)
    {
        std::stringstream sstr;
        for(int i = 0;i < shader->error_count;++i)
            sstr<< shader->errors[i].error_position<<":"<<shader->errors[i].error <<std::endl;
        ERR("Failed to parse shader:\n----\n%s\n----\n", sstr.str().c_str());
        MOJOSHADER_freeParseData(shader);
        return false;
    }

    TRACE("Parsed shader:\n----\n%s\n----\n", shader->output);

    mParent->getQueue().sendSync<CompilePShaderCmd>(this, shader);
    MOJOSHADER_freeParseData(shader);

    return mProgram != 0;
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
