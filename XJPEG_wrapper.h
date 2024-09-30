/**
 * @file XJPEG_wrapper.h
 * Copyright (c) 2020 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2020-12-18
 * @version : 1.0.0.0
 * @brief   : 封装 libjpeg 库，实现简单的 JPEG 数据的 编码/解码 相关操作。
 */

#ifndef __XJPG_WRAPPER_H__
#define __XJPG_WRAPPER_H__

#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#define J_NULL          0
#define J_FALSE         0
#define J_TRUE          1

typedef void            j_void_t;
typedef int             j_int_t;
typedef long            j_long_t;
typedef unsigned int    j_uint_t;
typedef unsigned long   j_ulong_t;
typedef unsigned char   j_byte_t;
typedef unsigned int    j_bool_t;

typedef size_t          j_size_t;
typedef fpos_t          j_fpos_t;

typedef void *          j_handle_t;
typedef const char *    j_cstring_t;
typedef unsigned char * j_mem_t;
typedef FILE *          j_fio_t;
typedef const char *    j_path_t;

/**
 * @enum  jpeg_color_space_t
 * @brief JPEG 图像所支持的像素格式。
 */
typedef enum jpeg_color_space_t
{
    JPEG_CS_UNKNOWN  ,  ///< error/unspecified
    JPEG_CS_GRAYSCALE,  ///< monochrome
    JPEG_CS_RGB      ,  ///< red/green/blue, standard RGB (sRGB)
    JPEG_CS_YCbCr    ,  ///< Y/Cb/Cr (also known as YUV), standard YCC
    JPEG_CS_CMYK     ,  ///< C/M/Y/K
    JPEG_CS_YCCK     ,  ///< Y/Cb/Cr/K
    JPEG_CS_BG_RGB   ,  ///< big gamut red/green/blue, bg-sRGB
    JPEG_CS_BG_YCC   ,  ///< big gamut Y/Cb/Cr, bg-sYCC
} jpeg_color_space_t;

/**
 * @enum  jctrl_color_space_t
 * @brief JPEG 编码/解码 所操作（输入/输出）的图像像素格式。
 * @note
 * 枚举值的 比特位 功能，定义如下：
 * - [  0 ~  7 ] : 用于表示每个像素所占的比特数，如 24，32 等值；
 * - [  8 ~ 15 ] : 用于标识小分类的类型，如区别 RGB24 与 BGR24；
 * - [ 16 ~ 23 ] : 用于区分颜色种类，如 RGB，YCBCR 等。
 * - [ 24 ~ 31 ] : 保留位。
 */
typedef enum jctrl_color_space_t
{
    JCTRL_CS_UNKNOWN = 0x00000000, ///< 未知格式
    JCTRL_CS_GRAY    = 0x00010008, ///< GRAY 8位 格式，即 灰度图
    JCTRL_CS_RGB     = 0x00010118, ///< RGB 24位 格式，顺序为 RGB
    JCTRL_CS_BGR     = 0x00010218, ///< RGB 24位 格式，顺序为 BGR
    JCTRL_CS_RGBA    = 0x00010120, ///< RGB 32位 格式，顺序为 RGBA，编码时忽略 ALPHA 通道值
    JCTRL_CS_BGRA    = 0x00010220, ///< RGB 32位 格式，顺序为 BGRA，编码时忽略 ALPHA 通道值
    JCTRL_CS_ARGB    = 0x00010320, ///< RGB 32位 格式，顺序为 ARGB，编码时忽略 ALPHA 通道值
    JCTRL_CS_ABGR    = 0x00010420, ///< RGB 32位 格式，顺序为 ABGR，编码时忽略 ALPHA 通道值
} jctrl_color_space_t;

/**
 * @enum  jctrl_mode_t
 * @brief JPEG 编码/解码 的操作模式。
 */
typedef enum jctrl_mode_t
{
    JCTRL_MODE_UNKNOWN,   ///< 未知模式
    JCTRL_MODE_MEM    ,   ///< 内存模式
    JCTRL_MODE_FIO    ,   ///< 文件流模式
    JCTRL_MODE_FILE   ,   ///< 文件模式
} jctrl_mode_t;

/**
 * @struct jpeg_info_t
 * @brief  JPEG 图像基本信息。
 */
typedef struct jpeg_info_t
{
    j_int_t jit_width;    ///< nominal image width (from SOF marker)
    j_int_t jit_height;   ///< nominal image height
    j_int_t jit_channels; ///< # of color components in JPEG image
    j_int_t jit_cstype;   ///< colorspace of JPEG image ( @see jpeg_color_space_t )
} jpeg_info_t, * jinfo_ptr_t;

////////////////////////////////////////////////////////////////////////////////
// JPEG 编码操作的相关接口

