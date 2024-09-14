/**
 * @file jerase.c
 * Copyright (c) 2024 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2024-09-14
 * @version : 1.0.0.0
 * @brief   : 擦除图片中的指定矩形区域。
 */

#include "../XJPEG_wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////

/**
 * @struct jrect_t
 * @brief  矩形区域描述结构体。
 */
typedef struct jrect_t
{
    j_int_t jit_x; ///< X 坐标
    j_int_t jit_y; ///< Y 坐标
    j_int_t jit_w; ///< 宽度
    j_int_t jit_h; ///< 高度
} jrect_t;

j_cstring_t jsz_ifile = J_NULL;
j_cstring_t jsz_ofile = J_NULL;
jrect_t     jrc_area  = { 0, 0, 0, 0 };
j_uint_t    jut_eclr  = 0x00FFFFFF;

/**********************************************************/
/**
 * @brief 解析命令行的输入参数，初始化工作参数。
 * 
 * @param [in ] jit_argc : 输入的命令行参数数量。
 * @param [in ] jsz_argv : 输入的命令行参数字符串数组。
 * 
 * @return j_int_t : 成功，返回 0；失败，返回 -1 。
 */
j_int_t jerase_getopt(j_int_t jit_argc, j_cstring_t jsz_argv[]);

/**********************************************************/
/**
 * @brief 执行区域擦除工作。
 */
j_void_t jerase_proc(void);

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 输出程序帮助信息。
 */
void usage(const char * xszt_name)
{
    printf("usage: %s -i input -o output -x xpos -y ypos -w width -h height [-c color]\n"
           "       -i : input jpeg file.\n"
           "       -o : output jpeg file.\n"
           "       -x : the x position of the erase area.\n"
           "       -y : the y position of the erase area.\n"
           "       -w : the width of the erase area.\n"
           "       -h : the height of the erase area.\n"
           "       -c : the erase color, default value: 0x00FFFFFF.\n",
           xszt_name);
}

/**********************************************************/
/**
 * @brief 程序入口的 主函数。
 */
