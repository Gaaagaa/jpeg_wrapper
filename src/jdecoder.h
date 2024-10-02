/**
 * @file jdecoder.h
 * Copyright (c) 2024 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2024-09-24
 * @version : 1.0.0.0
 * @brief   : 声明 JPEG 解码器的相关操作接口及数据类型。
 */

#ifndef __JDECODER_H__
#define __JDECODER_H__

#include "jcomm.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

/** 声明 JPEG 解码操作的上下文 结构体 */
struct jdec_ctx_t;

/** 定义 JPEG 解码操作的上下文 结构体指针 */
typedef struct jdec_ctx_t * jdec_this_t;

/**
 * @brief
 * 用于合成 jdec_ccs_t 枚举值，
 * 即 jctl_cs_t 与 jpeg_cs_t 的合成操作。
 */
#define JDEC_CCS_MAKE(jctl, jpeg)   (((jpeg) << 24) | (jctl))

/**
 * @enum  jdec_ccs_t
 * @brief JPEG 解码操作所支持的色彩空间转换枚举值。
 */
typedef enum jdec_ctl_color_space_t
{
    JDEC_CCS_UNKNOWN    = 0x00000000, ///< 未定义的操作

    JDEC_GRAY_TO_GRAY   = JDEC_CCS_MAKE(JCTL_CS_GRAY  , JPEG_CS_GRAY  ), ///< GRAY   => GRAY
    JDEC_RGB_TO_GRAY    = JDEC_CCS_MAKE(JCTL_CS_GRAY  , JPEG_CS_RGB   ), ///< RGB    => GRAY
    JDEC_YCC_TO_GRAY    = JDEC_CCS_MAKE(JCTL_CS_GRAY  , JPEG_CS_YCC   ), ///< YCC    => GRAY
    JDEC_BGYCC_TO_GRAY  = JDEC_CCS_MAKE(JCTL_CS_GRAY  , JPEG_CS_BG_YCC), ///< BG-YCC => GRAY

    JDEC_RGB_TO_RGB     = JDEC_CCS_MAKE(JCTL_CS_RGB   , JPEG_CS_RGB   ), ///< RGB    => RGB
    JDEC_GRAY_TO_RGB    = JDEC_CCS_MAKE(JCTL_CS_RGB   , JPEG_CS_GRAY  ), ///< GRAY   => RGB
    JDEC_YCC_TO_RGB     = JDEC_CCS_MAKE(JCTL_CS_RGB   , JPEG_CS_YCC   ), ///< YCC    => RGB
    JDEC_BGYCC_TO_RGB   = JDEC_CCS_MAKE(JCTL_CS_RGB   , JPEG_CS_BG_YCC), ///< BG-YCC => RGB

    JDEC_YCC_TO_YCC     = JDEC_CCS_MAKE(JCTL_CS_YCC   , JPEG_CS_YCC   ), ///< YCC    => YCC

    JDEC_BGRGB_TO_BGRGB = JDEC_CCS_MAKE(JCTL_CS_BG_RGB, JPEG_CS_BG_RGB), ///< BG-RGB => BG-RGB

    JDEC_BGYCC_TO_BGYCC = JDEC_CCS_MAKE(JCTL_CS_BG_YCC, JPEG_CS_BG_YCC), ///< BG-YCC => BG-YCC

    JDEC_CMYK_TO_CMYK   = JDEC_CCS_MAKE(JCTL_CS_CMYK  , JPEG_CS_CMYK  ), ///< CMYK   => CMYK
    JDEC_YCCK_TO_CMYK   = JDEC_CCS_MAKE(JCTL_CS_CMYK  , JPEG_CS_YCCK  ), ///< YCCK   => CMYK

    JDEC_YCCK_TO_YCCK   = JDEC_CCS_MAKE(JCTL_CS_YCCK  , JPEG_CS_YCCK  ), ///< YCCK   => YCCK
} jdec_ccs_t;

