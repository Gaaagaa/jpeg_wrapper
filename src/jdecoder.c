/**
 * @file jdecoder.c
 * Copyright (c) 2024 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2024-09-24
 * @version : 1.0.0.0
 * @brief   : 封装 libjpeg 库，实现简单 JPEG 图像 解码器 的相关操作接口。
 */

#include "jcomm.h"
#include "jdecoder.h"
#include <stdlib.h>
#include <string.h>

#include "jcomm.inl"

////////////////////////////////////////////////////////////////////////////////
// JPEG 解码器 相关常量与数据类型

/** 定义文件指针位置的 无效值 */
#define JFS_INVALID_FPOS     ((j_fpos_t)-1)

/** 定义 JPEG 解码操作的上下文 结构体类型标识 */
#define JDEC_HANDLE_TYPE     0x4A504547

/** 重定义 libjpeg 中的 JPEG 解码器结构体 名称 */
typedef struct jpeg_decompress_struct  jdec_obj_t;

/**
 * @struct jdec_ctx_t
 * @brief  JPEG 解码操作的上下文。
 */
typedef struct jdec_ctx_t
{
    j_uint_t        jut_size;  ///< 结构体大小
    j_uint_t        jut_type;  ///< 结构体类型标识（固定为 JDEC_HANDLE_TYPE）
    jerr_mgr_t      jerr_mgr;  ///< JPEG 操作错误管理对象
    jdec_obj_t      jdec_obj;  ///< JPEG 解码器 操作结构体

    j_bool_t        jbl_work;  ///< JPEG 解码器是否处于工作状态

    /**
     * @brief 解码输入源模式的相关工作参数。
     */
    struct
    {
        jctl_mode_t jct_mode;  ///< 输入模式

        union
        {
        j_size_t    jst_mlen;  ///< 内存模式，缓存大小
        j_fpos_t    jfp_spos;  ///< 文件流模式，记录解码数据读取前的 文件指针偏移量
        j_fstream_t jfs_file;  ///< 文件模式，保存打开的文件指针
        };

        union
        {
        j_fmemory_t jmt_iptr;  ///< 内存模式，其为输入 JPEG 数据的缓存地址
        j_fstream_t jfs_istr;  ///< 文件流模式，其为输入 JPEG 数据的文件流
        j_fszpath_t jsz_path;  ///< 文件模式，其为输入 JPEG 数据的文件路径
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
     * @brief 保存读取到的 JPEG 图像源基本信息。
     */
    jpeg_info_t     jinfo;

    /**
     * @brief 解码操作使用到的 像素输出行 工作参数。
     */
    struct
    {
        j_uint_t    jut_size;  ///< 行数组大小
        j_mptr_t  * jar_rows;  ///< 待编码像素行的各个缓存地址数组
    } jrows;
} jdec_ctx_t;

////////////////////////////////////////////////////////////////////////////////
// JPEG 解码器 内部接口函数

/**********************************************************/
/**
 * @brief 按照图像源输入模式，设置 JPEG 解码器的数据源。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
static j_int_t jdec_load_mode(jdec_this_t jdec_this)
{
    j_int_t jit_err = JDEC_ERR_UNKNOWN;

    //======================================

    switch (jdec_this->jmode.jct_mode)
    {
    case JCTL_MODE_FMEMORY:
        {
            jpeg_mem_src(
                &jdec_this->jdec_obj,
                jdec_this->jmode.jmt_iptr,
                jdec_this->jmode.jst_mlen);
            jit_err = JDEC_ERR_OK;
        }
        break;

    case JCTL_MODE_FSTREAM:
        if (0 != fgetpos(jdec_this->jmode.jfs_istr, &jdec_this->jmode.jfp_spos))
        {
            jit_err = JDEC_ERR_FGETPOS;
        }
        else
        {
            jpeg_stdio_src(&jdec_this->jdec_obj, jdec_this->jmode.jfs_istr);
            jit_err = JDEC_ERR_OK;
        }
        break;

    case JCTL_MODE_FSZPATH:
        jdec_this->jmode.jfs_file = fopen(jdec_this->jmode.jsz_path, "rb");
        if (J_NULL != jdec_this->jmode.jfs_file)
        {
            jpeg_stdio_src(&jdec_this->jdec_obj, jdec_this->jmode.jfs_file);
            jit_err = JDEC_ERR_OK;
        }
        else
        {
            jit_err = JDEC_ERR_FOPEN;
        }
        break;

    default:
        jit_err = JDEC_ERR_UNCONFIG;
        break;
    }

    //======================================

    return jit_err;
}

/**********************************************************/
/**
 * @brief 完成解码相关操作后，卸载输入模式的相关工作参数。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 */
static j_void_t jdec_unload_mode(jdec_this_t jdec_this)
{
    if (JCTL_MODE_FSTREAM == jdec_this->jmode.jct_mode)
    {
        // 文件流模式时，重置文件指针位置
        if (JFS_INVALID_FPOS != jdec_this->jmode.jfp_spos)
        {
            fsetpos(jdec_this->jmode.jfs_istr, &jdec_this->jmode.jfp_spos);
            jdec_this->jmode.jfp_spos = JFS_INVALID_FPOS;
        }
    }
    else if (JCTL_MODE_FSZPATH == jdec_this->jmode.jct_mode)
    {
        // 文件模式时，关闭打开的文件流
        if (J_NULL != jdec_this->jmode.jfs_file)
        {
            fclose(jdec_this->jmode.jfs_file);
            jdec_this->jmode.jfs_file = J_NULL;
        }
    }
}

/**********************************************************/
/**
 * @brief 更新内部 jpath 的字符串内容。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * @param [in ] jsz_path  : 更新的字符串内容。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
static j_int_t jdec_update_path(jdec_this_t jdec_this, j_fszpath_t jsz_path)
{
    j_size_t jst_size = strlen(jsz_path);

    if (jst_size >= jdec_this->jpath.jst_size)
    {
        if (J_NULL != jdec_this->jpath.jsz_path)
            free(jdec_this->jpath.jsz_path);

        jdec_this->jpath.jst_size = (j_size_t)jval_align(jst_size + 32, 32);
        jdec_this->jpath.jsz_path = (j_char_t *)calloc(
                                        jdec_this->jpath.jst_size,
                                        sizeof(j_char_t));
    }

    if (J_NULL == jdec_this->jpath.jsz_path)
    {
        return JDEC_ERR_MALLOC;
    }

    strcpy(jdec_this->jpath.jsz_path, jsz_path);
    return JDEC_ERR_OK;
}

/**********************************************************/
/**
 * @brief 配置输入源后，更新 JPEG 图像源基本信息。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
static j_int_t jdec_update_info(jdec_this_t jdec_this)
{
    j_int_t      jit_err  = JDEC_ERR_UNKNOWN;
    jdec_obj_t * jdec_ptr = &jdec_this->jdec_obj;

    //======================================

    do 
    {
        //======================================
        // 设置错误回调跳转代码

        if (0 != setjmp(jdec_this->jerr_mgr.jerr_jmp))
        {
            jit_err = JDEC_ERR_EXCEPTION;
            goto __EXIT_FUNC;
        }

        //======================================
        // 设置图像源

        jit_err = jdec_load_mode(jdec_this);
        if (JDEC_ERR_OK != jit_err)
        {
            break;
        }

        //======================================
        // 读取 JPEG 图像源基本信息

        if (JPEG_HEADER_OK != jpeg_read_header(jdec_ptr, J_TRUE))
        {
            jit_err = JDEC_ERR_READ_HEADER;
            break;
        }

        //======================================
        // 保存返回的 JPEG 图像源基本信息

        jdec_this->jinfo.jit_imgw = jdec_ptr->image_width;
        jdec_this->jinfo.jit_imgh = jdec_ptr->image_height;
        jdec_this->jinfo.jit_nchs = jdec_ptr->num_components;
        jdec_this->jinfo.jcs_type = jcs_to_comm(jdec_ptr->jpeg_color_space);

        //======================================
        jit_err = JDEC_ERR_OK;
    } while (0);

    //======================================

__EXIT_FUNC:
    if (J_NULL != jdec_ptr)
    {
        jpeg_abort_decompress(jdec_ptr);
        jdec_ptr = J_NULL;
    }

    jdec_unload_mode(jdec_this);

    //======================================

    return jit_err;
}

/**********************************************************/
/**
 * @brief 增长 像素输出行 数组的容量。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * @param [in ] jut_rlow  : 像素输出行 数组容量 的 最低值。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
static inline j_int_t jdec_grow_rows(jdec_this_t jdec_this, j_uint_t jut_rlow)
{
    if (jdec_this->jrows.jut_size < jut_rlow)
    {
        if (J_NULL != jdec_this->jrows.jar_rows)
            free(jdec_this->jrows.jar_rows);

        jdec_this->jrows.jut_size = jut_rlow;
        jdec_this->jrows.jar_rows = 
            (j_mptr_t *)malloc(jut_rlow * sizeof(j_mptr_t));
    }

    if (J_NULL == jdec_this->jrows.jar_rows)
    {
        return JDEC_ERR_MALLOC;
    }

    return JDEC_ERR_OK;
}

/**********************************************************/
/**
 * @brief 关闭解码器。
 */
static inline j_void_t jdec_shutdown(jdec_this_t jdec_this)
{
    jpeg_abort_decompress(&jdec_this->jdec_obj);
    jdec_unload_mode(jdec_this);
    jdec_this->jbl_work = J_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// JPEG 解码器 外部接口函数

/**********************************************************/
/**
 * @brief 申请 JPEG 解码操作的上下文对象。
 * 
 * @param [in ] jvt_reserved : 保留参数（当前使用 J_NULL 即可）。
 * 
 * @return jdec_this_t :
 * 返回 JPEG 解码操作的上下文对象，为 J_NULL 时表示申请失败。
 */
jdec_this_t jdec_alloc(j_void_t * jvt_reserved)
{
    jdec_this_t jdec_this = (jdec_this_t)calloc(1, sizeof(jdec_ctx_t));
    if (J_NULL == jdec_this)
    {
        return J_NULL;
    }

    //======================================

    jdec_this->jut_size = sizeof(jdec_ctx_t);
    jdec_this->jut_type = JDEC_HANDLE_TYPE;
    jdec_this->jbl_work = J_FALSE;

    jdec_this->jmode.jct_mode = JCTL_MODE_UNKNOWN;
    jdec_this->jmode.jst_mlen = 0;
    jdec_this->jmode.jmt_iptr = J_NULL;

    jdec_this->jpath.jst_size = 0;
    jdec_this->jpath.jsz_path = J_NULL;

    jdec_this->jinfo.jit_imgw = 0;
    jdec_this->jinfo.jit_imgh = 0;
    jdec_this->jinfo.jit_nchs = 0;
    jdec_this->jinfo.jcs_type = JPEG_CS_UNKNOWN;

    jdec_this->jrows.jut_size = 0;
    jdec_this->jrows.jar_rows = J_NULL;

    jdec_this->jdec_obj.err = 
        jpeg_std_error(&jdec_this->jerr_mgr.jerr_mgr);
    jdec_this->jerr_mgr.jerr_mgr.error_exit = jerr_exit_callback;

    jpeg_create_decompress(&jdec_this->jdec_obj);

    //======================================

    return jdec_this;
}

/**********************************************************/
/**
 * @brief 释放 JPEG 解码操作的上下文对象。
 */
j_void_t jdec_release(jdec_this_t jdec_this)
{
    if (jdec_valid(jdec_this))
    {
        jpeg_destroy_decompress(&jdec_this->jdec_obj);

        if (J_NULL != jdec_this->jpath.jsz_path)
            free(jdec_this->jpath.jsz_path);

        if (J_NULL != jdec_this->jrows.jar_rows)
            free(jdec_this->jrows.jar_rows);

        free(jdec_this);
    }
}

/**********************************************************/
/**
 * @brief 判断 JPEG 解码操作的上下文对象 是否有效。
 */
j_bool_t jdec_valid(jdec_this_t jdec_this)
{
    if (J_NULL == jdec_this)
    {
        return J_FALSE;
    }

    if (sizeof(jdec_ctx_t) != jdec_this->jut_size)
    {
        return J_FALSE;
    }

    if (JDEC_HANDLE_TYPE != jdec_this->jut_type)
    {
        return J_FALSE;
    }

    return J_TRUE;
}

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
            j_size_t    jst_mlen)
{
    JASSERT(jdec_valid(jdec_this));

    j_int_t jit_err = JDEC_ERR_UNKNOWN;

    //======================================

    if (jdec_this->jbl_work)
    {
        return JDEC_ERR_WORKING;
    }

    //======================================
    // 输入模式

    switch (jct_mode)
    {
    case JCTL_MODE_FMEMORY:
        if ((J_NULL != jfh_iptr) && (jst_mlen > 0))
        {
            jdec_this->jmode.jct_mode = JCTL_MODE_FMEMORY;
            jdec_this->jmode.jst_mlen = jst_mlen;
            jdec_this->jmode.jmt_iptr = (j_fmemory_t)jfh_iptr;

            jit_err = JDEC_ERR_OK;
        }
        break;

    case JCTL_MODE_FSTREAM:
        if (J_NULL != jfh_iptr)
        {
            jdec_this->jmode.jct_mode = JCTL_MODE_FSTREAM;
            jdec_this->jmode.jfp_spos = JFS_INVALID_FPOS;
            jdec_this->jmode.jfs_istr = (j_fstream_t)jfh_iptr;

            jit_err = JDEC_ERR_OK;
        }
        break;

    case JCTL_MODE_FSZPATH:
        if ((J_NULL != jfh_iptr) && ('\0' != ((j_fszpath_t)jfh_iptr)[0]))
        {
            jit_err = jdec_update_path(jdec_this, (j_fszpath_t)jfh_iptr);
            if (JDEC_ERR_OK == jit_err)
            {
                jdec_this->jmode.jct_mode = JCTL_MODE_FSZPATH;
                jdec_this->jmode.jfs_file = J_NULL;
                jdec_this->jmode.jsz_path = jdec_this->jpath.jsz_path;
            }
        }
        break;

    default:
        break;
    }

    if (JDEC_ERR_OK != jit_err)
    {
        jdec_this->jmode.jct_mode = JCTL_MODE_UNKNOWN;
        jdec_this->jmode.jst_mlen = J_NULL;
        jdec_this->jmode.jmt_iptr = J_NULL;

        return jit_err;
    }

    //======================================

    return JDEC_ERR_OK;
}

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
            jinfo_ptr_t jinfo_ptr)
{
    JASSERT(jdec_valid(jdec_this));

    j_int_t jit_err;

    if (J_NULL == jinfo_ptr)
    {
        return JDEC_ERR_EPARAM;
    }

    if (JCTL_MODE_UNKNOWN == jdec_this->jmode.jct_mode)
    {
        return JDEC_ERR_UNCONFIG;
    }

    if (jdec_this->jbl_work)
    {
        return JDEC_ERR_WORKING;
    }

    jit_err = jdec_update_info(jdec_this);
    if (JDEC_ERR_OK != jit_err)
    {
        return jit_err;
    }

    jinfo_ptr->jit_imgw = jdec_this->jinfo.jit_imgw;
    jinfo_ptr->jit_imgh = jdec_this->jinfo.jit_imgh;
    jinfo_ptr->jit_nchs = jdec_this->jinfo.jit_nchs;
    jinfo_ptr->jcs_type = jdec_this->jinfo.jcs_type;

    return JDEC_ERR_OK;
}

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
            jinfo_ptr_t jinfo_ptr)
{
    JASSERT(jdec_valid(jdec_this));

    j_int_t      jit_err  = JDEC_ERR_UNKNOWN;
    jdec_obj_t * jdec_ptr = J_NULL;

    //======================================
    // 清空 jinfo_ptr

    if (J_NULL != jinfo_ptr)
    {
        jinfo_ptr->jit_imgw = 0;
        jinfo_ptr->jit_imgh = 0;
        jinfo_ptr->jit_nchs = 0;
        jinfo_ptr->jcs_type = JPEG_CS_UNKNOWN;
    }

    //======================================

    do
    {
        //======================================

        if (jdec_this->jbl_work)
        {
            jit_err = JDEC_ERR_WORKING;
            break;
        }

        //======================================
        // 设置错误回调跳转代码

        if (0 != setjmp(jdec_this->jerr_mgr.jerr_jmp))
        {
            jit_err = JDEC_ERR_EXCEPTION;
            goto __EXIT_FUNC;
        }

        jdec_ptr = &jdec_this->jdec_obj;

        //======================================
        // 读取 JPEG 图像源基本信息

        jit_err = jdec_load_mode(jdec_this);
        if (JDEC_ERR_OK != jit_err)
        {
            break;
        }

        if (JPEG_HEADER_OK != jpeg_read_header(jdec_ptr, J_TRUE))
        {
            jit_err = JDEC_ERR_READ_HEADER;
            break;
        }

        jdec_this->jinfo.jit_imgw = jdec_ptr->image_width;
        jdec_this->jinfo.jit_imgh = jdec_ptr->image_height;
        jdec_this->jinfo.jit_nchs = jdec_ptr->num_components;
        jdec_this->jinfo.jcs_type = jcs_to_comm(jdec_ptr->jpeg_color_space);

        // 保存返回的 JPEG 图像源基本信息
        if ((J_NULL != jinfo_ptr) && 
            (JPEG_CS_UNKNOWN != jdec_this->jinfo.jcs_type))
        {
            jinfo_ptr->jit_imgw = jdec_this->jinfo.jit_imgw;
            jinfo_ptr->jit_imgh = jdec_this->jinfo.jit_imgh;
            jinfo_ptr->jit_nchs = jdec_this->jinfo.jit_nchs;
            jinfo_ptr->jcs_type = jdec_this->jinfo.jcs_type;
        }

        //======================================
        // 启动解码工作

        // 验证色彩空间的转换，是否支持
        if (!jdec_ccs_valid(JDEC_CCS_MAKE(jcs_conv, jdec_this->jinfo.jcs_type)))
        {
            jit_err = JDEC_ERR_CCS_UNIMPL;
            break;
        }

        // 设置输出像素的色彩空间
        jdec_ptr->out_color_space = jcs_to_lib(JCTL_CS_TYPE(jcs_conv));

        // 启动解码器
        if (!jpeg_start_decompress(jdec_ptr))
        {
            jit_err = JDEC_ERR_START_FAILED;
            break;
        }

        // 标识为工作状态
        jdec_this->jbl_work = J_TRUE;

        //======================================
        jdec_ptr = J_NULL;
        jit_err  = JDEC_ERR_OK;
    } while (0);

    //======================================

__EXIT_FUNC:
    if (J_NULL != jdec_ptr)
    {
        jdec_ptr = J_NULL;
        jdec_shutdown(jdec_this);
    }

    //======================================

    return jit_err;
}

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
            j_uint_t    jut_rows)
{
    JASSERT(jdec_valid(jdec_this));

    j_int_t  jit_err  = JDEC_ERR_UNKNOWN;
    j_uint_t jut_iter = 0;

    do
    {
        //======================================

        if (!jdec_this->jbl_work)
        {
            jit_err = JDEC_ERR_UNSTART;
            break;
        }

        // 若图像像素行已全部读取，则直接退出
        if (jdec_this->jdec_obj.output_scanline >=
            jdec_this->jdec_obj.output_height)
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

        if (jut_rows > jdec_this->jdec_obj.output_height)
        {
            jut_rows = jdec_this->jdec_obj.output_height;
        }

        //======================================
        // 构建接收输出像素行的数组

        jit_err = jdec_grow_rows(jdec_this, jut_rows);
        if (JDEC_ERR_OK != jit_err)
        {
            break;
        }

        for (jut_iter = 0; jut_iter < jut_rows; ++jut_iter)
        {
            jdec_this->jrows.jar_rows[jut_iter] = jmt_pxls;
            jmt_pxls += jit_step;
        }

        //======================================
        // 开始从 解码器 读取像素数据

        // 设置错误回调跳转代码后，再执行解码操作
        if (0 == setjmp(jdec_this->jerr_mgr.jerr_jmp))
        {
            for (jut_iter = 0; jut_iter < jut_rows; )
            {
                jut_iter += jpeg_read_scanlines(
                                &jdec_this->jdec_obj,
                                &jdec_this->jrows.jar_rows[jut_iter],
                                jut_rows - jut_iter);

                if (jdec_this->jdec_obj.output_scanline >=
                    jdec_this->jdec_obj.output_height)
                {
                    break;
                }
            }

            jit_err = (j_int_t)jut_iter;
        }
        else
        {
            // 异常标识
            jit_err = JDEC_ERR_EXCEPTION;

            // 关闭编码器
            jdec_shutdown(jdec_this);
        }

        //======================================
    } while (0);

    return jit_err;
}

