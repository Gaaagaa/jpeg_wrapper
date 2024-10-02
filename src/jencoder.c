/**
 * @file jencoder.c
 * Copyright (c) 2024 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2024-09-24
 * @version : 1.0.0.0
 * @brief   : 封装 libjpeg 库，实现简单 JPEG 图像 编码器 的相关操作接口。
 */

#include "jcomm.h"
#include "jencoder.h"
#include <stdlib.h>
#include <string.h>

#include "jcomm.inl"

////////////////////////////////////////////////////////////////////////////////
// JPEG 编码器 相关常量与数据类型

/** 默认的 JPEG 编码压缩质量值 */
#define JENC_DEF_QUALITY    75

/** 定义 JPEG 编码操作的上下文 结构体类型标识 */
#define JENC_HANDLE_TYPE    0x6A706567

/** 每次编码写入像素行数量 的 默认值 */
#define JENC_DEF_WROWS      16

/** 重定义 libjpeg 中的 JPEG 编码器结构体 名称 */
typedef struct jpeg_compress_struct  jenc_obj_t;

/**
 * @struct jenc_ctx_t
 * @brief  JPEG 编码操作句柄的描述信息。
 */
typedef struct jenc_ctx_t
{
    j_uint_t        jut_size;  ///< 结构体大小
    j_uint_t        jut_type;  ///< 结构体类型标识（固定为 JENC_HANDLE_TYPE）
    jerr_mgr_t      jerr_mgr;  ///< JPEG 操作错误管理对象
    jenc_obj_t      jenc_obj;  ///< JPEG 编码器 操作对象

    j_int_t         jit_qual;  ///< JPEG 编码的压缩质量（1 - 100）
    j_bool_t        jbl_work;  ///< JPEG 编码器是否处于工作状态

    /**
     * @brief 编码输出源模式的相关工作参数。
     */
    struct
    {
        jctl_mode_t jct_mode;  ///< 输出模式

        union
        {
        j_size_t    jst_mlen;  ///< 内存模式，缓存大小
        j_fpos_t    jfp_spos;  ///< 文件流模式，记录编码数据写入前的 文件指针偏移量
        j_fstream_t jfs_file;  ///< 文件模式，保存打开的文件指针
        };

        union
        {
        j_fmemory_t jmt_optr;  ///< 内存模式，其为输出 JPEG 数据的缓存地址
        j_fstream_t jfs_ostr;  ///< 文件流模式，其为输出 JPEG 数据的文件流
        j_fszpath_t jsz_path;  ///< 文件模式，其为输出 JPEG 数据的文件路径
        };
    } jmode;

    /**
     * @brief 在文件模式工作时，提供文件路径的字符串缓存。
     */
    struct
    {
        j_size_t    jst_size;  ///< 字符串缓存大小
        j_char_t  * jsz_path;  ///< 字符串缓存
    } jpath;

    /**
     * @brief 编码操作使用到的 像素输入行 工作参数。
     */
    struct
    {
        j_uint_t    jut_size;  ///< 行数组大小
        j_mptr_t  * jar_rows;  ///< 待编码像素行的各个缓存地址数组
    } jrows;

    /**
     * @brief 
     * 在内存模式工作时，保存 编码器 内部自动分配的缓存信息，
     * 即 最后编码生成的 JPEG 数据流。
     */
    struct
    {
        j_size_t    jst_size;  ///< 有效字节数
        j_mptr_t    jmt_mptr;  ///< 缓存地址
    } jbuff;
} jenc_ctx_t;

////////////////////////////////////////////////////////////////////////////////
// JPEG 编码器 内部接口函数

/**********************************************************/
/**
 * @brief 更新内部 jpath 的字符串内容。
 * 
 * @param [in ] jenc_this : JPEG 编码操作的上下文对象。
 * @param [in ] jsz_path  : 更新的字符串内容。
 * 
 * @return j_int_t : 错误码，请参看 jenc_errno_t 相关枚举值。
 */