/**********************************************************/
/**
 * @brief 获取 jdec_ccs_t 类型值对应的 字符串名称。
 */
static inline j_cstring_t jdec_ccs_name(jdec_ccs_t jdec_ccs)
{
    j_cstring_t jsz_name;
    switch (jdec_ccs)
    {
    case JDEC_GRAY_TO_GRAY   : jsz_name = "JDEC_GRAY_TO_GRAY"  ; break;
    case JDEC_RGB_TO_GRAY    : jsz_name = "JDEC_RGB_TO_GRAY"   ; break;
    case JDEC_YCC_TO_GRAY    : jsz_name = "JDEC_YCC_TO_GRAY"   ; break;
    case JDEC_BGYCC_TO_GRAY  : jsz_name = "JDEC_BGYCC_TO_GRAY" ; break;
    case JDEC_RGB_TO_RGB     : jsz_name = "JDEC_RGB_TO_RGB"    ; break;
    case JDEC_GRAY_TO_RGB    : jsz_name = "JDEC_GRAY_TO_RGB"   ; break;
    case JDEC_YCC_TO_RGB     : jsz_name = "JDEC_YCC_TO_RGB"    ; break;
    case JDEC_BGYCC_TO_RGB   : jsz_name = "JDEC_BGYCC_TO_RGB"  ; break;
    case JDEC_YCC_TO_YCC     : jsz_name = "JDEC_YCC_TO_YCC"    ; break;
    case JDEC_BGRGB_TO_BGRGB : jsz_name = "JDEC_BGRGB_TO_BGRGB"; break;
    case JDEC_BGYCC_TO_BGYCC : jsz_name = "JDEC_BGYCC_TO_BGYCC"; break;
    case JDEC_CMYK_TO_CMYK   : jsz_name = "JDEC_CMYK_TO_CMYK"  ; break;
    case JDEC_YCCK_TO_CMYK   : jsz_name = "JDEC_YCCK_TO_CMYK"  ; break;
    case JDEC_YCCK_TO_YCCK   : jsz_name = "JDEC_YCCK_TO_YCCK"  ; break;
    default                  : jsz_name = "JDEC_CCS_UNKNOWN"   ; break;
    }
    return jsz_name;
}

/**********************************************************/
/**
 * @brief 判断 jdec_ccs_t 类型值是否有效。
 */
static inline j_bool_t jdec_ccs_valid(jdec_ccs_t jdec_ccs)
{
    switch (jdec_ccs)
    {
    case JDEC_GRAY_TO_GRAY  :
    case JDEC_RGB_TO_GRAY   :
    case JDEC_YCC_TO_GRAY   :
    case JDEC_BGYCC_TO_GRAY :
    case JDEC_RGB_TO_RGB    :
    case JDEC_GRAY_TO_RGB   :
    case JDEC_YCC_TO_RGB    :
    case JDEC_BGYCC_TO_RGB  :
    case JDEC_YCC_TO_YCC    :
    case JDEC_BGRGB_TO_BGRGB:
    case JDEC_BGYCC_TO_BGYCC:
    case JDEC_CMYK_TO_CMYK  :
    case JDEC_YCCK_TO_CMYK  :
    case JDEC_YCCK_TO_YCCK  :
        return J_TRUE;
        break;

    default:
        break;
    }

    return J_FALSE;
}

/**
 * @enum  jdec_errno_t
 * @brief JPEG 解码操作的相关错误码表。
 */
