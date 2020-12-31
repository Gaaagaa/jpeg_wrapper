/**
 * @file XJPEG_wrapper.c
 * Copyright (c) 2020 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2020-12-18
 * @version : 1.0.0.0
 * @brief   : 封装 libjpeg 库，实现简单的 JPEG 数据的 编码/解码 相关操作。
 */

#include "XJPEG_wrapper.h"
#include "jpeglib.h"

#include <setjmp.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////

/**
 * @struct jpeg_error_mgr_t
 * @brief  JPEG 库调用过程中，错误回调处理的结构体描述信息。
 */
typedef struct __jpeg_error_mgr__
{
    struct jpeg_error_mgr   jerr_mgr;    ///< "public" fields
    jmp_buf                 j_jmpbuf;    ///< for return to caller
} jpeg_error_mgr_t;

/**********************************************************/
/**
 *  @brief JPEG 库调用过程中，错误回调处理接口。
 */
static j_void_t jpeg_error_exit_callback(j_common_ptr jcbk_ptr)
{
    /* jcbk_ptr->err really points to a my_error_mgr struct, so coerce pointer */
    jpeg_error_mgr_t * jerr_mgr_ptr = (jpeg_error_mgr_t *)jcbk_ptr->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    //(*jcbk_ptr->err->output_message)(jcbk_ptr);

    /* Return control to the setjmp point */
    longjmp(jerr_mgr_ptr->j_jmpbuf, 1);
}

////////////////////////////////////////////////////////////////////////////////

static inline j_int_t jval_abs(j_int_t jit_val)
{
    return ((jit_val >= 0) ? jit_val : -jit_val);
}

static inline j_uint_t jval_align(j_uint_t jut_val, j_uint_t jut_align)
{
    // assert(0 == (jut_align & (jut_align - 1)));
    return ((jut_val + jut_align - 1) & ~(jut_align - 1));
}

static j_void_t rgb_copy_bgr_line(
                    j_mem_t jct_optr,
                    j_mem_t jct_iptr,
                    j_int_t jit_imgw)
{
    while (jit_imgw-- > 0)
    {
        *jct_optr++ = *(jct_iptr + 2);
        *jct_optr++ = *(jct_iptr + 1);
        *jct_optr++ = *(jct_iptr + 0);

        jct_iptr += 3;
    }
}

static j_void_t rgb_copy_bgra_line(
                    j_mem_t jct_optr,
                    j_mem_t jct_iptr,
                    j_int_t jit_imgw)
{
    while (jit_imgw-- > 0)
    {
        *jct_optr++ = *(jct_iptr + 2);
        *jct_optr++ = *(jct_iptr + 1);
        *jct_optr++ = *(jct_iptr + 0);

        jct_iptr += 4;
    }
}

static j_void_t rgb_copy_rgba_line(
                    j_mem_t jct_optr,
                    j_mem_t jct_iptr,
                    j_int_t jit_imgw)
{
    while (jit_imgw-- > 0)
    {
        *jct_optr++ = *jct_iptr++;
        *jct_optr++ = *jct_iptr++;
        *jct_optr++ = *jct_iptr++;

        jct_iptr += 1;
    }
}

static j_void_t rgb_copy_abgr_line(
                    j_mem_t jct_optr,
                    j_mem_t jct_iptr,
                    j_int_t jit_imgw)
{
    while (jit_imgw-- > 0)
    {
        *jct_optr++ = *(jct_iptr + 3);
        *jct_optr++ = *(jct_iptr + 2);
        *jct_optr++ = *(jct_iptr + 1);

        jct_iptr += 4;
    }
}

static j_void_t rgb_copy_argb_line(
                    j_mem_t jct_optr,
                    j_mem_t jct_iptr,
                    j_int_t jit_imgw)
{
    while (jit_imgw-- > 0)
    {
        jct_iptr += 1;

        *jct_optr++ = *jct_iptr++;
        *jct_optr++ = *jct_iptr++;
        *jct_optr++ = *jct_iptr++;
    }
}

static j_void_t rgb_transto_bgr_line(
                    j_mem_t jct_lptr,
                    j_int_t jit_imgw)
{
    register j_uchar_t jct_swap = 0;

    while (jit_imgw-- > 0)
    {
        jct_swap  = *jct_lptr;
        *jct_lptr = *(jct_lptr + 2);
        *(jct_lptr + 2) = jct_swap;
        jct_lptr += 3;
    }
}

static j_void_t rgb_expandto_rgba_line(
                    j_mem_t   jct_mptr,
                    j_uchar_t jct_alpha,
                    j_int_t   jit_imgw)
{
    register j_mem_t jct_sptr = jct_mptr + 3 * jit_imgw - 1;
    register j_mem_t jct_dptr = jct_mptr + 4 * jit_imgw - 1;

    while (jit_imgw-- > 0)
    {
        *jct_dptr-- = jct_alpha;
        *jct_dptr-- = *jct_sptr--;
        *jct_dptr-- = *jct_sptr--;
        *jct_dptr-- = *jct_sptr--;
    }
}

static j_void_t rgb_expandto_bgra_line(
                    j_mem_t   jct_mptr,
                    j_uchar_t jct_alpha,
                    j_int_t   jit_imgw)
{
    register j_mem_t jct_sptr = jct_mptr + 3 * jit_imgw - 1;
    register j_mem_t jct_dptr = jct_mptr + 4 * jit_imgw - 1;

    while (jit_imgw-- > 0)
    {
        *jct_dptr-- = jct_alpha;
        *jct_dptr-- = *(jct_sptr - 2);
        *jct_dptr-- = *(jct_sptr - 1);
        *jct_dptr-- = *(jct_sptr - 0);
        jct_sptr -= 3;
    }
}

static j_void_t rgb_expandto_argb_line(
                    j_mem_t   jct_mptr,
                    j_uchar_t jct_alpha,
                    j_int_t   jit_imgw)
{
    register j_mem_t jct_sptr = jct_mptr + 3 * jit_imgw - 1;
    register j_mem_t jct_dptr = jct_mptr + 4 * jit_imgw - 1;

    while (jit_imgw-- > 0)
    {
        *jct_dptr-- = *jct_sptr--;
        *jct_dptr-- = *jct_sptr--;
        *jct_dptr-- = *jct_sptr--;
        *jct_dptr-- = jct_alpha;
    }
}

static j_void_t rgb_expandto_abgr_line(
                    j_mem_t   jct_mptr,
                    j_uchar_t jct_alpha,
                    j_int_t   jit_imgw)
{
    register j_mem_t jct_sptr = jct_mptr + 3 * jit_imgw - 1;
    register j_mem_t jct_dptr = jct_mptr + 4 * jit_imgw - 1;

    while (jit_imgw-- > 0)
    {
        *jct_dptr-- = *(jct_sptr - 2);
        *jct_dptr-- = *(jct_sptr - 1);
        *jct_dptr-- = *(jct_sptr - 0);
        *jct_dptr-- = jct_alpha;
        jct_sptr -= 3;
    }
}

////////////////////////////////////////////////////////////////////////////////
// JPEG 编码操作

//====================================================================

// 
// JPEG 编码操作 : 相关常量与数据类型
// 

/** 默认的 JPEG 编码压缩质量值 */
#define JENC_DEF_QUALITY     75

/** 定义 JPEG 编码操作的上下文 结构体类型标识 */
#define JENC_HANDLE_TYPE     0x6A706567

/** 重定义 libjpeg 中的 JPEG 编码器结构体 名称 */
typedef struct jpeg_compress_struct  jpeg_encoder_t;

