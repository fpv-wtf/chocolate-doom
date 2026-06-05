#include "ar_pixel.h"
#include "SDL.h"
#include <stdlib.h>
#include <stdalign.h>
#include <fcntl.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

#define MAX_BUFS  16
#define BUF_CNT 4

#define VO_DEV    0
#define VO_LAYER  0
#define VO_CHN    0


#define PANEL_ATTR_SIZE   0x248
#define OFF_PUB           0x050
#define OFF_DSI           0x0b0
#define OFF_USER          0x164
#define OFF_FPS           0x190
#define OFF_SYNC_HACT     (OFF_PUB + 0x0c + 0x10)
#define OFF_SYNC_VACT     (OFF_PUB + 0x0c + 0x0a)
#define OFF_SYNC_IOP      (OFF_PUB + 0x0c + 0x04)
#define OFF_PUB_INTFTYPE  (OFF_PUB + 0x04)
#define OFF_PWM_CHAN       0x00

#define CHK(call) do { \
    fprintf(stderr, #call " ... "); fflush(stderr); \
    uint32_t _r = (uint32_t)(call); \
    fprintf(stderr, "= 0x%x\n", _r); \
    if (_r) { return 1; } \
} while(0)

#define TRY(call) do { \
    fprintf(stderr, #call " ... "); fflush(stderr); \
    uint32_t _r = (uint32_t)(call); \
    fprintf(stderr, "= 0x%x%s\n", _r, _r?" (ign)":""); \
} while(0)

static int g_next_buf = 0;

static size_t   FILE_SZ_Y;
static size_t   FILE_SZ_UV;
static size_t   FILE_FRAME_SIZE;

static uint32_t STRIDE_Y_VAL;
static uint32_t STRIDE_UV_VAL;
static size_t   SZ_Y_VAL;
static size_t   SZ_U_VAL;
static size_t   SZ_V_VAL;
static size_t   FRAME_SIZE_VAL;

static int            g_blk[MAX_BUFS];
static uint32_t       g_pool      = 0;
static uint8_t       *g_frames[MAX_BUFS];
static uint64_t       g_phys[MAX_BUFS];
static uint32_t       g_pool_id[MAX_BUFS];
static void          *g_panel_handle = NULL;
static long           g_panel_obj    = 0;
static uint8_t        g_panel_attr[PANEL_ATTR_SIZE];
static char    g_panel_name[128] = "nd043fhdm30p";

static int            g_dev_up    = 0;
static int            g_dsi_up    = 0;
static int            g_layer_up  = 0;
static int            g_chn_up    = 0;