typedef enum __jdec_errno__
{
    JDEC_ERR_OK      =    0,   ///< 无错
    JDEC_ERR_UNKNOWN = -128,   ///< 未知错误

    JDEC_ERR_WORKING       ,   ///< JEPG 解码器处于工作状态中
    JDEC_ERR_FGETPOS       ,   ///< 调用 fgetpos() 操作失败
    JDEC_ERR_FOPEN         ,   ///< 调用 fopen() 操作失败
    JDEC_ERR_READ_HEADER   ,   ///< 读取 JPEG （文件头）信息失败
    JDEC_ERR_UNCONFIG      ,   ///< 未配置 JPEG 图像输入源
    JDEC_ERR_CCS_UNIMPL    ,   ///< 色彩空间的转换未实现（不支持）
    JDEC_ERR_START_FAILED  ,   ///< 解码器启动失败
    JDEC_ERR_UNSTART       ,   ///< 解码器未启动
    JDEC_ERR_OUT_EMPTY     ,   ///< 解码输出为空

    JDEC_ERR_MALLOC        ,   ///< 申请缓存失败
    JDEC_ERR_EPARAM        ,   ///< 输入参数有误
    JDEC_ERR_EXCEPTION     ,   ///< 编码操作过程产生异常错误
} jdec_errno_t;

/**********************************************************/
/**
 * @brief JPEG 解码操作的相关错误码的 名称。
 */
static inline j_cstring_t jdec_errno_name(j_int_t jit_err)
{
    j_cstring_t jsz_name = "";

    switch (jit_err)
    {
    case JDEC_ERR_OK           : jsz_name = "JDEC_ERR_OK"          ; break;
    case JDEC_ERR_UNKNOWN      : jsz_name = "JDEC_ERR_UNKNOWN"     ; break;
    case JDEC_ERR_WORKING      : jsz_name = "JDEC_ERR_WORKING"     ; break;
    case JDEC_ERR_FGETPOS      : jsz_name = "JDEC_ERR_FGETPOS"     ; break;
    case JDEC_ERR_FOPEN        : jsz_name = "JDEC_ERR_FOPEN"       ; break;
    case JDEC_ERR_READ_HEADER  : jsz_name = "JDEC_ERR_READ_HEADER" ; break;
    case JDEC_ERR_UNCONFIG     : jsz_name = "JDEC_ERR_UNCONFIG"    ; break;
    case JDEC_ERR_CCS_UNIMPL   : jsz_name = "JDEC_ERR_CCS_UNIMPL"  ; break;
    case JDEC_ERR_START_FAILED : jsz_name = "JDEC_ERR_START_FAILED"; break;
    case JDEC_ERR_UNSTART      : jsz_name = "JDEC_ERR_UNSTART"     ; break;
    case JDEC_ERR_OUT_EMPTY    : jsz_name = "JDEC_ERR_OUT_EMPTY"   ; break;
    case JDEC_ERR_MALLOC       : jsz_name = "JDEC_ERR_MALLOC"      ; break;
    case JDEC_ERR_EPARAM       : jsz_name = "JDEC_ERR_EPARAM"      ; break;
    case JDEC_ERR_EXCEPTION    : jsz_name = "JDEC_ERR_EXCEPTION"   ; break;
    default: break;
    }

    return jsz_name;
}

/**********************************************************/
/**
 * @brief 申请 JPEG 解码操作的上下文对象。
 * 
 * @param [in ] jvt_reserved : 保留参数（当前使用 J_NULL 即可）。
 * 
 * @return jdec_this_t :
 * 返回 JPEG 解码操作的上下文对象，为 J_NULL 时表示申请失败。
 */
jdec_this_t jdec_alloc(j_void_t * jvt_reserved);

/**********************************************************/
/**
 * @brief 释放 JPEG 解码操作的上下文对象。
 */
j_void_t jdec_release(jdec_this_t jdec_this);

/**********************************************************/
/**
 * @brief 判断 JPEG 解码操作的上下文对象 是否有效。
 */
j_bool_t jdec_valid(jdec_this_t jdec_this);

