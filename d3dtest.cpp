#include <stdio.h>

#include <d3d9.h>

typedef IDirect3D9*(WINAPI *LPDIRECT3DCREATE9)(UINT);

static HMODULE d3d9_handle = nullptr;
static LPDIRECT3DCREATE9 pDirect3DCreate9 = nullptr;

int main()
{
    IDirect3D9 *d3d = nullptr;
    HRESULT hr;

    // Only difference is the lib name and the API used to load the Create function
    d3d9_handle = LoadLibraryA("d3d9.dll");
    pDirect3DCreate9 = reinterpret_cast<LPDIRECT3DCREATE9>(GetProcAddress(d3d9_handle, "Direct3DCreate9"));

    if(!pDirect3DCreate9)
        fprintf(stderr, "Could not load Direct3DCreate9 from d3d9!\n");
    else
        d3d = pDirect3DCreate9(D3D_SDK_VERSION);
    if(d3d)
    {
        UINT numAdapters, i;

        numAdapters = d3d->GetAdapterCount();
        printf("Found %u adapter%s\n", numAdapters, (numAdapters==1)?"":"s");

        for(i = 0;i < numAdapters;i++)
        {
            D3DADAPTER_IDENTIFIER9 ident;
            D3DDISPLAYMODE disp;
            UINT modes;

            hr = d3d->GetAdapterIdentifier(i, 0, &ident);
            if(SUCCEEDED(hr))
            {
                printf("* Adapter %u:\n"
                       "   Driver:      %s\n"
                       "   Description: %s\n"
                       "   DeviceName:  %s\n",
                       i, ident.Driver, ident.Description, ident.DeviceName);
            }
            else
                printf("GetAdapterIdentifier failed: 0x%lx\n", hr);

            modes = d3d->GetAdapterModeCount(i, D3DFMT_X8R8G8B8);
            printf("  X8R8G8B8 modes available: %u\n", modes);
            modes = d3d->GetAdapterModeCount(i, D3DFMT_R5G6B5);
            printf("  R5G6B5 modes available: %u\n", modes);

            hr = d3d->GetAdapterDisplayMode(i, &disp);
            if(SUCCEEDED(hr))
            {
                D3DMULTISAMPLE_TYPE stype;
                DWORD q = 0;

                printf("  Current Mode:\n"
                       "   Width   = %u\n"
                       "   Height  = %u\n"
                       "   Refresh = %u\n"
                       "   Format  = 0x%x\n",
                       disp.Width, disp.Height, disp.RefreshRate, disp.Format);

                for(stype = D3DMULTISAMPLE_16_SAMPLES;stype > D3DMULTISAMPLE_NONE;stype=(D3DMULTISAMPLE_TYPE)((int)stype-1))
                {
                    hr = d3d->CheckDeviceMultiSampleType(i, D3DDEVTYPE_HAL, disp.Format, FALSE, stype, &q);
                    if(SUCCEEDED(hr))
                    {
                        printf("  %u-sample multisampling is supported (%lu quality level%s) on the current format\n", stype, q, (q==1)?"":"s");
                        break;
                    }
                }
                if(FAILED(hr))
                    printf("  Multisampling is not supported on the current format\n");

                hr = d3d->CheckDeviceType(i, D3DDEVTYPE_HAL, disp.Format, D3DFMT_A8R8G8B8, FALSE);
                printf("  Can%s use A8R8G8B8 on the current display format\n", SUCCEEDED(hr)?"":" not");

                hr = d3d->CheckDeviceFormatConversion(i, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, disp.Format);
                printf("  Can%s accelerate swapchain-conversions from A8R8G8B8 to current display format\n", SUCCEEDED(hr)?"":" not");

                hr = d3d->CheckDeviceFormat(i, D3DDEVTYPE_HAL, disp.Format, D3DUSAGE_QUERY_SRGBREAD, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8);
                printf("  Reading sRGB-corrected X8R8G8B8 textures is%s supported\n", SUCCEEDED(hr)?"":" not");

                D3DCAPS9 caps;
                hr = d3d->GetDeviceCaps(i, D3DDEVTYPE_HAL, &caps);
                printf("  VertexShader %lu.%lu and PixelShader %lu.%lu detected\n", D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));
            }
            else
                printf("GetAdapterDisplayMode failed: 0x%lx\n", hr);

            fflush(stdout);
        }

        d3d->Release();
    }

    return 0;
}
