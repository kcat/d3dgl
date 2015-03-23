#ifndef D3DGL_HPP
#define D3DGL_HPP

#include <atomic>
#include <vector>
#include <string>

#include <d3d9.h>

bool CreateFakeContext(HINSTANCE hInstance, HWND &hWnd, HDC &dc, HGLRC &glrc);


class D3DAdapter {
public:
    struct Limits {
        UINT buffers;
        UINT lights;
        UINT textures;
        UINT texture_coords;
        UINT vertex_uniform_blocks;
        UINT geometry_uniform_blocks;
        UINT fragment_uniform_blocks;
        UINT fragment_samplers;
        UINT vertex_samplers;
        UINT combined_samplers;
        UINT general_combiners;
        UINT clipplanes;
        UINT texture_size;
        UINT texture3d_size;
        float pointsize_max;
        float pointsize_min;
        UINT blends;
        UINT anisotropy;
        float shininess;
        UINT samples;
        UINT vertex_attribs;

        UINT glsl_varyings;
        UINT glsl_vs_float_constants;
        UINT glsl_ps_float_constants;

        UINT arb_vs_float_constants;
        UINT arb_vs_native_constants;
        UINT arb_vs_instructions;
        UINT arb_vs_temps;
        UINT arb_ps_float_constants;
        UINT arb_ps_local_constants;
        UINT arb_ps_native_constants;
        UINT arb_ps_instructions;
        UINT arb_ps_temps;
    };

private:
    UINT mOrdinal;
    bool mInited;

    std::wstring mDeviceName;

    WORD mVendorId;
    WORD mDeviceId;
    const char *mDescription;

    D3DCAPS9 mCaps;

    Limits mLimits;

    void init_limits();

    void init_caps();
    void init_ids();

public:
    D3DAdapter() = default;
    D3DAdapter(UINT adapter_num);

    bool init();
    UINT getOrdinal() const { return mOrdinal; }
    const Limits& getLimits() const { return mLimits; }
    const std::wstring &getDeviceName() const { return mDeviceName; }
    D3DCAPS9 getCaps() const { return mCaps; }
    WORD getVendorId() const { return mVendorId; }
    WORD getDeviceId() const { return mDeviceId; }
    const char *getDescription() const { return mDescription; }

    UINT getModeCount(D3DFORMAT format) const;
};


class Direct3DGL : public IDirect3D9 {
    std::atomic<ULONG> mRefCount;

    std::vector<D3DAdapter> mAdapters;

private:
    virtual ~Direct3DGL();

public:
    Direct3DGL();

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **obj);
    virtual ULONG WINAPI AddRef(void);
    virtual ULONG WINAPI Release(void);

    /*** IDirect3D9 methods ***/
    virtual HRESULT WINAPI RegisterSoftwareDevice(void *initFunction);
    virtual UINT WINAPI GetAdapterCount(void);
    virtual HRESULT WINAPI GetAdapterIdentifier(UINT adapter, DWORD flags, D3DADAPTER_IDENTIFIER9 *identifier);
    virtual UINT WINAPI GetAdapterModeCount(UINT adapter, D3DFORMAT format);
    virtual HRESULT WINAPI EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode, D3DDISPLAYMODE *displayMode);
    virtual HRESULT WINAPI GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE *displayMode);
    virtual HRESULT WINAPI CheckDeviceType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT displayFormat, D3DFORMAT backBufferFormat, WINBOOL windowed);
    virtual HRESULT WINAPI CheckDeviceFormat(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, DWORD usage, D3DRESOURCETYPE resType, D3DFORMAT checkFormat);
    virtual HRESULT WINAPI CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE devType, D3DFORMAT surfaceFormat, WINBOOL windowed, D3DMULTISAMPLE_TYPE multiSampleType, DWORD *qualityLevels);
    virtual HRESULT WINAPI CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE devType, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat);
    virtual HRESULT WINAPI CheckDeviceFormatConversion(UINT adapter, D3DDEVTYPE devType, D3DFORMAT srcFormat, D3DFORMAT dstFormat);
    virtual HRESULT WINAPI GetDeviceCaps(UINT adapter, D3DDEVTYPE devType, D3DCAPS9 *caps);
    virtual HMONITOR WINAPI GetAdapterMonitor(UINT adapter);
    virtual HRESULT WINAPI CreateDevice(UINT adapter, D3DDEVTYPE devType, HWND focusWindow, DWORD behaviorFlags, D3DPRESENT_PARAMETERS *presentParams, struct IDirect3DDevice9 **iface);
};

#endif /* D3DGL_HPP */