/**
 * @struct jenc_ctx_t
 * @brief  JPEG 编码操作句柄的描述信息。
 */
typedef struct jenc_ctx_t
{
    j_uint_t          jut_size;  ///< 结构体大小
    j_int_t           jit_type;  ///< 结构体类型标识（固定为 JENC_HANDLE_TYPE）
    jpeg_error_mgr_t  j_err_mgr; ///< JPEG 操作错误管理对象
    jpeg_encoder_t    j_encoder; ///< JPEG 编码器 操作结构体
    j_int_t           jit_encq;  ///< JPEG 编码的压缩质量（1 - 100）

    /**
     * @brief 
     * JPEG 编码输出源使用 文件流模式 时，是否自动重置 文件指针。
     * 默认情况下，其值为 J_TRUE，操作文件流后，会自动还原原文件指针位置。
     */
    j_bool_t jbt_rfio;

    /**
     * @brief 
     * 编码输出源信息：
     * - 内存模式  ，jht_optr 的类型为 j_mem_t ，其为输出 JPEG 数据的缓存地址；
     * - 文件流模式，jht_optr 的类型为 j_fio_t ，其为输出 JPEG 数据的文件流；
     * - 文件模式  ，jht_optr 的类型为 j_path_t，其为输出 JPEG 数据的文件路径。
     */
    struct
    {
        j_int_t       jit_mode;  ///< 输出模式
        j_handle_t    jht_optr;  ///< 指向输出目标的数据信息
        j_uint_t      jut_mlen;  ///< 只针对于 内存模式，表示缓存容量
    } jdst;

    /**
     * @brief 编码操作使用到的行缓存。
     */
    struct
    {
        j_mem_t       jmem_ptr;  ///< 缓存地址
        j_uint_t      jut_mlen;  ///< 缓存容量
    } jline;

    /**
     * @brief 
     * 保存 编码器 内部自动分配的缓存信息，
     * 即 最后编码生成的 JPEG 数据流。
     */
    struct
    {
        j_mem_t       jmem_ptr;  ///< 缓存地址
        j_uint_t      jut_size;  ///< 有效字节数
        j_uint_t      jut_mlen;  ///< 缓存容量
    } jcached;
} jenc_ctx_t;

/**
 * @struct jpeg_dst_t
 * @brief  JPEG 编码操作的数据输出源。
 * @note
 * 当 ((0 == jst_msize) && (J_NULL != jfio_ptr)) 时，使用 jfio_ptr，即 文件流模式；
 * 当 ((0 != jst_msize) && (J_NULL != jmem_ptr)) 时，使用 jmem_ptr，即 内存模式，
 *    此时 jst_msize 表示 jmem_ptr 的缓存容量；
 * 当 ((0 == jst_msize) && (J_NULL == jmem_ptr)) 时，使用 jmem_ptr，即 内存模式，
 *    此时 JPEG 数据输出缓存 会由 编码器内部 自动分配内存。
 */
typedef struct __jpeg_enc_output__
{
    union
    {
        j_mem_t jmem_dptr;  ///< JPEG 数据输出缓存
        j_fio_t jfio_dptr;  ///< JPEG 数据输出文件
    } jdst;  ///< 输出源

    /**
     * @brief jst_msize 主要用于表示 jmem_ptr 的缓存容量。
     */
    j_size_t  jst_msize;

#define Jmem_dptr  jdst.jmem_dptr
#define Jfio_dptr  jdst.jfio_dptr

} jpeg_dst_t, * jpeg_dstptr_t;

//====================================================================

// 
// JPEG 编码操作 : inner invoking
// 

/**********************************************************/
/**
 * @brief 获取（加载） JPEG 编码的 目标输出源 信息。
 * @note 在使用 文件模式 时，完成编码输出后，需要再关闭 输出源 的文件流对象。
 * 
 * @param [in ] jenc_cptr : JPEG 编码操作的上下文对象。
 * @param [out] jpeg_dptr : 操作成功返回的 JPEG 数据输出源 信息。
 * 
 * @return j_int_t 错误码，请参看 jenc_errno_table_t 相关枚举值。
 */
static j_int_t jenc_query_dst(jenc_ctxptr_t jenc_cptr, jpeg_dstptr_t jpeg_dptr)
{
    j_int_t jit_err = JENC_ERR_INVALID_DST;

    switch (jenc_cptr->jdst.jit_mode)
    {
    case JCTRL_MODE_MEM:
        if ((J_NULL == jenc_cptr->jdst.jht_optr) ||
            (0      == jenc_cptr->jdst.jut_mlen))
        {
            jpeg_dptr->Jmem_dptr = jenc_cptr->jcached.jmem_ptr;
            jpeg_dptr->jst_msize = jenc_cptr->jcached.jut_size;

            jit_err = JENC_ERR_OK;
        }
        else
        {
            jpeg_dptr->Jmem_dptr = (j_mem_t )jenc_cptr->jdst.jht_optr;
            jpeg_dptr->jst_msize = (j_size_t)jenc_cptr->jdst.jut_mlen;

            jit_err = JENC_ERR_OK;
        }
        break;

    case JCTRL_MODE_FIO:
        if (J_NULL != jenc_cptr->jdst.jht_optr)
        {
            jpeg_dptr->Jfio_dptr = (j_fio_t)jenc_cptr->jdst.jht_optr;
            jpeg_dptr->jst_msize = 0;

            jit_err = JENC_ERR_OK;
        }
        break;

    case JCTRL_MODE_FILE:
        if ((J_NULL != jenc_cptr->jdst.jht_optr) &&
            ('\0' != ((j_path_t)jenc_cptr->jdst.jht_optr)[0]))
        {
            jpeg_dptr->Jfio_dptr =
                fopen((j_path_t)jenc_cptr->jdst.jht_optr, "wb+");
            jpeg_dptr->jst_msize = 0;

            jit_err =
                (J_NULL != jpeg_dptr->Jfio_dptr) ?
                    JENC_ERR_OK : JENC_ERR_OPEN_FILE;
        }
        break;

    default:
        break;
    }

    return jit_err;
}

/**********************************************************/
/**
 * @brief 将 RGB 图像像素数据编码为 JPEG 数据流。
 * 
 * @param [in ] jenc_cptr   : JPEG 编码操作的上下文对象。
 * @param [out] jpeg_dptr   : JPEG 数据输出源。
 * @param [in ] jmem_iptr   : RGB 图像数据 缓存。
 * @param [in ] jit_stride  : RGB 图像数据 像素行 步进大小。
 * @param [in ] jit_width   : RGB 图像数据 宽度。
 * @param [in ] jit_height  : RGB 图像数据 高度。
 * @param [in ] jit_ctrlcs  : 图像色彩空间的转换方式（参看 jenc_ctrlcs_t）。
 * 
 * @return j_int_t : 
 * - 操作失败时，返回值 < 0，表示 错误码，
 *   请参看 jenc_errno_table_t 相关枚举值。
 * - 操作成功时：
 *   1. 使用 文件流模式 时，返回值 == JENC_ERR_OK；
 *   2. 使用 内存模式   时，返回值 > 0，表示输出源中存储的 JPEG 数据的有效字节数；
 *   3. 使用 内存模式   时，且 返回值 == JENC_ERR_OK，表示输出源的缓存容量不足，
 *      而输出的 JPEG 编码数据存储在 上下文对象 jenc_cptr 的缓存中，后续可通过
 *      jenc_cached_data() 和 jenc_cached_size() 获取此次编码得到的 JPEG 数据。
 */
