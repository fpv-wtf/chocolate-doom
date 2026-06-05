#pragma once
#include <stdint.h>

typedef enum {
    VIDEO_FIELD_TOP = 1,
    VIDEO_FIELD_BOTTOM = 2,
    VIDEO_FIELD_INTERLACED = 3,
    VIDEO_FIELD_FRAME = 4
} VIDEO_FIELD_E;

typedef enum {
    VIDEO_FORMAT_LINEAR = 0,
    VIDEO_FORMAT_TILE_64x16 = 1,
    VIDEO_FORMAT_TILE_16x8 = 2,
    VIDEO_FORMAT_LINEAR_DISCRETE = 3
} VIDEO_FORMAT_E;

typedef enum {
    PIXEL_FORMAT_RGB_444 = 0,
    PIXEL_FORMAT_RGB_555 = 1,
    PIXEL_FORMAT_RGB_565 = 2,
    PIXEL_FORMAT_RGB_888 = 3,
    PIXEL_FORMAT_BGR_444 = 4,
    PIXEL_FORMAT_BGR_555 = 5,
    PIXEL_FORMAT_BGR_565 = 6,
    PIXEL_FORMAT_BGR_888 = 7,
    PIXEL_FORMAT_ARGB_1555 = 8,
    PIXEL_FORMAT_ARGB_4444 = 9,
    PIXEL_FORMAT_ARGB_8565 = 10,
    PIXEL_FORMAT_ARGB_8888 = 11,
    PIXEL_FORMAT_ARGB_2BPP = 12,
    PIXEL_FORMAT_ABGR_1555 = 13,
    PIXEL_FORMAT_ABGR_4444 = 14,
    PIXEL_FORMAT_ABGR_8565 = 15,
    PIXEL_FORMAT_ABGR_8888 = 16,
    PIXEL_FORMAT_RGB_BAYER_8BPP = 17,
    PIXEL_FORMAT_RGB_BAYER_10BPP = 18,
    PIXEL_FORMAT_RGB_BAYER_12BPP = 19,
    PIXEL_FORMAT_RGB_BAYER_14BPP = 20,
    PIXEL_FORMAT_RGB_BAYER_16BPP = 21,
    PIXEL_FORMAT_YVU_PLANAR_422 = 22,
    PIXEL_FORMAT_YVU_PLANAR_420 = 23,
    PIXEL_FORMAT_YVU_PLANAR_444 = 24,
    PIXEL_FORMAT_YVU_SEMIPLANAR_422 = 25,
    PIXEL_FORMAT_YVU_SEMIPLANAR_420 = 26,
    PIXEL_FORMAT_YVU_SEMIPLANAR_444 = 27,
    PIXEL_FORMAT_YUV_SEMIPLANAR_422 = 28,
    PIXEL_FORMAT_YUV_SEMIPLANAR_420 = 29,
    PIXEL_FORMAT_YUV_SEMIPLANAR_444 = 30,
    PIXEL_FORMAT_UYVY_PACKAGE_422 = 31,
    PIXEL_FORMAT_YUYV_PACKAGE_422 = 32,
    PIXEL_FORMAT_VYUY_PACKAGE_422 = 33,
    PIXEL_FORMAT_YUV_400 = 34,
    PIXEL_FORMAT_UV_420 = 35,
    PIXEL_FORMAT_BGR_888_PLANAR = 36,
    PIXEL_FORMAT_HSV_888_PACKAGE = 37,
    PIXEL_FORMAT_HSV_888_PLANAR = 38,
    PIXEL_FORMAT_LAB_888_PACKAGE = 39,
    PIXEL_FORMAT_LAB_888_PLANAR = 40,
    PIXEL_FORMAT_S8C1 = 41,
    PIXEL_FORMAT_S8C2_PACKAGE = 42,
    PIXEL_FORMAT_S8C2_PLANAR = 43,
    PIXEL_FORMAT_S16C1 = 44,
    PIXEL_FORMAT_U8C1 = 45,
    PIXEL_FORMAT_U16C1 = 46,
    PIXEL_FORMAT_S32C1 = 47,
    PIXEL_FORMAT_U32C1 = 48,
    PIXEL_FORMAT_U64C1 = 49,
    PIXEL_FORMAT_S64C1 = 50,
    PIXEL_FORMAT_GRAY1 = 51,
    PIXEL_FORMAT_GRAY4 = 52,
    PIXEL_FORMAT_GRAY8 = 53,
    PIXEL_FORMAT_CF50 = 54,
    PIXEL_FORMAT_EBD = 55,
    PIXEL_FORMAT_AYUV_8888 = 56,
    PIXEL_FORMAT_RGB_UNPACKED_BAYER_10BPP = 57,
    PIXEL_FORMAT_RGB_UNPACKED_BAYER_12BPP = 58,
    PIXEL_FORMAT_RGB_UNPACKED_BAYER_14BPP = 59
} PIXEL_FORMAT_E;

typedef enum {
    COMPRESS_MODE_NONE = 0,
    COMPRESS_MODE_SEG = 1,
    COMPRESS_MODE_DPCM_6BITS = 2,
    COMPRESS_MODE_DPCM_8BITS = 3,
    COMPRESS_MODE_DPCM_10BITS = 4
} COMPRESS_MODE_E;