typedef uint32_t (*fn_SYS_Init)(void);
typedef void    *(*fn_SYS_Mmap)(uint64_t, uint32_t);
typedef uint32_t (*fn_SYS_Munmap)(void *, uint32_t);
typedef uint32_t (*fn_VB_Init)(void);
typedef uint32_t (*fn_VB_CreatePool)(void *);
typedef uint32_t (*fn_VB_DestroyPool)(uint32_t);
typedef int      (*fn_VB_GetBlock)(uint32_t, uint32_t, void *);
typedef uint64_t (*fn_VB_Handle2PhysAddr)(int);
typedef uint32_t (*fn_VB_Handle2PoolId)(int);
typedef uint32_t (*fn_VB_ReleaseBlock)(int);
typedef void    *(*fn_ar_panel_open)(const char *);
typedef int      (*fn_ar_panel_get_default_attr)(void *, int, void *);
typedef long     (*fn_ar_panel_init)(void *, void *);
typedef int      (*fn_ar_panel_control)(void *, long, int, void *);
typedef int      (*fn_ar_panel_uninit)(void *, long);
typedef uint32_t (*fn_VO_SetPubAttr)(int, void *);
typedef int      (*fn_VO_Enable)(int);
typedef uint32_t (*fn_VO_Disable)(int);
typedef uint32_t (*fn_VO_SubscribeEnable)(int, void *);
typedef uint32_t (*fn_VO_SubscribeDisable)(int);
typedef uint32_t (*fn_VO_SetUserIntfSyncInfo)(int, void *);
typedef uint32_t (*fn_VO_SetDevFrameRate)(float, int);
typedef uint32_t (*fn_VO_Dsi_SetAttr)(int, void *);
typedef uint32_t (*fn_VO_Dsi_Enable)(int);
typedef uint32_t (*fn_VO_Dsi_Disable)(int);
typedef uint32_t (*fn_VO_SetDisplayBufLen)(int, uint32_t);
typedef uint32_t (*fn_VO_SetVideoLayerPartitionMode)(int, uint32_t);
typedef uint32_t (*fn_VO_GetVideoLayerCSC)(int, void *);
typedef uint32_t (*fn_VO_SetVideoLayerCSC)(int, void *);
typedef uint32_t (*fn_VO_SetVideoLayerAttr)(int, VO_VIDEO_LAYER_ATTR_S *);
typedef uint32_t (*fn_VO_EnableVideoLayer)(int);
typedef uint32_t (*fn_VO_DisableVideoLayer)(int);
typedef uint32_t (*fn_VO_SetChnAttr)(int, int, VO_CHN_ATTR_S *);
typedef uint32_t (*fn_VO_EnableChn)(int, int);
typedef uint32_t (*fn_VO_DisableChn)(int, int);
typedef uint32_t (*fn_VO_SendFrame)(int, int, void *, int);

static fn_ar_panel_open              ar_panel_open;
static fn_ar_panel_get_default_attr  ar_panel_get_default_attr;
static fn_ar_panel_init              ar_panel_init;
static fn_ar_panel_control           ar_panel_control;
static fn_ar_panel_uninit            ar_panel_uninit;
static fn_SYS_Init                   SYS_Init;
static fn_SYS_Mmap                   SYS_Mmap;
static fn_SYS_Munmap                 SYS_Munmap;
static fn_VB_Init                    VB_Init;
static fn_VB_CreatePool              VB_CreatePool;
static fn_VB_DestroyPool             VB_DestroyPool;
static fn_VB_GetBlock                VB_GetBlock;
static fn_VB_Handle2PhysAddr         VB_Handle2PhysAddr;
static fn_VB_Handle2PoolId           VB_Handle2PoolId;
static fn_VB_ReleaseBlock            VB_ReleaseBlock;
static fn_VO_SetPubAttr              VO_SetPubAttr;
static fn_VO_Enable                  VO_Enable;
static fn_VO_Disable                 VO_Disable;
static fn_VO_SubscribeEnable         VO_SubscribeEnable;
static fn_VO_SubscribeDisable        VO_SubscribeDisable;
static fn_VO_SetUserIntfSyncInfo     VO_SetUserIntfSyncInfo;
static fn_VO_SetDevFrameRate         VO_SetDevFrameRate;
static fn_VO_Dsi_SetAttr             VO_Dsi_SetAttr;
static fn_VO_Dsi_Enable              VO_Dsi_Enable;
static fn_VO_Dsi_Disable             VO_Dsi_Disable;
static fn_VO_SetDisplayBufLen        VO_SetDisplayBufLen;
static fn_VO_SetVideoLayerPartitionMode VO_SetVideoLayerPartitionMode;
static fn_VO_GetVideoLayerCSC        VO_GetVideoLayerCSC;
static fn_VO_SetVideoLayerCSC        VO_SetVideoLayerCSC;
static fn_VO_SetVideoLayerAttr       VO_SetVideoLayerAttr;
static fn_VO_EnableVideoLayer        VO_EnableVideoLayer;
static fn_VO_DisableVideoLayer       VO_DisableVideoLayer;
static fn_VO_SetChnAttr              VO_SetChnAttr;
static fn_VO_EnableChn               VO_EnableChn;
static fn_VO_DisableChn              VO_DisableChn;
static fn_VO_SendFrame               VO_SendFrame;