static j_int_t jenc_from_rgb(
                jenc_ctxptr_t jenc_cptr,
                jpeg_dstptr_t jpeg_dptr,
                j_mem_t       jmem_iptr,
                j_int_t       jit_stride,
                j_int_t       jit_width,
                j_int_t       jit_height,
                j_int_t       jit_ctrlcs)
{
    j_int_t jit_err = JENC_ERR_UNKNOW;
    jpeg_encoder_t * jcs_ptr = &jenc_cptr->j_encoder;

    j_mem_t  jct_mptr = jpeg_dptr->Jmem_dptr;
    j_size_t jst_size = jpeg_dptr->jst_msize;

    /* 用于 RGB 数据的 像素行 遍历操作 */
    j_mem_t  jct_lptr = jmem_iptr;

    /* 判断 输出源 是否为 文件流模式 */
    j_bool_t jbt_ufio =
        ((0 == jpeg_dptr->jst_msize) && (J_NULL != jpeg_dptr->Jfio_dptr));

    do 
    {
        //======================================
        // 输入参数已经被检查过
#if 0
        if ((J_NULL == jmem_iptr) || (0 == jit_stride) ||
            (jit_width <= 0) || (jit_height <= 0))
        {
            jit_err = JENC_ERR_INVALID_PARAM;
            break;
        }

        switch (JENC_CTRLCS_CTRL(jit_ctrlcs))
        {
        case JCTRL_CS_RGB :
        case JCTRL_CS_BGR :
        case JCTRL_CS_RGBA:
        case JCTRL_CS_BGRA:
        case JCTRL_CS_ARGB:
        case JCTRL_CS_ABGR:
            break;
        default: jit_ctrlcs = JENC_CTRLCS_UNKNOW;
            break;
        }

        if (JENC_CTRLCS_UNKNOW == jit_ctrlcs)
        {
            jit_err = JENC_ERR_INVALID_PARAM;
            break;
        }
#endif
        //======================================
        // 设置错误回调跳转代码

        if (0 != setjmp(jenc_cptr->j_err_mgr.j_jmpbuf))
        {
            jit_err = JENC_ERR_EXCEPTION;
            goto __EXIT_FUNC__;
        }

        //======================================

        // 设置交换缓存
        if (jenc_cptr->jline.jut_mlen < (j_uint_t)jval_abs(3 * jit_width))
        {
            if (J_NULL != jenc_cptr->jline.jmem_ptr)
                free(jenc_cptr->jline.jmem_ptr);

            jenc_cptr->jline.jut_mlen = (j_uint_t)jval_abs(3 * jit_width);
            jenc_cptr->jline.jmem_ptr =
                (j_mem_t)calloc(jenc_cptr->jline.jut_mlen, sizeof(j_uchar_t));
        }

        if (J_NULL == jenc_cptr->jline.jmem_ptr)
        {
            jit_err = JENC_ERR_ALLOC_MEM;
            break;
        }

        //======================================

        if (jbt_ufio)
            jpeg_stdio_dest(jcs_ptr, jpeg_dptr->Jfio_dptr);
        else
            jpeg_mem_dest(jcs_ptr, &jct_mptr, &jst_size);

        jcs_ptr->image_width      = jit_width;
        jcs_ptr->image_height     = jit_height;
        jcs_ptr->input_components = 3;
        jcs_ptr->in_color_space   = JCS_RGB;

        jpeg_set_defaults(jcs_ptr);
        jpeg_set_quality(jcs_ptr, jenc_cptr->jit_encq, 1);

        switch (JENC_CTRLCS_JPEG(jit_ctrlcs))
        {
        case JPEG_CS_GRAYSCALE : jpeg_set_colorspace(jcs_ptr, JCS_GRAYSCALE); break;
        case JPEG_CS_RGB       : jpeg_set_colorspace(jcs_ptr, JCS_RGB      ); break;
        case JPEG_CS_YCbCr     : jpeg_set_colorspace(jcs_ptr, JCS_YCbCr    ); break;
        case JPEG_CS_BG_YCC    : jpeg_set_colorspace(jcs_ptr, JCS_BG_YCC   ); break;

        default:
            break;
        }

        //======================================

        jpeg_start_compress(jcs_ptr, 1);

        switch (JENC_CTRLCS_CTRL(jit_ctrlcs))
        {
        case JCTRL_CS_RGB:
            while (jcs_ptr->next_scanline < jcs_ptr->image_height)
            {
                jpeg_write_scanlines(jcs_ptr, &jct_lptr, 1);
                jct_lptr += jit_stride;
            }
            break;

        case JCTRL_CS_BGR:
            while (jcs_ptr->next_scanline < jcs_ptr->image_height)
            {
                rgb_copy_bgr_line(
                    jenc_cptr->jline.jmem_ptr, jct_lptr, jit_width);
                jpeg_write_scanlines(jcs_ptr, &jenc_cptr->jline.jmem_ptr, 1);
                jct_lptr += jit_stride;
            }
            break;

        case JCTRL_CS_RGBA:
            while (jcs_ptr->next_scanline < jcs_ptr->image_height)
            {
                rgb_copy_rgba_line(
                    jenc_cptr->jline.jmem_ptr, jct_lptr, jit_width);
                jpeg_write_scanlines(jcs_ptr, &jenc_cptr->jline.jmem_ptr, 1);
                jct_lptr += jit_stride;
            }
            break;

        case JCTRL_CS_BGRA:
            while (jcs_ptr->next_scanline < jcs_ptr->image_height)
            {
                rgb_copy_bgra_line(
                    jenc_cptr->jline.jmem_ptr, jct_lptr, jit_width);
                jpeg_write_scanlines(jcs_ptr, &jenc_cptr->jline.jmem_ptr, 1);
                jct_lptr += jit_stride;
            }
            break;

        case JCTRL_CS_ARGB:
            while (jcs_ptr->next_scanline < jcs_ptr->image_height)
            {
                rgb_copy_argb_line(
                    jenc_cptr->jline.jmem_ptr, jct_lptr, jit_width);
                jpeg_write_scanlines(jcs_ptr, &jenc_cptr->jline.jmem_ptr, 1);
                jct_lptr += jit_stride;
            }
            break;

        case JCTRL_CS_ABGR:
            while (jcs_ptr->next_scanline < jcs_ptr->image_height)
            {
                rgb_copy_abgr_line(
                    jenc_cptr->jline.jmem_ptr, jct_lptr, jit_width);
                jpeg_write_scanlines(jcs_ptr, &jenc_cptr->jline.jmem_ptr, 1);
                jct_lptr += jit_stride;
            }
            break;

        default:
            break;
        }

        jpeg_finish_compress(jcs_ptr);

        //======================================
        // 输出源 为 文件流 模式

        // 直接返回 JENC_ERR_OK
        if (jbt_ufio)
        {
            jit_err = JENC_ERR_OK;
            break;
        }

        //======================================
        // 输出源 为 内存 模式

        if (jct_mptr != jpeg_dptr->Jmem_dptr)
        {
            // 若 解码器 内部重新分配了数据输出缓存，
            // 则将这些缓存信息保存到上下文中（方便后续操作）

            if (J_NULL != jenc_cptr->jcached.jmem_ptr)
                free(jenc_cptr->jcached.jmem_ptr);

            jenc_cptr->jcached.jmem_ptr = jct_mptr;
            jenc_cptr->jcached.jut_size = (j_uint_t)jst_size;
            jenc_cptr->jcached.jut_mlen =
                (j_uint_t)(jst_size + jcs_ptr->dest->free_in_buffer);

            // 返回 JENC_ERR_OK，后续可通过
            // jenc_cached_data() 和 jenc_cached_size()
            // 获取此次编码得到的 JPEG 数据流
            jit_err = JENC_ERR_OK;
            break;
        }
        else if (jenc_cptr->jcached.jmem_ptr == jpeg_dptr->Jmem_dptr)
        {
            // 若 输出源 使用的缓存为 上下文内部的私有缓存，
            // 则需同步更新 上下文内部缓存 的有效字节数

            jenc_cptr->jcached.jut_size = (j_uint_t)jst_size;
        }
        else
        {
            // 解码器由始至终都将 JPEG 数据输出到 输出源 的缓存中，
            // 则将 上下文内部缓存 的 有效字节数重置

            jenc_cptr->jcached.jut_size = 0;
        }

        //======================================
        jit_err = (j_int_t)jst_size;
    } while (0);

__EXIT_FUNC__:
    if (J_NULL != jcs_ptr)
    {
        jpeg_abort_compress(jcs_ptr);
        jcs_ptr = J_NULL;
    }

    return jit_err;
}