typedef enum {
    DYNAMIC_RANGE_SDR8 = 0,
    DYNAMIC_RANGE_SDR10 = 1,
    DYNAMIC_RANGE_HDR10 = 2,
    DYNAMIC_RANGE_HLG = 3,
    DYNAMIC_RANGE_SLF = 4,
    DYNAMIC_RANGE_XDR = 5
} DYNAMIC_RANGE_E;

typedef enum {
    COLOR_GAMUT_BT601 = 0,
    COLOR_GAMUT_BT709 = 1,
    COLOR_GAMUT_BT2020 = 2,
    COLOR_GAMUT_USER = 3,
    COLOR_GAMUT_BUTT = 4
} COLOR_GAMUT_E;

typedef enum {
    VB_REMAP_MODE_NONE = 0,
    VB_REMAP_MODE_NOCACHE = 1,
    VB_REMAP_MODE_CACHED = 2,
    VB_REMAP_MODE_BUTT = 3
} VB_REMAP_MODE_E;

typedef struct {
    int32_t  s32X;
    int32_t  s32Y;
    uint32_t u32Width;
    uint32_t u32Height;
} RECT_S;

typedef struct {
    uint32_t u32Width;
    uint32_t u32Height;
} SIZE_S;

typedef struct {
    RECT_S   stDispRect;
    SIZE_S   stImageSize;
    float    u32DispFrmRt;
    uint32_t enPixFormat;
    uint32_t bDoubleFrame;
    uint32_t bClusterMode;
    uint32_t enDstDynamicRange;
    uint32_t bMirror;
    uint32_t bFlip;
    uint32_t u32Stride[3];
    uint8_t  lowDelayInfo[16];
    uint8_t  cf50Info[16];
    uint8_t  fifoInfo[16];
    uint8_t  alpha[8];
    uint32_t memMode;
} VO_VIDEO_LAYER_ATTR_S;

typedef struct {
    uint32_t u32Priority;
    RECT_S   stRect;
    uint32_t bDeflicker;
} VO_CHN_ATTR_S;

typedef struct {
    uint64_t u64JpegDCFPhyAddr;
    uint64_t u64IspInfoPhyAddr;
    uint64_t u64LowDelayPhyAddr;
    uint64_t u64MotionDataPhyAddr;
    uint64_t u64FrameDNGPhyAddr;

    void *pJpegDCFVirAddr;
    void *pIspInfoVirAddr;
    void *pLowDelayVirAddr;
    void *pMotionDataVirAddr;
    void *pFrameDNGVirAddr;
} VIDEO_SUPPLEMENT_S;

typedef struct {
    int32_t s32NalType;
    int32_t s32PicType;
    int32_t s32picTypeFirst;

    int32_t s32NumOfErrMBsInDisplay;
    int32_t s32NumOfTotMBsInDisplay;

    int32_t s32TopFieldFirst;
    int32_t s32RateNumerator;
    int32_t s32RateDenominator;
} VIDEO_EXT_S;

typedef struct {
    int32_t s32TileEn;
    int32_t s32TileFrmX;
    int32_t s32TileFrmY;
    int32_t s32TileIdxX;
    int32_t s32TileIdxY;
} VIDEO_FRAME_TILE_INFO_S;

typedef struct {
    uint32_t            u32FrameId, u32Width, u32Height;
    VIDEO_FIELD_E       enField;
    PIXEL_FORMAT_E      enPixelFormat;
    VIDEO_FORMAT_E      enVideoFormat;
    COMPRESS_MODE_E     enCompressMode;
    DYNAMIC_RANGE_E     enDynamicRange;
    COLOR_GAMUT_E       enColorGamut;
    uint32_t            u32HeaderStride[3];
    uint32_t            u32Stride[3];
    uint32_t            u32ExtStride[3];
    uint64_t            u64HeaderPhyAddr[3], u64HeaderVirAddr[3];
    uint64_t            u64PhyAddr[3], u64VirAddr[3];
    uint64_t            u64ExtPhyAddr[3], u64ExtVirAddr[3];
    int16_t             s16OffsetTop, s16OffsetBottom, s16OffsetLeft, s16OffsetRight;
    uint32_t            u32MaxLuminance, u32MinLuminance, u32TimeRef;
    uint64_t            u64PTS, u64PrivateData;
    uint32_t            u32FrameFlag;
    VIDEO_SUPPLEMENT_S  stSupplement;
    uint32_t            u32HeaderLen[3], u32Len[3];
    float               fLineTime;
    uint32_t            u32SplitOffset;
    int32_t             s32IndexFrameDisplay;
    VIDEO_EXT_S         stExt;
    VIDEO_FRAME_TILE_INFO_S stTileInfo;
    uint64_t            u64MNPTS;
} VIDEO_FRAME_S;

typedef struct {
    VIDEO_FRAME_S stVFrame;
    uint32_t u32PoolId;
    uint32_t enModId;
} VIDEO_FRAME_INFO_S;

typedef struct {
    uint64_t        blk_size;
    uint32_t        blk_cnt;
    VB_REMAP_MODE_E enRemapMode;
    char            acMmzName[17];
} VB_POOL_CONFIG;