/**********************************************************/
/**
 * @brief 在 jdec_read() 完成解码读取后，调用该接口关闭 解码器。
 * 
 * @param [in ] jdec_this : JPEG 解码操作的上下文对象。
 * 
 * @return j_int_t : 错误码，请参看 jdec_errno_t 相关枚举值。
 */
j_int_t jdec_finish(jdec_this_t jdec_this)
{
    JASSERT(jdec_valid(jdec_this));

    j_int_t jit_err = JDEC_ERR_UNKNOWN;

    //======================================

    if (!jdec_this->jbl_work)
    {
        return JDEC_ERR_UNSTART;
    }

    //======================================

    // 设置错误回调跳转代码后，再执行操作
    if (0 == setjmp(jdec_this->jerr_mgr.jerr_jmp))
    {
        jpeg_finish_decompress(&jdec_this->jdec_obj);
        jit_err = JDEC_ERR_OK;
    }
    else
    {
        // 异常处理
        jit_err = JDEC_ERR_EXCEPTION;
    }

    //======================================
    // 关闭解码器

    jdec_shutdown(jdec_this);

    //======================================

    return jit_err;
}

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
            jinfo_ptr_t jinfo_ptr)
{
    j_int_t jit_err;
    j_int_t jit_rows;

    jit_err = jdec_start(jdec_this, jcs_conv, jinfo_ptr);
    if (JDEC_ERR_OK != jit_err)
    {
        return jit_err;
    }

    jit_err = jdec_read(jdec_this, jmt_pxls, jit_step, jdec_this->jinfo.jit_imgh);
    if (jit_err < 0)
    {
        return jit_err;
    }

    jit_rows = jit_err;

    jit_err = jdec_finish(jdec_this);
    if (JDEC_ERR_OK != jit_err)
    {
        return jit_err;
    }

    return jit_rows;
}

////////////////////////////////////////////////////////////////////////////////