/** 声明 JPEG 编码操作的上下文 结构体 */
struct jenc_ctx_t;

/** 定义 JPEG 编码操作的上下文 结构体指针 */
typedef struct jenc_ctx_t * jenc_ctxptr_t;

/**
 * @brief
 * 用于合成 jenc_ctrlcs_t 枚举值，
 * 即 jctrl_color_space_t 与 jpeg_color_space_t 的合成操作。
 */
#define JENC_CTRLCS_MAKE(jctrl, jcs)    ((jctrl) | ((jcs) << 24))

/**
 * @brief
 * 从 jenc_ctrlcs_t 枚举值获取编码所操作的色彩空间值，
 * 即 jctrl_color_space_t 值。
 */
#define JENC_CTRLCS_CTRL(jctrlcs)       ((jctrlcs) & 0x00FFFFFF)

/**
 * @brief
 * 从 jenc_ctrlcs_t 枚举值获取编码所输出的色彩空间值，
 * 即 jpeg_color_space_t 值。
 */
#define JENC_CTRLCS_JPEG(jctrlcs)       ((jctrlcs) >> 24)

/**
 * @enum  jenc_ctrlcs_t
 * @brief JPEG 编码操作所支持的色彩空间转换枚举值。
 */
typedef enum jenc_ctrl_color_space_t
{
    JENC_CTRLCS_UNKNOWN = 0x00000000, ///< 未定义的操作

    JENC_GRAY_TO_GRAY   = JENC_CTRLCS_MAKE(JCTRL_CS_GRAY, JPEG_CS_GRAYSCALE), ///< GRAY => GRAY
    JENC_RGB_TO_GRAY    = JENC_CTRLCS_MAKE(JCTRL_CS_RGB , JPEG_CS_GRAYSCALE), ///< RGB  => GRAY
    JENC_BGR_TO_GRAY    = JENC_CTRLCS_MAKE(JCTRL_CS_BGR , JPEG_CS_GRAYSCALE), ///< BGR  => GRAY
    JENC_RGBA_TO_GRAY   = JENC_CTRLCS_MAKE(JCTRL_CS_RGBA, JPEG_CS_GRAYSCALE), ///< RGBA => GRAY ，忽略 ALPHA 通道值
    JENC_BGRA_TO_GRAY   = JENC_CTRLCS_MAKE(JCTRL_CS_BGRA, JPEG_CS_GRAYSCALE), ///< BGRA => GRAY ，忽略 ALPHA 通道值
    JENC_ARGB_TO_GRAY   = JENC_CTRLCS_MAKE(JCTRL_CS_ARGB, JPEG_CS_GRAYSCALE), ///< ARGB => GRAY ，忽略 ALPHA 通道值
    JENC_ABGR_TO_GRAY   = JENC_CTRLCS_MAKE(JCTRL_CS_ABGR, JPEG_CS_GRAYSCALE), ///< ABGR => GRAY ，忽略 ALPHA 通道值

    JENC_GRAY_TO_RGB    = JENC_CTRLCS_MAKE(JCTRL_CS_GRAY, JPEG_CS_RGB      ), ///< GRAY => RGB
    JENC_RGB_TO_RGB     = JENC_CTRLCS_MAKE(JCTRL_CS_RGB , JPEG_CS_RGB      ), ///< RGB  => RGB
    JENC_BGR_TO_RGB     = JENC_CTRLCS_MAKE(JCTRL_CS_BGR , JPEG_CS_RGB      ), ///< BGR  => RGB
    JENC_RGBA_TO_RGB    = JENC_CTRLCS_MAKE(JCTRL_CS_RGBA, JPEG_CS_RGB      ), ///< RGBA => RGB ，忽略 ALPHA 通道值
    JENC_BGRA_TO_RGB    = JENC_CTRLCS_MAKE(JCTRL_CS_BGRA, JPEG_CS_RGB      ), ///< BGRA => RGB ，忽略 ALPHA 通道值
    JENC_ARGB_TO_RGB    = JENC_CTRLCS_MAKE(JCTRL_CS_ARGB, JPEG_CS_RGB      ), ///< ARGB => RGB ，忽略 ALPHA 通道值
    JENC_ABGR_TO_RGB    = JENC_CTRLCS_MAKE(JCTRL_CS_ABGR, JPEG_CS_RGB      ), ///< ABGR => RGB ，忽略 ALPHA 通道值

    JENC_GRAY_TO_YCC    = JENC_CTRLCS_MAKE(JCTRL_CS_GRAY, JPEG_CS_YCbCr    ), ///< GRAY => YCbCr
    JENC_RGB_TO_YCC     = JENC_CTRLCS_MAKE(JCTRL_CS_RGB , JPEG_CS_YCbCr    ), ///< RGB  => YCbCr
    JENC_BGR_TO_YCC     = JENC_CTRLCS_MAKE(JCTRL_CS_BGR , JPEG_CS_YCbCr    ), ///< BGR  => YCbCr
    JENC_RGBA_TO_YCC    = JENC_CTRLCS_MAKE(JCTRL_CS_RGBA, JPEG_CS_YCbCr    ), ///< RGBA => YCbCr ，忽略 ALPHA 通道值
    JENC_BGRA_TO_YCC    = JENC_CTRLCS_MAKE(JCTRL_CS_BGRA, JPEG_CS_YCbCr    ), ///< BGRA => YCbCr ，忽略 ALPHA 通道值
    JENC_ARGB_TO_YCC    = JENC_CTRLCS_MAKE(JCTRL_CS_ARGB, JPEG_CS_YCbCr    ), ///< ARGB => YCbCr ，忽略 ALPHA 通道值
    JENC_ABGR_TO_YCC    = JENC_CTRLCS_MAKE(JCTRL_CS_ABGR, JPEG_CS_YCbCr    ), ///< ABGR => YCbCr ，忽略 ALPHA 通道值

    JENC_GRAY_TO_BGYCC  = JENC_CTRLCS_MAKE(JCTRL_CS_GRAY, JPEG_CS_BG_YCC   ), ///< GRAY => BG_YCC
    JENC_RGB_TO_BGYCC   = JENC_CTRLCS_MAKE(JCTRL_CS_RGB , JPEG_CS_BG_YCC   ), ///< RGB  => BG_YCC
    JENC_BGR_TO_BGYCC   = JENC_CTRLCS_MAKE(JCTRL_CS_BGR , JPEG_CS_BG_YCC   ), ///< BGR  => BG_YCC
    JENC_RGBA_TO_BGYCC  = JENC_CTRLCS_MAKE(JCTRL_CS_RGBA, JPEG_CS_BG_YCC   ), ///< RGBA => BG_YCC ，忽略 ALPHA 通道值
    JENC_BGRA_TO_BGYCC  = JENC_CTRLCS_MAKE(JCTRL_CS_BGRA, JPEG_CS_BG_YCC   ), ///< BGRA => BG_YCC ，忽略 ALPHA 通道值
    JENC_ARGB_TO_BGYCC  = JENC_CTRLCS_MAKE(JCTRL_CS_ARGB, JPEG_CS_BG_YCC   ), ///< ARGB => BG_YCC ，忽略 ALPHA 通道值
    JENC_ABGR_TO_BGYCC  = JENC_CTRLCS_MAKE(JCTRL_CS_ABGR, JPEG_CS_BG_YCC   ), ///< ABGR => BG_YCC ，忽略 ALPHA 通道值
} jenc_ctrlcs_t;

