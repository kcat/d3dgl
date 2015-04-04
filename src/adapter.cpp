
#include "adapter.hpp"

#include "glew.h"
#include "trace.hpp"
#include "d3dgl.hpp"


namespace
{

template<typename T, size_t N>
size_t countof(const T(&)[N])
{ return N; }


/* NOTE: We only bother with GL3+ capable cards here. Some more can be trimmed... */

/* The driver names reflect the lowest GPU supported
 * by a certain driver, so DRIVER_AMD_R300 supports
 * R3xx, R4xx and R5xx GPUs. */
enum d3dgl_display_driver {
    DRIVER_AMD_RAGE_128PRO,
    DRIVER_AMD_R100,
    DRIVER_AMD_R300,
    DRIVER_AMD_R600,
    DRIVER_INTEL_GMA800,
    DRIVER_INTEL_GMA900,
    DRIVER_INTEL_GMA950,
    DRIVER_INTEL_GMA3000,
    DRIVER_NVIDIA_GEFORCE8,
    DRIVER_VMWARE,
    DRIVER_UNKNOWN
};
enum d3dgl_driver_model {
    DRIVER_MODEL_WIN9X,
    DRIVER_MODEL_NT40,
    DRIVER_MODEL_NT5X,
    DRIVER_MODEL_NT6X
};

enum d3dgl_pci_vendor {
    HW_VENDOR_SOFTWARE              = 0x0000,
    HW_VENDOR_AMD                   = 0x1002,
    HW_VENDOR_NVIDIA                = 0x10de,
    HW_VENDOR_VMWARE                = 0x15ad,
    HW_VENDOR_INTEL                 = 0x8086,
};
enum d3dgl_pci_device {
    CARD_WINE                       = 0x0000,

    CARD_AMD_RAGE_128PRO            = 0x5246,
    CARD_AMD_RADEON_7200            = 0x5144,
    CARD_AMD_RADEON_8500            = 0x514c,
    CARD_AMD_RADEON_9500            = 0x4144,
    CARD_AMD_RADEON_XPRESS_200M     = 0x5955,
    CARD_AMD_RADEON_X700            = 0x5e4c,
    CARD_AMD_RADEON_X1600           = 0x71c2,
    CARD_AMD_RADEON_HD2350          = 0x94c7,
    CARD_AMD_RADEON_HD2600          = 0x9581,
    CARD_AMD_RADEON_HD2900          = 0x9400,
    CARD_AMD_RADEON_HD3200          = 0x9620,
    CARD_AMD_RADEON_HD4200M         = 0x9712,
    CARD_AMD_RADEON_HD4350          = 0x954f,
    CARD_AMD_RADEON_HD4600          = 0x9495,
    CARD_AMD_RADEON_HD4700          = 0x944e,
    CARD_AMD_RADEON_HD4800          = 0x944c,
    CARD_AMD_RADEON_HD5400          = 0x68f9,
    CARD_AMD_RADEON_HD5600          = 0x68d8,
    CARD_AMD_RADEON_HD5700          = 0x68be,
    CARD_AMD_RADEON_HD5800          = 0x6898,
    CARD_AMD_RADEON_HD5900          = 0x689c,
    CARD_AMD_RADEON_HD6300          = 0x9803,
    CARD_AMD_RADEON_HD6400          = 0x6770,
    CARD_AMD_RADEON_HD6410D         = 0x9644,
    CARD_AMD_RADEON_HD6550D         = 0x9640,
    CARD_AMD_RADEON_HD6600          = 0x6758,
    CARD_AMD_RADEON_HD6600M         = 0x6741,
    CARD_AMD_RADEON_HD6700          = 0x68ba,
    CARD_AMD_RADEON_HD6800          = 0x6739,
    CARD_AMD_RADEON_HD6900          = 0x6719,
    CARD_AMD_RADEON_HD7660D         = 0x9901,
    CARD_AMD_RADEON_HD7700          = 0x683d,
    CARD_AMD_RADEON_HD7800          = 0x6819,
    CARD_AMD_RADEON_HD7900          = 0x679a,
    CARD_AMD_RADEON_HD8600M         = 0x6660,
    CARD_AMD_RADEON_HD8670          = 0x6610,
    CARD_AMD_RADEON_HD8770          = 0x665c,
    CARD_AMD_RADEON_R3              = 0x9830,
    CARD_AMD_RADEON_R7              = 0x130f,
    CARD_AMD_RADEON_R9              = 0x67b1,

    CARD_NVIDIA_GEFORCE_8300GS      = 0x0423,
    CARD_NVIDIA_GEFORCE_8400GS      = 0x0404,
    CARD_NVIDIA_GEFORCE_8500GT      = 0x0421,
    CARD_NVIDIA_GEFORCE_8600GT      = 0x0402,
    CARD_NVIDIA_GEFORCE_8600MGT     = 0x0407,
    CARD_NVIDIA_GEFORCE_8800GTS     = 0x0193,
    CARD_NVIDIA_GEFORCE_8800GTX     = 0x0191,
    CARD_NVIDIA_GEFORCE_9200        = 0x086d,
    CARD_NVIDIA_GEFORCE_9300        = 0x086c,
    CARD_NVIDIA_GEFORCE_9400M       = 0x0863,
    CARD_NVIDIA_GEFORCE_9400GT      = 0x042c,
    CARD_NVIDIA_GEFORCE_9500GT      = 0x0640,
    CARD_NVIDIA_GEFORCE_9600GT      = 0x0622,
    CARD_NVIDIA_GEFORCE_9800GT      = 0x0614,
    CARD_NVIDIA_GEFORCE_210         = 0x0a23,
    CARD_NVIDIA_GEFORCE_GT220       = 0x0a20,
    CARD_NVIDIA_GEFORCE_GT240       = 0x0ca3,
    CARD_NVIDIA_GEFORCE_GTX260      = 0x05e2,
    CARD_NVIDIA_GEFORCE_GTX275      = 0x05e6,
    CARD_NVIDIA_GEFORCE_GTX280      = 0x05e1,
    CARD_NVIDIA_GEFORCE_315M        = 0x0a7a,
    CARD_NVIDIA_GEFORCE_320M        = 0x08a3,
    CARD_NVIDIA_GEFORCE_GT320M      = 0x0a2d,
    CARD_NVIDIA_GEFORCE_GT325M      = 0x0a35,
    CARD_NVIDIA_GEFORCE_GT330       = 0x0ca0,
    CARD_NVIDIA_GEFORCE_GTS350M     = 0x0cb0,
    CARD_NVIDIA_GEFORCE_410M        = 0x1055,
    CARD_NVIDIA_GEFORCE_GT420       = 0x0de2,
    CARD_NVIDIA_GEFORCE_GT430       = 0x0de1,
    CARD_NVIDIA_GEFORCE_GT440       = 0x0de0,
    CARD_NVIDIA_GEFORCE_GTS450      = 0x0dc4,
    CARD_NVIDIA_GEFORCE_GTX460      = 0x0e22,
    CARD_NVIDIA_GEFORCE_GTX460M     = 0x0dd1,
    CARD_NVIDIA_GEFORCE_GTX465      = 0x06c4,
    CARD_NVIDIA_GEFORCE_GTX470      = 0x06cd,
    CARD_NVIDIA_GEFORCE_GTX480      = 0x06c0,
    CARD_NVIDIA_GEFORCE_GT520       = 0x1040,
    CARD_NVIDIA_GEFORCE_GT540M      = 0x0df4,
    CARD_NVIDIA_GEFORCE_GTX550      = 0x1244,
    CARD_NVIDIA_GEFORCE_GT555M      = 0x04b8,
    CARD_NVIDIA_GEFORCE_GTX560TI    = 0x1200,
    CARD_NVIDIA_GEFORCE_GTX560      = 0x1201,
    CARD_NVIDIA_GEFORCE_GTX570      = 0x1081,
    CARD_NVIDIA_GEFORCE_GTX580      = 0x1080,
    CARD_NVIDIA_GEFORCE_GT610       = 0x104a,
    CARD_NVIDIA_GEFORCE_GT630       = 0x0f00,
    CARD_NVIDIA_GEFORCE_GT630M      = 0x0de9,
    CARD_NVIDIA_GEFORCE_GT640M      = 0x0fd2,
    CARD_NVIDIA_GEFORCE_GT650M      = 0x0fd1,
    CARD_NVIDIA_GEFORCE_GTX650      = 0x0fc6,
    CARD_NVIDIA_GEFORCE_GTX650TI    = 0x11c6,
    CARD_NVIDIA_GEFORCE_GTX660      = 0x11c0,
    CARD_NVIDIA_GEFORCE_GTX660M     = 0x0fd4,
    CARD_NVIDIA_GEFORCE_GTX660TI    = 0x1183,
    CARD_NVIDIA_GEFORCE_GTX670      = 0x1189,
    CARD_NVIDIA_GEFORCE_GTX670MX    = 0x11a1,
    CARD_NVIDIA_GEFORCE_GTX680      = 0x1180,
    CARD_NVIDIA_GEFORCE_GT750M      = 0x0fe9,
    CARD_NVIDIA_GEFORCE_GTX750      = 0x1381,
    CARD_NVIDIA_GEFORCE_GTX750TI    = 0x1380,
    CARD_NVIDIA_GEFORCE_GTX760      = 0x1187,
    CARD_NVIDIA_GEFORCE_GTX765M     = 0x11e2,
    CARD_NVIDIA_GEFORCE_GTX770M     = 0x11e0,
    CARD_NVIDIA_GEFORCE_GTX770      = 0x1184,
    CARD_NVIDIA_GEFORCE_GTX780      = 0x1004,
    CARD_NVIDIA_GEFORCE_GTX780TI    = 0x100a,
    CARD_NVIDIA_GEFORCE_GTX970      = 0x13c2,