//====================================================================

// 
// JPEG 编码操作 : public interfaces
// 

/**********************************************************/
/**
 * @brief 申请 JPEG 编码操作的上下文对象。
 * 
 * @param [in ] jvt_reserved : 保留参数（当前使用 J_NULL 即可）。
 * 
 * @return jenc_ctxptr_t :
 * 返回 JPEG 编码操作的上下文对象，为 J_NULL 时表示申请失败。
 */
jenc_ctxptr_t jenc_alloc(j_void_t * jvt_reserved)
{
    jenc_ctxptr_t jenc_cptr = (jenc_ctxptr_t)calloc(1, sizeof(jenc_ctx_t));
    if (J_NULL == jenc_cptr)
    {
        return J_NULL;
    }

    //======================================

    jenc_cptr->jut_size = sizeof(jenc_ctx_t);
    jenc_cptr->jit_type = JENC_HANDLE_TYPE;
    jenc_cptr->jit_encq = JENC_DEF_QUALITY;
    jenc_cptr->jbt_rfio = J_TRUE;

    jenc_cptr->jdst.jit_mode = JCTRL_MODE_UNKNOW;
    jenc_cptr->jdst.jht_optr = J_NULL;
    jenc_cptr->jdst.jut_mlen = 0;

    jenc_cptr->jline.jmem_ptr = J_NULL;
    jenc_cptr->jline.jut_mlen = 0;

    jenc_cptr->jcached.jmem_ptr = J_NULL;
    jenc_cptr->jcached.jut_size = 0;
    jenc_cptr->jcached.jut_mlen = 0;

    // 初始化 JPEG 编码器参数
    jenc_cptr->j_encoder.err =
        jpeg_std_error(&jenc_cptr->j_err_mgr.jerr_mgr);
    jenc_cptr->j_err_mgr.jerr_mgr.error_exit = jpeg_error_exit_callback;

    jpeg_create_compress(&jenc_cptr->j_encoder);

    //======================================

    return jenc_cptr;
}

/**********************************************************/
/**
 * @brief 释放 JPEG 编码操作的上下文对象。
 */
j_void_t jenc_release(jenc_ctxptr_t jenc_cptr)
{
    if (jenc_valid(jenc_cptr))
    {
        jpeg_destroy_compress(&jenc_cptr->j_encoder);

        if (J_NULL != jenc_cptr->jline.jmem_ptr)
        {
            free(jenc_cptr->jline.jmem_ptr);

            jenc_cptr->jline.jmem_ptr = J_NULL;
            jenc_cptr->jline.jut_mlen = 0;
        }

        if (J_NULL != jenc_cptr->jcached.jmem_ptr)
        {
            free(jenc_cptr->jcached.jmem_ptr);

            jenc_cptr->jcached.jmem_ptr = J_NULL;
            jenc_cptr->jcached.jut_size = 0;
            jenc_cptr->jcached.jut_mlen = 0;
        }

        free(jenc_cptr);
    }
}

/**********************************************************/
/**
 * @brief JPEG 编码操作句柄的有效判断。
 */
j_bool_t jenc_valid(jenc_ctxptr_t jenc_cptr)
{
    if (J_NULL == jenc_cptr)
    {
        return J_FALSE;
    }

    if (sizeof(jenc_ctx_t) != jenc_cptr->jut_size)
    {
        return J_FALSE;
    }

    if (JENC_HANDLE_TYPE != jenc_cptr->jit_type)
    {
        return J_FALSE;
    }

    return J_TRUE;
}

/**********************************************************/
/**
 * @brief 获取 JPEG 数据压缩质量。
 */
j_int_t jenc_get_quality(jenc_ctxptr_t jenc_cptr)
{
    return jenc_cptr->jit_encq;
}

/**********************************************************/
/**
 * @brief 设置 JPEG 数据压缩质量（1 - 100，为 0 时，取默认值）。
 */
j_void_t jenc_set_quality(jenc_ctxptr_t jenc_cptr, j_int_t jit_quality)
{
    if (0 == jit_quality)
        jenc_cptr->jit_encq = JENC_DEF_QUALITY;
    else if (jit_quality < 0)
        jenc_cptr->jit_encq = 1;
    else if (jit_quality > 100)
        jenc_cptr->jit_encq = 100;
    else
        jenc_cptr->jit_encq = jit_quality;
}

/**********************************************************/
/**
 * @brief 
 * JPEG 编码输出源使用 文件流模式 时，是否自动重置 文件指针。
 * 默认情况下，其值为 J_TRUE，操作文件流后，会自动还原原文件指针位置。
 */
j_bool_t jenc_is_rfio(jenc_ctxptr_t jenc_cptr)
{
    return jenc_cptr->jbt_rfio;
}

/**********************************************************/
/**
 * @brief 设置 文件流模式 时，是否自动重置 文件指针。
 */
j_void_t jenc_set_rfio(jenc_ctxptr_t jenc_cptr, j_bool_t jbt_rfio)
{
    jenc_cptr->jbt_rfio = jbt_rfio;
}

/**********************************************************/
/**
 * @brief 获取 JPEG 编码操作的上下文对象中，所缓存的 JPEG 数据流地址。
 */
j_mem_t jenc_cached_data(jenc_ctxptr_t jenc_cptr)
{
    return jenc_cptr->jcached.jmem_ptr;
}

/**********************************************************/
/**
 * @brief 获取 JPEG 编码操作的上下文对象中，所缓存的 JPEG 数据流有效字节数。
 */