/**
 * @enum  jenc_errno_table_t
 * @brief JPEG 编码操作的相关错误码表。
 */
typedef enum jenc_errno_table_t
{
    JENC_ERR_OK             =  0,   ///< 无错
    JENC_ERR_UNKNOWN        = -1,   ///< 未知错误
    JENC_ERR_INVALID_CTXPTR = -2,   ///< JPEG 编码操作的上下文对象无效
    JENC_ERR_INVALID_PARAM  = -3,   ///< 输入参数有误
    JENC_ERR_INVALID_DST    = -4,   ///< JPEG 编码的 数据输出源 未配置
    JENC_ERR_EXCEPTION      = -5,   ///< 编码操作过程产生异常错误
    JENC_ERR_ALLOC_MEM      = -6,   ///< 分配缓存失败
    JENC_ERR_OPEN_FILE      = -7,   ///< 打开文件失败
    JENC_ERR_FIO_FGETPOS    = -8,   ///< 获取 文件流对象 的当前文件指针位置信息 失败
} jenc_errno_table_t;

/**********************************************************/
/**
 * @brief JPEG 编码操作的相关错误码的 名称。
 */
static inline j_cstring_t jenc_errno_name(j_int_t jit_err)
{
    j_cstring_t jszt_name = "";

    switch (jit_err)
    {
    case JENC_ERR_OK             : jszt_name = "JENC_ERR_OK"            ; break;
    case JENC_ERR_UNKNOWN        : jszt_name = "JENC_ERR_UNKNOWN"       ; break;
    case JENC_ERR_INVALID_CTXPTR : jszt_name = "JENC_ERR_INVALID_CTXPTR"; break;
    case JENC_ERR_INVALID_PARAM  : jszt_name = "JENC_ERR_INVALID_PARAM" ; break;
    case JENC_ERR_INVALID_DST    : jszt_name = "JENC_ERR_INVALID_DST"   ; break;
    case JENC_ERR_EXCEPTION      : jszt_name = "JENC_ERR_EXCEPTION"     ; break;
    case JENC_ERR_ALLOC_MEM      : jszt_name = "JENC_ERR_ALLOC_MEM"     ; break;
    case JENC_ERR_OPEN_FILE      : jszt_name = "JENC_ERR_OPEN_FILE"     ; break;
    case JENC_ERR_FIO_FGETPOS    : jszt_name = "JENC_ERR_FIO_FGETPOS"   ; break;

    default: break;
    }

    return jszt_name;
}