int main(int argc, char * argv[])
{
    //======================================

    if (argc < 13)
    {
        usage(argv[0]);
        return -1;
    }

    //======================================

    if (0 != jerase_getopt(argc, argv))
    {
        return -1;
    }

    jerase_proc();

    //======================================

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 字符串忽略大小写的比对操作。
 *
 * @param [in ] xszt_lcmp : 比较操作的左值字符串。
 * @param [in ] xszt_rcmp : 比较操作的右值字符串。
 *
 * @return int
 *         - xszt_lcmp <  xszt_rcmp，返回 <= -1；
 *         - xszt_lcmp == xszt_rcmp，返回 ==  0；
 *         - xszt_lcmp >  xszt_rcmp，返回 >=  1；
 */
static int xstr_icmp(const char * xszt_lcmp, const char * xszt_rcmp)
{
    int xit_lvalue = 0;
    int xit_rvalue = 0;

    if (xszt_lcmp == xszt_rcmp)
        return 0;
    if (NULL == xszt_lcmp)
        return -1;
    if (NULL == xszt_rcmp)
        return 1;

    do
    {
        if (((xit_lvalue = (*(xszt_lcmp++))) >= 'A') && (xit_lvalue <= 'Z'))
            xit_lvalue -= ('A' - 'a');

        if (((xit_rvalue = (*(xszt_rcmp++))) >= 'A') && (xit_rvalue <= 'Z'))
            xit_rvalue -= ('A' - 'a');

    } while (xit_lvalue && (xit_lvalue == xit_rvalue));

    return (xit_lvalue - xit_rvalue);
}

/**********************************************************/
/**
 * @brief 解码 JPEG 图片文件 至 指定缓存中。
 * 
 * @param [in ] jdec_cptr  : JPEG 解码操作的工作对象。
 * @param [in ] jsz_path   : JPEG 图片文件路径。
 * @param [out] jmem_buf   : 解码输出的缓存。
 * @param [out] jit_size   : 缓存大小。
 * @param [out] jinfo_ptr  : 操作返回的 JPEG 图像基本信息。
 * 
 * @return j_int_t : 操作错误码，为 JDEC_ERR_OK 时，表示成功。
 */
j_int_t jpeg_decode(
            jdec_ctxptr_t jdec_cptr,
            j_cstring_t   jsz_path,
            j_mem_t     * jmem_buf,
            j_int_t     * jit_size,
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

        printf("input image: %s\n", jsz_path);
        printf("image size : %d x %d\n", jinfo_ptr->jit_width, jinfo_ptr->jit_height);
        *jit_size = 4 * jinfo_ptr->jit_width * (jinfo_ptr->jit_height + 16);
        assert(*jit_size > 0);
        *jmem_buf = (j_mem_t)malloc(*jit_size);
        assert(J_NULL != *jmem_buf);

        jdec_set_align(jdec_cptr, J_FALSE);
        jit_errno = jdec_src_to_rgb(
                            jdec_cptr,
                            *jmem_buf,
                            0,
                            *jit_size,
                            J_NULL,
                            J_NULL,
                            JCTRL_CS_BGR);
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

/**********************************************************/
/**
 * @brief 解析命令行的输入参数，初始化工作参数。
 * 
 * @param [in ] jit_argc : 输入的命令行参数数量。
 * @param [in ] jsz_argv : 输入的命令行参数字符串数组。
 * 
 * @return j_int_t : 成功，返回 0；失败，返回 -1 。
 */
j_int_t jerase_getopt(j_int_t jit_argc, j_cstring_t jsz_argv[])
{
    j_int_t jit_iter = 0;

    //======================================
    // 解析参数

    for (; jit_iter < jit_argc; )
    {
        if ((0 == xstr_icmp("-i", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            jsz_ifile = jsz_argv[++jit_iter];
            continue;
        }

        if ((0 == xstr_icmp("-o", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            jsz_ofile = jsz_argv[++jit_iter];
            continue;
        }

        if ((0 == xstr_icmp("-x", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            jrc_area.jit_x = (j_int_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        if ((0 == xstr_icmp("-y", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            jrc_area.jit_y = (j_int_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        if ((0 == xstr_icmp("-w", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            jrc_area.jit_w = (j_int_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        if ((0 == xstr_icmp("-h", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            jrc_area.jit_h = (j_int_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        if ((0 == xstr_icmp("-c", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            jut_eclr = (j_uint_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        ++jit_iter;
    }

    //======================================
    // 验证工作参数的有效性

    if (J_NULL == jsz_ifile)
    {
        usage(jsz_argv[0]);
        printf("Error: no input file was specified!\n");
        return -1;
    }

    if (J_NULL == jsz_ofile)
    {
        usage(jsz_argv[0]);
        printf("Error: no output file was specified!\n");
        return -1;
    }

    if ((jrc_area.jit_w <= 0) || (jrc_area.jit_h <= 0))
    {
        usage(jsz_argv[0]);
        printf("Error: the area[%d, %d, %d, %d] is empty!\n",
               jrc_area.jit_x,
               jrc_area.jit_y,
               jrc_area.jit_w,
               jrc_area.jit_h);
        return -1;
    }

    //======================================

    return 0;
}

/**********************************************************/
/**
 * @brief 执行区域擦除工作。
 */
j_void_t jerase_proc(void)
{
    j_int_t       jit_errno;

    jdec_ctxptr_t jdec_cptr = J_NULL;
    jenc_ctxptr_t jenc_cptr = J_NULL;

    j_int_t       jit_size  = -1;
    j_mem_t       jmem_buf  = J_NULL;
    jpeg_info_t   jinfo_ctx;

    j_int_t jit_x;
    j_int_t jit_y;
    j_int_t jit_i;

    j_int_t jit_l;
    j_int_t jit_t;
    j_int_t jit_r;
    j_int_t jit_b;

    //======================================
    // 解码，获取 BGR 图像像素数据

    jdec_cptr = jdec_alloc(J_NULL);
    assert(J_NULL != jdec_cptr);

    jit_errno = jpeg_decode(jdec_cptr, jsz_ifile, &jmem_buf, &jit_size, &jinfo_ctx);
    if (JDEC_ERR_OK != jit_errno)
    {
        goto __EXIT_FUNC;
    }

    //======================================
    // 执行区域擦除操作

    jit_l = jrc_area.jit_x % jinfo_ctx.jit_width;
    if (jit_l < 0) jit_l += jinfo_ctx.jit_width;

    jit_t = jrc_area.jit_y % jinfo_ctx.jit_height;
    if (jit_t < 0) jit_t += jinfo_ctx.jit_height;

    jit_r = jit_l + jrc_area.jit_w;
    if (jit_r > jinfo_ctx.jit_width) jit_r = jinfo_ctx.jit_width;

    jit_b = jit_t + jrc_area.jit_h;
    if (jit_b > jinfo_ctx.jit_height) jit_b = jinfo_ctx.jit_height;

    printf("erase rectangle: [ %d, %d, %d, %d ]\n",
           jit_l,
           jit_t,
           jit_r,
           jit_b);

    for (jit_y = jit_t; jit_y < jit_b; ++jit_y)
    {
        for (jit_x = jit_l; jit_x < jit_r; ++jit_x)
        {
            jit_i = 3 * (jinfo_ctx.jit_width * jit_y + jit_x);
            jmem_buf[jit_i + 0] = (j_uchar_t)((jut_eclr & 0x000000FF) >>  0); // B
            jmem_buf[jit_i + 1] = (j_uchar_t)((jut_eclr & 0x0000FF00) >>  8); // G
            jmem_buf[jit_i + 2] = (j_uchar_t)((jut_eclr & 0x00FF0000) >> 16); // R
        }
    }

    //======================================

    jenc_cptr = jenc_alloc(J_NULL);
    assert(J_NULL != jenc_cptr);

    jit_errno = jenc_config_dst(jenc_cptr, JCTRL_MODE_FILE, (j_handle_t)jsz_ofile, 0);
    if (JDEC_ERR_OK != jit_errno)
    {
        printf("jenc_config_dst() return error: %s\n", jenc_errno_name(jit_errno));
        goto __EXIT_FUNC;
    }

    jit_errno = jenc_rgb_to_dst(
                    jenc_cptr,
                    jmem_buf,
                    jinfo_ctx.jit_width * 3,
                    jinfo_ctx.jit_width,
                    jinfo_ctx.jit_height,
                    JENC_BGR_TO_RGB);
    if (JDEC_ERR_OK != jit_errno)
    {
        printf("jenc_rgb_to_dst() return error: %s\n", jenc_errno_name(jit_errno));
        goto __EXIT_FUNC;
    }

    printf("erase OK, output image: %s\n", jsz_ofile);

    //======================================
__EXIT_FUNC:

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

    //======================================
}

////////////////////////////////////////////////////////////////////////////////