j_uint_t jenc_cached_size(jenc_ctxptr_t jenc_cptr)
{
    return jenc_cptr->jcached.jut_size;
}

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
                j_uint_t      jut_mlen)
{
    j_int_t jit_err = JENC_ERR_INVALID_PARAM;

    switch (jit_mode)
    {
    case JCTRL_MODE_MEM:
        if ((J_NULL == jht_optr) || (0 == jut_mlen))
        {
            jenc_cptr->jdst.jit_mode = JCTRL_MODE_MEM;
            jenc_cptr->jdst.jht_optr = J_NULL;
            jenc_cptr->jdst.jut_mlen = 0;

            jit_err = JENC_ERR_OK;
        }
        else
        {
            jenc_cptr->jdst.jht_optr = jht_optr;
            jenc_cptr->jdst.jut_mlen = jut_mlen;

            jit_err = JENC_ERR_OK;
        }
        break;

    case JCTRL_MODE_FIO:
        if (J_NULL != jht_optr)
        {
            jenc_cptr->jdst.jit_mode = JCTRL_MODE_FIO;
            jenc_cptr->jdst.jht_optr = jht_optr;
            jenc_cptr->jdst.jut_mlen = 0;

            jit_err = JENC_ERR_OK;
        }
        break;

    case JCTRL_MODE_FILE:
        if ((J_NULL != jht_optr) && ('\0' != ((j_path_t)jht_optr)[0]))
        {
            jenc_cptr->jdst.jit_mode = JCTRL_MODE_FILE;
            jenc_cptr->jdst.jht_optr = jht_optr;
            jenc_cptr->jdst.jut_mlen = 0;

            jit_err = JENC_ERR_OK;
        }
        break;

    default:
        break;
    }

    return jit_err;
}

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
                j_int_t       jit_ctrlcs)
{
    j_int_t jit_err = JENC_ERR_UNKNOW;

    jpeg_dst_t jenc_dst;
    j_fpos_t   jpos_fio;

    do
    {
        //======================================
        // 检查输入参数的有效性

        if ((J_NULL == jmem_iptr) || (0 == jit_stride) ||
            (jit_width <= 0) || (jit_height <= 0))
        {
            jit_err = JENC_ERR_INVALID_PARAM;
            break;
        }

        switch (JENC_CTRLCS_CTRL(jit_ctrlcs))
        {
        case JCTRL_CS_RGB :
        case JCTRL_CS_BGR :
        case JCTRL_CS_RGBA:
        case JCTRL_CS_BGRA:
        case JCTRL_CS_ARGB:
        case JCTRL_CS_ABGR:
            break;
        default: jit_ctrlcs = JENC_CTRLCS_UNKNOW;
            break;
        }

        if (JENC_CTRLCS_UNKNOW == jit_ctrlcs)
        {
            jit_err = JENC_ERR_INVALID_PARAM;
            break;
        }

        //======================================
        // 依据配置的目标输出源模式，初始化 jenc_dst

        jit_err = jenc_query_dst(jenc_cptr, &jenc_dst);
        if (JENC_ERR_OK != jit_err)
        {
            break;
        }

        if ((JCTRL_MODE_FIO == jenc_cptr->jdst.jit_mode) &&
            jenc_cptr->jbt_rfio)
        {
            if (0 != fgetpos(jenc_dst.Jfio_dptr, &jpos_fio))
            {
                jit_err = JENC_ERR_FIO_FGETPOS;
                break;
            }
        }

        //======================================
        // 执行编码操作

        jit_err = 
            jenc_from_rgb(
                jenc_cptr,
                &jenc_dst,
                jmem_iptr,
                jit_stride,
                jit_width,
                jit_height,
                jit_ctrlcs);

        //======================================

        if (JCTRL_MODE_FILE == jenc_cptr->jdst.jit_mode)
        {
            fclose((j_fio_t)jenc_dst.Jfio_dptr);
        }
        else if ((JCTRL_MODE_FIO == jenc_cptr->jdst.jit_mode) &&
                 jenc_cptr->jbt_rfio)
        {
            fsetpos(jenc_dst.Jfio_dptr, &jpos_fio);
        }

        jenc_dst.Jfio_dptr = J_NULL;
        jenc_dst.jst_msize = 0;

        //======================================
    } while (0);

    return jit_err;
}

////////////////////////////////////////////////////////////////////////////////
// JPEG 解码操作

//====================================================================

// 
// JPEG 解码操作 : 相关常量与数据类型
// 

/** 定义 JPEG 解码操作的上下文 结构体类型标识 */
#define JDEC_HANDLE_TYPE     0x4A504547

/** 重定义 libjpeg 中的 JPEG 解码器结构体 名称 */
typedef struct jpeg_decompress_struct  jpeg_decoder_t;

/**
 * @struct jdec_ctx_t
 * @brief  JPEG 解码操作的上下文。
 */
typedef struct jdec_ctx_t
{
    j_uint_t         jut_size;   ///< 结构体大小
    j_int_t          jit_type;   ///< 结构体类型标识（固定为 JDEC_HANDLE_TYPE）
    jpeg_error_mgr_t j_err_mgr;  ///< JPEG 操作错误管理对象
    jpeg_decoder_t   j_decoder;  ///< JPEG 解码器 操作结构体

    /**
     * @brief 
     * JPEG 解码输出数据（RGB数据）时，是否按 4 字节对齐。
     * 默认情况下，是按 4 字节对齐。
     */
    j_bool_t jbt_align;

    /**
     * @brief 
     * 解码过程中 填充空白通道 的数值（例如输出 RGB32 的 ALPHA 通道）。
     * 默认情况下，其值为 0xFFFFFFFF。
     */
    j_uint_t jut_vpad;

    /**
     * @brief 
     * JPEG 解码输入源使用 文件流模式 时，是否自动重置 文件指针。
     * 默认情况下，其值为 J_TRUE，操作文件流后，会自动还原原文件指针位置。
     */
    j_bool_t jbt_rfio;

    /**
     * @brief 
     * 解码输入源信息：
     * - 内存模式  ，jht_iptr 的类型为 j_mem_t ，其为输入 JPEG 数据的缓存地址；
     * - 文件流模式，jht_iptr 的类型为 j_fio_t ，其为输入 JPEG 数据的文件流；
     * - 文件模式  ，jht_iptr 的类型为 j_path_t，其为输入 JPEG 数据的文件路径。
     */
    struct
    {
        j_int_t      jit_mode;   ///< 输入模式
        j_handle_t   jht_iptr;   ///< 指向输入源的数据信息
        j_uint_t     jut_size;   ///< 只针对于 内存模式，表示缓存中的有效字节数
    } jsrc;
} jdec_ctx_t;

/**
 * @struct jpeg_src_t
 * @brief  JPEG 解码操作的数据输入源。
 * @note
 * 在 jdec_to_rgb() 的操作过程中：
 * 当 ((0 == jst_msize) && (J_NULL != jfio_ptr)) 时，使用 jfio_ptr，即 文件流模式；
 * 当 ((0 != jst_msize) && (J_NULL != jmem_ptr)) 时，使用 jmem_ptr，即 内存模式，
 *    此时 jst_msize 表示 jmem_ptr 缓存的有效字节数 ；
 * 当 (J_NULL == jmem_ptr) 或 (J_NULL == jfio_ptr) 时，输入源无效 。
 */
typedef struct __jpeg_dec_input__
{
    union
    {
        j_mem_t jmem_sptr;  ///< JPEG 数据输入缓存
        j_fio_t jfio_sptr;  ///< JPEG 数据输入文件
    } jsrc;  ///< 输入源

    /**
     * @brief jst_msize 主要用于表示 jmem_ptr 缓存的有效字节数。
     */
    j_size_t jst_msize;

#define Jmem_sptr  jsrc.jmem_sptr
#define Jfio_sptr  jsrc.jfio_sptr

} jpeg_src_t, * jpeg_srcptr_t;

//====================================================================

// 
// JPEG 解码操作 : inner invoking
// 

/**********************************************************/
/**
 * @brief 获取（加载） JPEG 解码的 输入源 信息。
 * @note 在使用 文件模式 时，完成解码输出后，需要再关闭 输入源 的文件流对象。
 * 
 * @param [in ] jdec_cptr : JPEG 解码操作的上下文对象。
 * @param [out] jpeg_sptr : 操作成功返回的 JPEG 数据输入源 信息。
 * 
 * @return j_int_t 错误码，请参看 jdec_errno_table_t 相关枚举值。
 */