/**********************************************************/
/**
 * @brief 申请 JPEG 编码操作的上下文对象。
 * 
 * @param [in ] jvt_reserved : 保留参数（当前使用 J_NULL 即可）。
 * 
 * @return jenc_ctxptr_t :
 * 返回 JPEG 编码操作的上下文对象，为 J_NULL 时表示申请失败。
 */
jenc_ctxptr_t jenc_alloc(j_void_t * jvt_reserved);

/**********************************************************/
/**
 * @brief 释放 JPEG 编码操作的上下文对象。
 */
j_void_t jenc_release(jenc_ctxptr_t jenc_cptr);

/**********************************************************/
/**
 * @brief JPEG 编码操作句柄的有效判断。
 */
j_bool_t jenc_valid(jenc_ctxptr_t jenc_cptr);

/**********************************************************/
/**
 * @brief 获取 JPEG 数据压缩质量。
 */
j_int_t jenc_get_quality(jenc_ctxptr_t jenc_cptr);

/**********************************************************/
/**
 * @brief 设置 JPEG 数据压缩质量（1 - 100，为 0 时，取默认值）。
 */
j_void_t jenc_set_quality(jenc_ctxptr_t jenc_cptr, j_int_t jit_quality);

/**********************************************************/
/**
 * @brief 
 * JPEG 编码输出源使用 文件流模式 时，是否自动重置 文件指针。
 * 默认情况下，其值为 J_TRUE，操作文件流后，会自动还原原文件指针位置。
 */
j_bool_t jenc_is_rfio(jenc_ctxptr_t jenc_cptr);

/**********************************************************/
/**
 * @brief 设置 文件流模式 时，是否自动重置 文件指针。
 */
j_void_t jenc_set_rfio(jenc_ctxptr_t jenc_cptr, j_bool_t jbl_rfio);

/**********************************************************/
/**
 * @brief 获取 JPEG 编码操作的上下文对象中，所缓存的 JPEG 数据流地址。
 */
j_mem_t jenc_cached_data(jenc_ctxptr_t jenc_cptr);

/**********************************************************/
/**
 * @brief 获取 JPEG 编码操作的上下文对象中，所缓存的 JPEG 数据流有效字节数。
 */
j_uint_t jenc_cached_size(jenc_ctxptr_t jenc_cptr);

/**********************************************************/
/**
 * @brief 配置 JPEG 编码操作的目标输出源信息。
 * 
 * @param [in ] jenc_cptr : JPEG 编码操作的上下文对象。
 * @param [in ] jit_mode  : JPEG 编码输出模式（参看 jctrl_mode_t ）。
 * 
 * @param [in ] jht_optr  : 指向输出源的操作信息。
 * - 内存模式，jht_optr 的类型为 j_mem_t ，其为输出 JPEG 数据的缓存地址，
 *   若 ((J_NULL == jht_optr) || (0 == jut_mlen)) 时，则取内部缓存地址；
 * - 文件流模式，jht_optr 的类型为 j_fio_t ，其为输出 JPEG 数据的文件流；
 * - 文件模式，jht_optr 的类型为 j_path_t ，其为输出 JPEG 数据的文件路径。
 * 
 * @param [in ] jut_mlen  : 只针对于 内存模式，表示缓存容量。
 * 
 * @return j_int_t : 错误码，请参看 jenc_errno_table_t 相关枚举值。
 */
j_int_t jenc_config_dst(
                jenc_ctxptr_t jenc_cptr,
                j_int_t       jit_mode,
                j_handle_t    jht_optr,
                j_uint_t      jut_mlen);