    CARD_VMWARE_SVGA3D              = 0x0405,

    CARD_INTEL_830M                 = 0x3577,
    CARD_INTEL_855GM                = 0x3582,
    CARD_INTEL_845G                 = 0x2562,
    CARD_INTEL_865G                 = 0x2572,
    CARD_INTEL_915G                 = 0x2582,
    CARD_INTEL_E7221G               = 0x258a,
    CARD_INTEL_915GM                = 0x2592,
    CARD_INTEL_945G                 = 0x2772,
    CARD_INTEL_945GM                = 0x27a2,
    CARD_INTEL_945GME               = 0x27ae,
    CARD_INTEL_Q35                  = 0x29b2,
    CARD_INTEL_G33                  = 0x29c2,
    CARD_INTEL_Q33                  = 0x29d2,
    CARD_INTEL_PNVG                 = 0xa001,
    CARD_INTEL_PNVM                 = 0xa011,
    CARD_INTEL_965Q                 = 0x2992,
    CARD_INTEL_965G                 = 0x2982,
    CARD_INTEL_946GZ                = 0x2972,
    CARD_INTEL_965GM                = 0x2a02,
    CARD_INTEL_965GME               = 0x2a12,
    CARD_INTEL_GM45                 = 0x2a42,
    CARD_INTEL_IGD                  = 0x2e02,
    CARD_INTEL_Q45                  = 0x2e12,
    CARD_INTEL_G45                  = 0x2e22,
    CARD_INTEL_G41                  = 0x2e32,
    CARD_INTEL_B43                  = 0x2e92,
    CARD_INTEL_ILKD                 = 0x0042,
    CARD_INTEL_ILKM                 = 0x0046,
    CARD_INTEL_SNBD                 = 0x0122,
    CARD_INTEL_SNBM                 = 0x0126,
    CARD_INTEL_SNBS                 = 0x010a,
    CARD_INTEL_IVBD                 = 0x0162,
    CARD_INTEL_IVBM                 = 0x0166,
    CARD_INTEL_IVBS                 = 0x015a,
    CARD_INTEL_HWM                  = 0x0416,
};

static const struct d3dgl_renderer_table
{
    const char renderer[16];
    enum d3dgl_pci_device id;
}
cards_nvidia_binary[] =
{
    /* Direct 3D 11 */
    {"GTX 970",                     CARD_NVIDIA_GEFORCE_GTX970},    /* GeForce 900 - highend */
    {"GTX 780 Ti",                  CARD_NVIDIA_GEFORCE_GTX780TI},  /* Geforce 700 - highend */
    {"GTX 780",                     CARD_NVIDIA_GEFORCE_GTX780},    /* Geforce 700 - highend */
    {"GTX 770M",                    CARD_NVIDIA_GEFORCE_GTX770M},   /* Geforce 700 - midend high mobile */
    {"GTX 770",                     CARD_NVIDIA_GEFORCE_GTX770},    /* Geforce 700 - highend */
    {"GTX 765M",                    CARD_NVIDIA_GEFORCE_GTX765M},   /* Geforce 700 - midend high mobile */
    {"GTX 760",                     CARD_NVIDIA_GEFORCE_GTX760},    /* Geforce 700 - midend high  */
    {"GTX 750 Ti",                  CARD_NVIDIA_GEFORCE_GTX750TI},  /* Geforce 700 - midend */
    {"GTX 750",                     CARD_NVIDIA_GEFORCE_GTX750},    /* Geforce 700 - midend */
    {"GT 750M",                     CARD_NVIDIA_GEFORCE_GT750M},    /* Geforce 700 - midend mobile */
    {"GTX 680",                     CARD_NVIDIA_GEFORCE_GTX680},    /* Geforce 600 - highend */
    {"GTX 670MX",                   CARD_NVIDIA_GEFORCE_GTX670MX},  /* Geforce 600 - highend */
    {"GTX 670",                     CARD_NVIDIA_GEFORCE_GTX670},    /* Geforce 600 - midend high */
    {"GTX 660 Ti",                  CARD_NVIDIA_GEFORCE_GTX660TI},  /* Geforce 600 - midend high */
    {"GTX 660M",                    CARD_NVIDIA_GEFORCE_GTX660M},   /* Geforce 600 - midend high mobile */
    {"GTX 660",                     CARD_NVIDIA_GEFORCE_GTX660},    /* Geforce 600 - midend high */
    {"GTX 650 Ti",                  CARD_NVIDIA_GEFORCE_GTX650TI},  /* Geforce 600 - lowend */
    {"GTX 650",                     CARD_NVIDIA_GEFORCE_GTX650},    /* Geforce 600 - lowend */
    {"GT 650M",                     CARD_NVIDIA_GEFORCE_GT650M},    /* Geforce 600 - midend mobile */
    {"GT 640M",                     CARD_NVIDIA_GEFORCE_GT640M},    /* Geforce 600 - midend mobile */
    {"GT 630M",                     CARD_NVIDIA_GEFORCE_GT630M},    /* Geforce 600 - midend mobile */
    {"GT 630",                      CARD_NVIDIA_GEFORCE_GT630},     /* Geforce 600 - lowend */
    {"GT 610",                      CARD_NVIDIA_GEFORCE_GT610},     /* Geforce 600 - lowend */
    {"GTX 580",                     CARD_NVIDIA_GEFORCE_GTX580},    /* Geforce 500 - highend */
    {"GTX 570",                     CARD_NVIDIA_GEFORCE_GTX570},    /* Geforce 500 - midend high */
    {"GTX 560 Ti",                  CARD_NVIDIA_GEFORCE_GTX560TI},  /* Geforce 500 - midend */
    {"GTX 560",                     CARD_NVIDIA_GEFORCE_GTX560},    /* Geforce 500 - midend */
    {"GT 555M",                     CARD_NVIDIA_GEFORCE_GT555M},    /* Geforce 500 - midend mobile */
    {"GTX 550 Ti",                  CARD_NVIDIA_GEFORCE_GTX550},    /* Geforce 500 - midend */
    {"GT 540M",                     CARD_NVIDIA_GEFORCE_GT540M},    /* Geforce 500 - midend mobile */
    {"GT 520",                      CARD_NVIDIA_GEFORCE_GT520},     /* Geforce 500 - lowend */
    {"GTX 480",                     CARD_NVIDIA_GEFORCE_GTX480},    /* Geforce 400 - highend */
    {"GTX 470",                     CARD_NVIDIA_GEFORCE_GTX470},    /* Geforce 400 - midend high */
    /* Direct 3D 10 */
    {"GTX 465",                     CARD_NVIDIA_GEFORCE_GTX465},    /* Geforce 400 - midend */
    {"GTX 460M",                    CARD_NVIDIA_GEFORCE_GTX460M},   /* Geforce 400 - highend mobile */
    {"GTX 460",                     CARD_NVIDIA_GEFORCE_GTX460},    /* Geforce 400 - midend */
    {"GTS 450",                     CARD_NVIDIA_GEFORCE_GTS450},    /* Geforce 400 - midend low */
    {"GT 440",                      CARD_NVIDIA_GEFORCE_GT440},     /* Geforce 400 - lowend */
    {"GT 430",                      CARD_NVIDIA_GEFORCE_GT430},     /* Geforce 400 - lowend */
    {"GT 420",                      CARD_NVIDIA_GEFORCE_GT420},     /* Geforce 400 - lowend */
    {"410M",                        CARD_NVIDIA_GEFORCE_410M},      /* Geforce 400 - lowend mobile */
    {"GT 330",                      CARD_NVIDIA_GEFORCE_GT330},     /* Geforce 300 - highend */
    {"GTS 360M",                    CARD_NVIDIA_GEFORCE_GTS350M},   /* Geforce 300 - highend mobile */
    {"GTS 350M",                    CARD_NVIDIA_GEFORCE_GTS350M},   /* Geforce 300 - highend mobile */
    {"GT 330M",                     CARD_NVIDIA_GEFORCE_GT325M},    /* Geforce 300 - midend mobile */
    {"GT 325M",                     CARD_NVIDIA_GEFORCE_GT325M},    /* Geforce 300 - midend mobile */
    {"GT 320M",                     CARD_NVIDIA_GEFORCE_GT320M},    /* Geforce 300 - midend mobile */
    {"320M",                        CARD_NVIDIA_GEFORCE_320M},      /* Geforce 300 - midend mobile */
    {"315M",                        CARD_NVIDIA_GEFORCE_315M},      /* Geforce 300 - midend mobile */
    {"GTX 295",                     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
    {"GTX 285",                     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
    {"GTX 280",                     CARD_NVIDIA_GEFORCE_GTX280},    /* Geforce 200 - highend */
    {"GTX 275",                     CARD_NVIDIA_GEFORCE_GTX275},    /* Geforce 200 - midend high */
    {"GTX 260",                     CARD_NVIDIA_GEFORCE_GTX260},    /* Geforce 200 - midend */
    {"GT 240",                      CARD_NVIDIA_GEFORCE_GT240},     /* Geforce 200 - midend */
    {"GT 220",                      CARD_NVIDIA_GEFORCE_GT220},     /* Geforce 200 - lowend */
    {"GeForce 310",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GeForce 305",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GeForce 210",                 CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"G 210",                       CARD_NVIDIA_GEFORCE_210},       /* Geforce 200 - lowend */
    {"GTS 250",                     CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
    {"GTS 150",                     CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
    {"9800",                        CARD_NVIDIA_GEFORCE_9800GT},    /* Geforce 9 - highend / Geforce 200 - midend */
    {"GT 140",                      CARD_NVIDIA_GEFORCE_9600GT},    /* Geforce 9 - midend */
    {"9600",                        CARD_NVIDIA_GEFORCE_9600GT},    /* Geforce 9 - midend */
    {"GT 130",                      CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
    {"GT 120",                      CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
    {"9500",                        CARD_NVIDIA_GEFORCE_9500GT},    /* Geforce 9 - midend low / Geforce 200 - low */
    {"9400M",                       CARD_NVIDIA_GEFORCE_9400M},     /* Geforce 9 - lowend */
    {"9400",                        CARD_NVIDIA_GEFORCE_9400GT},    /* Geforce 9 - lowend */
    {"9300",                        CARD_NVIDIA_GEFORCE_9300},      /* Geforce 9 - lowend low */
    {"9200",                        CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
    {"9100",                        CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
    {"G 100",                       CARD_NVIDIA_GEFORCE_9200},      /* Geforce 9 - lowend low */
    {"8800 GTX",                    CARD_NVIDIA_GEFORCE_8800GTX},   /* Geforce 8 - highend high */
    {"8800",                        CARD_NVIDIA_GEFORCE_8800GTS},   /* Geforce 8 - highend */
    {"8600M",                       CARD_NVIDIA_GEFORCE_8600MGT},   /* Geforce 8 - midend mobile */
    {"8600 M",                      CARD_NVIDIA_GEFORCE_8600MGT},   /* Geforce 8 - midend mobile */
    {"8700",                        CARD_NVIDIA_GEFORCE_8600GT},    /* Geforce 8 - midend */
    {"8600",                        CARD_NVIDIA_GEFORCE_8600GT},    /* Geforce 8 - midend */
    {"8500",                        CARD_NVIDIA_GEFORCE_8400GS},    /* Geforce 8 - mid-lowend */
    {"8400",                        CARD_NVIDIA_GEFORCE_8400GS},    /* Geforce 8 - mid-lowend */
    {"8300",                        CARD_NVIDIA_GEFORCE_8300GS},    /* Geforce 8 - lowend */
    {"8200",                        CARD_NVIDIA_GEFORCE_8300GS},    /* Geforce 8 - lowend */
    {"8100",                        CARD_NVIDIA_GEFORCE_8300GS},    /* Geforce 8 - lowend */
};

static const struct d3dgl_vendor_table
{
    const char vendor[16];
    enum d3dgl_pci_vendor id;
    const d3dgl_renderer_table *renderer_list;
    size_t list_size;
}
device_vendors[] = {
    {"NVIDIA", HW_VENDOR_NVIDIA,  cards_nvidia_binary, countof(cards_nvidia_binary) },
};


struct gpu_description
{
    d3dgl_pci_vendor vendor;        /* reported PCI card vendor ID  */
    d3dgl_pci_device card;          /* reported PCI card device ID  */
    const char *description;        /* Description of the card e.g. NVIDIA RIVA TNT */
    d3dgl_display_driver driver;
    unsigned int vidmem;
};

/* The amount of video memory stored in the gpu description table is the minimum amount of video memory
 * found on a board containing a specific GPU. */
static const gpu_description gpu_description_table[] =
{
    /* Nvidia cards */
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8300GS,     "NVIDIA GeForce 8300 GS",           DRIVER_NVIDIA_GEFORCE8,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8400GS,     "NVIDIA GeForce 8400 GS",           DRIVER_NVIDIA_GEFORCE8,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600GT,     "NVIDIA GeForce 8600 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600MGT,    "NVIDIA GeForce 8600M GT",          DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTS,    "NVIDIA GeForce 8800 GTS",          DRIVER_NVIDIA_GEFORCE8,  320 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTX,    "NVIDIA GeForce 8800 GTX",          DRIVER_NVIDIA_GEFORCE8,  768 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9200,       "NVIDIA GeForce 9200",              DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9300,       "NVIDIA GeForce 9300",              DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400M,      "NVIDIA GeForce 9400M",             DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400GT,     "NVIDIA GeForce 9400 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9500GT,     "NVIDIA GeForce 9500 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9600GT,     "NVIDIA GeForce 9600 GT",           DRIVER_NVIDIA_GEFORCE8,  384 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9800GT,     "NVIDIA GeForce 9800 GT",           DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_210,        "NVIDIA GeForce 210",               DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT220,      "NVIDIA GeForce GT 220",            DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT240,      "NVIDIA GeForce GT 240",            DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX260,     "NVIDIA GeForce GTX 260",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX275,     "NVIDIA GeForce GTX 275",           DRIVER_NVIDIA_GEFORCE8,  896 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX280,     "NVIDIA GeForce GTX 280",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_315M,       "NVIDIA GeForce 315M",              DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_320M,       "NVIDIA GeForce 320M",              DRIVER_NVIDIA_GEFORCE8,  256},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_410M,       "NVIDIA GeForce 410M",              DRIVER_NVIDIA_GEFORCE8,  512},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT320M,     "NVIDIA GeForce GT 320M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT325M,     "NVIDIA GeForce GT 325M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT330,      "NVIDIA GeForce GT 330",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS350M,    "NVIDIA GeForce GTS 350M",          DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT420,      "NVIDIA GeForce GT 420",            DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT430,      "NVIDIA GeForce GT 430",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT440,      "NVIDIA GeForce GT 440",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS450,     "NVIDIA GeForce GTS 450",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX460,     "NVIDIA GeForce GTX 460",           DRIVER_NVIDIA_GEFORCE8,  768 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX460M,    "NVIDIA GeForce GTX 460M",          DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX465,     "NVIDIA GeForce GTX 465",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX470,     "NVIDIA GeForce GTX 470",           DRIVER_NVIDIA_GEFORCE8,  1280},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX480,     "NVIDIA GeForce GTX 480",           DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT520,      "NVIDIA GeForce GT 520",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT540M,     "NVIDIA GeForce GT 540M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX550,     "NVIDIA GeForce GTX 550 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT555M,     "NVIDIA GeForce GT 555M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560TI,   "NVIDIA GeForce GTX 560 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560,     "NVIDIA GeForce GTX 560",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX570,     "NVIDIA GeForce GTX 570",           DRIVER_NVIDIA_GEFORCE8,  1280},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX580,     "NVIDIA GeForce GTX 580",           DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT610,      "NVIDIA GeForce GT 610",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT630,      "NVIDIA GeForce GT 630",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT630M,     "NVIDIA GeForce GT 630M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT640M,     "NVIDIA GeForce GT 640M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT650M,     "NVIDIA GeForce GT 650M",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX650,     "NVIDIA GeForce GTX 650",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX650TI,   "NVIDIA GeForce GTX 650 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660,     "NVIDIA GeForce GTX 660",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660M,    "NVIDIA GeForce GTX 660M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660TI,   "NVIDIA GeForce GTX 660 Ti",        DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX670,     "NVIDIA GeForce GTX 670",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX670MX,   "NVIDIA GeForce GTX 670MX",         DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX680,     "NVIDIA GeForce GTX 680",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT750M,     "NVIDIA GeForce GT 750M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX750,     "NVIDIA GeForce GTX 750",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX750TI,   "NVIDIA GeForce GTX 750 Ti",        DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX760,     "NVIDIA Geforce GTX 760",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX765M,    "NVIDIA GeForce GTX 765M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX770M,    "NVIDIA GeForce GTX 770M",          DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX770,     "NVIDIA GeForce GTX 770",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX780,     "NVIDIA GeForce GTX 780",           DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX780TI,   "NVIDIA GeForce GTX 780 Ti",        DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX970,     "NVIDIA GeForce GTX 970",           DRIVER_NVIDIA_GEFORCE8,  4096},

    /* AMD cards */
    {HW_VENDOR_AMD,        CARD_AMD_RAGE_128PRO,           "ATI Rage Fury",                    DRIVER_AMD_RAGE_128PRO,  16  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_7200,           "ATI RADEON 7200 SERIES",           DRIVER_AMD_R100,         32  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_8500,           "ATI RADEON 8500 SERIES",           DRIVER_AMD_R100,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_9500,           "ATI Radeon 9500",                  DRIVER_AMD_R300,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_XPRESS_200M,    "ATI RADEON XPRESS 200M Series",    DRIVER_AMD_R300,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_X700,           "ATI Radeon X700 SE",               DRIVER_AMD_R300,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_X1600,          "ATI Radeon X1600 Series",          DRIVER_AMD_R300,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2350,         "ATI Mobility Radeon HD 2350",      DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2600,         "ATI Mobility Radeon HD 2600",      DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2900,         "ATI Radeon HD 2900 XT",            DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD3200,         "ATI Radeon HD 3200 Graphics",      DRIVER_AMD_R600,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4200M,        "ATI Mobility Radeon HD 4200",      DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4350,         "ATI Radeon HD 4350",               DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4600,         "ATI Radeon HD 4600 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4700,         "ATI Radeon HD 4700 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4800,         "ATI Radeon HD 4800 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5400,         "ATI Radeon HD 5400 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5600,         "ATI Radeon HD 5600 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5700,         "ATI Radeon HD 5700 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5800,         "ATI Radeon HD 5800 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5900,         "ATI Radeon HD 5900 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6300,         "AMD Radeon HD 6300 series Graphics", DRIVER_AMD_R600,       1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6400,         "AMD Radeon HD 6400 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6410D,        "AMD Radeon HD 6410D",              DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6550D,        "AMD Radeon HD 6550D",              DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6600,         "AMD Radeon HD 6600 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6600M,        "AMD Radeon HD 6600M Series",       DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6700,         "AMD Radeon HD 6700 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6800,         "AMD Radeon HD 6800 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6900,         "AMD Radeon HD 6900 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7660D,        "AMD Radeon HD 7660D",              DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7700,         "AMD Radeon HD 7700 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7800,         "AMD Radeon HD 7800 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7900,         "AMD Radeon HD 7900 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8600M,        "AMD Radeon HD 8600M Series",       DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8670,         "AMD Radeon HD 8670",               DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8770,         "AMD Radeon HD 8770",               DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R3,             "AMD Radeon HD 8400 / R3 Series",   DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R7,             "AMD Radeon(TM) R7 Graphics",       DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R9,             "AMD Radeon R9 290",                DRIVER_AMD_R600,         4096},

    /* VMware */
    {HW_VENDOR_VMWARE,     CARD_VMWARE_SVGA3D,             "VMware SVGA 3D (Microsoft Corporation - WDDM)",             DRIVER_VMWARE,        1024},

    /* Intel cards */
    {HW_VENDOR_INTEL,      CARD_INTEL_830M,                "Intel(R) 82830M Graphics Controller",                       DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_855GM,               "Intel(R) 82852/82855 GM/GME Graphics Controller",           DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_845G,                "Intel(R) 845G",                                             DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_865G,                "Intel(R) 82865G Graphics Controller",                       DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_915G,                "Intel(R) 82915G/GV/910GL Express Chipset Family",           DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_E7221G,              "Intel(R) E7221G",                                           DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_915GM,               "Mobile Intel(R) 915GM/GMS,910GML Express Chipset Family",   DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945G,                "Intel(R) 945G",                                             DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945GM,               "Mobile Intel(R) 945GM Express Chipset Family",              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945GME,              "Intel(R) 945GME",                                           DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_Q35,                 "Intel(R) Q35",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_G33,                 "Intel(R) G33",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_Q33,                 "Intel(R) Q33",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_PNVG,                "Intel(R) IGD",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_PNVM,                "Intel(R) IGD",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_965Q,                "Intel(R) 965Q",                                             DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965G,                "Intel(R) 965G",                                             DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_946GZ,               "Intel(R) 946GZ",                                            DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965GM,               "Mobile Intel(R) 965 Express Chipset Family",                DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965GME,              "Intel(R) 965GME",                                           DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_GM45,                "Mobile Intel(R) GM45 Express Chipset Family",               DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_IGD,                 "Intel(R) Integrated Graphics Device",                       DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_G45,                 "Intel(R) G45/G43",                                          DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_Q45,                 "Intel(R) Q45/Q43",                                          DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_G41,                 "Intel(R) G41",                                              DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_B43,                 "Intel(R) B43",                                              DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_ILKD,                "Intel(R) Ironlake Desktop",                                 DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_ILKM,                "Intel(R) Ironlake Mobile",                                  DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBD,                "Intel(R) Sandybridge Desktop",                              DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBM,                "Intel(R) Sandybridge Mobile",                               DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBS,                "Intel(R) Sandybridge Server",                               DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBD,                "Intel(R) Ivybridge Desktop",                                DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBM,                "Intel(R) Ivybridge Mobile",                                 DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBS,                "Intel(R) Ivybridge Server",                                 DRIVER_INTEL_GMA3000, 1024},
    {HW_VENDOR_INTEL,      CARD_INTEL_HWM,                 "Intel(R) Haswell Mobile",                                   DRIVER_INTEL_GMA3000, 1024},
};

}


D3DAdapter::D3DAdapter(UINT adapter_num)
  : mOrdinal(adapter_num)
  , mInited(false)
  , mVendorId(HW_VENDOR_SOFTWARE)
  , mDeviceId(CARD_WINE)
  , mDescription("Unknown Device")
{
    memset(&mCaps, 0, sizeof(mCaps));
}


void D3DAdapter::init_limits()
{
    GLfloat gl_floatv[2];
    GLint gl_max;

    mLimits.blends = 1;
    mLimits.buffers = 1;
    mLimits.textures = 1;
    mLimits.texture_coords = 1;
    mLimits.vertex_uniform_blocks = 0;
    mLimits.geometry_uniform_blocks = 0;
    mLimits.fragment_uniform_blocks = 0;
    mLimits.fragment_samplers = 1;
    mLimits.vertex_samplers = 0;
    mLimits.combined_samplers = mLimits.fragment_samplers + mLimits.vertex_samplers;
    mLimits.vertex_attribs = 16;
    mLimits.glsl_vs_float_constants = 0;
    mLimits.glsl_ps_float_constants = 0;
    mLimits.arb_vs_float_constants = 0;
    mLimits.arb_vs_native_constants = 0;
    mLimits.arb_vs_instructions = 0;
    mLimits.arb_vs_temps = 0;
    mLimits.arb_ps_float_constants = 0;
    mLimits.arb_ps_local_constants = 0;
    mLimits.arb_ps_instructions = 0;
    mLimits.arb_ps_temps = 0;

    glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
    mLimits.clipplanes = std::min(D3DMAXUSERCLIPPLANES, gl_max);
    TRACE("Clip plane support - max planes %d.\n", gl_max);

    glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    mLimits.lights = gl_max;
    TRACE("Light support - max lights %d.\n", gl_max);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max);
    mLimits.texture_size = gl_max;
    TRACE("Maximum texture size support - max texture size %d.\n", gl_max);

    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, gl_floatv);
    mLimits.pointsize_min = gl_floatv[0];
    mLimits.pointsize_max = gl_floatv[1];
    TRACE("Maximum point size support - max point size %f.\n", gl_floatv[1]);

    if(GLEW_ARB_map_buffer_alignment)
    {
        glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &gl_max);
        TRACE("Minimum buffer map alignment: %d.\n", gl_max);
    }
    else
    {
        WARN("Driver doesn't guarantee a minimum buffer map alignment.\n");
    }
    if(GLEW_NV_register_combiners)
    {
        glGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &gl_max);
        mLimits.general_combiners = gl_max;
        TRACE("Max general combiners: %d.\n", gl_max);
    }

    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &gl_max);
    mLimits.buffers = gl_max;
    TRACE("Max draw buffers: %u.\n", gl_max);

    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &gl_max);
    mLimits.textures = std::min(MAX_TEXTURES, gl_max);
    TRACE("Max textures: %d.\n", mLimits.textures);

    {
        GLint tmp;
        glGetIntegerv(GL_MAX_TEXTURE_COORDS, &gl_max);
        mLimits.texture_coords = std::min(MAX_TEXTURES, gl_max);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &tmp);
        mLimits.fragment_samplers = std::min(MAX_FRAGMENT_SAMPLERS, tmp);
    }
    TRACE("Max texture coords: %d.\n", mLimits.texture_coords);
    TRACE("Max fragment samplers: %d.\n", mLimits.fragment_samplers);

    {
        GLint tmp;
        glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &tmp);
        mLimits.vertex_samplers = tmp;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &tmp);
        mLimits.combined_samplers = tmp;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &tmp);
        mLimits.vertex_attribs = tmp;

        /* Loading GLSL sampler uniforms is much simpler if we can assume that the sampler setup
         * is known at shader link time. In a vertex shader + pixel shader combination this isn't
         * an issue because then the sampler setup only depends on the two shaders. If a pixel
         * shader is used with fixed function vertex processing we're fine too because fixed function
         * vertex processing doesn't use any samplers. If fixed function fragment processing is
         * used we have to make sure that all vertex sampler setups are valid together with all
         * possible fixed function fragment processing setups. This is true if vsamplers + MAX_TEXTURES
         * <= max_samplers. This is true on all d3d9 cards that support vtf(gf 6 and gf7 cards).
         * dx9 radeon cards do not support vertex texture fetch. DX10 cards have 128 samplers, and
         * dx9 is limited to 8 fixed function texture stages and 4 vertex samplers. DX10 does not have
         * a fixed function pipeline anymore.
         *
         * So this is just a check to check that our assumption holds true. If not, write a warning
         * and reduce the number of vertex samplers or probably disable vertex texture fetch.
         */
        if(mLimits.vertex_samplers && mLimits.combined_samplers < 12
            && MAX_TEXTURES + mLimits.vertex_samplers > mLimits.combined_samplers)
        {
            FIXME("OpenGL implementation supports %u vertex samplers and %u total samplers.\n",
                  mLimits.vertex_samplers, mLimits.combined_samplers);
            FIXME("Expected vertex samplers + MAX_TEXTURES(=8) > combined_samplers.\n");
            if(mLimits.combined_samplers > MAX_TEXTURES)
                mLimits.vertex_samplers = mLimits.combined_samplers - MAX_TEXTURES;
            else
                mLimits.vertex_samplers = 0;
        }
    }
    TRACE("Max vertex samplers: %u.\n", mLimits.vertex_samplers);
    TRACE("Max combined samplers: %u.\n", mLimits.combined_samplers);

    if(GLEW_ARB_vertex_blend)
    {
        glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max);
        mLimits.blends = gl_max;
        TRACE("Max blends: %u.\n", mLimits.blends);
    }

    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &gl_max);
    mLimits.texture3d_size = gl_max;
    TRACE("Max texture3D size: %d.\n", mLimits.texture3d_size);

    if(GLEW_EXT_texture_filter_anisotropic)
    {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max);
        mLimits.anisotropy = gl_max;
        TRACE("Max anisotropy: %d.\n", mLimits.anisotropy);
    }
    if(GLEW_ARB_fragment_program)
    {
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max);
        mLimits.arb_ps_float_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM float constants: %d.\n", mLimits.arb_ps_float_constants);
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max);
        mLimits.arb_ps_native_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native float constants: %d.\n",
                mLimits.arb_ps_native_constants);
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max);
        mLimits.arb_ps_temps = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native temporaries: %d.\n", mLimits.arb_ps_temps);
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max);
        mLimits.arb_ps_instructions = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM native instructions: %d.\n", mLimits.arb_ps_instructions);
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &gl_max);
        mLimits.arb_ps_local_constants = gl_max;
        TRACE("Max ARB_FRAGMENT_PROGRAM local parameters: %d.\n", mLimits.arb_ps_instructions);
    }
    if(GLEW_ARB_vertex_program)
    {
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max);
        mLimits.arb_vs_float_constants = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM float constants: %d.\n", mLimits.arb_vs_float_constants);
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max);
        mLimits.arb_vs_native_constants = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native float constants: %d.\n", mLimits.arb_vs_native_constants);
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max);
        mLimits.arb_vs_temps = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native temporaries: %d.\n", mLimits.arb_vs_temps);
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max);
        mLimits.arb_vs_instructions = gl_max;
        TRACE("Max ARB_VERTEX_PROGRAM native instructions: %d.\n", mLimits.arb_vs_instructions);
    }

    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &gl_max);
    mLimits.glsl_vs_float_constants = gl_max / 4;
    TRACE("Max ARB_VERTEX_SHADER float constants: %u.\n", mLimits.glsl_vs_float_constants);

    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &gl_max);
    mLimits.vertex_uniform_blocks = std::min(gl_max, D3DGL_MAX_CBS);
    TRACE("Max vertex uniform blocks: %u (%d).\n", mLimits.vertex_uniform_blocks, gl_max);

    glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &gl_max);
    mLimits.geometry_uniform_blocks = std::min(gl_max, D3DGL_MAX_CBS);
    TRACE("Max geometry uniform blocks: %u (%d).\n", mLimits.geometry_uniform_blocks, gl_max);

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &gl_max);
    mLimits.glsl_ps_float_constants = gl_max / 4;
    TRACE("Max ARB_FRAGMENT_SHADER float constants: %u.\n", mLimits.glsl_ps_float_constants);
    glGetIntegerv(GL_MAX_VARYING_FLOATS, &gl_max);
    mLimits.glsl_varyings = gl_max;
    TRACE("Max GLSL varyings: %u (%u 4 component varyings).\n", gl_max, gl_max / 4);

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &gl_max);
    mLimits.fragment_uniform_blocks = std::min(gl_max, D3DGL_MAX_CBS);
    TRACE("Max fragment uniform blocks: %u (%d).\n", mLimits.fragment_uniform_blocks, gl_max);

    glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &gl_max);
    TRACE("Max combined uniform blocks: %d.\n", gl_max);
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &gl_max);
    TRACE("Max uniform buffer bindings: %d.\n", gl_max);

    if(GLEW_NV_light_max_exponent)
        glGetFloatv(GL_MAX_SHININESS_NV, &mLimits.shininess);
    else
        mLimits.shininess = 128.0f;

    glGetIntegerv(GL_MAX_SAMPLES, &gl_max);
    mLimits.samples = gl_max;
}

