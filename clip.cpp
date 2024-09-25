/**
 * @file clip.cpp
 * Copyright (c) 2021 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2021-01-03
 * @version : 1.0.0.0
 * @brief   : 裁剪出 JPEG 图片的中部子图片。
 */

#include "xtypes.h"
#include "XJPEG_wrapper.h"
#include <stdio.h>

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// for mkdir()

#if (defined(_WIN32) || defined(_WIN64))
#include <direct.h>
#elif defined(__linux__) // linux
#include <sys/stat.h>
#include <sys/types.h>
#else // unknow
#error "The current platform is not supported"
#endif // (defined(_WIN32) || defined(_WIN64))

x_int32_t X_mkdir(x_cstring_t xszt_path, x_uint32_t xut_mode)
{
#if (defined(_WIN32) || defined(_WIN64))
    if (0 != _mkdir(xszt_path))
#elif defined(__linux__) // linux
    if (0 != mkdir(xszt_path, xut_mode))
#else // unknow
#error "The current platform is not supported"
#endif // (defined(_WIN32) || defined(_WIN64))
    {
        if (EEXIST != errno)
            return -1;
    }

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
 * @brief 从文件路径名中，获取 文件扩展名 字符串的起始位置。
 * 
 * @param [in ] xszt_fpath   : 文件路径名。
 * @param [in ] xbt_with_dot : 返回的 文件扩展名，是否带上 “.” 分割号。
 * 
 * @return x_cstring_t : 文件扩展名。
 */
x_cstring_t file_ext_name(x_cstring_t xszt_fpath, x_bool_t xbt_with_dot)
{
    x_char_t * xct_iter = (x_char_t *)xszt_fpath;
    x_char_t * xct_vpos = X_NULL;

    if (X_NULL == xszt_fpath)
    {
        return X_NULL;
    }
    else if (xbt_with_dot && ('.' == xszt_fpath[0]))
    {
        // "." or ".." will return to tail.
        if ('\0' == xszt_fpath[1])
            return xszt_fpath + 1;
        else if (('.' == xszt_fpath[1]) && ('\0' == xszt_fpath[2]))
            return xszt_fpath + 2;
    }

    while (*xct_iter)
    {
        if ('.' == *xct_iter)
            xct_vpos = xct_iter + 1;
        xct_iter += 1;
    }

    if (X_NULL == xct_vpos)
        xct_vpos = xct_iter;
    else if (xbt_with_dot)
        xct_vpos -= 1;

    return (x_cstring_t)xct_vpos;
}

////////////////////////////////////////////////////////////////////////////////

jenc_handle_t           jencoder;
jdec_handle_t           jdecoder;
std::vector< x_byte_t > xrgb_buf;
x_int32_t               xit_width;
x_int32_t               xit_height;

x_int32_t dec_to_buf(x_cstring_t xszt_file, j_int_t jit_ctrlcs)
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    xit_err = jdecoder.config_src(JCTRL_MODE_FILE, (j_handle_t)xszt_file, 0);
    if (JDEC_ERR_OK != xit_err)
    {
        printf("jdec_config_src(..., [%s], ...) return error: %s\n",
               xszt_file, jdec_errno_name(xit_err));
        return xit_err;
    }

    jpeg_info_t jinfo;
    xit_err = jdecoder.src_info(&jinfo);
    if (JDEC_ERR_OK != xit_err)
    {
        printf("jdec_src_info() return error: %s\n", jdec_errno_name(xit_err));
        return xit_err;
    }

    // resize buffer
    if (xrgb_buf.size() < (x_size_t)(4 * jinfo.jit_width * jinfo.jit_height))
    {
        xrgb_buf.resize((x_size_t)(4 * jinfo.jit_width * jinfo.jit_height));
    }

    xit_err = jdecoder.src_to_rgb(
                        xrgb_buf.data(),
                        0,
                        (j_uint_t)xrgb_buf.size(),
                        &xit_width,
                        &xit_height,
                        jit_ctrlcs);
    if (JDEC_ERR_OK != xit_err)
    {
        printf("jdec_src_to_rgb(..., jit_ctrlcs[0x%08X]) return error: %s\n",
               jit_ctrlcs, jdec_errno_name(xit_err));
        return xit_err;
    }

    return JDEC_ERR_OK;
}

x_int32_t enc_to_file(x_cstring_t xszt_file, j_int_t jit_ctrlcs)
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    xit_err = jencoder.config_dst(JCTRL_MODE_FILE, (j_handle_t)xszt_file, 0);
    if (JDEC_ERR_OK != xit_err)
    {
        printf("jenc_config_dst(..., [%s], ...) return error: %s\n",
               xszt_file, jdec_errno_name(xit_err));
        return xit_err;
    }

    xit_err = jencoder.rgb_to_dst(
                        xrgb_buf.data(),
                        0,
                        xit_width,
                        xit_height,
                        jit_ctrlcs);
    if (JENC_ERR_OK == xit_err)
    {
        printf("jenc_rgb_to_dst(..., jit_ctrlcs[0x%08X]) return error: %s\n",
               jit_ctrlcs, jenc_errno_name(xit_err));
        return xit_err;
    }

    return xit_err;
}

int main(int argc, char * argv[])
{
    if (2 != argc)
    {
        printf("usage: %s < jpeg file >\n", argv[0]);
        return -1;
    }

    std::string xstr_file = argv[1];

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
