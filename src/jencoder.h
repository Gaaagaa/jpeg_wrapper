/**
 * @file jencoder.h
 * Copyright (c) 2024 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2024-09-24
 * @version : 1.0.0.0
 * @brief   : 声明 JPEG 编码器的相关操作接口及数据类型。
 */

#ifndef __JENCODER_H__
#define __JENCODER_H__

#include "jcomm.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

/** 声明 JPEG 编码操作的上下文 结构体 */
struct jenc_ctx_t;

/** 定义 JPEG 编码操作的上下文 结构体指针 */
typedef struct jenc_ctx_t * jenc_this_t;

/**
 * @brief
 * 用于合成 jenc_ccs_t 枚举值，
 * 即 jctl_cs_t 与 jpeg_cs_t 的合成操作。
 */
#define JENC_CCS_MAKE(jctl, jpeg)   (((jpeg) << 24) | (jctl))

/**
 * @enum  jenc_ccs_t
 * @brief JPEG 编码操作所支持的色彩空间转换枚举值。
 */
typedef enum jenc_ctl_color_space_t
{
    JENC_CCS_UNKNOWN    = 0x00000000, ///< 未定义的操作

    JENC_GRAY_TO_GRAY   = JENC_CCS_MAKE(JCTL_CS_GRAY  , JPEG_CS_GRAY  ), ///< GRAY   => GRAY
    JENC_RGB_TO_GRAY    = JENC_CCS_MAKE(JCTL_CS_RGB   , JPEG_CS_GRAY  ), ///< RGB    => GRAY
    JENC_YCC_TO_GRAY    = JENC_CCS_MAKE(JCTL_CS_YCC   , JPEG_CS_GRAY  ), ///< YCC    => GRAY
    JENC_BGYCC_TO_GRAY  = JENC_CCS_MAKE(JCTL_CS_BG_YCC, JPEG_CS_GRAY  ), ///< BG-YCC => GRAY

    JENC_RGB_TO_RGB     = JENC_CCS_MAKE(JCTL_CS_RGB   , JPEG_CS_RGB   ), ///< RGB    => RGB

    JENC_BGRGB_TO_BGRGB = JENC_CCS_MAKE(JCTL_CS_BG_RGB, JPEG_CS_BG_RGB), ///< BG-RGB => BG-RGB

    JENC_YCC_TO_YCC     = JENC_CCS_MAKE(JCTL_CS_YCC   , JPEG_CS_YCC   ), ///< YCC    => YCC
    JENC_RGB_TO_YCC     = JENC_CCS_MAKE(JCTL_CS_RGB   , JPEG_CS_YCC   ), ///< RGB    => YCC

    JENC_BGYCC_TO_BGYCC = JENC_CCS_MAKE(JCTL_CS_BG_YCC, JPEG_CS_BG_YCC), ///< BG-YCC => BG-YCC
    JENC_RGB_TO_BGYCC   = JENC_CCS_MAKE(JCTL_CS_RGB   , JPEG_CS_BG_YCC), ///< RGB    => BG-YCC
    JENC_YCC_TO_BGYCC   = JENC_CCS_MAKE(JCTL_CS_YCC   , JPEG_CS_BG_YCC), ///< YCC    => BG-YCC

    JENC_CMYK_TO_CMYK   = JENC_CCS_MAKE(JCTL_CS_CMYK  , JPEG_CS_CMYK  ), ///< CMYK   => CMYK

    JENC_YCCK_TO_YCCK   = JENC_CCS_MAKE(JCTL_CS_YCCK  , JPEG_CS_YCCK  ), ///< YCCK   => YCCK
    JENC_CMYK_TO_YCCK   = JENC_CCS_MAKE(JCTL_CS_CMYK  , JPEG_CS_YCCK  ), ///< CMYK   => YCCK
} jenc_ccs_t;

/**
 * @brief 从 jenc_ccs_t 中提取编码输入像素的 通道数量。
 */
#define JENC_CCS_NUMC(jccs) JCTL_CS_NUMC(jccs)

/**
 * @brief 从 jenc_ccs_t 中提取编码输入的 色彩空间值（jpeg_cs_t 类型值）。
 */
#define JENC_CCS_IN(jccs)   JCTL_CS_TYPE(jccs)

/**
 * @brief 从 jenc_ccs_t 中提取编码输出的 色彩空间值（jpeg_cs_t 类型值）。
 */
#define JENC_CCS_OUT(jccs)  ((jccs) >> 24)

/**********************************************************/
/**
 * @brief 获取 jenc_ccs_t 类型值对应的 字符串名称。
 */