void D3DAdapter::init_caps()
{
    mCaps.DeviceType = D3DDEVTYPE_HAL;
    mCaps.AdapterOrdinal = mOrdinal;

    mCaps.Caps  = 0;
    mCaps.Caps2 = D3DCAPS2_CANRENDERWINDOWED |
                  D3DCAPS2_FULLSCREENGAMMA |
                  D3DCAPS2_DYNAMICTEXTURES;
    if(GLEW_SGIS_generate_mipmap)
        mCaps.Caps2 |= D3DCAPS2_CANAUTOGENMIPMAP;

    mCaps.Caps3 = D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD |
                  D3DCAPS3_COPY_TO_VIDMEM                   |
                  D3DCAPS3_COPY_TO_SYSTEMMEM;

    mCaps.PresentationIntervals = D3DPRESENT_INTERVAL_IMMEDIATE  |
                                  D3DPRESENT_INTERVAL_ONE;

    mCaps.CursorCaps = D3DCURSORCAPS_COLOR            |
                       D3DCURSORCAPS_LOWRES;

    mCaps.DevCaps = D3DDEVCAPS_EXECUTESYSTEMMEMORY |
                    D3DDEVCAPS_TLVERTEXSYSTEMMEMORY|
                    D3DDEVCAPS_TLVERTEXVIDEOMEMORY |
                    D3DDEVCAPS_DRAWPRIMTLVERTEX    |
                    D3DDEVCAPS_HWTRANSFORMANDLIGHT |
                    D3DDEVCAPS_EXECUTEVIDEOMEMORY  |
                    D3DDEVCAPS_PUREDEVICE          |
                    D3DDEVCAPS_HWRASTERIZATION     |
                    D3DDEVCAPS_TEXTUREVIDEOMEMORY  |
                    D3DDEVCAPS_TEXTURESYSTEMMEMORY |
                    D3DDEVCAPS_CANRENDERAFTERFLIP  |
                    D3DDEVCAPS_DRAWPRIMITIVES2     |
                    D3DDEVCAPS_DRAWPRIMITIVES2EX;

    mCaps.PrimitiveMiscCaps = D3DPMISCCAPS_CULLNONE              |
                              D3DPMISCCAPS_CULLCCW               |
                              D3DPMISCCAPS_CULLCW                |
                              D3DPMISCCAPS_COLORWRITEENABLE      |
                              D3DPMISCCAPS_CLIPTLVERTS           |
                              D3DPMISCCAPS_CLIPPLANESCALEDPOINTS |
                              D3DPMISCCAPS_MASKZ                 |
                              D3DPMISCCAPS_BLENDOP               |
                              D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING |
                              D3DPMISCCAPS_SEPARATEALPHABLEND    |
                              D3DPMISCCAPS_INDEPENDENTWRITEMASKS;
                              /* TODO:
                              D3DPMISCCAPS_NULLREFERENCE
                              D3DPMISCCAPS_FOGANDSPECULARALPHA
                              D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
                              D3DPMISCCAPS_FOGVERTEXCLAMPED */

    mCaps.RasterCaps = D3DPRASTERCAPS_DITHER    |
                       D3DPRASTERCAPS_PAT       |
                       D3DPRASTERCAPS_WFOG      |
                       D3DPRASTERCAPS_ZFOG      |
                       D3DPRASTERCAPS_FOGVERTEX |
                       D3DPRASTERCAPS_FOGTABLE  |
                       D3DPRASTERCAPS_ZTEST     |
                       D3DPRASTERCAPS_SCISSORTEST   |
                       D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS |
                       D3DPRASTERCAPS_DEPTHBIAS;

    if(GLEW_EXT_texture_filter_anisotropic)
        mCaps.RasterCaps |= D3DPRASTERCAPS_ANISOTROPY    |
                            D3DPRASTERCAPS_ZBIAS         |
                            D3DPRASTERCAPS_MIPMAPLODBIAS;

    mCaps.ZCmpCaps = D3DPCMPCAPS_ALWAYS       |
                     D3DPCMPCAPS_EQUAL        |
                     D3DPCMPCAPS_GREATER      |
                     D3DPCMPCAPS_GREATEREQUAL |
                     D3DPCMPCAPS_LESS         |
                     D3DPCMPCAPS_LESSEQUAL    |
                     D3DPCMPCAPS_NEVER        |
                     D3DPCMPCAPS_NOTEQUAL;

    /* D3DPBLENDCAPS_BOTHINVSRCALPHA and D3DPBLENDCAPS_BOTHSRCALPHA
     * are legacy settings for srcblend only. */
    mCaps.SrcBlendCaps  =  D3DPBLENDCAPS_BOTHINVSRCALPHA |
                           D3DPBLENDCAPS_BOTHSRCALPHA    |
                           D3DPBLENDCAPS_DESTALPHA       |
                           D3DPBLENDCAPS_DESTCOLOR       |
                           D3DPBLENDCAPS_INVDESTALPHA    |
                           D3DPBLENDCAPS_INVDESTCOLOR    |
                           D3DPBLENDCAPS_INVSRCALPHA     |
                           D3DPBLENDCAPS_INVSRCCOLOR     |
                           D3DPBLENDCAPS_ONE             |
                           D3DPBLENDCAPS_SRCALPHA        |
                           D3DPBLENDCAPS_SRCALPHASAT     |
                           D3DPBLENDCAPS_SRCCOLOR        |
                           D3DPBLENDCAPS_ZERO;

    mCaps.DestBlendCaps = D3DPBLENDCAPS_DESTALPHA       |
                          D3DPBLENDCAPS_DESTCOLOR       |
                          D3DPBLENDCAPS_INVDESTALPHA    |
                          D3DPBLENDCAPS_INVDESTCOLOR    |
                          D3DPBLENDCAPS_INVSRCALPHA     |
                          D3DPBLENDCAPS_INVSRCCOLOR     |
                          D3DPBLENDCAPS_ONE             |
                          D3DPBLENDCAPS_SRCALPHA        |
                          D3DPBLENDCAPS_SRCCOLOR        |
                          D3DPBLENDCAPS_ZERO            |
                          D3DPBLENDCAPS_SRCALPHASAT     |
                          D3DPBLENDCAPS_BLENDFACTOR     |
                          D3DPBLENDCAPS_BLENDFACTOR;

    mCaps.AlphaCmpCaps = D3DPCMPCAPS_ALWAYS       |
                         D3DPCMPCAPS_EQUAL        |
                         D3DPCMPCAPS_GREATER      |
                         D3DPCMPCAPS_GREATEREQUAL |
                         D3DPCMPCAPS_LESS         |
                         D3DPCMPCAPS_LESSEQUAL    |
                         D3DPCMPCAPS_NEVER        |
                         D3DPCMPCAPS_NOTEQUAL;

    mCaps.ShadeCaps = D3DPSHADECAPS_SPECULARGOURAUDRGB |
                      D3DPSHADECAPS_COLORGOURAUDRGB    |
                      D3DPSHADECAPS_ALPHAGOURAUDBLEND  |
                      D3DPSHADECAPS_FOGGOURAUD;

    mCaps.TextureCaps = D3DPTEXTURECAPS_ALPHA              |
                        D3DPTEXTURECAPS_ALPHAPALETTE       |
                        D3DPTEXTURECAPS_MIPMAP             |
                        D3DPTEXTURECAPS_PROJECTED          |
                        D3DPTEXTURECAPS_PERSPECTIVE        |
                        D3DPTEXTURECAPS_VOLUMEMAP          |
                        D3DPTEXTURECAPS_MIPVOLUMEMAP       |
                        D3DPTEXTURECAPS_CUBEMAP            |
                        D3DPTEXTURECAPS_MIPCUBEMAP         |
                        D3DPTEXTURECAPS_POW2 |
                        D3DPTEXTURECAPS_NONPOW2CONDITIONAL |
                        D3DPTEXTURECAPS_VOLUMEMAP_POW2 |
                        D3DPTEXTURECAPS_CUBEMAP_POW2;

    mCaps.TextureFilterCaps = D3DPTFILTERCAPS_MAGFLINEAR       |
                              D3DPTFILTERCAPS_MAGFPOINT        |
                              D3DPTFILTERCAPS_MINFLINEAR       |
                              D3DPTFILTERCAPS_MINFPOINT        |
                              D3DPTFILTERCAPS_MIPFLINEAR       |
                              D3DPTFILTERCAPS_MIPFPOINT;
    if(GLEW_EXT_texture_filter_anisotropic)
        mCaps.TextureFilterCaps |= D3DPTFILTERCAPS_MAGFANISOTROPIC |
                                   D3DPTFILTERCAPS_MINFANISOTROPIC;

    mCaps.CubeTextureFilterCaps = D3DPTFILTERCAPS_MAGFLINEAR       |
                                  D3DPTFILTERCAPS_MAGFPOINT        |
                                  D3DPTFILTERCAPS_MINFLINEAR       |
                                  D3DPTFILTERCAPS_MINFPOINT        |
                                  D3DPTFILTERCAPS_MIPFLINEAR       |
                                  D3DPTFILTERCAPS_MIPFPOINT;

    if(GLEW_EXT_texture_filter_anisotropic)
        mCaps.CubeTextureFilterCaps |= D3DPTFILTERCAPS_MAGFANISOTROPIC |
                                       D3DPTFILTERCAPS_MINFANISOTROPIC;

    mCaps.VolumeTextureFilterCaps = D3DPTFILTERCAPS_MAGFLINEAR       |
                                    D3DPTFILTERCAPS_MAGFPOINT        |
                                    D3DPTFILTERCAPS_MINFLINEAR       |
                                    D3DPTFILTERCAPS_MINFPOINT        |
                                    D3DPTFILTERCAPS_MIPFLINEAR       |
                                    D3DPTFILTERCAPS_MIPFPOINT;

    mCaps.TextureAddressCaps = D3DPTADDRESSCAPS_INDEPENDENTUV |
                               D3DPTADDRESSCAPS_CLAMP         |
                               D3DPTADDRESSCAPS_WRAP;
    mCaps.TextureAddressCaps |= D3DPTADDRESSCAPS_BORDER;
    mCaps.TextureAddressCaps |= D3DPTADDRESSCAPS_MIRROR;
    mCaps.TextureAddressCaps |= D3DPTADDRESSCAPS_MIRRORONCE;

    mCaps.VolumeTextureAddressCaps = D3DPTADDRESSCAPS_INDEPENDENTUV |
                                     D3DPTADDRESSCAPS_CLAMP  |
                                     D3DPTADDRESSCAPS_WRAP;
    mCaps.VolumeTextureAddressCaps |= D3DPTADDRESSCAPS_BORDER;
    mCaps.VolumeTextureAddressCaps |= D3DPTADDRESSCAPS_MIRROR;
    mCaps.VolumeTextureAddressCaps |= D3DPTADDRESSCAPS_MIRRORONCE;

    mCaps.LineCaps = D3DLINECAPS_TEXTURE       |
                     D3DLINECAPS_ZTEST         |
                     D3DLINECAPS_BLEND         |
                     D3DLINECAPS_ALPHACMP      |
                     D3DLINECAPS_FOG;
    /* D3DLINECAPS_ANTIALIAS is not supported on Windows, and dx and gl seem to have a different
     * idea how generating the smoothing alpha values works; the result is different
     */

    mCaps.MaxTextureWidth = mLimits.texture_size;
    mCaps.MaxTextureHeight = mLimits.texture_size;

    mCaps.MaxVolumeExtent = mLimits.texture3d_size;

    mCaps.MaxTextureRepeat = 32768;
    mCaps.MaxTextureAspectRatio = mLimits.texture_size;
    mCaps.MaxVertexW = 1.0f;

    mCaps.GuardBandLeft = 0.0f;
    mCaps.GuardBandTop = 0.0f;
    mCaps.GuardBandRight = 0.0f;
    mCaps.GuardBandBottom = 0.0f;

    mCaps.ExtentsAdjust = 0.0f;

    mCaps.StencilCaps = D3DSTENCILCAPS_DECRSAT |
                        D3DSTENCILCAPS_INCRSAT |
                        D3DSTENCILCAPS_INVERT  |
                        D3DSTENCILCAPS_KEEP    |
                        D3DSTENCILCAPS_REPLACE |
                        D3DSTENCILCAPS_ZERO;
    if(GLEW_EXT_stencil_wrap)
        mCaps.StencilCaps |= D3DSTENCILCAPS_DECR  |
                             D3DSTENCILCAPS_INCR;
    mCaps.StencilCaps |= D3DSTENCILCAPS_TWOSIDED;

    mCaps.MaxAnisotropy = mLimits.anisotropy;
    mCaps.MaxPointSize = mLimits.pointsize_max;

    mCaps.MaxPrimitiveCount = 0x555555; /* Taken from an AMD Radeon HD 5700 (Evergreen) GPU. */
    mCaps.MaxVertexIndex    = 0xffffff; /* Taken from an AMD Radeon HD 5700 (Evergreen) GPU. */
    mCaps.MaxStreams        = MAX_STREAMS;
    mCaps.MaxStreamStride   = 1024;

    mCaps.DevCaps2 = D3DDEVCAPS2_STREAMOFFSET                       |
                     D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET |
                     D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES;
    mCaps.MaxNpatchTessellationLevel = 0;
    mCaps.MasterAdapterOrdinal       = 0;
    mCaps.AdapterOrdinalInGroup      = 0;
    mCaps.NumberOfAdaptersInGroup    = 1;

    mCaps.NumSimultaneousRTs = std::min<UINT>(D3D_MAX_SIMULTANEOUS_RENDERTARGETS, mLimits.buffers);

    mCaps.StretchRectFilterCaps = D3DPTFILTERCAPS_MINFPOINT  |
                                  D3DPTFILTERCAPS_MAGFPOINT  |
                                  D3DPTFILTERCAPS_MINFLINEAR |
                                  D3DPTFILTERCAPS_MAGFLINEAR;
    mCaps.VertexTextureFilterCaps = 0;

    /* Add shader misc caps. Only some of them belong to the shader parts of the pipeline */
    mCaps.PrimitiveMiscCaps |= D3DPMISCCAPS_TSSARGTEMP |
                               D3DPMISCCAPS_PERSTAGECONSTANT;

    /* We require GL 3.3, which is more than capable of handling SM3.0 with GLSL. So just set that. */
    mCaps.VertexShaderVersion = D3DVS_VERSION(3,0);
    mCaps.MaxVertexShaderConst = std::min(256u, mLimits.glsl_vs_float_constants);

    mCaps.PixelShaderVersion = D3DPS_VERSION(3,0);
    mCaps.PixelShader1xMaxValue = 1024.0f;

    mCaps.TextureOpCaps = D3DTEXOPCAPS_DISABLE |
                          D3DTEXOPCAPS_SELECTARG1 |
                          D3DTEXOPCAPS_SELECTARG2 |
                          D3DTEXOPCAPS_MODULATE4X |
                          D3DTEXOPCAPS_MODULATE2X |
                          D3DTEXOPCAPS_MODULATE |
                          D3DTEXOPCAPS_ADDSIGNED2X |
                          D3DTEXOPCAPS_ADDSIGNED |
                          D3DTEXOPCAPS_ADD |
                          D3DTEXOPCAPS_SUBTRACT |
                          D3DTEXOPCAPS_ADDSMOOTH |
                          D3DTEXOPCAPS_BLENDCURRENTALPHA |
                          D3DTEXOPCAPS_BLENDFACTORALPHA |
                          D3DTEXOPCAPS_BLENDTEXTUREALPHA |
                          D3DTEXOPCAPS_BLENDDIFFUSEALPHA |
                          D3DTEXOPCAPS_BLENDTEXTUREALPHAPM |
                          D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR |
                          D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |
                          D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
                          D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |
                          D3DTEXOPCAPS_DOTPRODUCT3 |
                          D3DTEXOPCAPS_MULTIPLYADD |
                          D3DTEXOPCAPS_LERP |
                          D3DTEXOPCAPS_BUMPENVMAP |
                          D3DTEXOPCAPS_BUMPENVMAPLUMINANCE;
    mCaps.MaxTextureBlendStages   = 8;
    mCaps.MaxSimultaneousTextures = std::min(mLimits.fragment_samplers, 8u);

    mCaps.MaxUserClipPlanes         = mLimits.clipplanes;
    mCaps.MaxActiveLights           = mLimits.lights;
    mCaps.MaxVertexBlendMatrices    = 1;
    mCaps.MaxVertexBlendMatrixIndex = 0;
    mCaps.VertexProcessingCaps      = D3DVTXPCAPS_TEXGEN |
                                      D3DVTXPCAPS_MATERIALSOURCE7 |
                                      D3DVTXPCAPS_DIRECTIONALLIGHTS |
                                      D3DVTXPCAPS_POSITIONALLIGHTS |
                                      D3DVTXPCAPS_LOCALVIEWER |
                                      D3DVTXPCAPS_TEXGEN_SPHEREMAP;
    mCaps.FVFCaps                   = D3DFVFCAPS_PSIZE | 8; /* 8 texture coordinates. */
    mCaps.RasterCaps               |= D3DPRASTERCAPS_FOGRANGE;

    /* The following caps are shader specific, but they are things we cannot detect, or which
     * are the same among all shader models. So to avoid code duplication set the shader version
     * specific, but otherwise constant caps here
     */
    if(mCaps.VertexShaderVersion >= D3DVS_VERSION(3,0))
    {
        /* Where possible set the caps based on OpenGL extensions and if they
         * aren't set (in case of software rendering) use the VS 3.0 from
         * MSDN or else if there's OpenGL spec use a hardcoded value minimum
         * VS3.0 value. */
        mCaps.VS20Caps.Caps = D3DVS20CAPS_PREDICATION;
        /* VS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        mCaps.VS20Caps.DynamicFlowControlDepth = D3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH;
        mCaps.VS20Caps.NumTemps = std::max(32u, mLimits.arb_vs_temps);
        /* level of nesting in loops / if-statements; VS 3.0 requires MAX (4) */
        mCaps.VS20Caps.StaticFlowControlDepth = D3DVS20_MAX_STATICFLOWCONTROLDEPTH;

        mCaps.MaxVShaderInstructionsExecuted    = 65535; /* VS 3.0 needs at least 65535, some cards even use 2^32-1 */
        mCaps.MaxVertexShader30InstructionSlots = std::max(512u, mLimits.arb_vs_instructions);
        mCaps.VertexTextureFilterCaps = D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MAGFPOINT;
    }
    else if(mCaps.VertexShaderVersion >= D3DVS_VERSION(2,0))
    {
        mCaps.VS20Caps.Caps = 0;
        mCaps.VS20Caps.DynamicFlowControlDepth = D3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH;
        mCaps.VS20Caps.NumTemps = std::max(12u, mLimits.arb_vs_temps);
        mCaps.VS20Caps.StaticFlowControlDepth = 1;

        mCaps.MaxVShaderInstructionsExecuted    = 65535;
        mCaps.MaxVertexShader30InstructionSlots = 0;
    }
    else /* VS 1.x */
    {
        mCaps.VS20Caps.Caps = 0;
        mCaps.VS20Caps.DynamicFlowControlDepth = 0;
        mCaps.VS20Caps.NumTemps = 0;
        mCaps.VS20Caps.StaticFlowControlDepth = 0;

        mCaps.MaxVShaderInstructionsExecuted    = 0;
        mCaps.MaxVertexShader30InstructionSlots = 0;
    }

    if(mCaps.PixelShaderVersion >= D3DPS_VERSION(3,0))
    {
        /* Where possible set the caps based on OpenGL extensions and if they
         * aren't set (in case of software rendering) use the PS 3.0 from
         * MSDN or else if there's OpenGL spec use a hardcoded value minimum
         * PS 3.0 value. */

        /* Caps is more or less undocumented on MSDN but it appears to be
         * used for PS20Caps based on results from R9600/FX5900/Geforce6800
         * cards from Windows */
        mCaps.PS20Caps.Caps = D3DPS20CAPS_ARBITRARYSWIZZLE     |
                              D3DPS20CAPS_GRADIENTINSTRUCTIONS |
                              D3DPS20CAPS_PREDICATION          |
                              D3DPS20CAPS_NODEPENDENTREADLIMIT |
                              D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT;
        /* PS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        mCaps.PS20Caps.DynamicFlowControlDepth = D3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH;
        mCaps.PS20Caps.NumTemps = std::max(32u, mLimits.arb_ps_temps);
        /* PS 3.0 requires MAX_STATICFLOWCONTROLDEPTH (4) */
        mCaps.PS20Caps.StaticFlowControlDepth = D3DPS20_MAX_STATICFLOWCONTROLDEPTH;
        /* PS 3.0 requires MAX_NUMINSTRUCTIONSLOTS (512) */
        mCaps.PS20Caps.NumInstructionSlots = D3DPS20_MAX_NUMINSTRUCTIONSLOTS;

        mCaps.MaxPShaderInstructionsExecuted   = 65535;
        mCaps.MaxPixelShader30InstructionSlots = std::max<UINT>(D3DMIN30SHADERINSTRUCTIONS,
                                                                mLimits.arb_ps_instructions);
    }
    else if(mCaps.PixelShaderVersion >= D3DPS_VERSION(2,0))
    {
        /* Below we assume PS2.0 specs, not extended 2.0a(GeforceFX)/2.0b(Radeon R3xx) ones */
        mCaps.PS20Caps.Caps = 0;
        mCaps.PS20Caps.DynamicFlowControlDepth = 0; /* D3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH = 0 */
        mCaps.PS20Caps.NumTemps = std::max(12u, mLimits.arb_ps_temps);
        mCaps.PS20Caps.StaticFlowControlDepth = D3DPS20_MIN_STATICFLOWCONTROLDEPTH; /* Minimum: 1 */
        /* Minimum number (64 ALU + 32 Texture), a GeforceFX uses 512 */
        mCaps.PS20Caps.NumInstructionSlots = D3DPS20_MIN_NUMINSTRUCTIONSLOTS;

        mCaps.MaxPShaderInstructionsExecuted   = 512; /* Minimum value, a GeforceFX uses 1024 */
        mCaps.MaxPixelShader30InstructionSlots = 0;
    }
    else /* PS 1.x */
    {
        mCaps.PS20Caps.Caps = 0;
        mCaps.PS20Caps.DynamicFlowControlDepth = 0;
        mCaps.PS20Caps.NumInstructionSlots = 0;
        mCaps.PS20Caps.StaticFlowControlDepth = 0;
        mCaps.PS20Caps.NumInstructionSlots = 0;

        mCaps.MaxPShaderInstructionsExecuted   = 0;
        mCaps.MaxPixelShader30InstructionSlots = 0;
    }

    if(mCaps.VertexShaderVersion >= D3DVS_VERSION(2,0))
    {
        /* OpenGL supports all the formats below, perhaps not always
         * without conversion, but it supports them.
         * Further GLSL doesn't seem to have an official unsigned type so
         * don't advertise it yet as I'm not sure how we handle it.
         * We might need to add some clamping in the shader engine to
         * support it.
         * TODO: D3DDTCAPS_USHORT2N, D3DDTCAPS_USHORT4N, D3DDTCAPS_UDEC3, D3DDTCAPS_DEC3N */
        mCaps.DeclTypes = D3DDTCAPS_UBYTE4    |
                          D3DDTCAPS_UBYTE4N   |
                          D3DDTCAPS_SHORT2N   |
                          D3DDTCAPS_SHORT4N;
        if(GLEW_ARB_half_float_vertex)
            mCaps.DeclTypes |= D3DDTCAPS_FLOAT16_2 |
                               D3DDTCAPS_FLOAT16_4;
    }
    else
        mCaps.DeclTypes = 0;
}

