/**
 * @file jcomm.inl
 * Copyright (c) 2024 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2024-09-24
 * @version : 1.0.0.0
 * @brief   : 为 JPEG 编码器/解码器 相关实现，提供一些内部封装的公共代码模块。
 */

#ifndef __JCOMM_H__
#error "Please include jcomm.h before this file!"
#endif // __JCOMM_H__

#include "jpeglib.h"
#include <setjmp.h>

////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#define JASSERT(jexp)  assert(jexp)

////////////////////////////////////////////////////////////////////////////////

/**
 * @struct __jerror_manger__
 * @brief  JPEG 库调用过程中，错误回调处理的结构体描述信息。
 */
typedef struct __jerror_manger__
{
    struct jpeg_error_mgr jerr_mgr; ///< "public" fields
    jmp_buf               jerr_jmp; ///< for return to caller
} jerr_mgr_t;

/**********************************************************/
/**
 *  @brief JPEG 库调用过程中，错误回调处理接口。
 */
static j_void_t jerr_exit_callback(j_common_ptr jcbk_ptr)
{
    // return control to the setjmp point
    longjmp(((jerr_mgr_t *)jcbk_ptr->err)->jerr_jmp, 1);
}

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief J_COLOR_SPACE => jpeg_cs_t.
 */
static inline jpeg_cs_t jcs_to_comm(J_COLOR_SPACE jcs_lib)
{
    jpeg_cs_t jcs_comm;

    switch (jcs_lib)
    {
    case JCS_GRAYSCALE: jcs_comm = JPEG_CS_GRAY   ; break;
    case JCS_RGB      : jcs_comm = JPEG_CS_RGB    ; break;
    case JCS_YCbCr    : jcs_comm = JPEG_CS_YCC    ; break;
    case JCS_CMYK     : jcs_comm = JPEG_CS_CMYK   ; break;
    case JCS_YCCK     : jcs_comm = JPEG_CS_YCCK   ; break;
    case JCS_BG_RGB   : jcs_comm = JPEG_CS_BG_RGB ; break;
    case JCS_BG_YCC   : jcs_comm = JPEG_CS_BG_YCC ; break;
    default           : jcs_comm = JPEG_CS_UNKNOWN; break;
    }

    return jcs_comm;
}

/**********************************************************/
/**
 * @brief jpeg_cs_t => J_COLOR_SPACE.
 */
static inline J_COLOR_SPACE jcs_to_lib(jpeg_cs_t jcs_comm)
{
    J_COLOR_SPACE jcs_lib;

    switch (jcs_comm)
    {
    case JPEG_CS_GRAY   : jcs_lib = JCS_GRAYSCALE; break;
    case JPEG_CS_RGB    : jcs_lib = JCS_RGB      ; break;
    case JPEG_CS_YCC    : jcs_lib = JCS_YCbCr    ; break;
    case JPEG_CS_CMYK   : jcs_lib = JCS_CMYK     ; break;
    case JPEG_CS_YCCK   : jcs_lib = JCS_YCCK     ; break;
    case JPEG_CS_BG_RGB : jcs_lib = JCS_BG_RGB   ; break;
    case JPEG_CS_BG_YCC : jcs_lib = JCS_BG_YCC   ; break;
    default             : jcs_lib = JCS_UNKNOWN  ; break;
    }

    return jcs_lib;
}

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 取 绝对值。
 */
static inline j_int_t jval_abs(j_int_t jit_val)
{
    return ((jit_val >= 0) ? jit_val : -jit_val);
}

/**********************************************************/
/**
 * @brief 数值 对齐 操作。
 */
static inline j_uint_t jval_align(j_uint_t jut_val, j_uint_t jut_align)
{
    // assert(0 == (jut_align & (jut_align - 1)));
    return ((jut_val + jut_align - 1) & ~(jut_align - 1));
}

////////////////////////////////////////////////////////////////////////////////