/**********************************************************/
/**
 * @brief 将 RGB 图像数据编码成 JPEG 数据流，输出至所配置的输出源中。
 * @note 调用该接口前，必须使用 jenc_config_dst() 配置好输出源信息。
 * 
 * @param [in ] jenc_cptr  : JPEG 编码操作的上下文对象。
 * @param [in ] jmem_iptr  : RGB 图像数据 缓存。
 * @param [in ] jit_stride : RGB 图像数据 像素行 步进大小。
 * @param [in ] jit_width  : RGB 图像数据 宽度。
 * @param [in ] jit_height : RGB 图像数据 高度。
 * @param [in ] jit_ctrlcs : 图像色彩空间的转换方式（参看 jenc_ctrlcs_t）。
 * 
 * @return j_int_t :
 * - 操作失败时，返回值 < 0，表示 错误码，参看 jenc_errno_table_t 相关枚举值。
 * - 操作成功时：
 *   1. 使用 文件流模式 或 文件模式 时，返回值 == JENC_ERR_OK；
 *   2. 使用 内存模式 时，返回值 > 0，表示输出源中存储的 JPEG 数据的有效字节数；
 *   3. 使用 内存模式 时，且 返回值 == JENC_ERR_OK，表示输出源的缓存容量不足，
 *      而输出的 JPEG 编码数据存储在 上下文对象 jenc_cptr 的缓存中，后续可通过
 *      jenc_cached_data() 和 jenc_cached_size() 获取此次编码得到的 JPEG 数据。
 */
j_int_t jenc_rgb_to_dst(
                jenc_ctxptr_t jenc_cptr,
                j_mem_t       jmem_iptr,
                j_int_t       jit_stride,
                j_int_t       jit_width,
                j_int_t       jit_height,
                j_int_t       jit_ctrlcs);

////////////////////////////////////////////////////////////////////////////////
// JPEG 解码操作的相关接口。

/** 声明 JPEG 解码操作的上下文 结构体 */
struct jdec_ctx_t;

/** 定义 JPEG 解码操作的上下文 结构体指针 */
typedef struct jdec_ctx_t * jdec_ctxptr_t;

/**
 * @enum  jdec_errno_table_t
 * @brief JPEG 解码操作的相关错误码表。
 */
typedef enum __jdec_errno_table__
{
    JDEC_ERR_OK             =   0,   ///< 无错
    JDEC_ERR_UNKNOWN        = - 1,   ///< 未知错误
    JDEC_ERR_INVALID_CTXPTR = - 2,   ///< JPEG 解码操作的上下文对象无效
    JDEC_ERR_INVALID_PARAM  = - 3,   ///< 输入参数有误
    JDEC_ERR_INVALID_SRC    = - 4,   ///< JPEG 解码的输入源 未配置
    JDEC_ERR_READ_HEADER    = - 5,   ///< 读取 JPEG （文件头）信息失败
    JDEC_ERR_EXCEPTION      = - 6,   ///< 解码操作过程产生异常错误
    JDEC_ERR_COLOR_FORMAT   = - 7,   ///< 不支持该颜色格式进行解码操作
    JDEC_ERR_STRIDE         = - 8,   ///< 输出的图像像素数据缓存的行步进值太小
    JDEC_ERR_CAPACITY       = - 9,   ///< 输出的图像像素数据缓存的容量不足
    JDEC_ERR_OPEN_FILE      = -10,   ///< 打开文件失败
    JDEC_ERR_FIO_FGETPOS    = -11,   ///< 获取 文件流对象 的当前文件指针位置信息 失败
} jdec_errno_table_t;

/**********************************************************/
/**
 * @brief JPEG 解码操作的相关错误码的 名称。
 */
static inline j_cstring_t jdec_errno_name(j_int_t jit_err)
{
    j_cstring_t jszt_name = "";

    switch (jit_err)
    {
    case JDEC_ERR_OK             : jszt_name = "JDEC_ERR_OK"            ; break;
    case JDEC_ERR_UNKNOWN        : jszt_name = "JDEC_ERR_UNKNOWN"       ; break;
    case JDEC_ERR_INVALID_CTXPTR : jszt_name = "JDEC_ERR_INVALID_CTXPTR"; break;
    case JDEC_ERR_INVALID_PARAM  : jszt_name = "JDEC_ERR_INVALID_PARAM" ; break;
    case JDEC_ERR_INVALID_SRC    : jszt_name = "JDEC_ERR_INVALID_SRC"   ; break;
    case JDEC_ERR_READ_HEADER    : jszt_name = "JDEC_ERR_READ_HEADER"   ; break;
    case JDEC_ERR_EXCEPTION      : jszt_name = "JDEC_ERR_EXCEPTION"     ; break;
    case JDEC_ERR_COLOR_FORMAT   : jszt_name = "JDEC_ERR_COLOR_FORMAT"  ; break;
    case JDEC_ERR_STRIDE         : jszt_name = "JDEC_ERR_STRIDE"        ; break;
    case JDEC_ERR_CAPACITY       : jszt_name = "JDEC_ERR_CAPACITY"      ; break;
    case JDEC_ERR_OPEN_FILE      : jszt_name = "JDEC_ERR_OPEN_FILE"     ; break;
    case JDEC_ERR_FIO_FGETPOS    : jszt_name = "JDEC_ERR_FIO_FGETPOS"   ; break;

    default: break;
    }

    return jszt_name;
}