void D3DAdapter::init_ids()
{
    const char *vendor_str = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char *renderer_str = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    TRACE("GL_VENDOR=%s, GL_RENDERER=%s\n", vendor_str, renderer_str);

    for(auto &vendor : device_vendors)
    {
        if(strstr(vendor_str, vendor.vendor) == 0)
            continue;

        const d3dgl_renderer_table *renderer_list = vendor.renderer_list;
        for(size_t i = 0;i < vendor.list_size;++i)
        {
            if(strstr(renderer_str, renderer_list[i].renderer) == 0)
                continue;

            mVendorId = vendor.id;
            mDeviceId = renderer_list[i].id;

            i = 0;
            while(i < countof(gpu_description_table) && gpu_description_table[i].vendor != mVendorId)
                ++i;
            while(i < countof(gpu_description_table) && gpu_description_table[i].vendor == mVendorId)
            {
                if(gpu_description_table[i].card == mDeviceId)
                {
                    mDescription = gpu_description_table[i].description;
                    break;
                }
                ++i;
            }

            TRACE("Detected GPU %04x:%04x, \"%s\"\n", mVendorId, mDeviceId, mDescription);
            return;
        }
    }

    WARN("Failed to match a GPU, using %04x:%04x for adapter %u", mVendorId, mDeviceId, mOrdinal);
}