static const char *deps[] = {
    "libbinder_ipc.so","libmpp_service.so","librpc_proxy.so","librpc_fs.so",
    "libosal.so","libutils.so","libmbuf.so","libhal_dbglog.so", "libhal_gpio.so",
    "libhal_sys.so","libhal_vb.so","libhal_vo.so","libhal_region.so",
    "libmpi_region.so","libmpi_vpss.so","libhal_aio.so",
    "libmpi_sys.so","libmpi_vb.so","libmpi_vo.so", "libpanel_ctrl.so", NULL
};


static uint32_t align64(uint32_t v)
{
    return (v + 63u) & ~63u;
}

static void *ms(void *h, const char *n) {
    void *p = dlsym(h, n);
    if (!p) { fprintf(stderr, "missing symbol: %s\n", n); exit(1); }
    return p;
}
static void *ts(void *h, const char *n) { return dlsym(h, n); }

static void load(void) {
    for (int i = 0; deps[i]; i++) dlopen(deps[i], RTLD_LAZY|RTLD_GLOBAL);
    void *hp = dlopen("libpanel_ctrl.so", RTLD_NOW|RTLD_GLOBAL);
    if (!hp) hp = dlopen("/usr/lib/libpanel_ctrl.so", RTLD_NOW|RTLD_GLOBAL);
    if (!hp) {
        // Try panel-specific .so derived from panel name
        char soname[160];
        snprintf(soname, sizeof(soname), "libpn_%s.so", g_panel_name);
        hp = dlopen(soname, RTLD_NOW|RTLD_GLOBAL);
        if (!hp) {
            snprintf(soname, sizeof(soname), "libpn_%s.so", g_panel_name);
            hp = dlopen(soname, RTLD_NOW|RTLD_GLOBAL);
        }
    }
    if (!hp) { fprintf(stderr, "panel so: %s\n", dlerror()); exit(1); }
    ar_panel_open             = ms(hp, "ar_panel_open");
    ar_panel_get_default_attr = ms(hp, "ar_panel_get_default_attr");
    ar_panel_init             = ms(hp, "ar_panel_init");
    ar_panel_control          = ms(hp, "ar_panel_control");
    ar_panel_uninit           = ts(hp, "ar_panel_uninit");
    void *hs = dlopen("libmpi_sys.so", RTLD_NOW|RTLD_GLOBAL);
    void *hb = dlopen("libmpi_vb.so",  RTLD_NOW|RTLD_GLOBAL);
    void *hv = dlopen("libmpi_vo.so",  RTLD_NOW|RTLD_GLOBAL);
    if (!hs || !hb || !hv) { fprintf(stderr, "mpi: %s\n", dlerror()); exit(1); }
    SYS_Init   = ms(hs, "AR_MPI_SYS_Init");
    SYS_Mmap   = ms(hs, "AR_MPI_SYS_Mmap");
    SYS_Munmap = ms(hs, "AR_MPI_SYS_Munmap");
    VB_Init            = ms(hb, "AR_MPI_VB_Init");
    VB_CreatePool      = ms(hb, "AR_MPI_VB_CreatePool");
    VB_DestroyPool     = ms(hb, "AR_MPI_VB_DestroyPool");
    VB_GetBlock        = ms(hb, "AR_MPI_VB_GetBlock");
    VB_Handle2PhysAddr = ms(hb, "AR_MPI_VB_Handle2PhysAddr");
    VB_Handle2PoolId   = ms(hb, "AR_MPI_VB_Handle2PoolId");
    VB_ReleaseBlock    = ms(hb, "AR_MPI_VB_ReleaseBlock");
    VO_SetPubAttr                = ms(hv, "AR_MPI_VO_SetPubAttr");
    VO_Enable                    = ms(hv, "AR_MPI_VO_Enable");
    VO_Disable                   = ts(hv, "AR_MPI_VO_Disable");
    VO_SubscribeEnable           = ts(hv, "AR_MPI_VO_SubscribeEnable");
    VO_SubscribeDisable          = ts(hv, "AR_MPI_VO_SubscribeDisable");
    VO_SetUserIntfSyncInfo       = ts(hv, "AR_MPI_VO_SetUserIntfSyncInfo");
    VO_SetDevFrameRate           = ts(hv, "AR_MPI_VO_SetDevFrameRate");
    VO_Dsi_SetAttr               = ts(hv, "AR_MPI_VO_Dsi_SetAttr");
    VO_Dsi_Enable                = ts(hv, "AR_MPI_VO_Dsi_Enable");
    VO_Dsi_Disable               = ts(hv, "AR_MPI_VO_Dsi_Disable");
    VO_SetDisplayBufLen          = ms(hv, "AR_MPI_VO_SetDisplayBufLen");
    VO_SetVideoLayerPartitionMode= ms(hv, "AR_MPI_VO_SetVideoLayerPartitionMode");
    VO_GetVideoLayerCSC          = ts(hv, "AR_MPI_VO_GetVideoLayerCSC");
    VO_SetVideoLayerCSC          = ts(hv, "AR_MPI_VO_SetVideoLayerCSC");
    VO_SetVideoLayerAttr         = ms(hv, "AR_MPI_VO_SetVideoLayerAttr");
    VO_EnableVideoLayer          = ms(hv, "AR_MPI_VO_EnableVideoLayer");
    VO_DisableVideoLayer         = ts(hv, "AR_MPI_VO_DisableVideoLayer");
    VO_SetChnAttr                = ms(hv, "AR_MPI_VO_SetChnAttr");
    VO_EnableChn                 = ms(hv, "AR_MPI_VO_EnableChn");
    VO_DisableChn                = ts(hv, "AR_MPI_VO_DisableChn");
    VO_SendFrame                 = ms(hv, "AR_MPI_VO_SendFrame");
}