static j_int_t jenc_update_path(jenc_this_t jenc_this, j_fszpath_t jsz_path)
{
    j_size_t jst_size = strlen(jsz_path);

    if (jst_size >= jenc_this->jpath.jst_size)
    {
        if (J_NULL != jenc_this->jpath.jsz_path)
            free(jenc_this->jpath.jsz_path);

        jenc_this->jpath.jst_size = (j_size_t)jval_align(jst_size + 32, 32);
        jenc_this->jpath.jsz_path = (j_char_t *)calloc(
                                        jenc_this->jpath.jst_size,
                                        sizeof(j_char_t));
    }

    if (J_NULL == jenc_this->jpath.jsz_path)
    {
        return JENC_ERR_MALLOC;
    }

    strcpy(jenc_this->jpath.jsz_path, jsz_path);
    return JENC_ERR_OK;
}

/**********************************************************/
/**
 * @brief 增长 像素输入行 数组的容量。
 * 
 * @param [in ] jenc_this : JPEG 编码操作的上下文对象。
 * @param [in ] jut_rlow  : 像素输出行 数组容量 的 最低值。
 * 
 * @return j_int_t : 错误码，请参看 jenc_errno_t 相关枚举值。
 */
static inline j_int_t jenc_grow_rows(jenc_this_t jenc_this, j_uint_t jut_rlow)
{
    if (jenc_this->jrows.jut_size < jut_rlow)
    {
        if (J_NULL != jenc_this->jrows.jar_rows)
            free(jenc_this->jrows.jar_rows);

        jenc_this->jrows.jut_size = jut_rlow;
        jenc_this->jrows.jar_rows = 
            (j_mptr_t *)malloc(jut_rlow * sizeof(j_mptr_t));
    }

    if (J_NULL == jenc_this->jrows.jar_rows)
    {
        return JENC_ERR_MALLOC;
    }

    return JENC_ERR_OK;
}

/**********************************************************/
/**
 * @brief 关闭编码器工作。
 */