void D3DAdapter::init_usage()
{
    if(!GLEW_VERSION_4_3 && !GLEW_ARB_internalformat_query2)
    {
        ERR("GL4.3 nor GL_ARB_internalformat_query2 are supported! Lots of checks will fail...\n");
        return;
    }

    ResTypeFormatPair typefmt;
    DWORD usage;
    for(const auto &format : gFormatList)
    {
        GLint res;

        // SURFACE is either a plain surface (PBO), or a RenderTarget/DepthStencil surface (Renderbuffer)
        typefmt = std::make_pair(D3DRTYPE_SURFACE, (D3DFORMAT)format.first);
        usage = D3DUSAGE_DYNAMIC;
        if(format.second.color)
        {
            res = GL_FALSE;
            glGetInternalformativ(GL_RENDERBUFFER, format.second.internalformat, GL_FRAMEBUFFER_RENDERABLE, 1, &res);
            if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_RENDERTARGET;

            res = GL_FALSE;
            glGetInternalformativ(GL_RENDERBUFFER, format.second.internalformat, GL_FRAMEBUFFER_BLEND, 1, &res);
            if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
        }
        else
        {
            res = GL_FALSE;
            glGetInternalformativ(GL_RENDERBUFFER, format.second.internalformat, GL_FRAMEBUFFER_RENDERABLE, 1, &res);
            if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_DEPTHSTENCIL;
        }
        mUsage.insert(std::make_pair(typefmt, usage));

        // VOLUME is a 3D plain surface (PBO). Not valid as a RenderTarget or DepthStencil surface.
        typefmt = std::make_pair(D3DRTYPE_VOLUME, (D3DFORMAT)format.first);
        usage = D3DUSAGE_DYNAMIC;
        mUsage.insert(std::make_pair(typefmt, usage));

        const struct {
            DWORD rtype;
            GLenum gltype;
        } texture_types[]{
            // TEXTURE is a normal 2D texture (potentially usable as a RenderTarget or DepthStencil)
            { D3DRTYPE_TEXTURE, GL_TEXTURE_2D },
            // VOLUMETEXTURE is a normal 3D texture (potentially usable as a RenderTarget or DepthStencil)
            { D3DRTYPE_VOLUMETEXTURE, GL_TEXTURE_3D },
            // CUBETEXTURE is a 6-faced texture cube (potentially usable as a RenderTarget or DepthStencil)
            { D3DRTYPE_CUBETEXTURE, GL_TEXTURE_CUBE_MAP }
        };
        for(const auto &textype : texture_types)
        {
            typefmt = std::make_pair(textype.rtype, (D3DFORMAT)format.first);
            usage = D3DUSAGE_DYNAMIC | D3DUSAGE_QUERY_WRAPANDMIP;

            res = GL_FALSE;
            glGetInternalformativ(textype.gltype, format.second.internalformat, GL_MANUAL_GENERATE_MIPMAP, 1, &res);
            if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_AUTOGENMIPMAP;

            res = GL_FALSE;
            glGetInternalformativ(textype.gltype, format.second.internalformat, GL_FILTER, 1, &res);
            if(res != GL_FALSE) usage |= D3DUSAGE_QUERY_FILTER;

            res = GL_FALSE;
            glGetInternalformativ(textype.gltype, format.second.internalformat, GL_VERTEX_TEXTURE, 1, &res);
            if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_QUERY_VERTEXTEXTURE;

            res = GL_FALSE;
            glGetInternalformativ(textype.gltype, format.second.internalformat, GL_SRGB_DECODE_ARB, 1, &res);
            if(res != GL_FALSE) usage |= D3DUSAGE_QUERY_SRGBREAD;

            if(format.second.color)
            {
                res = GL_FALSE;
                glGetInternalformativ(textype.gltype, format.second.internalformat, GL_FRAMEBUFFER_RENDERABLE, 1, &res);
                if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_RENDERTARGET;

                res = GL_FALSE;
                glGetInternalformativ(textype.gltype, format.second.internalformat, GL_FRAMEBUFFER_BLEND, 1, &res);
                if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;

                res = GL_FALSE;
                glGetInternalformativ(textype.gltype, format.second.internalformat, GL_SRGB_WRITE, 1, &res);
                if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_QUERY_SRGBWRITE;
            }
            else
            {
                res = GL_FALSE;
                glGetInternalformativ(textype.gltype, format.second.internalformat, GL_FRAMEBUFFER_RENDERABLE, 1, &res);
                if(res == GL_FULL_SUPPORT) usage |= D3DUSAGE_DEPTHSTENCIL;
            }
            mUsage.insert(std::make_pair(typefmt, usage));
        }
    }

    typefmt = std::make_pair(D3DRTYPE_VERTEXBUFFER, D3DFMT_VERTEXDATA);
    usage = D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING;
    mUsage.insert(std::make_pair(typefmt, usage));

    typefmt = std::make_pair(D3DRTYPE_INDEXBUFFER, D3DFMT_INDEX16);
    usage = D3DUSAGE_DYNAMIC;
    mUsage.insert(std::make_pair(typefmt, usage));

    typefmt = std::make_pair(D3DRTYPE_INDEXBUFFER, D3DFMT_INDEX32);
    usage = D3DUSAGE_DYNAMIC;
    mUsage.insert(std::make_pair(typefmt, usage));
}


