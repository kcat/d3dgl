
#include "vertexshader.hpp"

#include <sstream>

#include "mojoshader/mojoshader.h"
#include "device.hpp"
#include "trace.hpp"
#include "private_iids.hpp"


void D3DGLVertexShader::compileShaderGL(const MOJOSHADER_parseData *shader)
{
    mProgram = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &shader->output);
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
            FIXME("Compile failure log:\n----\n%s\n----\n", log.data());
        }
        FIXME("Shader text:\n----\n%s\n----\n", shader->output);
        checkGLError();
        return;
    }

    TRACE("Created vertex shader program 0x%x\n", mProgram);

    GLint logLen = 0;
    glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen);
    if(logLen > 4)
    {
        std::vector<char> log(logLen+1);
        glGetProgramInfoLog(mProgram, logLen, &logLen, log.data());
        WARN("Compile warning log:\n----\n%s\n----\n", log.data());
        WARN("Shader text:\n----\n%s\n----\n", shader->output);
    }

    GLuint v4f_idx = glGetUniformBlockIndex(mProgram, "vs_vec4");
    if(v4f_idx != GL_INVALID_INDEX)
        glUniformBlockBinding(mProgram, v4f_idx, VSF_BINDING_IDX);
    GLuint pos_fixup_idx = glGetUniformBlockIndex(mProgram, "pos_fixup");
    if(pos_fixup_idx != GL_INVALID_INDEX)
        glUniformBlockBinding(mProgram, pos_fixup_idx, POSFIXUP_BINDING_IDX);

    for(int i = 0;i < shader->attribute_count;++i)
    {
        GLint loc = glGetAttribLocation(mProgram, shader->attributes[i].name);
        TRACE("Got attribute %s at location %d\n", shader->attributes[i].name, loc);
        mUsageMap[(shader->attributes[i].usage<<8) | shader->attributes[i].index] = loc;
    }

    for(int i = 0;i < shader->sampler_count;++i)
    {
        GLint loc = glGetUniformLocation(mProgram, shader->samplers[i].name);
        TRACE("Got sampler %s:%d at location %d\n", shader->samplers[i].name, shader->samplers[i].index, loc);
        glProgramUniform1i(mProgram, loc, shader->samplers[i].index+MAX_FRAGMENT_SAMPLERS);
    }

    checkGLError();
}
class CompileVShaderCmd : public Command {
    D3DGLVertexShader *mTarget;
    const MOJOSHADER_parseData *mShader;

public:
    CompileVShaderCmd(D3DGLVertexShader *target, const MOJOSHADER_parseData *shader)
      : mTarget(target), mShader(shader)
    { }

    virtual ULONG execute()
    {
        mTarget->compileShaderGL(mShader);
        return sizeof(*this);
    }
};

void D3DGLVertexShader::deinitGL()
{
    glDeleteProgram(mProgram);
}
class DeinitVShaderCmd : public Command {
    D3DGLVertexShader *mTarget;

public:
    DeinitVShaderCmd(D3DGLVertexShader *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->deinitGL();
        return sizeof(*this);
    }
};


D3DGLVertexShader::D3DGLVertexShader(D3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
  , mProgram(0)
{
    mParent->AddRef();
}

D3DGLVertexShader::~D3DGLVertexShader()
{
    mParent->getQueue().sendSync<DeinitVShaderCmd>(this);
    mParent->Release();
}

bool D3DGLVertexShader::init(const DWORD *data)
{
    if(*data>>16 != 0xfffe)
    {
        WARN("Shader is not a vertex shader (0x%04lx, expected 0xfffe)\n", *data>>16);
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

    mParent->getQueue().sendSync<CompileVShaderCmd>(this, shader);
    MOJOSHADER_freeParseData(shader);

    return true;
}


HRESULT D3DGLVertexShader::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLVertexShader);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DVertexShader9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLVertexShader::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG D3DGLVertexShader::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLVertexShader::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);

    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

HRESULT D3DGLVertexShader::GetFunction(void *data, UINT *size)
{
    TRACE("iface %p, data %p, size %p\n", this, data, size);

    *size = mCode.size();
    if(data)
        memcpy(data, mCode.data(), mCode.size());
    return D3D_OK;
}