static inline j_cstring_t jenc_ccs_name(jenc_ccs_t jenc_ccs)
{
    j_cstring_t jsz_name;
    switch (jenc_ccs)
    {
    case JENC_GRAY_TO_GRAY   : jsz_name = "JENC_GRAY_TO_GRAY"  ; break;
    case JENC_RGB_TO_GRAY    : jsz_name = "JENC_RGB_TO_GRAY"   ; break;
    case JENC_YCC_TO_GRAY    : jsz_name = "JENC_YCC_TO_GRAY"   ; break;
    case JENC_BGYCC_TO_GRAY  : jsz_name = "JENC_BGYCC_TO_GRAY" ; break;
    case JENC_RGB_TO_RGB     : jsz_name = "JENC_RGB_TO_RGB"    ; break;
    case JENC_BGRGB_TO_BGRGB : jsz_name = "JENC_BGRGB_TO_BGRGB"; break;
    case JENC_YCC_TO_YCC     : jsz_name = "JENC_YCC_TO_YCC"    ; break;
    case JENC_RGB_TO_YCC     : jsz_name = "JENC_RGB_TO_YCC"    ; break;
    case JENC_BGYCC_TO_BGYCC : jsz_name = "JENC_BGYCC_TO_BGYCC"; break;
    case JENC_RGB_TO_BGYCC   : jsz_name = "JENC_RGB_TO_BGYCC"  ; break;
    case JENC_YCC_TO_BGYCC   : jsz_name = "JENC_YCC_TO_BGYCC"  ; break;
    case JENC_CMYK_TO_CMYK   : jsz_name = "JENC_CMYK_TO_CMYK"  ; break;
    case JENC_YCCK_TO_YCCK   : jsz_name = "JENC_YCCK_TO_YCCK"  ; break;
    case JENC_CMYK_TO_YCCK   : jsz_name = "JENC_CMYK_TO_YCCK"  ; break;
    default                  : jsz_name = "JENC_CCS_UNKNOWN"   ; break;
    }
    return jsz_name;
}

/**********************************************************/
/**
 * @brief 判断 jenc_ccs_t 类型值是否有效。
 */
static inline j_bool_t jenc_ccs_valid(jenc_ccs_t jenc_ccs)
{
    switch (jenc_ccs)
    {
    case JENC_GRAY_TO_GRAY  :
    case JENC_RGB_TO_GRAY   :
    case JENC_YCC_TO_GRAY   :
    case JENC_BGYCC_TO_GRAY :
    case JENC_RGB_TO_RGB    :
    case JENC_BGRGB_TO_BGRGB:
    case JENC_YCC_TO_YCC    :
    case JENC_RGB_TO_YCC    :
    case JENC_BGYCC_TO_BGYCC:
    case JENC_RGB_TO_BGYCC  :
    case JENC_YCC_TO_BGYCC  :
    case JENC_CMYK_TO_CMYK  :
    case JENC_YCCK_TO_YCCK  :
    case JENC_CMYK_TO_YCCK  :
        return J_TRUE;
        break;

    default:
        break;
    }

    return J_FALSE;
}

/**
 * @enum  jenc_errno_table_t
 * @brief JPEG 编码操作的相关错误码表。
 */
typedef enum jenc_errno_table_t
{
    JENC_ERR_OK      =    0,   ///< 无错
    JENC_ERR_UNKNOWN = -128,   ///< 未知错误

    JENC_ERR_WORKING       ,   ///< JEPG 编码器处于工作状态中
    JENC_ERR_CCS_VALUE     ,   ///< 设置转换的色彩空间 参数错误
    JENC_ERR_IMAGE_ZEROSIZE,   ///< 编码操作的图像（宽、高）为 空值
    JENC_ERR_IMAGE_OVERSIZE,   ///< 编码操作的图像（宽、高）太大
    JENC_ERR_FSTREAM_ISNULL,   ///< 指定输出的文件流为 J_NULL
    JENC_ERR_FSZPATH_ISNULL,   ///< 指定输出的文件路径为 J_NULL
    JENC_ERR_UNCONFIG      ,   ///< 未正确配置输出模式
    JENC_ERR_UNSTART       ,   ///< JPEG 编码器未启动
    JENC_ERR_FOPEN         ,   ///< 打开文件失败
    JENC_ERR_MALLOC        ,   ///< 申请缓存失败
    JENC_ERR_EPARAM        ,   ///< 输入参数有误
    JENC_ERR_EXCEPTION     ,   ///< 编码操作过程产生异常错误
} jenc_errno_t;

/**********************************************************/
/**
 * @brief JPEG 编码操作的相关错误码的 名称。
 */