bool D3DAdapter::init()
{
    if(mInited)
        return true;
    TRACE("Initializing adapter %u info\n", mOrdinal);

    DISPLAY_DEVICEW display_device;
    memset(&display_device, 0, sizeof(display_device));
    display_device.cb = sizeof(display_device);
    EnumDisplayDevicesW(NULL, mOrdinal, &display_device, 0);
    mDeviceName = display_device.DeviceName;

    TRACE("Got device name \"%ls\"\n", mDeviceName.c_str());

    HWND hWnd; HDC hDc; HGLRC hGlrc;
    if(!CreateFakeContext(GetModuleHandleW(nullptr), hWnd, hDc, hGlrc))
        return false;

    bool retval = false;
    wglMakeCurrent(hDc, hGlrc);

    if(!GLEW_VERSION_3_3)
    {
        ERR("Required GL 3.3 not supported!");
        goto done;
    }
    if(!GLEW_EXT_direct_state_access)
    {
        ERR("Required GL_EXT_direct_state_access not supported!");
        goto done;
    }

    init_limits();
    init_caps();
    init_ids();
    init_usage();

    retval = true;
done:
    wglMakeCurrent(hDc, nullptr);
    wglDeleteContext(hGlrc);
    ReleaseDC(hWnd, hDc);
    DestroyWindow(hWnd);

    mInited = retval;
    return retval;
}