/**********************************************************/
/**
 * @brief 申请 JPEG 解码操作的上下文对象。
 * 
 * @param [in ] jvt_reserved : 保留参数（当前使用 J_NULL 即可）。
 * 
 * @return jdec_ctxptr_t :
 * 返回 JPEG 解码操作的上下文对象，为 J_NULL 时表示申请失败。
 */
jdec_ctxptr_t jdec_alloc(j_void_t * jvt_reserved);

/**********************************************************/
/**
 * @brief 释放 JPEG 解码操作的上下文对象。
 */
j_void_t jdec_release(jdec_ctxptr_t jdec_cptr);

/**********************************************************/
/**
 * @brief 判断 JPEG 解码操作的上下文对象 是否有效。
 */
j_bool_t jdec_valid(jdec_ctxptr_t jdec_cptr);

/**********************************************************/
/**
 * @brief 设置解码输出数据（RGB数据）时，是否按 4 字节对齐。
 * @note  默认情况是按 4 字节对齐的。
 */
j_void_t jdec_set_align(jdec_ctxptr_t jdec_cptr, j_bool_t jbl_align);

/**********************************************************/
/**
 * @brief 设置解码过程中 填充空白通道 的数值（例如输出 RGB32 的 ALPHA 通道）。
 * @note 默认的填充值是 0xFFFFFFFF。
 */
j_void_t jdec_set_vpad(jdec_ctxptr_t jdec_cptr, j_uint_t jut_vpad);

/**********************************************************/
/**
 * @brief 
 * JPEG 解码输入源使用 文件流模式 时，是否自动重置 文件指针。
 * 默认情况下，其值为 J_TRUE，操作文件流后，会自动还原原文件指针位置。
 */
j_bool_t jdec_is_rfio(jdec_ctxptr_t jdec_cptr);

/**********************************************************/
/**
 * @brief 设置 文件流模式 时，是否自动重置 文件指针。
 */
j_void_t jdec_set_rfio(jdec_ctxptr_t jdec_cptr, j_bool_t jbl_rfio);

/**********************************************************/
/**
 * @brief 配置 JPEG 编码操作的数据输入源信息。
 * 
 * @param [in ] jenc_cptr : JPEG 编码操作的上下文对象。
 * @param [in ] jit_mode  : JPEG 编码输入模式（参看 jctrl_mode_t ）。
 * 
 * @param [in ] jht_optr  : 指向输入源的操作信息。
 * - 内存模式，jht_dst 的类型为 j_mem_t ，其为输入 JPEG 数据的缓存地址；
 * - 文件流模式，jht_dst 的类型为 j_fio_t ，其为输入 JPEG 数据的文件流；
 * - 文件模式，jht_dst 的类型为 j_path_t ，其为输入 JPEG 数据的文件路径。
 * 
 * @param [in ] jut_size  : 只针对于 内存模式，表示缓存中的有效字节数。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_table_t 相关枚举值。
 */
j_int_t jdec_config_src(
                jdec_ctxptr_t jdec_cptr,
                j_int_t       jit_mode,
                j_handle_t    jht_iptr,
                j_uint_t      jut_size);

/**********************************************************/
/**
 * @brief 获取 JPEG 输入源中的图像基本信息。
 * @note 调用该接口前，必须使用 jdec_config_src() 配置好输入源。
 * 
 * @param [in ] jdec_cptr : JPEG 解码操作的上下文对象。
 * @param [out] jinfo_ptr : 操作成功返回的图像基本信息。
 * 
 * @return j_int_t : 错误码（参看 jdec_errno_table_t 相关枚举值）。
 */
j_int_t jdec_src_info(
                jdec_ctxptr_t jdec_cptr,
                jinfo_ptr_t   jinfo_ptr);

/**********************************************************/
/**
 * @brief 将 JPEG 输入源图像 解码成 RGB 图像像素数据。
 * @note 调用该接口前，必须使用 jdec_config_src() 配置好输入源。
 * 
 * @param [in ] jdec_cptr : JPEG 解码操作的上下文对象。
 * @param [out] jmem_optr : 输出的 RGB 图像像素数据 缓存。
 * 
 * @param [in ] jit_stride : 
 * 输出的 RGB 图像像素行 步进大小；若为 0 时，
 * 则取 ((3 or 4) * jit_width) 为值（可能会 按 4 字节对齐）。
 * 
 * @param [in ] jut_mlen : 
 * 输出 RGB 缓存容量，其值应 >= abs(jit_stride * jit_height) ，
 * 所指定覆盖的内存区域为 [jmem_optr, jmem_optr + jit_stride * jit_height]。
 * 
 * @param [out] jit_width  : 操作成功返回的图像宽度（像素为单位）。
 * @param [out] jit_height : 操作成功返回的图像高度（像素为单位）。
 * @param [in ] jit_ctrlcs : 要求输出的 RGB 像素格式（参看 jctrl_color_space_t ）。
 * 
 * @return j_int_t : 错误码（参看 jdec_errno_table_t 相关枚举值）。
 * @retval JDEC_ERR_STRIDE : 
 * 可通过返回的 jit_width 值，重新调整 jit_stride 等参数，再进行尝试。
 * @retval JDEC_ERR_CAPACITY :
 * 可通过返回的 jit_height 值，重新调整 jut_mlen 等参数，再进行尝试。
 */