static inline j_cstring_t jenc_errno_name(j_int_t jit_err)
{
    j_cstring_t jsz_name = "";

    switch (jit_err)
    {
    case JENC_ERR_OK             : jsz_name = "JENC_ERR_OK"            ; break;
    case JENC_ERR_UNKNOWN        : jsz_name = "JENC_ERR_UNKNOWN"       ; break;
    case JENC_ERR_WORKING        : jsz_name = "JENC_ERR_WORKING"       ; break;
    case JENC_ERR_CCS_VALUE      : jsz_name = "JENC_ERR_CCS_VALUE"     ; break;
    case JENC_ERR_IMAGE_ZEROSIZE : jsz_name = "JENC_ERR_IMAGE_ZEROSIZE"; break;
    case JENC_ERR_IMAGE_OVERSIZE : jsz_name = "JENC_ERR_IMAGE_OVERSIZE"; break;
    case JENC_ERR_FSTREAM_ISNULL : jsz_name = "JENC_ERR_FSTREAM_ISNULL"; break;
    case JENC_ERR_FSZPATH_ISNULL : jsz_name = "JENC_ERR_FSZPATH_ISNULL"; break;
    case JENC_ERR_UNCONFIG       : jsz_name = "JENC_ERR_UNCONFIG"      ; break;
    case JENC_ERR_UNSTART        : jsz_name = "JENC_ERR_UNSTART"       ; break;
    case JENC_ERR_MALLOC         : jsz_name = "JENC_ERR_MALLOC"        ; break;
    case JENC_ERR_FOPEN          : jsz_name = "JENC_ERR_FOPEN"         ; break;
    case JENC_ERR_EPARAM         : jsz_name = "JENC_ERR_EPARAM"        ; break;
    case JENC_ERR_EXCEPTION      : jsz_name = "JENC_ERR_EXCEPTION"     ; break;
    default: break;
    }

    return jsz_name;
}

/**********************************************************/
/**
 * @brief 申请 JPEG 编码操作的上下文对象。
 * 
 * @param [in ] jvt_reserved : 保留参数（当前使用 J_NULL 即可）。
 * 
 * @return jenc_this_t :
 *  返回 JPEG 编码操作的上下文对象，为 J_NULL 时表示申请失败。
 */
jenc_this_t jenc_alloc(j_void_t * jvt_reserved);

/**********************************************************/
/**
 * @brief 释放 JPEG 编码操作的上下文对象。
 */
j_void_t jenc_release(jenc_this_t jenc_this);

/**********************************************************/
/**
 * @brief JPEG 编码操作句柄的有效判断。
 */
j_bool_t jenc_valid(jenc_this_t jenc_this);

/**********************************************************/
/**
 * @brief 执行 JPEG 编码工作前，配置相关工作参数（目标输出模式 等）。
 * @note
 * 1. jct_mode == JCTL_MODE_FMEMORY, typeof(jht_optr) == j_fmemory_t;
 *    内存模式，即编码压缩后的数据，输出至指定的缓存，
 *    若 ((J_NULL == jht_optr) || (0 == jst_mlen))，或者压缩过程中，指定缓存
 *    大小不足，则取内部自动分配缓存，使用 jenc_fmdata() 和 jenc_fmsize()
 *    两个接口获得这些压缩数据信息。
 * 2. jct_mode == JCTL_MODE_FSTREAM, typeof(jht_optr) == j_fstream_t;
 *    文件流模式，即编码压缩后的数据，输出至指定的文件流中。
 * 3. jct_mode == JCTL_MODE_FSZPATH, typeof(jht_optr) == j_fszpath_t;
 *    文件模式，即编码压缩后的数据，输出至指定路径的外部文件中。
 * 
 * @param [in ] jenc_this : JPEG 编码操作的上下文对象。
 * @param [in ] jct_mode  : JPEG 编码输出模式（参看 jctl_mode_t ）。
 * @param [in ] jht_optr  : 指向目标输出的操作对象。
 * @param [in ] jst_mlen  : 只针对于 内存模式，表示目标输出缓存的容量（按字节计）。
 * @param [in ] jut_qual  : JPEG 编码压缩数据的质量（1 - 100，为 0 时，取默认值）。
 * 
 * @return j_int_t : 错误码，请参看 jenc_errno_t 相关枚举值。
 */
j_int_t jenc_config(
                jenc_this_t jenc_this,
                jctl_mode_t jct_mode,
                j_fhandle_t jht_optr,
                j_size_t    jst_mlen,
                j_uint_t    jut_qual);

/**********************************************************/
/**
 * @brief 在内存模式下，执行编码压缩操作后，所缓存的 JPEG 数据流地址。
 */