void ARTY_Update(const uint8_t *indexed, const SDL_Color *palette)
{
    int bi = g_next_buf;
    uint8_t *dst;
    uint32_t r;

    if (!indexed || !palette || !g_chn_up)
        return;

    dst = SYS_Mmap(g_phys[bi], (uint32_t)FRAME_SIZE_VAL);
    if (!dst)
        return;

    for (int y = 0; y < 200; y++) {
        uint8_t *row = dst + (size_t)y * STRIDE_Y_VAL;
        const uint8_t *src = indexed + (size_t)y * 320;

        for (int x = 0; x < 320; x++) {
            SDL_Color c = palette[src[x]];

            row[x * 4 + 0] = c.b;
            row[x * 4 + 1] = c.g;
            row[x * 4 + 2] = c.r;
            row[x * 4 + 3] = 0x00;
        }
    }

    SYS_Munmap(dst, (uint32_t)FRAME_SIZE_VAL);

    r = VO_SendFrame(VO_LAYER, VO_CHN, g_frames[bi], 0xffffffff);
    if (r != 0) {
        fprintf(stderr, "ARTY_Update: VO_SendFrame[%d] failed: 0x%x\n", bi, r);
        return;
    }

    g_next_buf = (g_next_buf + 1) % BUF_CNT;
}