j_int_t jdec_src_to_rgb(
                jdec_ctxptr_t jdec_cptr,
                j_mem_t       jmem_optr,
                j_int_t       jit_stride,
                j_uint_t      jut_mlen,
                j_int_t     * jit_width,
                j_int_t     * jit_height,
                j_int_t       jit_ctrlcs);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
};
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus

////////////////////////////////////////////////////////////////////////////////
// 针对 C++ 封装的 JPEG 编码操作类

/**
 * @class jenc_handle_t
 * @brief JPEG 编码器。
 */
class jenc_handle_t
{
    // constructor/destuctor
public:
    jenc_handle_t(void)
    {
        m_jenc_cptr = jenc_alloc(J_NULL);
    }

    ~jenc_handle_t(void)
    {
        jenc_release(m_jenc_cptr);
        m_jenc_cptr = J_NULL;
    }

    // public interfaces
public:
    /**********************************************************/
    /**
     * @brief JPEG 编码操作句柄的有效判断。
     * @note 详情请参看 jenc_valid() 的说明。
     */
    inline j_bool_t valid(void) const
    {
        return jenc_valid(m_jenc_cptr);
    }

    /**********************************************************/
    /**
     * @brief 获取 JPEG 数据压缩质量。
     * @note 详情请参看 jenc_get_quality() 的说明。
     */
    inline j_int_t get_quality(void) const
    {
        return jenc_get_quality(m_jenc_cptr);
    }

    /**********************************************************/
    /**
     * @brief 设置 JPEG 数据压缩质量（1 - 100，为 0 时，取默认值）。
     * @note 详情请参看 jenc_set_quality() 的说明。
     */
    inline j_void_t set_quality(j_int_t jit_quality)
    {
        jenc_set_quality(m_jenc_cptr, jit_quality);
    }

    /**********************************************************/
    /**
     * @brief 
     * JPEG 编码输出源使用 文件流模式 时，是否自动重置 文件指针。
     * 默认情况下，其值为 J_TRUE，操作文件流后，会自动还原原文件指针位置。
     * @note 详情请参看 jenc_is_rfio() 的说明。
     */
    inline j_bool_t is_rfio(void) const
    {
        return jenc_is_rfio(m_jenc_cptr);
    }

    /**********************************************************/
    /**
     * @brief 设置 文件流模式 时，是否自动重置 文件指针。
     * @note 详情请参看 jenc_set_rfio() 的说明。
     */
    inline j_void_t set_rfio(j_bool_t jbl_rfio)
    {
        jenc_set_rfio(m_jenc_cptr, jbl_rfio);
    }

    /**********************************************************/
    /**
     * @brief 获取 JPEG 编码操作的上下文对象中，所缓存的 JPEG 数据流地址。
     * @note 详情请参看 jenc_cached_data() 的说明。
     */
    inline j_mem_t cached_data(void) const
    {
        return jenc_cached_data(m_jenc_cptr);
    }

    /**********************************************************/
    /**
     * @brief 获取 JPEG 编码操作的上下文对象中，所缓存的 JPEG 数据流有效字节数。
     * @note 详情请参看 jenc_cached_size() 的说明。
     */
    inline j_uint_t cached_size(void) const
    {
        return jenc_cached_size(m_jenc_cptr);
    }

    /**********************************************************/
    /**
     * @brief 配置 JPEG 编码操作的目标输出源信息。
     * @note 详情请参看 jenc_config_dst() 的说明。
     */
    inline j_int_t config_dst(
                    j_int_t    jit_mode,
                    j_handle_t jht_optr,
                    j_uint_t   jut_mlen)
    {
        return jenc_config_dst(
                    m_jenc_cptr,
                    jit_mode,
                    jht_optr,
                    jut_mlen);
    }

