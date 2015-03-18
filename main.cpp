
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#include <cstdio>
#include <vector>


#define DECLSPEC_HOTPATCH __attribute__((__ms_hook_prologue__))
#define DECLSPEC_EXPORT __declspec (dllexport)

enum eLogLevel {
    ERR_=0,
    FIXME_=1,
    WARN_=2,
    TRACE_=3
};
eLogLevel LogLevel = FIXME_;
FILE *LogFile = stderr;


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


extern "C" {


BOOL WINAPI DllMain(HINSTANCE /*hModule*/, DWORD reason, void */*lpReserved*/)
{
    const char *str;

    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            str = getenv("D3DGL_LOGFILE");
            if(str && str[0] != '\0')
            {
                FILE *logfile = fopen(str, "wb");
                if(logfile) LogFile = logfile;
                else ERR("Failed to open %s for writing\n", str);
            }

            str = getenv("D3DGL_LOGLEVEL");
            if(str && str[0] != '\0')
            {
                char *end = nullptr;
                unsigned long val = strtoul(str, &end, 10);
                if(end && *end == '\0')
                    LogLevel = eLogLevel(std::min<unsigned long>(val, TRACE_));
                else
                    ERR("Invalid log level: %s\n", str);
            }
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}


static int D3DPERF_event_level = 0;


void WINAPI DebugSetMute(void)
{
    FIXME("stub\n");
}

DECLSPEC_EXPORT IDirect3D9* WINAPI DECLSPEC_HOTPATCH Direct3DCreate9(UINT sdk_version)
{
    FIXME("(%u)\n", sdk_version);
    return nullptr;
}

DECLSPEC_EXPORT HRESULT WINAPI DECLSPEC_HOTPATCH Direct3DCreate9Ex(UINT sdk_version, IDirect3D9Ex **d3d9ex)
{
    FIXME("(%u, %p)\n", sdk_version, d3d9ex);
    *d3d9ex = nullptr;
    return D3DERR_NOTAVAILABLE;
}

/*******************************************************************
 *       Direct3DShaderValidatorCreate9 (D3D9.@)
 *
 * No documentation available for this function.
 * SDK only says it is internal and shouldn't be used.
 */
DECLSPEC_EXPORT void* WINAPI Direct3DShaderValidatorCreate9(void)
{
    static int once;
    if(!once++) FIXME("stub\n");
    return nullptr;
}

/***********************************************************************
 *              D3DPERF_BeginEvent (D3D9.@)
 */
DECLSPEC_EXPORT int WINAPI D3DPERF_BeginEvent(D3DCOLOR color, const WCHAR *name)
{
    TRACE("color 0x%08lx, name %ls\n", color, name);
    return D3DPERF_event_level++;
}

/***********************************************************************
 *              D3DPERF_EndEvent (D3D9.@)
 */
DECLSPEC_EXPORT int WINAPI D3DPERF_EndEvent(void)
{
    TRACE("(void) : stub\n");
    return --D3DPERF_event_level;
}

/***********************************************************************
 *              D3DPERF_GetStatus (D3D9.@)
 */
DECLSPEC_EXPORT DWORD WINAPI D3DPERF_GetStatus(void)
{
    FIXME("(void) : stub\n");
    return 0;
}

/***********************************************************************
 *              D3DPERF_SetOptions (D3D9.@)
 */
DECLSPEC_EXPORT void WINAPI D3DPERF_SetOptions(DWORD options)
{
    FIXME("(0x%lx) : stub\n", options);
}

/***********************************************************************
 *              D3DPERF_QueryRepeatFrame (D3D9.@)
 */
DECLSPEC_EXPORT BOOL WINAPI D3DPERF_QueryRepeatFrame(void)
{
    FIXME("(void) : stub\n");
    return FALSE;
}

/***********************************************************************
 *              D3DPERF_SetMarker (D3D9.@)
 */
DECLSPEC_EXPORT void WINAPI D3DPERF_SetMarker(D3DCOLOR color, const WCHAR *name)
{
    FIXME("color 0x%08lx, name %ls stub!\n", color, name);
}

/***********************************************************************
 *              D3DPERF_SetRegion (D3D9.@)
 */
DECLSPEC_EXPORT void WINAPI D3DPERF_SetRegion(D3DCOLOR color, const WCHAR *name)
{
    FIXME("color 0x%08lx, name %ls stub!\n", color, name);
}

} // extern "C"