static j_void_t jenc_shutdown(jenc_this_t jenc_this)
{
    if (JCTL_MODE_FMEMORY == jenc_this->jmode.jct_mode)
    {
        if (jenc_this->jbuff.jmt_mptr == jenc_this->jmode.jmt_optr)
        {
            jenc_this->jbuff.jst_size = 0;
            jenc_this->jbuff.jmt_mptr = J_NULL;
        }
    }
    else if (JCTL_MODE_FSTREAM == jenc_this->jmode.jct_mode)
    {
        jenc_this->jmode.jfp_spos = 0;
    }
    else if (JCTL_MODE_FSZPATH == jenc_this->jmode.jct_mode)
    {
        if (J_NULL != jenc_this->jmode.jfs_file)
        {
            fclose(jenc_this->jmode.jfs_file);
            jenc_this->jmode.jfs_file = J_NULL;
        }
    }

    jpeg_abort_compress(&jenc_this->jenc_obj);

    jenc_this->jbl_work = J_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// JPEG 编码器 外部接口函数

/**********************************************************/
/**
 * @brief 申请 JPEG 编码操作的上下文对象。
 * 
 * @param [in ] jvt_reserved : 保留参数（当前使用 J_NULL 即可）。
 * 
 * @return jenc_this_t :
 * 返回 JPEG 编码操作的上下文对象，为 J_NULL 时表示申请失败。
 */
jenc_this_t jenc_alloc(j_void_t * jvt_reserved)
{
    jenc_this_t jenc_this = (jenc_this_t)calloc(1, sizeof(jenc_ctx_t));
    if (J_NULL == jenc_this)
    {
        return J_NULL;
    }

    //======================================

    jenc_this->jut_size = sizeof(jenc_ctx_t);
    jenc_this->jut_type = JENC_HANDLE_TYPE;
    jenc_this->jit_qual = JENC_DEF_QUALITY;
    jenc_this->jbl_work = J_FALSE;

    jenc_this->jmode.jct_mode = JCTL_MODE_UNKNOWN;
    jenc_this->jmode.jst_mlen = 0;
    jenc_this->jmode.jmt_optr = J_NULL;

    jenc_this->jpath.jst_size = 0;
    jenc_this->jpath.jsz_path = J_NULL;

    jenc_this->jrows.jut_size = 0;
    jenc_this->jrows.jar_rows = J_NULL;

    jenc_this->jbuff.jst_size = 0;
    jenc_this->jbuff.jmt_mptr = J_NULL;

    jenc_this->jenc_obj.err =
        jpeg_std_error(&jenc_this->jerr_mgr.jerr_mgr);
    jenc_this->jerr_mgr.jerr_mgr.error_exit = jerr_exit_callback;

    jpeg_create_compress(&jenc_this->jenc_obj);

    //======================================

    return jenc_this;
}

/**********************************************************/
/**
 * @brief 释放 JPEG 编码操作的上下文对象。
 */
j_void_t jenc_release(jenc_this_t jenc_this)
{
    if (jenc_valid(jenc_this))
    {
        jpeg_destroy_compress(&jenc_this->jenc_obj);

        if (J_NULL != jenc_this->jpath.jsz_path)
            free(jenc_this->jpath.jsz_path);

        if (J_NULL != jenc_this->jrows.jar_rows)
            free(jenc_this->jrows.jar_rows);

        if (J_NULL != jenc_this->jbuff.jmt_mptr)
            free(jenc_this->jbuff.jmt_mptr);

        free(jenc_this);
    }
}

/**********************************************************/
/**
 * @brief JPEG 编码操作句柄的有效判断。
 */
j_bool_t jenc_valid(jenc_this_t jenc_this)
{
    if (J_NULL == jenc_this)
    {
        return J_FALSE;
    }

    if (sizeof(jenc_ctx_t) != jenc_this->jut_size)
    {
        return J_FALSE;
    }

    if (JENC_HANDLE_TYPE != jenc_this->jut_type)
    {
        return J_FALSE;
    }

    return J_TRUE;
}

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
                j_uint_t    jut_qual)
{
    JASSERT(jenc_valid(jenc_this));

    j_int_t jit_err = JENC_ERR_UNKNOWN;

    //======================================

    if (jenc_this->jbl_work)
    {
        return JENC_ERR_WORKING;
    }

    //======================================
    // 输出模式

    jit_err = JENC_ERR_EPARAM;

    switch (jct_mode)
    {
    case JCTL_MODE_FMEMORY:
        {
            jenc_this->jmode.jct_mode = JCTL_MODE_FMEMORY;
            if ((J_NULL == jht_optr) || (0 == jst_mlen))
            {
                jenc_this->jmode.jst_mlen = 0;
                jenc_this->jmode.jmt_optr = J_NULL;
            }
            else
            {
                jenc_this->jmode.jst_mlen = jst_mlen;
                jenc_this->jmode.jmt_optr = (j_mptr_t)jht_optr;
            }

            jit_err = JENC_ERR_OK;
        }
        break;

    case JCTL_MODE_FSTREAM:
        if (J_NULL != jht_optr)
        {
            jenc_this->jmode.jct_mode = JCTL_MODE_FSTREAM;
            jenc_this->jmode.jfp_spos = 0;
            jenc_this->jmode.jfs_ostr = (j_fstream_t)jht_optr;

            jit_err = JENC_ERR_OK;
        }
        break;

    case JCTL_MODE_FSZPATH:
        if ((J_NULL != jht_optr) && ('\0' != ((j_fszpath_t)jht_optr)[0]))
        {
            jit_err = jenc_update_path(jenc_this, (j_fszpath_t)jht_optr);
            if (JENC_ERR_OK == jit_err)
            {
                jenc_this->jmode.jct_mode = JCTL_MODE_FSZPATH;
                jenc_this->jmode.jfs_file = J_NULL;
                jenc_this->jmode.jsz_path = jenc_this->jpath.jsz_path;
            }
        }
        break;

    default:
        break;
    }

    if (JENC_ERR_OK != jit_err)
    {
        jenc_this->jmode.jct_mode = JCTL_MODE_UNKNOWN;
        jenc_this->jmode.jst_mlen = J_NULL;
        jenc_this->jmode.jmt_optr = J_NULL;

        return jit_err;
    }

    //======================================

    if (0 == jut_qual)
        jenc_this->jit_qual = JENC_DEF_QUALITY;
    else if (jut_qual > 100)
        jenc_this->jit_qual = 100;
    else
        jenc_this->jit_qual = (j_int_t)jut_qual;

    //======================================

    return JENC_ERR_OK;
}

/**********************************************************/
/**
 * @brief 在内存模式下，执行编码压缩操作后，所缓存的 JPEG 数据流地址。
 */
j_mptr_t jenc_fmdata(jenc_this_t jenc_this)
{
    return jenc_this->jbuff.jmt_mptr;
}

/**********************************************************/
/**
 * @brief 在内存模式下，执行编码压缩操作后，所缓存的 JPEG 数据流有效字节数。
 */
