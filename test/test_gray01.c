/**
 * @file test_gray01.c
 * Copyright (c) 2023 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2023-11-23
 * @version : 1.0.0.0
 * @brief   : JPEG 编码/解码 灰度图。
 */

#include "../XJPEG_wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 解码 JPEG 图片文件 至 指定缓存中。
 * 
 * @param [in ] jdec_cptr  : JPEG 解码操作的工作对象。
 * @param [in ] jsz_path   : JPEG 图片文件路径。
 * @param [in ] jit_ctrlcs : 解码输出的像素格式（参看 jctrl_color_space_t 相关枚举值）。
 * @param [out] jmem_buf   : 解码输出的缓存。
 * @param [in ] jut_size   : 缓存大小。
 * @param [out] jinfo_ptr  : 操作返回的 JPEG 图像基本信息。
 * 
 * @return j_int_t : 操作错误码，为 JDEC_ERR_OK 时，表示成功。
 */
j_int_t jpeg_decode(
            jdec_ctxptr_t jdec_cptr,
            j_cstring_t   jsz_path,
            j_int_t       jit_ctrlcs,
            j_mem_t       jmem_buf,
            j_uint_t      jut_size,
            jinfo_ptr_t   jinfo_ptr)
{
    j_int_t jit_errno = JDEC_ERR_UNKNOW;

    do
    {
        //======================================

        jit_errno = jdec_config_src(jdec_cptr, JCTRL_MODE_FILE, (j_handle_t)jsz_path, 0);
        if (JDEC_ERR_OK != jit_errno)
        {
            printf("jdec_config_src() return error : %s\n", jdec_errno_name(jit_errno));
            break;
        }

        jit_errno = jdec_src_info(jdec_cptr, jinfo_ptr);
        if (JDEC_ERR_OK != jit_errno)
        {
            printf("jdec_src_info() return error : %s\n", jdec_errno_name(jit_errno));
            break;
        }

        printf("decode image size: %d x %d\n", jinfo_ptr->jit_width, jinfo_ptr->jit_height);

        jit_errno = jdec_src_to_rgb(
                            jdec_cptr,
                            jmem_buf,
                            jinfo_ptr->jit_width * ((jit_ctrlcs & 0xFF) >> 3),
                            jut_size,
                            J_NULL,
                            J_NULL,
                            jit_ctrlcs);
        if (JDEC_ERR_OK != jit_errno)
        {
            printf("jdec_src_to_rgb() return error : %s\n", jdec_errno_name(jit_errno));
            break;
        }

        //======================================
        jit_errno = JDEC_ERR_OK;
    } while (0);
    
    return jit_errno;
}

////////////////////////////////////////////////////////////////////////////////

void usage(const char * xszt_name)
{
    printf("usage: %s in_file mcs out_file\n"
           "       -mcs 0: JCTRL_CS_GRAY\n"
           "            1: JCTRL_CS_RGB \n"
           "            2: JCTRL_CS_BGR \n"
           "            3: JCTRL_CS_RGBA\n"
           "            4: JCTRL_CS_BGRA\n"
           "            5: JCTRL_CS_ARGB\n"
           "            6: JCTRL_CS_ABGR\n",
           xszt_name);
}

/**********************************************************/
/**
 * @brief 程序入口的 主函数。
 */
int main(int argc, char * argv[])
{
    j_cstring_t jsz_ifile  = J_NULL;
    j_cstring_t jsz_ofile  = J_NULL;
    j_int_t     jit_ctrlcs = JCTRL_CS_UNKNOW;

    jdec_ctxptr_t jdec_cptr = J_NULL;
    jenc_ctxptr_t jenc_cptr = J_NULL;

    jpeg_info_t jinfo_ctx;

    j_uint_t jut_size = 20 * 1024 * 1024;
    j_mem_t  jmem_buf = J_NULL;

    j_int_t jit_errno;

    //======================================

    if ((4 != argc) || ((2 == argc) && (0 == strcmp(argv[1], "--help"))))
    {
        usage(argv[0]);
        goto __EXIT;
    }

    jsz_ifile = argv[1];
    jsz_ofile = argv[3];

    jit_ctrlcs = (j_int_t)strtol(argv[2], NULL, 10);
    switch (jit_ctrlcs)
    {
    case 0: jit_ctrlcs = JCTRL_CS_GRAY; break;
    case 1: jit_ctrlcs = JCTRL_CS_RGB ; break;
    case 2: jit_ctrlcs = JCTRL_CS_BGR ; break;
    case 3: jit_ctrlcs = JCTRL_CS_RGBA; break;
    case 4: jit_ctrlcs = JCTRL_CS_BGRA; break;
    case 5: jit_ctrlcs = JCTRL_CS_ARGB; break;
    case 6: jit_ctrlcs = JCTRL_CS_ABGR; break;

    default:
        usage(argv[0]);
        goto __EXIT;
        break;
    }

    //======================================

    jdec_cptr = jdec_alloc(J_NULL);
    assert(J_NULL != jdec_cptr);

    jenc_cptr = jenc_alloc(J_NULL);
    assert(J_NULL != jenc_cptr);

    jmem_buf = (j_mem_t)malloc(jut_size);
    assert(J_NULL != jmem_buf);

    //======================================

    jit_errno = jpeg_decode(jdec_cptr, jsz_ifile, jit_ctrlcs, jmem_buf, jut_size, &jinfo_ctx);
    if (JDEC_ERR_OK != jit_errno)
    {
        goto __EXIT;
    }

    //======================================

    jit_errno = jenc_config_dst(jenc_cptr, JCTRL_MODE_FILE, (j_handle_t)jsz_ofile, 0);
    if (JDEC_ERR_OK != jit_errno)
    {
        printf("jenc_config_dst() return error: %s\n", jenc_errno_name(jit_errno));
        goto __EXIT;
    }

    jit_errno = jenc_rgb_to_dst(
                    jenc_cptr,
                    jmem_buf,
                    jinfo_ctx.jit_width * ((jit_ctrlcs & 0xFF) >> 3),
                    jinfo_ctx.jit_width,
                    jinfo_ctx.jit_height,
                    JENC_CTRLCS_MAKE(jit_ctrlcs, JPEG_CS_GRAYSCALE));
    if (JDEC_ERR_OK != jit_errno)
    {
        printf("jenc_rgb_to_dst() return error: %s\n", jenc_errno_name(jit_errno));
        goto __EXIT;
    }

    //======================================
__EXIT:
    if (J_NULL != jdec_cptr)
    {
        jdec_release(jdec_cptr);
        jdec_cptr = J_NULL;
    }

    if (J_NULL != jenc_cptr)
    {
        jenc_release(jenc_cptr);
        jenc_cptr = J_NULL;
    }

    if (J_NULL != jmem_buf)
    {
        free(jmem_buf);
        jmem_buf = J_NULL;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