/**********************************************************/
/**
 * @brief 执行 JPEG 解码工作前，配置相关工作参数（输入源模式 等）。
 * @note
 * 1. jct_mode == JCTL_MODE_FMEMORY, typeof(jht_optr) == j_fmemory_t;
 *    内存模式，即解码输入源的数据，来源于指定的缓存。
 * 2. jct_mode == JCTL_MODE_FSTREAM, typeof(jht_optr) == j_fstream_t;
 *    文件流模式，即解码输入源的数据，来源于指定的文件流。
 * 3. jct_mode == JCTL_MODE_FSZPATH, typeof(jht_optr) == j_fszpath_t;
 *    文件模式，即解码输入源的数据，来源于指定文件路径的外部文件。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * @param [in ] jct_mode  : JPEG 解码输入模式（参看 jctl_mode_t ）。
 * @param [in ] jfh_iptr  : 指向输入源的操作对象。
 * @param [in ] jst_mlen  : 只针对于 内存模式，表示输入源缓存的有效字节数（按字节计）。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
j_int_t jdec_config(
            jdec_this_t jdec_this,
            jctl_mode_t jct_mode,
            j_fhandle_t jfh_iptr,
            j_size_t    jst_mlen);

/**********************************************************/
/**
 * @brief 读取 JPEG 图像信息。
 * @note  调用该接口前，先使用 jdec_config() 接口配置输入源。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * @param [out] jinfo_ptr : 操作成功返回的 JPEG 图像信息。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
j_int_t jdec_info(
            jdec_this_t jdec_this,
            jinfo_ptr_t jinfo_ptr);

/**********************************************************/
/**
 * @brief 启动 JPEG 解码操作。
 * @note  调用该接口前，应先使用 jdec_config() 配置好输入源模式。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * @param [in ] jcs_conv  : JPEG 解码输出的像素（色彩空间）格式。
 * 
 * @param [out] jinfo_ptr : 
 * 接收返回的 JPEG 图像基本信息，操作时需要注意以下两点：
 * 1. 无论 jdec_start() 操作是否成功，
 *    只要 (JPEG_CS_UNKNOWN != jinfo_ptr->jcs_type)，
 *    则说明读取到了 JPEG 图像基本信息；
 * 2. 若入参时为 J_NULL，则忽略读取图像基本信息。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
j_int_t jdec_start(
            jdec_this_t jdec_this,
            jctl_cs_t   jcs_conv,
            jinfo_ptr_t jinfo_ptr);

/**********************************************************/
/**
 * @brief 向 JPEG 编码器写入 图像像素 数据，执行编码操作。
 * @note 
 * 执行该操作前，应先调用 jdec_start() 启动解码器；
 * 而完成所有图像像素行解码操作后，调用 jdec_finish() 结束解码工作。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * @param [out] jmt_pxls  : JPEG 解码输出的像素缓存，其色彩空间在 jdec_start() 已设置。
 * @param [in ] jit_step  : 遍历像素行时的 步长值（以 字节 为单位）。
 * @param [in ] jut_rows  : 此次计划读取像素行的数量。
 * 
 * @return j_int_t : 
 * - 返回值 < 0，表示操作失败，产生错误返回的 错误码，请参看 jdec_errno_t 相关枚举值。
 * - 返回值 > 0，表示读取的图像像素行数量。
 * - 返回值 = 0，表示图像像素行已全部读取，不用再继续。
 */
j_int_t jdec_read(
            jdec_this_t jdec_this,
            j_mptr_t    jmt_pxls,
            j_int_t     jit_step,
            j_uint_t    jut_rows);

/**********************************************************/
/**
 * @brief 在 jdec_read() 完成解码读取后，调用该接口关闭 解码器。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
j_int_t jdec_finish(jdec_this_t jdec_this);

/**********************************************************/
/**
 * @brief 对整幅 JPEG 图像进行 解码操作。
 * @note 
 * 调用该接口前，应先使用 jdec_config() 配置好输入源模式。
 * 该接口使用 jdec_start()/jdec_read()/jdec_finish() 实现。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * @param [in ] jcs_conv  : JPEG 解码输出的像素（色彩空间）格式。
 * @param [out] jmt_pxls  : JPEG 解码输出的像素缓存（注意，其大小必须 足够大）。
 * @param [in ] jit_step  : 遍历像素行时的 步长值（以 字节 为单位）。
 * @param [out] jinfo_ptr : 操作返回的 JPEG 图像源基本信息（可传入 J_NULL 忽略返回）。
 * 
 * @return j_int_t : 
 * - 返回值 <  0，表示操作失败，产生错误返回的 错误码，请参看 jdec_errno_t 相关枚举值。
 * - 返回值 >= 0，表示读取的图像像素行数量。
 */