j_mptr_t jenc_fmdata(jenc_this_t jenc_this);

/**********************************************************/
/**
 * @brief 在内存模式下，执行编码压缩操作后，所缓存的 JPEG 数据流有效字节数。
 */
j_uint_t jenc_fmsize(jenc_this_t jenc_this);

/**********************************************************/
/**
 * @brief 启动 JPEG 编码操作。
 * @note  启动编码器前，应先调用 jenc_config() 配置好输出模式。
 * 
 * @param [in ] jenc_this : JPEG 编码操作的上下文对象。
 * @param [in ] jccs_conv : 设置色彩空间的转换方式（参看 jenc_ccs_t）。
 * @param [in ] jut_imgw  : 设置编码输出图像的宽度（以像素为单位）。
 * @param [in ] jut_imgh  : 设置编码输出图像的高度（以像素为单位）。
 * 
 * @return j_int_t : 错误码，请参看 jenc_errno_t 相关枚举值。
 */
j_int_t jenc_start(
                jenc_this_t jenc_this,
                jenc_ccs_t  jccs_conv,
                j_uint_t    jut_imgw,
                j_uint_t    jut_imgh);

/**********************************************************/
/**
 * @brief 向 JPEG 编码器写入 图像像素 数据，执行编码操作。
 * @note 
 * 执行该操作前，应先调用 jenc_start() 启动编码器；
 * 而完成所有图像像素行编码操作后，调用 jenc_finish() 结束编码工作。
 * 
 * @param [in ] jenc_this : JPEG 编码操作的上下文对象。
 * @param [in ] jmt_pxls  : JPEG 编码输入的像素缓存，其色彩空间在 jenc_start() 已设置。
 * @param [in ] jit_step  : 遍历像素行时的 步长值（以 字节 为单位）。
 * @param [in ] jut_rows  : 此次计划写入像素行的数量。
 * 
 * @return j_int_t : 
 * - 返回值 < 0，表示操作失败，产生错误返回的 错误码，请参看 jenc_errno_t 相关枚举值。
 * - 返回值 > 0，表示写入的图像像素行数量。
 * - 返回值 = 0，表示图像像素行已经写满，不可再继续写。
 */
j_int_t jenc_write(
                jenc_this_t jenc_this,
                j_mptr_t    jmt_pxls,
                j_int_t     jit_step,
                j_uint_t    jut_rows);

/**********************************************************/
/**
 * @brief 完成（结束） JPEG 编码操作。
 * 
 * @param [in ] jenc_this : JPEG 编码操作的上下文对象。
 * 
 * @return j_int_t :
 * - 操作失败时，返回值 < 0，表示 错误码，参看 jenc_errno_t 相关枚举值。
 * - 操作成功时：
 *   1. 使用 文件流模式 或 文件模式 时，返回值 == JENC_ERR_OK；
 *   2. 使用 内存模式 时，返回值 > 0，表示输出源中存储的 JPEG 数据的有效字节数；
 *   3. 使用 内存模式 时，且 返回值 == JENC_ERR_OK，表示输出源的缓存容量不足，
 *      而输出的 JPEG 编码数据存储在 jenc_this 的内部缓存中，后续可通过
 *      jenc_fmdata() 和 jenc_fmsize() 获取此次编码得到的 JPEG 数据。
 */
j_int_t jenc_finish(jenc_this_t jenc_this);

/**********************************************************/
/**
 * @brief 对整幅图像像素数据进行 JPEG 编码压缩操作。
 * @note 
 * 操作前，应先使用 jenc_config() 配置好输出模式。
 * 该接口使用 jenc_start()/jenc_write()/jenc_finish() 实现。
 * 
 * @param [in ] jenc_this : JPEG 编码操作的上下文对象。
 * @param [in ] jccs_conv : 设置色彩空间的转换方式（参看 jenc_ccs_t）。
 * @param [in ] jmt_pxls  : 图像的像素缓存。
 * @param [in ] jit_step  : 遍历像素行时的 步长值（以 字节 为单位）。
 * @param [in ] jut_imgw  : 设置编码输出图像的宽度（以像素为单位）。
 * @param [in ] jut_imgh  : 设置编码输出图像的高度（以像素为单位）。
 * 
 * @return j_int_t :
 * - 操作失败时，返回值 < 0，表示 错误码，参看 jenc_errno_t 相关枚举值。
 * - 操作成功时：
 *   1. 使用 文件流模式 或 文件模式 时，返回值 == JENC_ERR_OK；
 *   2. 使用 内存模式 时，返回值 > 0，表示输出源中存储的 JPEG 数据的有效字节数；
 *   3. 使用 内存模式 时，且 返回值 == JENC_ERR_OK，表示输出源的缓存容量不足，
 *      而输出的 JPEG 编码数据存储在 jenc_this 的内部缓存中，后续可通过
 *      jenc_fmdata() 和 jenc_fmsize() 获取此次编码得到的 JPEG 数据。
 */