static int get_panel_attr(void *handle, int tw, int th, float tfps,
                           int progressive, uint8_t *out) {
    uint8_t tmp[PANEL_ATTR_SIZE];
    for (int id = 0; id < 16; id++) {
        memset(tmp, 0, PANEL_ATTR_SIZE);
        if (ar_panel_get_default_attr(handle, id, tmp) < 0) {
            fprintf(stderr, "get_default_attr(%d) failed\n", id); return -1;
        }
        uint16_t hact, vact; uint32_t bIop; float fps;
        memcpy(&hact, tmp+OFF_SYNC_HACT, 2);
        memcpy(&vact, tmp+OFF_SYNC_VACT, 2);
        memcpy(&bIop, tmp+OFF_SYNC_IOP,  4);
        memcpy(&fps,  tmp+OFF_FPS,       4);
        fprintf(stderr, "  typeId=%d: %ux%u fps=%.1f bIop=%u\n",
                id, hact, vact, (double)fps, bIop);
        if (hact==(uint16_t)tw && vact==(uint16_t)th &&
            bIop==(uint32_t)progressive && (int)(fps-tfps)==0) {
            fprintf(stderr, "  --> matched typeId=%d\n", id);
            memcpy(out, tmp, PANEL_ATTR_SIZE); return id;
        }
    }
    fprintf(stderr, "  no exact match, using typeId=0\n");
    memset(tmp, 0, PANEL_ATTR_SIZE);
    ar_panel_get_default_attr(handle, 0, tmp);
    memcpy(out, tmp, PANEL_ATTR_SIZE); return 0;
}

static void dump_panel_attr(void) {
    fprintf(stderr, "[panel_attr hex dump]\n");
    for (int i = 0; i < PANEL_ATTR_SIZE; i += 16) {
        fprintf(stderr, "  %04x: ", i);
        for (int j = 0; j < 16 && i+j < PANEL_ATTR_SIZE; j++)
            fprintf(stderr, "%02x ", g_panel_attr[i+j]);
        fprintf(stderr, "\n");
    }
}

static void init_frame(int i, int w, int h) {
    VIDEO_FRAME_INFO_S *fi = (VIDEO_FRAME_INFO_S *)g_frames[i];
    memset(fi, 0, sizeof(*fi));
    VIDEO_FRAME_S *f = &fi->stVFrame;
    f->u32FrameId    = (uint32_t)i;
    f->u32Width      = (uint32_t)w;
    f->u32Height     = (uint32_t)h;
    f->enField       = VIDEO_FIELD_FRAME;
    f->enPixelFormat = PIXEL_FORMAT_ARGB_8888;
    f->enVideoFormat = VIDEO_FORMAT_LINEAR;
    f->enCompressMode= COMPRESS_MODE_NONE;
    f->enDynamicRange= DYNAMIC_RANGE_SDR8;
    f->enColorGamut  = COLOR_GAMUT_BT709;
    f->u32Stride[0]  = STRIDE_Y_VAL;
    f->u64PhyAddr[0] = g_phys[i];
    f->u64VirAddr[0] = g_phys[i];
    f->u32Len[0]     = (uint32_t)SZ_Y_VAL;
    f->u64PTS        = (uint64_t)i * 1000000ULL;
    fi->u32PoolId    = g_pool_id[i];
    fi->enModId      = 0;
}

