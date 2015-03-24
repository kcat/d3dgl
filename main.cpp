
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <d3d9.h>

#include <vector>
#include <sstream>

#include "glew.h"
#include "trace.hpp"
#include "d3dgl.hpp"


#define DECLSPEC_HOTPATCH __attribute__((__ms_hook_prologue__))
#define DECLSPEC_EXPORT __declspec (dllexport)


eLogLevel LogLevel = FIXME_;
FILE *LogFile = stderr;

static const wchar_t WndClassName[] = L"D3DGLWndClass";

bool CreateFakeContext(HINSTANCE hInstance, HWND &hWnd, HDC &dc, HGLRC &glrc)
{
    hWnd = nullptr;
    dc   = nullptr;
    glrc = nullptr;

    hWnd = CreateWindowExW(0, WndClassName, L"D3DGL Fake Window", WS_OVERLAPPEDWINDOW,
                           0, 0, 640, 480, nullptr, nullptr, hInstance, nullptr);
    if(!hWnd)
    {
        ERR("Failed to create a window\n");
        return false;
    }
    dc = GetDC(hWnd);

    PIXELFORMATDESCRIPTOR pfd = PIXELFORMATDESCRIPTOR{
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //Flags
        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
        32,                       //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                       //Number of bits for the depthbuffer
        8,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    int pfid = ChoosePixelFormat(dc, &pfd);
    SetPixelFormat(dc, pfid, &pfd);

    glrc = wglCreateContext(dc);
    if(!glrc)
    {
        ERR("Failed to create WGL context");
        ReleaseDC(hWnd, dc);
        DestroyWindow(hWnd);
        return false;
    }

    return true;
}


extern "C" {

static int D3DPERF_event_level = 0;


BOOL WINAPI DllMain(HINSTANCE hModule, DWORD reason, void */*lpReserved*/)
{
    const char *str;

    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);

            str = getenv("D3DGL_LOGFILE");
            if(str && str[0] != '\0')
            {
                std::stringstream sstr;
                sstr<< str<<"-"<<getpid()<<".log";
                FILE *logfile = fopen(sstr.str().c_str(), "wb");
                if(logfile) LogFile = logfile;
                else ERR("Failed to open %s for writing\n", sstr.str().c_str());
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
            TRACE("DLL_PROCESS_ATTACH\n");
            break;

        case DLL_PROCESS_DETACH:
            TRACE("DLL_PROCESS_DETACH\n");
            break;
    }
    return TRUE;
}


static bool init_d3dgl(void)
{
    static bool inited = false;
    if(inited) return true;

    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = DefWindowProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszClassName = WndClassName;
    wc.style         = CS_OWNDC;
    if(!RegisterClassW(&wc))
    {
        ERR("Failed to register fake GL window class");
        return false;
    }

    HWND hWnd;
    HDC dc;
    HGLRC glrc;
    if(!CreateFakeContext(hInstance, hWnd, dc, glrc))
        return false;

    wglMakeCurrent(dc, glrc);

    GLenum err = glewInit();

    wglMakeCurrent(dc, nullptr);
    wglDeleteContext(glrc);

    ReleaseDC(hWnd, dc);
    DestroyWindow(hWnd);

    if(err != GLEW_OK)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        ERR("GLEW error: %s\n", glewGetErrorString(err));
        return false;
    }

    TRACE("Initialized GLEW %s\n", glewGetString(GLEW_VERSION));
    inited = true;
    return true;
}


DECLSPEC_EXPORT void WINAPI DebugSetMute(void)
{
    FIXME("stub\n");
}

DECLSPEC_EXPORT IDirect3D9* WINAPI DECLSPEC_HOTPATCH Direct3DCreate9(UINT sdk_version)
{
    FIXME("(%u)\n", sdk_version);

    init_d3dgl();

    Direct3DGL *d3d = new Direct3DGL();
    if(!d3d->init())
    {
        delete d3d;
        return nullptr;
    }

    d3d->AddRef();
    return d3d;
}

DECLSPEC_EXPORT HRESULT WINAPI DECLSPEC_HOTPATCH Direct3DCreate9Ex(UINT sdk_version, IDirect3D9Ex **d3d9ex)
{
    FIXME("(%u, %p)\n", sdk_version, d3d9ex);

    init_d3dgl();

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