j_int_t jenc_image(
                jenc_this_t jenc_this,
                jenc_ccs_t  jccs_conv,
                j_mptr_t    jmt_pxls,
                j_int_t     jit_step,
                j_uint_t    jut_imgw,
                j_uint_t    jut_imgh);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus

/**
 * @class jencoder_t
 * @brief 封装 JPEG 编码器相关操作接口 成 C++ 类。
 */
class jencoder_t
{
    // public interfaces
public:
    jencoder_t(j_void_t * jvt_reserved = J_NULL)
    {
        m_jenc_this = jenc_alloc(jvt_reserved);
    }

    ~jencoder_t(void)
    {
        jenc_release(m_jenc_this);
        m_jenc_this = J_NULL;
    }

    // public interfaces
public:
    /**********************************************************/
    /**
     * @brief JPEG 编码操作句柄的有效判断。
     * @note  详情请参看 jenc_valid() 的说明。
     */
    inline j_bool_t valid(jenc_this_t jenc_this)
    {
        return jenc_valid(m_jenc_this);
    }

    /**********************************************************/
    /**
     * @brief 执行 JPEG 编码工作前，配置相关工作参数（目标输出模式 等）。
     * @note  详情请参看 jenc_config() 的说明。
     */
    inline j_int_t config(
                    jctl_mode_t jct_mode,
                    j_fhandle_t jht_optr,
                    j_size_t    jst_mlen,
                    j_uint_t    jut_qual = 0)
    {
        return jenc_config(
                    m_jenc_this,
                    jct_mode,
                    jht_optr,
                    jst_mlen,
                    jut_qual);
    }

    /**********************************************************/
    /**
     * @brief 在内存模式下，执行编码压缩操作后，所缓存的 JPEG 数据流地址。
     * @note  详情请参看 jenc_fmdata() 的说明。
     */
    inline j_mptr_t fmdata(void)
    {
        return jenc_fmdata(m_jenc_this);
    }

    /**********************************************************/
    /**
     * @brief 在内存模式下，执行编码压缩操作后，所缓存的 JPEG 数据流有效字节数。
     * @note  详情请参看 jenc_fmsize() 的说明。
     */
    inline j_uint_t fmsize(void)
    {
        return jenc_fmsize(m_jenc_this);
    }

    /**********************************************************/
    /**
     * @brief 启动 JPEG 编码操作。
     * @note  详情请参看 jenc_start() 的说明。
     */
    inline j_int_t start(
                    jenc_ccs_t jccs_conv,
                    j_uint_t   jut_imgw,
                    j_uint_t   jut_imgh)
    {
        return jenc_start(
                    m_jenc_this,
                    jccs_conv,
                    jut_imgw,
                    jut_imgh);
    }

    /**********************************************************/
    /**
     * @brief 向 JPEG 编码器写入 图像像素 数据，执行编码操作。
     * @note  详情请参看 jenc_write() 的说明。
     */
    inline j_int_t write(
                    j_mptr_t jmt_pxls,
                    j_int_t  jit_step,
                    j_uint_t jut_rows)
    {
        return jenc_write(
                    m_jenc_this,
                    jmt_pxls,
                    jit_step,
                    jut_rows);
    }

    /**********************************************************/
    /**
     * @brief 完成（结束） JPEG 编码操作。
     * @note  详情请参看 jenc_finish() 的说明。
     */
    inline j_int_t finish(void)
    {
        return jenc_finish(m_jenc_this);
    }

    /**********************************************************/
    /**
     * @brief 对整幅图像像素数据进行 JPEG 编码压缩操作。
     * @note  详情请参看 jenc_image() 的说明。
     */
    inline j_int_t encode_image(
                    jenc_ccs_t jccs_conv,
                    j_mptr_t   jmt_pxls,
                    j_int_t    jit_step,
                    j_uint_t   jut_imgw,
                    j_uint_t   jut_imgh)
    {
        return jenc_image(
                    m_jenc_this,
                    jccs_conv,
                    jmt_pxls,
                    jit_step,
                    jut_imgw,
                    jut_imgh);
    }

    // data members
private:
    jenc_this_t m_jenc_this; ///< JPEG 编码操作的上下文对象
};

#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __JENCODER_H__
