
#include "query.hpp"

#include "device.hpp"
#include "private_iids.hpp"


void D3DGLQuery::initGL()
{
    glGenQueries(1, &mQueryId);
    checkGLError();
}
class QueryInitCmd : public Command {
    D3DGLQuery *mTarget;

public:
    QueryInitCmd(D3DGLQuery *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->initGL();
        return sizeof(*this);
    }
};

class QueryDeinitCmd : public Command {
    GLuint mQueryId;

public:
    QueryDeinitCmd(GLuint queryid) : mQueryId(queryid) { }

    virtual ULONG execute()
    {
        glDeleteQueries(1, &mQueryId);
        checkGLError();
        return sizeof(*this);
    }
};

void D3DGLQuery::beginQueryGL()
{
    glBeginQuery(mQueryType, mQueryId);
    checkGLError();
}
class BeginQueryCmd : public Command {
    D3DGLQuery *mTarget;

public:
    BeginQueryCmd(D3DGLQuery *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->beginQueryGL();
        return sizeof(*this);
    }
};

void D3DGLQuery::endQueryGL()
{
    glEndQuery(mQueryType);
    checkGLError();
}
class EndQueryCmd : public Command {
    D3DGLQuery *mTarget;

public:
    EndQueryCmd(D3DGLQuery *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->endQueryGL();
        return sizeof(*this);
    }
};

void D3DGLQuery::queryDataGL()
{
    GLint avail = GL_FALSE;
    glGetQueryObjectiv(mQueryId, GL_QUERY_RESULT_AVAILABLE, &avail);
    if(avail)
    {
        glGetQueryObjectuiv(mQueryId, GL_QUERY_RESULT, &mQueryResult);
        mState = Signaled;
    }
    --mPendingQueries;
    checkGLError();
}
class QueryDataCmd : public Command {
    D3DGLQuery *mTarget;

public:
    QueryDataCmd(D3DGLQuery *target) : mTarget(target) { }

    virtual ULONG execute()
    {
        mTarget->queryDataGL();
        return sizeof(*this);
    }
};


D3DGLQuery::D3DGLQuery(D3DGLDevice *parent)
  : mRefCount(0)
  , mParent(parent)
  , mQueryType(GL_NONE)
  , mQueryId(0)
  , mPendingQueries(0)
  , mState(Signaled)
{
    mParent->AddRef();
}

D3DGLQuery::~D3DGLQuery()
{
    if(mQueryId)
    {
        mParent->getQueue().send<QueryDeinitCmd>(mQueryId);
        while(mPendingQueries)
            mParent->getQueue().wakeAndWait();
        mQueryId = 0;
    }

    mParent->Release();
}

bool D3DGLQuery::init(D3DQUERYTYPE type)
{
    mType = type;

    if(mType == D3DQUERYTYPE_OCCLUSION)
        mQueryType = GL_SAMPLES_PASSED;
    else
    {
        FIXME("Query type %s unsupported\n", d3dquery_to_str(mType));
        return false;
    }

    mParent->getQueue().sendSync<QueryInitCmd>(this);
    return true;
}


HRESULT D3DGLQuery::QueryInterface(REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p\n", this, debugstr_guid(riid), obj);

    *obj = NULL;
    RETURN_IF_IID_TYPE(obj, riid, D3DGLQuery);
    RETURN_IF_IID_TYPE(obj, riid, IDirect3DQuery9);
    RETURN_IF_IID_TYPE(obj, riid, IUnknown);

    return E_NOINTERFACE;
}

ULONG D3DGLQuery::AddRef()
{
    ULONG ret = ++mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    return ret;
}

ULONG D3DGLQuery::Release()
{
    ULONG ret = --mRefCount;
    TRACE("%p New refcount: %lu\n", this, ret);
    if(ret == 0) delete this;
    return ret;
}


HRESULT D3DGLQuery::GetDevice(IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p\n", this, device);
    *device = mParent;
    (*device)->AddRef();
    return D3D_OK;
}

D3DQUERYTYPE D3DGLQuery::GetType()
{
    TRACE("iface %p\n", this);
    return mType;
}

DWORD D3DGLQuery::GetDataSize()
{
    TRACE("iface %p\n", this);

    if(mType == D3DQUERYTYPE_OCCLUSION)
        return sizeof(DWORD);

    ERR("Unexpected query type: %s\n", d3dquery_to_str(mType));
    return 0;
}

HRESULT D3DGLQuery::Issue(DWORD flags)
{
    TRACE("iface %p, flags 0x%lx\n", this, flags);

    if((flags&~(D3DISSUE_END|D3DISSUE_BEGIN)))
    {
        FIXME("Unhandled flags: 0x%lx\n", flags);
        return D3DERR_INVALIDCALL;
    }

    // FIXME: Make sure another occlusion query object isn't building.
    if((flags&D3DISSUE_END) && mState == Building)
    {
        mState = Issued;
        mParent->getQueue().send<EndQueryCmd>(this);
    }

    if((flags&D3DISSUE_BEGIN))
    {
        // Need to wait for any data queries to finish first
        while(mPendingQueries)
            mParent->getQueue().wakeAndWait();
        mState = Building;
        mParent->getQueue().send<BeginQueryCmd>(this);
    }

    return D3D_OK;
}

HRESULT D3DGLQuery::GetData(void *data, DWORD size, DWORD flags)
{
    TRACE("iface %p, data %p, size %lu, flags 0x%lx\n", this, data, size, flags);

    if(mState == Issued)
    {
        ULONG zero = 0;
        if(mPendingQueries.compare_exchange_strong(zero, 1))
            mParent->getQueue().send<QueryDataCmd>(this);
        if((flags&D3DGETDATA_FLUSH))
            mParent->getQueue().flush();
        return S_FALSE;
    }

    union {
        void *pointer;
        DWORD *occlusion_result;
    };
    pointer = data;

    if(mState == Signaled)
    {
        if(mType == D3DQUERYTYPE_OCCLUSION)
        {
            if(size < sizeof(*occlusion_result))
            {
                WARN("Size %lu too small\n", size);
                return D3DERR_INVALIDCALL;
            }
            *occlusion_result = mQueryResult;
            return D3D_OK;
        }

        ERR("Unexpected query type: %s\n", d3dquery_to_str(mType));
        return E_NOTIMPL;
    }

    WARN("Called while building query\n");
    return D3DERR_INVALIDCALL;
}