DWORD D3DAdapter::getUsage(DWORD restype, D3DFORMAT format) const
{
    UsageMap::const_iterator usage = mUsage.find(std::make_pair(restype, format));
    if(usage != mUsage.end()) return usage->second;
    ERR("Usage flags not found for resource type 0x%lx, %s\n",
        restype, d3dfmt_to_str(format));
    return 0;
}


UINT D3DAdapter::getModeCount(D3DFORMAT format) const
{
    DWORD bpp;
    if(format == D3DFMT_X8R8G8B8) bpp = 32;
    else if(format == D3DFMT_R5G6B5) bpp = 16;
    else if(format == D3DFMT_X1R5G5B5) bpp = 15;
    else
    {
        ERR("Unhandled format: %s\n", d3dfmt_to_str(format));
        return 0;
    }

    DEVMODEW m;
    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);

    int j = 0, i = 0;
    while(EnumDisplaySettingsW(mDeviceName.c_str(), j++, &m))
    {
        if(m.dmBitsPerPel == bpp)
            i++;
    }
    return i;
}

HRESULT D3DAdapter::getModeInfo(D3DFORMAT format, UINT mode, D3DDISPLAYMODE *info) const
{
    DWORD bpp;
    if(format == D3DFMT_X8R8G8B8) bpp = 32;
    else if(format == D3DFMT_R5G6B5) bpp = 16;
    else if(format == D3DFMT_X1R5G5B5) bpp = 15;
    else
    {
        ERR("Unhandled format: %s\n", d3dfmt_to_str(format));
        return D3DERR_INVALIDCALL;
    }

    DEVMODEW m;
    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);

    UINT i = 0;
    int j = 0;
    while(i <= mode && EnumDisplaySettingsW(mDeviceName.c_str(), j++, &m))
    {
        if(m.dmBitsPerPel == bpp)
        {
            if(i++ == mode)
            {
                info->Width = m.dmPelsWidth;
                info->Height = m.dmPelsHeight;
                info->RefreshRate = 0;
                if((m.dmFields&DM_DISPLAYFREQUENCY))
                    info->RefreshRate = m.dmDisplayFrequency;
                info->Format = format;
                return D3D_OK;
            }
        }
    }

    WARN("Mode %u out of range for format %s (max: %u)\n", mode, d3dfmt_to_str(format), i);
    return D3DERR_INVALIDCALL;
}


std::vector<D3DAdapter> gAdapterList;
