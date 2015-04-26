
#include "pixelshader.hpp"

#include <sstream>

#include "mojoshader/mojoshader.h"
#include "device.hpp"
#include "trace.hpp"
#include "private_iids.hpp"


GLuint D3DGLPixelShader::compileShaderGL(UINT shadowmask)
{
    const MOJOSHADER_parseData *shader = nullptr;
    GLuint program = mProgram.exchange(0);
    if(program)
    {
        glDeleteProgram(program);
        program = 0;
    }

    shader = MOJOSHADER_parse(MOJOSHADER_PROFILE_GLSL330,
        reinterpret_cast<const unsigned char*>(mCode.data()),
        mCode.size() * sizeof(decltype(mCode)::value_type),
        nullptr, 0, nullptr, 0, shadowmask
    );
    if(shader->error_count > 0)
    {
        std::stringstream sstr;
        for(int i = 0;i < shader->error_count;++i)
            sstr<< shader->errors[i].error_position<<":"<<shader->errors[i].error <<std::endl;
        ERR("Failed to parse shader:\n----\n%s\n----\n", sstr.str().c_str());
        goto done;
    }
    if(mCode.size() != (std::size_t)shader->token_count)
        ERR("Token count mismatch (previous: %u, now: %d)\n",
            mCode.size(), shader->token_count);
    TRACE("Parsed shader:\n----\n%s\n----\n", shader->output);

    {
        program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &shader->output);
        checkGLError();
        if(!program)
        {
            FIXME("Failed to create shader program\n");
            goto done;
        }

        GLint logLen = 0;
        GLint status = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if(status == GL_FALSE)
        {
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> log(logLen+1);
            glGetProgramInfoLog(program, logLen, &logLen, log.data());
            FIXME("Shader not linked:\n----\n%s\n----\nShader text:\n----\n%s\n----\n",
                  log.data(), shader->output);

            glDeleteProgram(program);
            program = 0;
            checkGLError();

            goto done;
        }

        mProgram = program;
        TRACE("Created fragment shader program 0x%x\n", program);

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        if(logLen > 4)
        {
            std::vector<char> log(logLen+1);
            glGetProgramInfoLog(program, logLen, &logLen, log.data());
            WARN("Compile warning log:\n----\n%s\n----\nShader text:\n----\n%s\n----\n",
                 log.data(), shader->output);
        }
    }

    {
        GLuint v4f_idx = glGetUniformBlockIndex(program, "ps_vec4");
        if(v4f_idx != GL_INVALID_INDEX)
            glUniformBlockBinding(program, v4f_idx, PSF_BINDING_IDX);
    }

    for(int i = 0;i < shader->sampler_count;++i)
    {
        GLint loc = glGetUniformLocation(program, shader->samplers[i].name);
        TRACE("Got sampler %s:%d at location %d\n", shader->samplers[i].name,
            shader->samplers[i].index, loc);
        glProgramUniform1i(program, loc, shader->samplers[i].index);
    }

    checkGLError();

done:
    MOJOSHADER_freeParseData(shader);

    --mPendingUpdates;
    return program;
}


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
  , mPendingUpdates(0)
  , mProgram(0)
  , mSamplerMask(0)
  , mShadowSamplers(0)
{
    mParent->AddRef();
}

D3DGLPixelShader::~D3DGLPixelShader()
{
    if(mPendingUpdates > 0)
        Sleep(1);

    if(GLuint program = mProgram.exchange(0))
        mParent->getQueue().send<DeinitPShaderCmd>(program);
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
          (*data>>8)&0xff, *data&0xff, MOJOSHADER_PROFILE_GLSL330);

    const MOJOSHADER_parseData *shader = MOJOSHADER_parse(MOJOSHADER_PROFILE_GLSL330,
        reinterpret_cast<const unsigned char*>(data), 0, nullptr, 0, nullptr, 0, 0
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
    // Save the tokens used
    mCode.insert(mCode.end(), data, data+shader->token_count);

    TRACE("Parsed shader:\n----\n%s\n----\n", shader->output);

    for(int i = 0;i < shader->sampler_count;++i)
        mSamplerMask |= 1<<shader->samplers[i].index;

    MOJOSHADER_freeParseData(shader);

    return true;
}

void D3DGLPixelShader::checkShadowSamplers(UINT mask)
{
    if(mShadowSamplers == (mask&mSamplerMask))
        return;

    WARN("Rebuilding shader %p because of shadow mismatch: %u / %u\n", this, mShadowSamplers, (mask&mSamplerMask));
    mShadowSamplers = (mask&mSamplerMask);
    addPendingUpdate();
    mParent->getQueue().doSend<CompileAndSetPShaderCmd>(this, mParent->getShaderPipeline(), mShadowSamplers);
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

    *size = mCode.size() * sizeof(decltype(mCode)::value_type);
    if(data)
        memcpy(data, mCode.data(), mCode.size() * sizeof(decltype(mCode)::value_type));
    return D3D_OK;
}
