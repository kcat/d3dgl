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

#endif /* TRACE_HPP */