static j_int_t jdec_query_src(jdec_ctxptr_t jdec_cptr, jpeg_srcptr_t jpeg_sptr)
{
    j_int_t jit_err = JDEC_ERR_INVALID_SRC;

    switch (jdec_cptr->jsrc.jit_mode)
    {
    case JCTRL_MODE_MEM:
        if ((J_NULL != jdec_cptr->jsrc.jht_iptr) &&
            (jdec_cptr->jsrc.jut_size > 0))
        {
            jpeg_sptr->Jmem_sptr = (j_mem_t )jdec_cptr->jsrc.jht_iptr;
            jpeg_sptr->jst_msize = (j_size_t)jdec_cptr->jsrc.jut_size;

            jit_err = JDEC_ERR_OK;
        }
        break;

    case JCTRL_MODE_FIO:
        if (J_NULL != jdec_cptr->jsrc.jht_iptr)
        {
            jpeg_sptr->Jfio_sptr = (j_fio_t)jdec_cptr->jsrc.jht_iptr;
            jpeg_sptr->jst_msize = 0;

            jit_err = JDEC_ERR_OK;
        }
        break;

    case JCTRL_MODE_FILE:
        if ((J_NULL != jdec_cptr->jsrc.jht_iptr) &&
            ('\0' != ((j_path_t)jdec_cptr->jsrc.jht_iptr)[0]))
        {
            jpeg_sptr->Jfio_sptr =
                fopen((j_path_t)jdec_cptr->jsrc.jht_iptr, "rb");
            jpeg_sptr->jst_msize = 0;

            jit_err =
                (J_NULL != jpeg_sptr->Jfio_sptr) ?
                    JDEC_ERR_OK : JDEC_ERR_OPEN_FILE;
        }
        break;

    default:
        break;
    }

    return jit_err;
}

/**********************************************************/
/**
 * @brief 获取 JPEG 数据源中的图像基本信息。
 * 
 * @param [in ] jdec_cptr : JPEG 解码操作的上下文对象。
 * @param [in ] jpeg_sptr : JPEG 数据输入源。
 * @param [out] jinfo_ptr : 操作成功返回的图像基本信息。
 * 
 * @return j_int_t : 错误码（参看 jdec_errno_table_t 相关枚举值）。
 */
static j_int_t jdec_info(
                jdec_ctxptr_t jdec_cptr,
                jpeg_srcptr_t jpeg_sptr,
                jinfo_ptr_t   jinfo_ptr)
{
    j_int_t jit_err = JDEC_ERR_UNKNOW;
    jpeg_decoder_t * jds_ptr = &jdec_cptr->j_decoder;

    do 
    {
        //======================================
        // 输入参数已经被检查过
#if 0
        if ((J_NULL == jpeg_sptr->Jmem_sptr) || (J_NULL == jinfo_ptr))
        {
            jit_err = JDEC_ERR_INVALID_PARAM;
            break;
        }
#endif
        //======================================

        // 设置错误回调跳转代码
        if (0 != setjmp(jdec_cptr->j_err_mgr.j_jmpbuf))
        {
            jit_err = JDEC_ERR_EXCEPTION;
            goto __EXIT_FUNC__;
        }

        //======================================

        // 开启 JPEG 解码
        if (jpeg_sptr->jst_msize > 0)
            jpeg_mem_src(jds_ptr, jpeg_sptr->Jmem_sptr, jpeg_sptr->jst_msize);
        else
            jpeg_stdio_src(jds_ptr, jpeg_sptr->Jfio_sptr);

        if (JPEG_HEADER_OK != jpeg_read_header(jds_ptr, 1))
        {
            jit_err = JDEC_ERR_READ_HEADER;
            break;
        }

        // 设置返回的 JPEG 图像信息
        if (J_NULL != jinfo_ptr)
        {
            jinfo_ptr->jit_width    = (j_int_t)jds_ptr->image_width;
            jinfo_ptr->jit_height   = (j_int_t)jds_ptr->image_height;
            jinfo_ptr->jit_channels = (j_int_t)jds_ptr->num_components;

            switch (jds_ptr->jpeg_color_space)
            {
            case JCS_UNKNOWN   : jinfo_ptr->jit_cstype = JPEG_CS_UNKNOWN  ; break;
            case JCS_GRAYSCALE : jinfo_ptr->jit_cstype = JPEG_CS_GRAYSCALE; break;
            case JCS_RGB       : jinfo_ptr->jit_cstype = JPEG_CS_RGB      ; break;
            case JCS_YCbCr     : jinfo_ptr->jit_cstype = JPEG_CS_YCbCr    ; break;
            case JCS_CMYK      : jinfo_ptr->jit_cstype = JPEG_CS_CMYK     ; break;
            case JCS_YCCK      : jinfo_ptr->jit_cstype = JPEG_CS_YCCK     ; break;
            case JCS_BG_RGB    : jinfo_ptr->jit_cstype = JPEG_CS_BG_RGB   ; break;
            case JCS_BG_YCC    : jinfo_ptr->jit_cstype = JPEG_CS_BG_YCC   ; break;
            default            : jinfo_ptr->jit_cstype = JPEG_CS_UNKNOWN  ; break;
            }
        }

        //======================================
        jit_err = JDEC_ERR_OK;
    } while (0);

__EXIT_FUNC__:
    if (J_NULL != jds_ptr)
    {
        jpeg_abort_decompress(jds_ptr);
        jds_ptr = J_NULL;
    }

    return jit_err;
}

