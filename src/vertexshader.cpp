
#include "vertexshader.hpp"

#include <sstream>

#include "mojoshader/mojoshader.h"
#include "device.hpp"
#include "trace.hpp"
#include "private_iids.hpp"


void D3DGLVertexShader::compileShaderGL(const DWORD *data)
{
    TRACE("Parsing %s shader %lu.%lu using profile %s\n",
          (((*data>>16)==0xfffe) ? "vertex" : ((*data>>16)==0xffff) ? "pixel" : "unknown"),
          (*data>>8)&0xff, *data&0xff, MOJOSHADER_PROFILE_GLSL120);

    mShader = MOJOSHADER_parse(MOJOSHADER_PROFILE_GLSL120,
        reinterpret_cast<const unsigned char*>(data), 0,
        nullptr, 0, nullptr, 0, nullptr, nullptr, nullptr
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
    mProgram = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &mShader->output);
    checkGLError();

    if(!mProgram)
    {
        GLint logLen;
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen);
        if(logLen > 0)
        {
            std::vector<char> log(logLen+1);
            glGetProgramInfoLog(mProgram, logLen, &logLen, log.data());
            ERR("Compile log:\n----\n%s\n----\n", log.data());
        }
        ERR("Failed to compile shader program\n");
        checkGLError();
        return;
    }

    TRACE("Created vertex shader program 0x%x\n", mProgram);

    GLint logLen = 0;
    glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen);
    if(logLen > 1)
    {
        std::vector<char> log(logLen+1);
        glGetProgramInfoLog(mProgram, logLen, &logLen, log.data());
        WARN("Compile log:\n----\n%s\n----\n", log.data());
    }

    GLuint v4f_idx = glGetUniformBlockIndex(mProgram, "vs_vec4f");
    glUniformBlockBinding(mProgram, v4f_idx, VSF_BINDING_IDX);
    checkGLError();
}
class CompileVShaderCmd : public Command {
    D3DGLVertexShader *mTarget;
    const DWORD *mData;

public:
    CompileVShaderCmd(D3DGLVertexShader *target, const DWORD *data) : mTarget(target), mData(data)
    { }

    virtual ULONG execute()
    {
        mTarget->compileShaderGL(mData);
        return sizeof(*this);
    }
};

void D3DGLVertexShader::deinitGL()
{
    glDeleteProgram(mProgram);
    MOJOSHADER_freeParseData(mShader);
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
  , mShader(nullptr)
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
    mParent->getQueue().sendSync<CompileVShaderCmd>(this, data);

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
