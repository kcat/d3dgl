#ifndef ADAPTER_HPP
#define ADAPTER_HPP

#include <vector>
#include <string>
#include <map>
#include <d3d9.h>


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
    typedef std::pair<DWORD,D3DFORMAT> ResTypeFormatPair;
    typedef std::map<ResTypeFormatPair,DWORD> UsageMap;

private:
    UINT mOrdinal;
    bool mInited;

    std::wstring mDeviceName;

    WORD mVendorId;
    WORD mDeviceId;
    const char *mDescription;

    D3DCAPS9 mCaps;
    UsageMap mUsage;

    Limits mLimits;

    void init_limits();
    void init_caps();
    void init_ids();
    void init_usage();

public:
    D3DAdapter(UINT adapter_num);

    bool init();
    UINT getOrdinal() const { return mOrdinal; }
    const Limits& getLimits() const { return mLimits; }
    const std::wstring &getDeviceName() const { return mDeviceName; }
    D3DCAPS9 getCaps() const { return mCaps; }
    WORD getVendorId() const { return mVendorId; }
    WORD getDeviceId() const { return mDeviceId; }
    const char *getDescription() const { return mDescription; }
    DWORD getUsage(DWORD restype, D3DFORMAT format) const;

    UINT getModeCount(D3DFORMAT format) const;
    HRESULT getModeInfo(D3DFORMAT format, UINT mode, D3DDISPLAYMODE *info) const;
};
extern std::vector<D3DAdapter> gAdapterList;

#endif /* ADAPTER_HPP */