/**********************************************************/
/**
 * @brief 将 JPEG 数据源图像 解码成 RGB 图像像素数据。
 * 
 * @param [in ] jdec_cptr : JPEG 解码操作的上下文对象。
 * @param [in ] jpeg_sptr : 待解码的 JPEG 数据输入源。
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
static j_int_t jdec_to_rgb(
                jdec_ctxptr_t jdec_cptr,
                jpeg_srcptr_t jpeg_sptr,
                j_mem_t       jmem_optr,
                j_int_t       jit_stride,
                j_uint_t      jut_mlen,
                j_int_t     * jit_width,
                j_int_t     * jit_height,
                j_int_t       jit_ctrlcs)
{
    j_int_t   jit_err   = JDEC_ERR_UNKNOW;
    j_mem_t   jct_mptr  = jmem_optr;
    j_int_t   jit_wlen  = 0;
    j_uchar_t jct_alpha = (j_uchar_t)jdec_cptr->jut_vpad;

    jpeg_decoder_t * jds_ptr = &jdec_cptr->j_decoder;

    do 
    {
        //======================================
        // 输入参数已经被检查过
#if 0
        if ((J_NULL == jpeg_sptr->Jmem_sptr) ||
            (J_NULL == jmem_optr) || (jut_mlen <= 0))
        {
            jit_err = JDEC_ERR_INVALID_PARAM;
            break;
        }

        switch (jit_ctrlcs)
        {
        case JCTRL_CS_RGB :
        case JCTRL_CS_BGR :
        case JCTRL_CS_RGBA:
        case JCTRL_CS_BGRA:
        case JCTRL_CS_ARGB:
        case JCTRL_CS_ABGR:
            break;
        default: jit_ctrlcs = JCTRL_CS_UNKNOW;
            break;
        }

        if (JCTRL_CS_UNKNOW == jit_ctrlcs)
        {
            jit_err = JDEC_ERR_INVALID_PARAM;
            break;
        }
#endif
        //======================================

        // 设置错误回调跳转代码
        if (0 != setjmp(jdec_cptr->j_err_mgr.j_jmpbuf))
        {
            jit_err = JDEC_ERR_EXCEPTION;
            goto __EXIT_FUNC__;
        }

        //======================================
        // 读取 JPEG 图像信息，并检查（或设置）工作参数

        if (jpeg_sptr->jst_msize > 0)
            jpeg_mem_src(jds_ptr, jpeg_sptr->Jmem_sptr, jpeg_sptr->jst_msize);
        else
            jpeg_stdio_src(jds_ptr, jpeg_sptr->Jfio_sptr);

        if (JPEG_HEADER_OK != jpeg_read_header(jds_ptr, 1))
        {
            jit_err = JDEC_ERR_READ_HEADER;
            break;
        }

        // 设置解码输出的色彩空间，libjpeg.txt 文档中，只支持
        // YCbCr, BG_YCC, GRAYSCALE, RGB 这几种模式转 RGB
        jit_err = JDEC_ERR_UNKNOW;
        switch (jds_ptr->jpeg_color_space)
        {
        case JCS_GRAYSCALE :
        case JCS_RGB       :
        case JCS_YCbCr     :
        case JCS_BG_YCC    :
            jds_ptr->out_color_space = JCS_RGB;
            break;

        default:
            jit_err = JDEC_ERR_COLOR_FORMAT;
            break;
        }
        if (JDEC_ERR_COLOR_FORMAT == jit_err)
        {
            break;
        }

        // 设置返回的 JPEG 图像信息
        if (J_NULL != jit_width ) *jit_width  = (j_int_t)jds_ptr->image_width;
        if (J_NULL != jit_height) *jit_height = (j_int_t)jds_ptr->image_height;

        jit_wlen = ((jit_ctrlcs & 0xFF) >> 3) * (j_int_t)jds_ptr->image_width;

        // 判断 行步进大小 是否满足解码输出要求
        if (0 == jit_stride)
        {
            if (jdec_cptr->jbt_align)
                jit_stride = jval_align(jit_wlen, 4);
            else
                jit_stride = jit_wlen;
        }
        else if (jval_abs(jit_stride) < (j_int_t)jval_align(jit_wlen, 4))
        {
            jit_err = JDEC_ERR_STRIDE;
            break;
        }

        // 判断输出 RGB24 数据缓存的容量大小是否足够
        if (jut_mlen <
                (j_uint_t)jval_abs(jit_stride * (j_int_t)jds_ptr->image_height))
        {
            jit_err = JDEC_ERR_CAPACITY;
            break;
        }

        //======================================

        // 开启 JPEG 解码
        jpeg_start_decompress(jds_ptr);

        switch (jit_ctrlcs)
        {
        case JCTRL_CS_RGB:
            while (jds_ptr->output_scanline < jds_ptr->output_height)
            {
                jpeg_read_scanlines(jds_ptr, (JSAMPARRAY)&jct_mptr, 1);
                jct_mptr += jit_stride;
            }
            break;

        case JCTRL_CS_BGR:
            while (jds_ptr->output_scanline < jds_ptr->output_height)
            {
                jpeg_read_scanlines(jds_ptr, (JSAMPARRAY)&jct_mptr, 1);
                rgb_transto_bgr_line(jct_mptr, (j_int_t)jds_ptr->output_width);
                jct_mptr += jit_stride;
            }
            break;

        case JCTRL_CS_RGBA:
            while (jds_ptr->output_scanline < jds_ptr->output_height)
            {
                jpeg_read_scanlines(jds_ptr, (JSAMPARRAY)&jct_mptr, 1);
                rgb_expandto_rgba_line(
                            jct_mptr,
                            jct_alpha,
                            jds_ptr->output_width);
                jct_mptr += jit_stride;
            }
            break;

        case JCTRL_CS_BGRA:
            while (jds_ptr->output_scanline < jds_ptr->output_height)
            {
                jpeg_read_scanlines(jds_ptr, (JSAMPARRAY)&jct_mptr, 1);
                rgb_expandto_bgra_line(
                            jct_mptr,
                            jct_alpha,
                            jds_ptr->output_width);
                jct_mptr += jit_stride;
            }
            break;

        case JCTRL_CS_ARGB:
            while (jds_ptr->output_scanline < jds_ptr->output_height)
            {
                jpeg_read_scanlines(jds_ptr, (JSAMPARRAY)&jct_mptr, 1);
                rgb_expandto_argb_line(
                            jct_mptr,
                            jct_alpha,
                            jds_ptr->output_width);
                jct_mptr += jit_stride;
            }
            break;

        case JCTRL_CS_ABGR:
            while (jds_ptr->output_scanline < jds_ptr->output_height)
            {
                jpeg_read_scanlines(jds_ptr, (JSAMPARRAY)&jct_mptr, 1);
                rgb_expandto_abgr_line(
                            jct_mptr,
                            jct_alpha,
                            jds_ptr->output_width);
                jct_mptr += jit_stride;
            }
            break;

        default:
            break;
        }

        // 结束 JPEG 解码
        jpeg_finish_decompress(jds_ptr);
        jds_ptr = J_NULL;

        //======================================
        jit_err = JDEC_ERR_OK;
    } while (0);

__EXIT_FUNC__:
    if (J_NULL != jds_ptr)
    {
        jpeg_abort_decompress(jds_ptr);
        jds_ptr = J_NULL;
    }

    return jit_err;
}

//====================================================================

// 
// JPEG 解码操作 : public interfaces
// 

/**********************************************************/
/**
 * @brief 申请 JPEG 解码操作的上下文对象。
 * 
 * @param [in ] jvt_reserved : 保留参数（当前使用 J_NULL 即可）。
 * 
 * @return jdec_ctxptr_t :
 * 返回 JPEG 解码操作的上下文对象，为 J_NULL 时表示申请失败。
 */
jdec_ctxptr_t jdec_alloc(j_void_t * jvt_reserved)
{
    jdec_ctxptr_t jdec_cptr = (jdec_ctxptr_t)calloc(1, sizeof(jdec_ctx_t));
    if (J_NULL == jdec_cptr)
    {
        return J_NULL;
    }

    //======================================

    jdec_cptr->jut_size = sizeof(jdec_ctx_t);
    jdec_cptr->jit_type = JDEC_HANDLE_TYPE;

    // 初始化 JPEG 解码器参数
    jdec_cptr->j_decoder.err =
            jpeg_std_error(&jdec_cptr->j_err_mgr.jerr_mgr);
    jdec_cptr->j_err_mgr.jerr_mgr.error_exit = jpeg_error_exit_callback;

    jpeg_create_decompress(&jdec_cptr->j_decoder);

    jdec_cptr->jbt_align = J_TRUE;
    jdec_cptr->jut_vpad  = 0xFFFFFFFF;
    jdec_cptr->jbt_rfio  = J_TRUE;

    jdec_cptr->jsrc.jit_mode = JCTRL_MODE_UNKNOW;
    jdec_cptr->jsrc.jht_iptr = J_NULL;
    jdec_cptr->jsrc.jut_size = 0;

    //======================================

    return jdec_cptr;
}

/**********************************************************/
/**
 * @brief 释放 JPEG 解码操作的上下文对象。
 */
j_void_t jdec_release(jdec_ctxptr_t jdec_cptr)
{
    if (jdec_valid(jdec_cptr))
    {
        jpeg_destroy_decompress(&jdec_cptr->j_decoder);
        free(jdec_cptr);
    }
}