j_uint_t jenc_fmsize(jenc_this_t jenc_this)
{
    return (j_uint_t)jenc_this->jbuff.jst_size;
}

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
                j_uint_t    jut_imgh)
{
    JASSERT(jenc_valid(jenc_this));

    j_int_t      jit_err  = JENC_ERR_UNKNOWN;
    jenc_obj_t * jenc_ptr = J_NULL;
    j_uint_t     jut_size = 0;

    do
    {
        //======================================

        if (jenc_this->jbl_work)
        {
            jit_err = JENC_ERR_WORKING;
            break;
        }

        jenc_ptr = &jenc_this->jenc_obj;

        //======================================
        // 输入参数已经被检查过

        if (!jenc_ccs_valid(jccs_conv))
        {
            jit_err = JENC_ERR_CCS_VALUE;
            break;
        }

        if ((jut_imgw <= 0) || (jut_imgh <= 0))
        {
            jit_err = JENC_ERR_IMAGE_ZEROSIZE;
            break;
        }

        if ((jut_imgw > JPEG_MAX_DIMENSION) || (jut_imgh > JPEG_MAX_DIMENSION))
        {
            jit_err = JENC_ERR_IMAGE_OVERSIZE;
            break;
        }

        //======================================
        // 设置错误回调跳转代码

        if (0 != setjmp(jenc_this->jerr_mgr.jerr_jmp))
        {
            jit_err = JENC_ERR_EXCEPTION;
            goto __EXIT_FUNC;
        }

        //======================================
        // 设置输出目标

        if (JCTL_MODE_FMEMORY == jenc_this->jmode.jct_mode)
        {
            if (J_NULL != jenc_this->jbuff.jmt_mptr)
            {
                free(jenc_this->jbuff.jmt_mptr);
            }

            jenc_this->jbuff.jst_size = jenc_this->jmode.jst_mlen;
            jenc_this->jbuff.jmt_mptr = jenc_this->jmode.jmt_optr;

            jpeg_mem_dest(
                jenc_ptr,
                &jenc_this->jbuff.jmt_mptr,
                &jenc_this->jbuff.jst_size);
        }
        else if (JCTL_MODE_FSTREAM == jenc_this->jmode.jct_mode)
        {
            if (J_NULL == jenc_this->jmode.jfs_ostr)
            {
                jit_err = JENC_ERR_FSTREAM_ISNULL;
                break;
            }

            fgetpos(jenc_this->jmode.jfs_ostr, &jenc_this->jmode.jfp_spos);
            jpeg_stdio_dest(jenc_ptr, jenc_this->jmode.jfs_ostr);
        }
        else if (JCTL_MODE_FSZPATH == jenc_this->jmode.jct_mode)
        {
            if ((J_NULL == jenc_this->jmode.jsz_path) ||
                ('\0' == jenc_this->jmode.jsz_path[0]))
            {
                jit_err = JENC_ERR_FSZPATH_ISNULL;
                break;
            }

            jenc_this->jmode.jfs_file = 
                fopen((j_fszpath_t)jenc_this->jmode.jsz_path, "wb+");
            if (J_NULL == jenc_this->jmode.jfs_file)
            {
                jit_err = JENC_ERR_FOPEN;
                break;
            }

            jpeg_stdio_dest(jenc_ptr, jenc_this->jmode.jfs_file);
        }
        else
        {
            jit_err = JENC_ERR_UNCONFIG;
            break;
        }

        //======================================

        // 参数验证阶段，可保证 以下断言代码不会被触发
        JASSERT(JCS_UNKNOWN != jcs_to_lib(JENC_CCS_IN(jccs_conv)));
        JASSERT(JCS_UNKNOWN != jcs_to_lib(JENC_CCS_OUT(jccs_conv)));

        jenc_ptr->image_width  = (j_int_t)jut_imgw;
        jenc_ptr->image_height = (j_int_t)jut_imgh;

        // 配置 JPEG 编码输入的色彩空间及通道数量
        jenc_ptr->input_components = JENC_CCS_NUMC(jccs_conv);
        jenc_ptr->in_color_space   = jcs_to_lib(JENC_CCS_IN(jccs_conv));

        jpeg_set_defaults(jenc_ptr);
        jpeg_set_quality(jenc_ptr, jenc_this->jit_qual, J_TRUE);

        // 配置 JPEG 编码输出的色彩空间
        jpeg_set_colorspace(jenc_ptr, jcs_to_lib(JENC_CCS_OUT(jccs_conv)));

        //======================================

        jpeg_start_compress(jenc_ptr, J_TRUE);

        // 至此，标识 编码器 处于工作状态，等待 jenc_write() 操作
        jenc_this->jbl_work = J_TRUE;

        //======================================
        jenc_ptr = J_NULL;
        jit_err  = JENC_ERR_OK;
    } while (0);

    //======================================

__EXIT_FUNC:
    if (J_NULL != jenc_ptr)
    {
        jenc_shutdown(jenc_this);
        jenc_ptr = J_NULL;
    }

    return jit_err;
}

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
                j_uint_t    jut_rows)
{
    JASSERT(jenc_valid(jenc_this));

    j_int_t  jit_err  = JENC_ERR_UNKNOWN;
    j_uint_t jut_iter = 0;

    do
    {
        //======================================

        if (!jenc_this->jbl_work)
        {
            jit_err = JENC_ERR_UNSTART;
            break;
        }

        // 若图像已经写满，则直接退出
        if (jenc_this->jenc_obj.next_scanline >=
            jenc_this->jenc_obj.image_height)
        {
            jit_err = 0;
            break;
        }

        //======================================
        // 验证参数有效性

        if ((J_NULL == jmt_pxls) || (jut_rows <= 0))
        {
            jit_err = 0;
            break;
        }

        //======================================
        // 构建输入像素行数组

        jit_err = jenc_grow_rows(jenc_this, jut_rows);
        if (JENC_ERR_OK != jit_err)
        {
            break;
        }

        for (jut_iter = 0; jut_iter < jut_rows; ++jut_iter)
        {
            jenc_this->jrows.jar_rows[jut_iter] = jmt_pxls;
            jmt_pxls += jit_step;
        }

        //======================================
        // 开始向 编码器 写入像素数据

        // 设置错误回调跳转代码后，再执行编码操作
        if (0 == setjmp(jenc_this->jerr_mgr.jerr_jmp))
        {
            for (jut_iter = 0; jut_iter < jut_rows; )
            {
                jut_iter += jpeg_write_scanlines(
                                &jenc_this->jenc_obj,
                                &jenc_this->jrows.jar_rows[jut_iter],
                                jut_rows - jut_iter);

                if (jenc_this->jenc_obj.next_scanline >=
                    jenc_this->jenc_obj.image_height)
                {
                    break;
                }
            }

            jit_err = (j_int_t)jut_iter;
        }
        else
        {
            // 异常处理
            jit_err = JENC_ERR_EXCEPTION;

            // 关闭编码器
            jenc_shutdown(jenc_this);
        }

        //======================================
    } while (0);

    return jit_err;
}

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
j_int_t jenc_finish(jenc_this_t jenc_this)
{
    JASSERT(jenc_valid(jenc_this));

    j_int_t jit_err = JENC_ERR_UNKNOWN;

    //======================================

    if (!jenc_this->jbl_work)
    {
        return JENC_ERR_UNSTART;
    }

    //======================================

    // 设置错误回调跳转代码后，再执行操作
    if (0 == setjmp(jenc_this->jerr_mgr.jerr_jmp))
    {
        jpeg_finish_compress(&jenc_this->jenc_obj);

        if ((JCTL_MODE_FMEMORY == jenc_this->jmode.jct_mode) &&
            (jenc_this->jbuff.jmt_mptr == jenc_this->jmode.jmt_optr))
        {
            jit_err = (j_int_t)jenc_this->jbuff.jst_size;
        }
        else
        {
            jit_err = JENC_ERR_OK;
        }
    }
    else
    {
        // 异常处理
        jit_err = JENC_ERR_EXCEPTION;
    }

    //======================================
    // 关闭编码器

    jenc_shutdown(jenc_this);

    //======================================

    return jit_err;
}

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
                j_uint_t    jut_imgh)
{
    j_int_t jit_err;

    jit_err = jenc_start(jenc_this, jccs_conv, jut_imgw, jut_imgh);
    if (JENC_ERR_OK != jit_err)
    {
        return jit_err;
    }

    jit_err = jenc_write(jenc_this, jmt_pxls, jit_step, jut_imgh);
    if (jit_err < 0)
    {
        return jit_err;
    }

    return jenc_finish(jenc_this);
}

////////////////////////////////////////////////////////////////////////////////
