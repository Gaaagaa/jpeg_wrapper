/**
 * @file jcomm.h
 * Copyright (c) 2024 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2024-09-24
 * @version : 1.0.0.0
 * @brief   : 为 JPEG 编码器/解码器 相关实现，声明公共的数据类型及常量。
 */

#ifndef __JCOMM_H__
#define __JCOMM_H__

#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

#define J_NULL          0
#define J_FALSE         0
#define J_TRUE          1

typedef void            j_void_t;
typedef char            j_char_t;
typedef int             j_int_t;
typedef long            j_long_t;
typedef unsigned int    j_uint_t;
typedef unsigned long   j_ulong_t;
typedef unsigned char   j_byte_t;
typedef unsigned int    j_bool_t;
typedef unsigned char * j_mptr_t;
typedef const char *    j_cstring_t;

typedef size_t          j_size_t;
typedef fpos_t          j_fpos_t;

typedef void *          j_fhandle_t;
typedef unsigned char * j_fmemory_t;
typedef FILE *          j_fstream_t;
typedef const char *    j_fszpath_t;

/**
 * @enum  jpeg_color_space_t
 * @brief JPEG 图像所支持的像素格式。
 */
typedef enum jpeg_color_space_t
{
    JPEG_CS_UNKNOWN,  ///< error/unspecified
    JPEG_CS_GRAY   ,  ///< monochrome
    JPEG_CS_RGB    ,  ///< red/green/blue, standard RGB (sRGB)
    JPEG_CS_YCC    ,  ///< Y/Cb/Cr (also known as YUV), standard YCC
    JPEG_CS_CMYK   ,  ///< C/M/Y/K
    JPEG_CS_YCCK   ,  ///< Y/Cb/Cr/K
    JPEG_CS_BG_RGB ,  ///< big gamut red/green/blue, bg-sRGB
    JPEG_CS_BG_YCC ,  ///< big gamut Y/Cb/Cr, bg-sYCC
} jpeg_cs_t;

/**********************************************************/
/**
 * @brief 获取 jpeg_cs_t 类型值对应的 字符串名称。
 */
static inline j_cstring_t jpeg_cs_name(jpeg_cs_t jpeg_cs)
{
    j_cstring_t jsz_name;
    switch (jpeg_cs)
    {
    case JPEG_CS_GRAY    : jsz_name = "JPEG_CS_GRAY"   ; break;
    case JPEG_CS_RGB     : jsz_name = "JPEG_CS_RGB"    ; break;
    case JPEG_CS_YCC     : jsz_name = "JPEG_CS_YCC"    ; break;
    case JPEG_CS_CMYK    : jsz_name = "JPEG_CS_CMYK"   ; break;
    case JPEG_CS_YCCK    : jsz_name = "JPEG_CS_YCCK"   ; break;
    case JPEG_CS_BG_RGB  : jsz_name = "JPEG_CS_BG_RGB" ; break;
    case JPEG_CS_BG_YCC  : jsz_name = "JPEG_CS_BG_YCC" ; break;
    default              : jsz_name = "JPEG_CS_UNKNOWN"; break;
    }
    return jsz_name;
}

/**
 * @enum  jctl_color_space_t
 * @brief JPEG 编码/解码 所操作（输入/输出）的图像像素格式。
 * @note
 * 枚举值的 比特位 功能，定义如下：
 * - [  0 ~  7 ] : 用于表示每个像素所占的比特数，如 24，32 等值；
 * - [  8 ~ 15 ] : 通道数量；
 * - [ 16 ~ 23 ] : 色彩空间；
 * - [ 24 ~ 31 ] : 保留位。
 */
typedef enum jctl_color_space_t
{
    JCTL_CS_UNKNOWN = 0x00000000,                                   ///< 未知格式
    JCTL_CS_GRAY    = ((JPEG_CS_GRAY   & 0xFF) << 16) | 0x00000108, ///< GRAY 8位 格式，即 灰度图
    JCTL_CS_RGB     = ((JPEG_CS_RGB    & 0xFF) << 16) | 0x00000318, ///< RGB24
    JCTL_CS_YCC     = ((JPEG_CS_YCC    & 0xFF) << 16) | 0x00000318, ///< YCC（YUV444）
    JCTL_CS_CMYK    = ((JPEG_CS_CMYK   & 0xFF) << 16) | 0x00000420, ///< CMYK
    JCTL_CS_YCCK    = ((JPEG_CS_YCCK   & 0xFF) << 16) | 0x00000420, ///< YCCK
    JCTL_CS_BG_RGB  = ((JPEG_CS_BG_RGB & 0xFF) << 16) | 0x00000318, ///< bg-sRGB
    JCTL_CS_BG_YCC  = ((JPEG_CS_BG_YCC & 0xFF) << 16) | 0x00000318, ///< bg-sYCC
} jctl_cs_t;