    /**********************************************************/
    /**
     * @brief 将 RGB 图像数据编码成 JPEG 数据流，输出至所配置的输出源中。
     * @note 调用该接口前，必须使用 config_dst() 配置好输出源信息。
     * @note 详情请参看 jenc_rgb_to_dst() 的说明。
     */
    inline j_int_t rgb_to_dst(
                    j_mem_t jmem_iptr,
                    j_int_t jit_stride,
                    j_int_t jit_width,
                    j_int_t jit_height,
                    j_int_t jit_ctrlcs)
    {
        return jenc_rgb_to_dst(
                    m_jenc_cptr,
                    jmem_iptr,
                    jit_stride,
                    jit_width,
                    jit_height,
                    jit_ctrlcs);
    }

    // data members
protected:
    jenc_ctxptr_t m_jenc_cptr;   ///< JPEG 编码操作的上下文对象
};

////////////////////////////////////////////////////////////////////////////////
// 针对 C++ 封装的 JPEG 解码操作类

/**
 * @class jdec_handle_t
 * @brief JPEG 解码器。
 */
class jdec_handle_t
{
    // constructor/destructor
public:
    jdec_handle_t(void)
    {
        m_jdec_cptr = jdec_alloc(J_NULL);
    }

    ~jdec_handle_t(void)
    {
        jdec_release(m_jdec_cptr);
        m_jdec_cptr = J_NULL;
    }

    // public interfaces
public:
    /**********************************************************/
    /**
     * @brief 当前对象是否有效。
     */
    inline j_bool_t valid(void) const
    {
        return jdec_valid(m_jdec_cptr);
    }

    /**********************************************************/
    /**
     * @brief 设置解码输出数据（RGB数据）时，是否按 4 字节对齐。
     * @note  详情请参看 jdec_set_align() 的说明。
     */
    inline j_void_t set_align(j_bool_t jbl_align)
    {
        return jdec_set_align(m_jdec_cptr, jbl_align);
    }

    /**********************************************************/
    /**
     * @brief 设置解码过程中 填充空白通道 的数值（例如输出 RGB32 的 ALPHA 通道）。
     * @note  详情请参看 jdec_set_vpad() 的说明。
     */
    inline j_void_t set_vpad(j_uint_t jut_vpad)
    {
        return jdec_set_vpad(m_jdec_cptr, jut_vpad);
    }

    /**********************************************************/
    /**
     * @brief 
     * JPEG 解码输入源使用 文件流模式 时，是否自动重置 文件指针。
     * 默认情况下，其值为 J_TRUE，操作文件流后，会自动还原原文件指针位置。
     * @note  详情请参看 jdec_is_rfio() 的说明。
     */
    inline j_bool_t is_rfio(void) const
    {
        return jdec_is_rfio(m_jdec_cptr);
    }

    /**********************************************************/
    /**
     * @brief 设置 文件流模式 时，是否自动重置 文件指针。
     * @note  详情请参看 jdec_set_rfio() 的说明。
     */
    inline j_void_t set_rfio(j_bool_t jbl_rfio)
    {
        jdec_set_rfio(m_jdec_cptr, jbl_rfio);
    }

    /**********************************************************/
    /**
     * @brief 配置 JPEG 编码操作的数据输入源信息。
     * @note  详情请参看 jdec_config_src() 的说明。
     */
    inline j_int_t config_src(
                    j_int_t    jit_mode,
                    j_handle_t jht_iptr,
                    j_uint_t   jut_size)
    {
        return jdec_config_src(
                    m_jdec_cptr,
                    jit_mode,
                    jht_iptr,
                    jut_size);
    }

    /**********************************************************/
    /**
     * @brief 获取 JPEG 输入源中的图像基本信息。
     * @note 调用该接口前，必须使用 config_src() 配置好输入源。
     * @note 详情请参看 jdec_src_info() 的说明。
     */
    inline j_int_t src_info(jinfo_ptr_t jinfo_ptr)
    {
        return jdec_src_info(m_jdec_cptr, jinfo_ptr);
    }

    /**********************************************************/
    /**
     * @brief 将 JPEG 输入源图像 解码成 RGB 图像像素数据。
     * @note 调用该接口前，必须使用 config_src() 配置好输入源。
     * @note 详情请参看 jdec_src_to_rgb() 的说明。
     */
    inline j_int_t src_to_rgb(
                    j_mem_t   jmem_optr,
                    j_int_t   jit_stride,
                    j_uint_t  jut_mlen,
                    j_int_t * jit_width,
                    j_int_t * jit_height,
                    j_int_t   jit_ctrlcs)
    {
        return jdec_src_to_rgb(
                    m_jdec_cptr,
                    jmem_optr,
                    jit_stride,
                    jut_mlen,
                    jit_width,
                    jit_height,
                    jit_ctrlcs);
    }

    // data members
private:
    jdec_ctxptr_t  m_jdec_cptr;   ///< JPEG 解码操作的上下文对象
};

////////////////////////////////////////////////////////////////////////////////

#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __XJPG_WRAPPER_H__