/**********************************************************/
/**
 * @brief 判断 JPEG 解码操作的上下文对象 是否有效。
 */
j_bool_t jdec_valid(jdec_ctxptr_t jdec_cptr)
{
    if (J_NULL == jdec_cptr)
    {
        return J_FALSE;
    }

    if (sizeof(jdec_ctx_t) != jdec_cptr->jut_size)
    {
        return J_FALSE;
    }

    if (JDEC_HANDLE_TYPE != jdec_cptr->jit_type)
    {
        return J_FALSE;
    }

    return J_TRUE;
}

/**********************************************************/
/**
 * @brief 设置解码输出数据（RGB数据）时，是否按 4 字节对齐。
 * @note  默认情况是按 4 字节对齐的。
 */
j_void_t jdec_set_align(jdec_ctxptr_t jdec_cptr, j_bool_t jbt_align)
{
    jdec_cptr->jbt_align = jbt_align;
}

/**********************************************************/
/**
 * @brief 设置解码过程中 填充空白通道 的数值（例如输出 RGB32 的 ALPHA 通道）。
 * @note 默认的填充值是 0xFFFFFFFF。
 */
j_void_t jdec_set_vpad(jdec_ctxptr_t jdec_cptr, j_uint_t jut_vpad)
{
    jdec_cptr->jut_vpad = jut_vpad;
}

/**********************************************************/
/**
 * @brief 
 * JPEG 解码输入源使用 文件流模式 时，是否自动重置 文件指针。
 * 默认情况下，其值为 J_TRUE，操作文件流后，会自动还原原文件指针位置。
 */
j_bool_t jdec_is_rfio(jdec_ctxptr_t jdec_cptr)
{
    return jdec_cptr->jbt_rfio;
}

/**********************************************************/
/**
 * @brief 设置 文件流模式 时，是否自动重置 文件指针。
 */
j_void_t jdec_set_rfio(jdec_ctxptr_t jdec_cptr, j_bool_t jbt_rfio)
{
    jdec_cptr->jbt_rfio = jbt_rfio;
}

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
                j_uint_t      jut_size)
{
    j_int_t jit_err = JDEC_ERR_INVALID_PARAM;

    switch (jit_mode)
    {
    case JCTRL_MODE_MEM:
        if ((J_NULL != jht_iptr) && (jut_size > 0))
        {
            jdec_cptr->jsrc.jit_mode = JCTRL_MODE_MEM;
            jdec_cptr->jsrc.jht_iptr = jht_iptr;
            jdec_cptr->jsrc.jut_size = jut_size;

            jit_err = JDEC_ERR_OK;
        }
        break;

    case JCTRL_MODE_FIO:
        if (J_NULL != jht_iptr)
        {
            jdec_cptr->jsrc.jit_mode = JCTRL_MODE_FIO;
            jdec_cptr->jsrc.jht_iptr = jht_iptr;
            jdec_cptr->jsrc.jut_size = 0;

            jit_err = JDEC_ERR_OK;
        }
        break;

    case JCTRL_MODE_FILE:
        if ((J_NULL != jht_iptr) && ('\0' != ((j_path_t)jht_iptr)[0]))
        {
            jdec_cptr->jsrc.jit_mode = JCTRL_MODE_FILE;
            jdec_cptr->jsrc.jht_iptr = jht_iptr;
            jdec_cptr->jsrc.jut_size = 0;

            jit_err = JDEC_ERR_OK;
        }
        break;

    default:
        break;
    }

    return jit_err;
}

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
                jinfo_ptr_t   jinfo_ptr)
{
    j_int_t jit_err = JDEC_ERR_UNKNOW;

    jpeg_src_t jdec_src;
    j_fpos_t   jpos_fio;

    do
    {
        //======================================
        // 检查参数有效性

        if (J_NULL == jinfo_ptr)
        {
            jit_err = JDEC_ERR_INVALID_PARAM;
            break;
        }

        //======================================
        // 获取输入源信息

        jit_err = jdec_query_src(jdec_cptr, &jdec_src);
        if (JDEC_ERR_OK != jit_err)
        {
            break;
        }

        if ((JCTRL_MODE_FIO == jdec_cptr->jsrc.jit_mode) &&
            jdec_cptr->jbt_rfio)
        {
            if (0 != fgetpos(jdec_src.Jfio_sptr, &jpos_fio))
            {
                jit_err = JDEC_ERR_FIO_FGETPOS;
                break;
            }
        }

        //======================================
        // 读取图像基本信息

        jit_err = jdec_info(jdec_cptr, &jdec_src, jinfo_ptr);

        //======================================
        // 释放相关资源

        if (JCTRL_MODE_FILE == jdec_cptr->jsrc.jit_mode)
        {
            fclose(jdec_src.Jfio_sptr);
        }
        else if ((JCTRL_MODE_FIO == jdec_cptr->jsrc.jit_mode) &&
                 jdec_cptr->jbt_rfio)
        {
            fsetpos(jdec_src.Jfio_sptr, &jpos_fio);
        }

        jdec_src.Jfio_sptr = J_NULL;
        jdec_src.jst_msize = 0;

        //======================================
    } while (0);

    return jit_err;
}

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
                j_int_t       jit_ctrlcs)
{
    j_int_t jit_err = JDEC_ERR_UNKNOW;

    jpeg_src_t jdec_src;
    j_fpos_t   jpos_fio;

    do
    {
        //======================================
        // 检查参数有效性

        if ((J_NULL == jmem_optr) || (jut_mlen <= 0))
        {
            jit_err = JDEC_ERR_INVALID_PARAM;
            break;
        }

        switch (jit_ctrlcs)
        {
        case JCTRL_CS_RGB :
        case JCTRL_CS_BGR :
        case JCTRL_CS_RGBA:
        case JCTRL_CS_BGRA:
        case JCTRL_CS_ARGB:
        case JCTRL_CS_ABGR:
            break;
        default: jit_ctrlcs = JCTRL_CS_UNKNOW;
            break;
        }

        if (JCTRL_CS_UNKNOW == jit_ctrlcs)
        {
            jit_err = JDEC_ERR_INVALID_PARAM;
            break;
        }

        //======================================
        // 获取输入源信息

        jit_err = jdec_query_src(jdec_cptr, &jdec_src);
        if (JDEC_ERR_OK != jit_err)
        {
            break;
        }

        if ((JCTRL_MODE_FIO == jdec_cptr->jsrc.jit_mode) &&
            jdec_cptr->jbt_rfio)
        {
            if (0 != fgetpos(jdec_src.Jfio_sptr, &jpos_fio))
            {
                jit_err = JDEC_ERR_FIO_FGETPOS;
                break;
            }
        }

        //======================================
        // 执行解码操作

        jit_err = 
            jdec_to_rgb(
                jdec_cptr,
                &jdec_src,
                jmem_optr,
                jit_stride,
                jut_mlen,
                jit_width,
                jit_height,
                jit_ctrlcs);

        //======================================
        // 释放相关资源

        if (JCTRL_MODE_FILE == jdec_cptr->jsrc.jit_mode)
        {
            fclose(jdec_src.Jfio_sptr);
        }
        else if ((JCTRL_MODE_FIO == jdec_cptr->jsrc.jit_mode) &&
                 jdec_cptr->jbt_rfio)
        {
            fsetpos(jdec_src.Jfio_sptr, &jpos_fio);
        }

        jdec_src.Jfio_sptr = J_NULL;
        jdec_src.jst_msize = 0;

        //======================================
    } while (0);

    return jit_err;
}

////////////////////////////////////////////////////////////////////////////////
