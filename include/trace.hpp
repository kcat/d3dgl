#ifndef TRACE_HPP
#define TRACE_HPP

#include <cstdio>


enum eLogLevel {
    ERR_=0,
    FIXME_=1,
    WARN_=2,
    TRACE_=3
};
extern eLogLevel LogLevel;
extern FILE *LogFile;


#define D3DGL_PRINT(MSG, ...) do {                                            \
    fprintf(LogFile, "D3DGL: %s: " MSG, __FUNCTION__ , ## __VA_ARGS__);       \
    fflush(LogFile);                                                          \
} while(0)

#define TRACE(...) do {                                                       \
    if(LogLevel >= TRACE_)                                                    \
        D3DGL_PRINT(__VA_ARGS__);                                             \
} while(0)

#define WARN(...) do {                                                        \
    if(LogLevel >= WARN_)                                                     \
        D3DGL_PRINT(__VA_ARGS__);                                             \
} while(0)

#define FIXME(...) do {                                                       \
    if(LogLevel >= FIXME_)                                                    \
        D3DGL_PRINT(__VA_ARGS__);                                             \
} while(0)

#define ERR(...) do {                                                         \
    if(LogLevel >= ERR_)                                                      \
        D3DGL_PRINT(__VA_ARGS__);                                             \
} while(0)


class debugstr_guid {
    char mStr[64];

public:
    debugstr_guid(const IID &id)
    {
        snprintf(mStr, sizeof(mStr), "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 id.Data1, id.Data2, id.Data3,
                 id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
                 id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);
    }
    debugstr_guid(const GUID *id)
    {
        if(!id)
            snprintf(mStr, sizeof(mStr), "(null)");
        else
            snprintf(mStr, sizeof(mStr), "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                     id->Data1, id->Data2, id->Data3,
                     id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                     id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    }

    operator const char*() const
    { return mStr; }
};


#endif /* TRACE_HPP */