int ARTY_Init(int src_w, int src_h, int disp_w, int disp_h, float fps)
{
    STRIDE_Y_VAL   = align64((uint32_t)src_w * 4);
    STRIDE_UV_VAL  = 0;
    SZ_Y_VAL       = (size_t)STRIDE_Y_VAL * (size_t)src_h;
    SZ_U_VAL = SZ_V_VAL = 0;
    FRAME_SIZE_VAL = SZ_Y_VAL;
    FILE_SZ_Y      = (size_t)src_w * (size_t)src_h * 4;
    FILE_SZ_UV     = 0;
    FILE_FRAME_SIZE = FILE_SZ_Y;

    for (int i = 0; i < MAX_BUFS; i++) g_blk[i] = -1;
    load();
    SYS_Init();
    VB_Init();

    { VB_POOL_CONFIG pc = {
          .blk_size = FRAME_SIZE_VAL,
          .blk_cnt  = (uint64_t)(BUF_CNT + 2)
      };
      g_pool = VB_CreatePool(&pc);
      fprintf(stderr, "VB_CreatePool(blk_size=%zu, cnt=%d) = pool %u\n",
              FRAME_SIZE_VAL, BUF_CNT+2, g_pool);
    }
    for (int i = 0; i < BUF_CNT; i++) {
        g_frames[i] = calloc(1, sizeof(VIDEO_FRAME_INFO_S));
        if (!g_frames[i]) { fprintf(stderr,"OOM\n"); return 1; }
        g_blk[i] = VB_GetBlock(g_pool, (uint32_t)FRAME_SIZE_VAL, NULL);
        if (g_blk[i] < 0) { fprintf(stderr,"VB_GetBlock[%d] failed\n",i); return 1; }
        g_phys[i]    = VB_Handle2PhysAddr(g_blk[i]);
        g_pool_id[i] = VB_Handle2PoolId(g_blk[i]);
        void *vp = SYS_Mmap(g_phys[i], (uint32_t)FRAME_SIZE_VAL);
        if (!vp) { fprintf(stderr,"Mmap[%d] failed\n",i); return 1; }
        SYS_Munmap(vp, (uint32_t)FRAME_SIZE_VAL);
        init_frame(i, src_w, src_h);
    }
    fprintf(stderr,"phys[0]=0x%llx  stride Y=%u UV=%u  frame_size=%zu\n",
            (unsigned long long)g_phys[0], STRIDE_Y_VAL, STRIDE_UV_VAL, FRAME_SIZE_VAL);

    fprintf(stderr,"ar_panel_open(\"%s\") ... ", g_panel_name); fflush(stderr);
    g_panel_handle = ar_panel_open(g_panel_name);
    fprintf(stderr,"%s\n", g_panel_handle?"ok":"FAILED");
    if (!g_panel_handle) { return 1; }

    fprintf(stderr,"scanning panel presets for %dx%d @%.1ffps:\n",
            disp_w, disp_h, (double)fps);
    if (get_panel_attr(g_panel_handle, disp_w, disp_h, fps, 1, g_panel_attr) < 0)
        { return 1; }

    uint32_t pwm_chan = 1;
    memcpy(g_panel_attr + OFF_PWM_CHAN, &pwm_chan, 4);
    { uint32_t pwm_chan; memcpy(&pwm_chan, g_panel_attr+OFF_PWM_CHAN, 4);
      uint32_t bpp;      memcpy(&bpp,     g_panel_attr+OFF_DSI+0x10, 4);
      float    dphy;     memcpy(&dphy,    g_panel_attr+OFF_DSI+0x34, 4);
      fprintf(stderr,"  pwm_chan=%u bpp=%u dphy=%.1fMHz\n",
              pwm_chan, bpp, (double)dphy); }

    fprintf(stderr,"ar_panel_init ... "); fflush(stderr);

    g_panel_obj = ar_panel_init(g_panel_handle, g_panel_attr);
    fprintf(stderr,"= %ld %s\n", g_panel_obj, g_panel_obj?"ok":"FAILED");
    if (!g_panel_obj) { return 1; }

    fprintf(stderr,"ar_panel_control(1=power_on) = %d\n",
            ar_panel_control(g_panel_handle, g_panel_obj, 1, g_panel_attr));
    usleep(20000);

    if (VO_SetDevFrameRate) TRY(VO_SetDevFrameRate(fps, 0));
    CHK(VO_SetPubAttr(VO_DEV, g_panel_attr + OFF_PUB));
   // if (VO_SetUserIntfSyncInfo)
  //      TRY(VO_SetUserIntfSyncInfo(VO_DEV, g_panel_attr + OFF_USER));
    CHK(VO_Enable(VO_DEV)); g_dev_up = 1;

    fprintf(stderr,"ar_panel_control(0=dsi_init) = %d\n",
            ar_panel_control(g_panel_handle, g_panel_obj, 0, g_panel_attr));

    if (VO_Dsi_SetAttr)  TRY(VO_Dsi_SetAttr(VO_DEV, g_panel_attr + OFF_DSI));
    if (VO_Dsi_Enable) {
        TRY(VO_Dsi_Enable(VO_DEV));
        g_dsi_up = 1;
        usleep(500000);
    }


    fprintf(stderr,"ar_panel_control(5=detect) = %d (1=ok)\n",
            ar_panel_control(g_panel_handle, g_panel_obj, 5, g_panel_attr));

    CHK(VO_SetDisplayBufLen(VO_DEV, 3));
    CHK(VO_SetVideoLayerPartitionMode(VO_LAYER, 1));

    {
        VO_VIDEO_LAYER_ATTR_S la; memset(&la,0,sizeof(la));

        la.stDispRect.s32X      = 0;
        la.stDispRect.s32Y      = 0;
        la.stDispRect.u32Width  = (uint32_t)disp_w;
        la.stDispRect.u32Height = (uint32_t)disp_h;

        la.stImageSize.u32Width  = (uint32_t)src_w;
        la.stImageSize.u32Height = (uint32_t)src_h;
        la.u32DispFrmRt = fps;
        la.enPixFormat  = (uint32_t)PIXEL_FORMAT_ARGB_8888;

        la.bFlip = 0;
        la.bMirror = 0;
        la.u32Stride[0] = STRIDE_Y_VAL;
        la.u32Stride[1] = STRIDE_UV_VAL;
        la.u32Stride[2] = STRIDE_UV_VAL;
        uint64_t stride_ext = (uint64_t)align64((uint32_t)src_w) << 32;
        memcpy((uint8_t *)&la + 0x48, &stride_ext, 8);
        la.memMode = 1;
        fprintf(stderr,
            "[layer] dispRect=%dx%d  imageSize=%dx%d  strides=%u/%u\n",
            disp_w, disp_h, src_w, src_h,
            STRIDE_Y_VAL, STRIDE_UV_VAL);
        CHK(VO_SetVideoLayerAttr(VO_LAYER, &la));
    }

    CHK(VO_EnableVideoLayer(VO_LAYER)); g_layer_up = 1;

    {
        VO_CHN_ATTR_S ca; memset(&ca,0,sizeof(ca));
        ca.stRect.s32X      = 0;
        ca.stRect.s32Y      = 0;
        ca.stRect.u32Width  = (uint32_t)(disp_w & ~1);
        ca.stRect.u32Height = (uint32_t)(disp_h & ~1);
        CHK(VO_SetChnAttr(VO_LAYER, VO_CHN, &ca));
    }

    CHK(VO_EnableChn(VO_LAYER, VO_CHN)); g_chn_up = 1;


    for (int i = 0; i < 4; i++) {
        void *vp = SYS_Mmap(g_phys[i], FRAME_SIZE_VAL);
        memset(vp, 0, FRAME_SIZE_VAL);
        SYS_Munmap(vp, FRAME_SIZE_VAL);
        uint32_t r = VO_SendFrame(VO_LAYER, VO_CHN, g_frames[i], 0xffffffff);
    }

 fprintf(stderr,"ARTY_Init done ... ");
    return 0;
}

void ARTY_Shutdown(void)
{
    int i;

    fprintf(stderr, "ARTY_Shutdown\n");

    if (g_chn_up && VO_DisableChn)
    {
        VO_DisableChn(VO_LAYER, VO_CHN);
        g_chn_up = 0;
    }

    if (g_layer_up && VO_DisableVideoLayer)
    {
        VO_DisableVideoLayer(VO_LAYER);
        g_layer_up = 0;
    }

    if (g_dsi_up && VO_Dsi_Disable)
    {
        VO_Dsi_Disable(VO_DEV);
        g_dsi_up = 0;
    }

    if (g_dev_up && VO_Disable)
    {
        VO_Disable(VO_DEV);
        g_dev_up = 0;
    }


    for (i = 0; i < BUF_CNT; i++)
    {
        if (g_blk[i] >= 0)
        {
            VB_ReleaseBlock(g_blk[i]);
            g_blk[i] = -1;
        }

        free(g_frames[i]);
        g_frames[i] = NULL;
    }

    if (g_pool)
    {
        VB_DestroyPool(g_pool);
        g_pool = 0;
    }
}