j_int_t jdec_image(
            jdec_this_t jdec_this,
            jctl_cs_t   jcs_conv,
            j_mptr_t    jmt_pxls,
            j_int_t     jit_step,
            jinfo_ptr_t jinfo_ptr);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus

/**
 * @class jdecoder_t
 * @brief 封装 JPEG 解码器相关操作接口 成 C++ 类。
 */
class jdecoder_t
{
    // constructor/destructor
public:
    jdecoder_t(j_void_t * jvt_reserved = J_NULL)
    {
        m_jdec_this = jdec_alloc(jvt_reserved);
    }

    ~jdecoder_t(void)
    {
        jdec_release(m_jdec_this);
        m_jdec_this = J_NULL;
    }

    // public interfaces
public:
    /**********************************************************/
    /**
     * @brief 判断 当前对象 是否有效。
     * @note  详情请参看 jdec_valid() 的说明。
     */
    inline j_bool_t valid(void)
    {
        return jdec_valid(m_jdec_this);
    }

    /**********************************************************/
    /**
     * @brief 执行 JPEG 解码工作前，配置相关工作参数（输入源模式 等）。
     * @note  详情请参看 jdec_config() 的说明。
     */
    inline j_int_t config(
                jctl_mode_t jct_mode,
                j_fhandle_t jfh_iptr,
                j_size_t    jst_mlen)
    {
        return jdec_config(m_jdec_this, jct_mode, jfh_iptr, jst_mlen);
    }

    /**********************************************************/
    /**
     * @brief 读取 JPEG 图像信息。
     * @note  详情请参看 jdec_info() 的说明。
     */
    inline j_int_t info(jinfo_ptr_t jinfo_ptr)
    {
        return jdec_info(m_jdec_this, jinfo_ptr);
    }

    /**********************************************************/
    /**
     * @brief 启动 JPEG 解码操作。
     * @note  详情请参看 jdec_start() 的说明。
     */
    inline j_int_t start(jctl_cs_t jcs_conv, jinfo_ptr_t jinfo_ptr = J_NULL)
    {
        return jdec_start(m_jdec_this, jcs_conv, jinfo_ptr);
    }

    /**********************************************************/
    /**
     * @brief 向 JPEG 编码器写入 图像像素 数据，执行编码操作。
     * @note  详情请参看 jdec_read() 的说明。
     */
    inline j_int_t read(
                j_mptr_t jmt_pxls,
                j_int_t  jit_step,
                j_uint_t jut_rows)
    {
        return jdec_read(m_jdec_this, jmt_pxls, jit_step, jut_rows);
    }

    /**********************************************************/
    /**
     * @brief 在 jdec_read() 完成解码读取后，调用该接口关闭 解码器。
     * @note  详情请参看 jdec_finish() 的说明。
     */
    inline j_int_t finish(void)
    {
        return jdec_finish(m_jdec_this);
    }

    /**********************************************************/
    /**
     * @brief 对整幅 JPEG 图像进行 解码操作。
     * @note  详情请参看 jdec_image() 的说明。
     */
    inline j_int_t decode_image(
                jctl_cs_t   jcs_conv,
                j_mptr_t    jmt_pxls,
                j_int_t     jit_step,
                jinfo_ptr_t jinfo_ptr = J_NULL)
    {
        return jdec_image(
                    m_jdec_this,
                    jcs_conv,
                    jmt_pxls,
                    jit_step,
                    jinfo_ptr);
    }

    // data members
private:
    jdec_this_t m_jdec_this; ///< JPEG 解码操作的上下文对象
};

#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __JDECODER_H__