/** 读取 jctl_cs_t 的每像素比特数 */
#define JCTL_CS_BITS(jctl)  (((jctl) & 0x000000FF))

/** 读取 jctl_cs_t 的通道数量 */
#define JCTL_CS_NUMC(jctl)  (((jctl) & 0x0000FF00) >> 8)

/** 读取 jctl_cs_t 的色彩空间类型 */
#define JCTL_CS_TYPE(jctl)  ((jpeg_cs_t)(((jctl) & 0x00FF0000) >> 16))

/**********************************************************/
/**
 * @brief 获取 jctl_cs_t 类型值对应的 字符串名称。
 */
static inline j_cstring_t jctl_cs_name(jctl_cs_t jctl_cs)
{
    j_cstring_t jsz_name;
    switch (jctl_cs)
    {
    case JCTL_CS_GRAY    : jsz_name = "JCTL_CS_GRAY"   ; break;
    case JCTL_CS_RGB     : jsz_name = "JCTL_CS_RGB"    ; break;
    case JCTL_CS_YCC     : jsz_name = "JCTL_CS_YCC"    ; break;
    case JCTL_CS_CMYK    : jsz_name = "JCTL_CS_CMYK"   ; break;
    case JCTL_CS_YCCK    : jsz_name = "JCTL_CS_YCCK"   ; break;
    case JCTL_CS_BG_RGB  : jsz_name = "JCTL_CS_BG_RGB" ; break;
    case JCTL_CS_BG_YCC  : jsz_name = "JCTL_CS_BG_YCC" ; break;
    default              : jsz_name = "JCTL_CS_UNKNOWN"; break;
    }
    return jsz_name;
}

/**********************************************************/
/**
 * @brief jpeg_cs_t => jctl_cs_t 。
 */
static inline jctl_cs_t jcs_mapto_ctl(jpeg_cs_t jpeg_cs)
{
    jctl_cs_t jctl_cs;

    switch (jpeg_cs)
    {
    case JPEG_CS_GRAY   : jctl_cs = JCTL_CS_GRAY   ; break;
    case JPEG_CS_RGB    : jctl_cs = JCTL_CS_RGB    ; break;
    case JPEG_CS_YCC    : jctl_cs = JCTL_CS_YCC    ; break;
    case JPEG_CS_CMYK   : jctl_cs = JCTL_CS_CMYK   ; break;
    case JPEG_CS_YCCK   : jctl_cs = JCTL_CS_YCCK   ; break;
    case JPEG_CS_BG_RGB : jctl_cs = JCTL_CS_BG_RGB ; break;
    case JPEG_CS_BG_YCC : jctl_cs = JCTL_CS_BG_YCC ; break;
    default             : jctl_cs = JCTL_CS_UNKNOWN; break;
    }

    return jctl_cs;
}

/**
 * @enum  jctl_mode_t
 * @brief JPEG 编码/解码 的操作模式。
 */
typedef enum jctl_mode_t
{
    JCTL_MODE_UNKNOWN,   ///< 未知模式
    JCTL_MODE_FMEMORY,   ///< 内存模式
    JCTL_MODE_FSTREAM,   ///< 文件流模式
    JCTL_MODE_FSZPATH,   ///< 文件模式
} jctl_mode_t;

/**
 * @struct jpeg_info_t
 * @brief  JPEG 图像基本信息。
 */
typedef struct jpeg_info_t
{
    j_int_t   jit_imgw; ///< nominal image width (from SOF marker)
    j_int_t   jit_imgh; ///< nominal image height
    j_int_t   jit_nchs; ///< # of color components in JPEG image
    jpeg_cs_t jcs_type; ///< colorspace of JPEG image ( @see jpeg_color_space_t )
} jpeg_info_t, * jinfo_ptr_t;

////////////////////////////////////////////////////////////////////////////////

#endif // __JCOMM_H__
