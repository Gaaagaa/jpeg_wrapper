/**
 * @file test.cpp
 * Copyright (c) Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2020-12-23
 * @version : 1.0.0.0
 * @brief   : 提取图片的中间四分之一的子图片，
 *            用于测试封装 JPEG 编码/解码 操作 API。
 */

#include "xtypes.h"
#include "XJPEG_wrapper.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif // _MSC_VER

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
    if (0 != mkdir(xszt_path))
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

////////////////////////////////////////////////////////////////////////////////

// #define TEST_ONLY_ONE

#ifndef TEST_ONLY_ONE
#define ARGS_LIST_LEN   6
#define ARGS_TEXT_LEN   260
#endif // TEST_ONLY_ONE

////////////////////////////////////////////////////////////////////////////////

#ifndef TEST_ONLY_ONE

class subimage
{
public:

subimage(void)
{

}

~subimage(void)
{

}

#endif // TEST_ONLY_ONE

//======================================
// 解码/编码 操作的 RGB 图像相关的变量

std::vector< x_byte_t > xvec_rgb;
x_int32_t xit_width  = 0;
x_int32_t xit_height = 0;
x_int32_t xit_rgbxx  = JCTRL_CS_RGB;

//======================================
// 解码操作相关的变量

jdec_handle_t jdecoder;
x_int32_t     xit_dmode = JCTRL_MODE_UNKNOW;
std::string   xstr_input;

//======================================
// 编码操作相关的变量

jenc_handle_t jencoder;
x_int32_t     xit_emode = JCTRL_MODE_UNKNOW;
std::string   xstr_output;

//======================================

x_int32_t dec_mode_mem(void)
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    printf("[dec_mode_mem]\n");

    //======================================
    // 读取 JPEG 图片文件数据

    std::ifstream xfr_stream(xstr_input.c_str(), std::ios_base::binary);
    if (!xfr_stream.is_open())
    {
        printf("open input file [%s] error!", xstr_input.c_str());
        return X_ERR_UNKNOW;
    }

    x_uint32_t xut_size = 4 * 1024 * 1024;
    std::vector< x_byte_t > xvec_jpeg(xut_size, 0);

    xut_size = (x_uint32_t)(xfr_stream.read((x_char_t *)xvec_jpeg.data(), xut_size).gcount());
    if (xut_size <= 0)
    {
        printf("input file [%s] read() return 0!\n", xstr_input.c_str());
        return X_ERR_UNKNOW;
    }

    if (!xfr_stream.eof())
    {
        printf("input file [%s] size > 4 MB !\n", xstr_input.c_str());
        return X_ERR_UNKNOW;
    }

    xfr_stream.close();

    printf("input file size : %d\n", xut_size);

    //======================================

    do
    {

        //======================================
        // 配置解码输入源，并读取 JPEG 图片基本信息

        xit_err = jdecoder.config_src(JCTRL_MODE_MEM, (j_handle_t)xvec_jpeg.data(), xut_size);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.config_src() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        jpeg_info_t jinfo;
        xit_err = jdecoder.src_info(&jinfo);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.src_info() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        printf(
            "input jpeg info: [width, height, channels, clrspace] = [%d, %d, %d, %d]\n",
            jinfo.jit_width, jinfo.jit_height, jinfo.jit_channels, jinfo.jit_cstype);

        //======================================
        // 解码操作

        xit_width  = jinfo.jit_width;
        xit_height = jinfo.jit_height;
        xvec_rgb.resize(std::abs(4 * xit_width * xit_height), 0);

        xit_err = jdecoder.src_to_rgb(
            xvec_rgb.data(), 0, (x_uint32_t)xvec_rgb.size(), J_NULL, J_NULL, xit_rgbxx);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.src_to_rgb() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        printf("succeed in decode!\n");

        //======================================
        xit_err = X_ERR_OK;
    } while (0);

    //======================================

    return X_ERR_OK;
}

x_int32_t dec_mode_fio(void)
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    printf("[dec_mode_fio]\n");

    //======================================
    // 打开 JPEG 图片文件流

    j_fio_t jfio_ptr = fopen(xstr_input.c_str(), "rb");
    if (J_NULL == jfio_ptr)
    {
        printf("fopen([%s], \"rb\") failed!\n", xstr_input.c_str());
        return X_ERR_UNKNOW;
    }

    //======================================

    do
    {

        //======================================
        // 配置解码输入源，并读取 JPEG 图片基本信息

        xit_err = jdecoder.config_src(JCTRL_MODE_FIO, (j_handle_t)jfio_ptr, 0);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.config_src() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        jpeg_info_t jinfo;
        xit_err = jdecoder.src_info(&jinfo);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.src_info() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        printf(
            "input jpeg info: [width, height, channels, clrspace] = [%d, %d, %d, %d]\n",
            jinfo.jit_width, jinfo.jit_height, jinfo.jit_channels, jinfo.jit_cstype);

        //======================================
        // 解码操作

        xit_width  = jinfo.jit_width;
        xit_height = jinfo.jit_height;
        xvec_rgb.resize(std::abs(4 * xit_width * xit_height), 0);

        xit_err = jdecoder.src_to_rgb(
            xvec_rgb.data(), 0, (x_uint32_t)xvec_rgb.size(), J_NULL, J_NULL, xit_rgbxx);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.src_to_rgb() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        printf("succeed in decode!\n");

        //======================================
        xit_err = X_ERR_OK;
    } while (0);

    //======================================

    fclose(jfio_ptr);
    jfio_ptr = J_NULL;

    return xit_err;
}

x_int32_t dec_mode_file(void)
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    printf("[dec_mode_file]\n");

    do
    {

        //======================================
        // 配置解码输入源，并读取 JPEG 图片基本信息

        xit_err = jdecoder.config_src(JCTRL_MODE_FILE, (j_handle_t)xstr_input.c_str(), 0);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.config_src() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        jpeg_info_t jinfo;
        xit_err = jdecoder.src_info(&jinfo);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.src_info() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        printf(
            "input jpeg info: [width, height, channels, clrspace] = [%d, %d, %d, %d]\n",
            jinfo.jit_width, jinfo.jit_height, jinfo.jit_channels, jinfo.jit_cstype);

        //======================================
        // 解码操作

        xit_width  = jinfo.jit_width;
        xit_height = jinfo.jit_height;
        xvec_rgb.resize(std::abs(4 * xit_width * xit_height), 0);

        xit_err = jdecoder.src_to_rgb(
            xvec_rgb.data(), 0, (x_uint32_t)xvec_rgb.size(), J_NULL, J_NULL, xit_rgbxx);
        if (JDEC_ERR_OK != xit_err)
        {
            printf("jdecoder.src_to_rgb() return error : %s\n", jdec_errno_name(xit_err));
            break;
        }

        printf("succeed in decode!\n");

        //======================================
        xit_err = X_ERR_OK;
    } while (0);

    //======================================

    return xit_err;
}

x_int32_t enc_mode_mem(void)
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    printf("[enc_mode_mem]\n");

    //======================================
    // 配置编码输出源

    xit_err = jencoder.config_dst(JCTRL_MODE_MEM, J_NULL, 0);
    if (JENC_ERR_OK != xit_err)
    {
        printf("jencoder.config_dst(JCTRL_MODE_MEM, J_NULL, 0) return error : %s\n",
               jenc_errno_name(xit_err));
        return xit_err;
    }
    
    //======================================
    // 执行编码操作

    j_mem_t jmem_iptr  = J_NULL;
    j_int_t jit_stride = 0;
    j_int_t jit_width  = xit_width  / 2;
    j_int_t jit_height = xit_height / 2;

    switch (xit_rgbxx)
    {
    case JCTRL_CS_RGB:
    case JCTRL_CS_BGR:
        {
            jit_stride = (3 * xit_width + 3) & ~3;
            jmem_iptr  = xvec_rgb.data() + jit_stride * xit_height / 4 + 3 * xit_width / 4;
        }
        break;

    case JCTRL_CS_RGBA:
    case JCTRL_CS_BGRA:
    case JCTRL_CS_ARGB:
    case JCTRL_CS_ABGR:
        {
            jit_stride = 4 * xit_width;
            jmem_iptr  = xvec_rgb.data() + jit_stride * xit_height / 4 + xit_width;
        }
        break;
    
    default:
        break;
    }

    xit_err = jencoder.rgb_to_dst(
        jmem_iptr, jit_stride, jit_width, jit_height, xit_rgbxx);
    if (xit_err < 0)
    {
        printf("jencoder.rgb_to_dst(, [%d], [%d], [%d], ) return error : %s\n",
               jit_stride, jit_width, jit_height, jenc_errno_name(xit_err));
        return xit_err;
    }

    //======================================
    // 输出压缩后的 JPEG 数据到文件

    std::ofstream ofw_stream(xstr_output.c_str(), std::ios_base::binary);
    if (!ofw_stream.is_open())
    {
        printf("open output file [%s] error!\n", xstr_output.c_str());
        return X_ERR_UNKNOW;
    }

    ofw_stream.write((x_char_t *)jencoder.cached_data(), jencoder.cached_size());
    ofw_stream.close();

    printf("succeed in encode!\n");

    //======================================

    return xit_err;
}

x_int32_t enc_mode_fio(void)
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    printf("[enc_mode_fio]\n");

    //======================================
    // 打开输出文件流，并配置编码输出源

    j_fio_t jfio_ptr = fopen(xstr_output.c_str(), "wb+");
    if (J_NULL == jfio_ptr)
    {
        printf("fopen([%s], \"wb+\") error!\n", xstr_output.c_str());
        return X_ERR_UNKNOW;
    }

    xit_err = jencoder.config_dst(JCTRL_MODE_FIO, (j_handle_t)jfio_ptr, 0);
    if (JENC_ERR_OK != xit_err)
    {
        printf("jencoder.config_dst(JCTRL_MODE_FIO, [], 0) return error : %s\n",
               jenc_errno_name(xit_err));

        fclose(jfio_ptr);
        jfio_ptr = J_NULL;

        return xit_err;
    }
    
    //======================================
    // 执行编码操作

    j_mem_t jmem_iptr  = J_NULL;
    j_int_t jit_stride = 0;
    j_int_t jit_width  = xit_width  / 2;
    j_int_t jit_height = xit_height / 2;

    switch (xit_rgbxx)
    {
    case JCTRL_CS_RGB:
    case JCTRL_CS_BGR:
        {
            jit_stride = (3 * xit_width + 3) & ~3;
            jmem_iptr  = xvec_rgb.data() + jit_stride * xit_height / 4 + 3 * xit_width / 4;
        }
        break;

    case JCTRL_CS_RGBA:
    case JCTRL_CS_BGRA:
    case JCTRL_CS_ARGB:
    case JCTRL_CS_ABGR:
        {
            jit_stride = 4 * xit_width;
            jmem_iptr  = xvec_rgb.data() + jit_stride * xit_height / 4 + xit_width;
        }
        break;
    
    default:
        break;
    }

    xit_err = jencoder.rgb_to_dst(
        jmem_iptr, jit_stride, jit_width, jit_height, xit_rgbxx);
    if (xit_err < 0)
    {
        printf("jencoder.rgb_to_dst(, [%d], [%d], [%d], ) return error : %s\n",
               jit_stride, jit_width, jit_height, jenc_errno_name(xit_err));

        fclose(jfio_ptr);
        jfio_ptr = J_NULL;

        return xit_err;
    }

    printf("succeed in encode!\n");

    //======================================

    return xit_err;
}

x_int32_t enc_mode_file(void)
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    printf("[enc_mode_file]\n");

    //======================================
    // 并配置编码输出源

    xit_err = jencoder.config_dst(JCTRL_MODE_FILE, (j_handle_t)xstr_output.c_str(), 0);
    if (JENC_ERR_OK != xit_err)
    {
        printf("jencoder.config_dst(JCTRL_MODE_FILE, [%s], 0) return error : %s\n",
               xstr_output.c_str(), jenc_errno_name(xit_err));
        return xit_err;
    }

    //======================================
    // 执行编码操作

    j_mem_t jmem_iptr  = J_NULL;
    j_int_t jit_stride = 0;
    j_int_t jit_width  = xit_width  / 2;
    j_int_t jit_height = xit_height / 2;

    switch (xit_rgbxx)
    {
    case JCTRL_CS_RGB:
    case JCTRL_CS_BGR:
        {
            jit_stride = (3 * xit_width + 3) & ~3;
            jmem_iptr  = xvec_rgb.data() + jit_stride * xit_height / 4 + 3 * xit_width / 4;
        }
        break;

    case JCTRL_CS_RGBA:
    case JCTRL_CS_BGRA:
    case JCTRL_CS_ARGB:
    case JCTRL_CS_ABGR:
        {
            jit_stride = 4 * xit_width;
            jmem_iptr  = xvec_rgb.data() + jit_stride * xit_height / 4 + xit_width;
        }
        break;
    
    default:
        break;
    }

    xit_err = jencoder.rgb_to_dst(
        jmem_iptr, jit_stride, jit_width, jit_height, xit_rgbxx);
    if (xit_err < 0)
    {
        printf("jencoder.rgb_to_dst(, [%d], [%d], [%d], ) return error : %s\n",
               jit_stride, jit_width, jit_height, jenc_errno_name(xit_err));
        return xit_err;
    }

    printf("succeed in encode!\n");

    //======================================

    return xit_err;
}

x_void_t usage(void)
{
    printf("subimage < dec mode > < in > < enc mode > < out > [ rgb: rgb ]\n"
           "      dec mode : mem | fio | file .                           \n"
           "      in       : the jpeg image filepath of input .           \n"
           "      enc mode : mem | fio | file .                           \n"
           "      out      : the jpeg subimage filepath of output .       \n"
           "      rgb      : rgb | bgr | rgba | bgra | argb | abgr .      \n");
}

x_int32_t get_mode(x_cstring_t xszt_mode)
{
    if (0 == xstr_icmp(xszt_mode, "mem"))
        return JCTRL_MODE_MEM;
    if (0 == xstr_icmp(xszt_mode, "fio"))
        return JCTRL_MODE_FIO;
    if (0 == xstr_icmp(xszt_mode, "file"))
        return JCTRL_MODE_FILE;
    return JCTRL_MODE_UNKNOW;
}

x_int32_t get_rgbxx(x_cstring_t xszt_rgbxx)
{
    if (0 == xstr_icmp(xszt_rgbxx, "rgb"))
        return JCTRL_CS_RGB;
    if (0 == xstr_icmp(xszt_rgbxx, "bgr"))
        return JCTRL_CS_BGR;
    if (0 == xstr_icmp(xszt_rgbxx, "rgba"))
        return JCTRL_CS_RGBA;
    if (0 == xstr_icmp(xszt_rgbxx, "bgra"))
        return JCTRL_CS_BGRA;
    if (0 == xstr_icmp(xszt_rgbxx, "argb"))
        return JCTRL_CS_ARGB;
    if (0 == xstr_icmp(xszt_rgbxx, "abgr"))
        return JCTRL_CS_ABGR;
    return JCTRL_CS_UNKNOW;
}

#ifndef TEST_ONLY_ONE
int main(int argc, char argv[ARGS_LIST_LEN][ARGS_TEXT_LEN])
#else // !TEST_ONLY_ONE
int main(int argc, char * argv[])
#endif // TEST_ONLY_ONE
{
    x_int32_t xit_err = X_ERR_UNKNOW;

    //======================================

    if (argc < 5)
    {
        usage();
        return -1;
    }

    //======================================
    // 解析输入参数

    xit_dmode = get_mode(argv[1]);
    if (JCTRL_MODE_UNKNOW == xit_dmode)
    {
        usage();
        printf("args < decode mode: %s > error!\n", argv[1]);
        return -1;
    }

    xstr_input = argv[2];

    xit_emode = get_mode(argv[3]);
    if (JCTRL_MODE_UNKNOW == xit_emode)
    {
        usage();
        printf("args < encode mode: %s > error!\n", argv[3]);
        return -1;
    }

    xstr_output = argv[4];

    if (argc >= 6)
    {
        xit_rgbxx = get_rgbxx(argv[5]);
        if (JCTRL_CS_UNKNOW == xit_rgbxx)
        {
            usage();
            printf("args [ rgb: %s ] error! use default rgb.\n", argv[6]);
            xit_rgbxx = JCTRL_CS_RGB;
        }
    }

    //======================================
    // 解码操作

    switch (xit_dmode)
    {
    case JCTRL_MODE_MEM : xit_err = dec_mode_mem (); break;
    case JCTRL_MODE_FIO : xit_err = dec_mode_fio (); break;
    case JCTRL_MODE_FILE: xit_err = dec_mode_file(); break;
    default: xit_err = X_ERR_UNKNOW; break;
    }
    if (X_ERR_OK != xit_err)
    {
        printf("decode proc return error : %d\n", xit_err);
        return xit_err;
    }

    //======================================
    // 编码操作

    switch (xit_emode)
    {
    case JCTRL_MODE_MEM : xit_err = enc_mode_mem (); break;
    case JCTRL_MODE_FIO : xit_err = enc_mode_fio (); break;
    case JCTRL_MODE_FILE: xit_err = enc_mode_file(); break;
    default: xit_err = X_ERR_UNKNOW; break;
    }
    if (X_ERR_OK != xit_err)
    {
        printf("encode proc return error : %d\n", xit_err);
        return xit_err;
    }

    //======================================

    return 0;
}

#ifndef TEST_ONLY_ONE
};
#endif // TEST_ONLY_ONE

////////////////////////////////////////////////////////////////////////////////

#ifndef TEST_ONLY_ONE

x_cstring_t file_base_name(x_cstring_t xszt_fpath)
{
    x_char_t * xct_iter = (x_char_t *)xszt_fpath;
    x_char_t * xct_vpos = (x_char_t *)xszt_fpath;

    if (X_NULL == xct_vpos)
    {
        return X_NULL;
    }

    while (*xct_iter)
    {
        if (('/' == *xct_iter) || ('\\' == *xct_iter))
            xct_vpos = xct_iter + 1;
        xct_iter += 1;
    }

    return (x_cstring_t)xct_vpos;
}

int main(int argc, char * argv[])
{
    //======================================

    x_char_t xct_argv[][ARGS_LIST_LEN][ARGS_TEXT_LEN] =
    {
        { "subimage", "mem" , "", "mem" , "", "rgb"  },
        { "subimage", "mem" , "", "mem" , "", "bgr"  },
        { "subimage", "mem" , "", "mem" , "", "rgba" },
        { "subimage", "mem" , "", "mem" , "", "bgra" },
        { "subimage", "mem" , "", "mem" , "", "argb" },
        { "subimage", "mem" , "", "mem" , "", "abgr" },

        { "subimage", "mem" , "", "fio" , "", "rgb"  },
        { "subimage", "mem" , "", "fio" , "", "bgr"  },
        { "subimage", "mem" , "", "fio" , "", "rgba" },
        { "subimage", "mem" , "", "fio" , "", "bgra" },
        { "subimage", "mem" , "", "fio" , "", "argb" },
        { "subimage", "mem" , "", "fio" , "", "abgr" },

        { "subimage", "mem" , "", "file", "", "rgb"  },
        { "subimage", "mem" , "", "file", "", "bgr"  },
        { "subimage", "mem" , "", "file", "", "rgba" },
        { "subimage", "mem" , "", "file", "", "bgra" },
        { "subimage", "mem" , "", "file", "", "argb" },
        { "subimage", "mem" , "", "file", "", "abgr" },

        { "subimage", "fio" , "", "mem" , "", "rgb"  },
        { "subimage", "fio" , "", "mem" , "", "bgr"  },
        { "subimage", "fio" , "", "mem" , "", "rgba" },
        { "subimage", "fio" , "", "mem" , "", "bgra" },
        { "subimage", "fio" , "", "mem" , "", "argb" },
        { "subimage", "fio" , "", "mem" , "", "abgr" },

        { "subimage", "fio" , "", "fio" , "", "rgb"  },
        { "subimage", "fio" , "", "fio" , "", "bgr"  },
        { "subimage", "fio" , "", "fio" , "", "rgba" },
        { "subimage", "fio" , "", "fio" , "", "bgra" },
        { "subimage", "fio" , "", "fio" , "", "argb" },
        { "subimage", "fio" , "", "fio" , "", "abgr" },

        { "subimage", "fio" , "", "file", "", "rgb"  },
        { "subimage", "fio" , "", "file", "", "bgr"  },
        { "subimage", "fio" , "", "file", "", "rgba" },
        { "subimage", "fio" , "", "file", "", "bgra" },
        { "subimage", "fio" , "", "file", "", "argb" },
        { "subimage", "fio" , "", "file", "", "abgr" },

        { "subimage", "file", "", "mem" , "", "rgb"  },
        { "subimage", "file", "", "mem" , "", "bgr"  },
        { "subimage", "file", "", "mem" , "", "rgba" },
        { "subimage", "file", "", "mem" , "", "bgra" },
        { "subimage", "file", "", "mem" , "", "argb" },
        { "subimage", "file", "", "mem" , "", "abgr" },

        { "subimage", "file", "", "fio" , "", "rgb"  },
        { "subimage", "file", "", "fio" , "", "bgr"  },
        { "subimage", "file", "", "fio" , "", "rgba" },
        { "subimage", "file", "", "fio" , "", "bgra" },
        { "subimage", "file", "", "fio" , "", "argb" },
        { "subimage", "file", "", "fio" , "", "abgr" },

        { "subimage", "file", "", "file", "", "rgb"  },
        { "subimage", "file", "", "file", "", "bgr"  },
        { "subimage", "file", "", "file", "", "rgba" },
        { "subimage", "file", "", "file", "", "bgra" },
        { "subimage", "file", "", "file", "", "argb" },
        { "subimage", "file", "", "file", "", "abgr" }
    };

    const x_int32_t xit_count = sizeof(xct_argv) / (ARGS_LIST_LEN * ARGS_TEXT_LEN);

    //======================================

    if (argc < 2)
    {
        printf("%s < input jpeg file path >\n", argv[0]);
        return -1;
    }

    x_cstring_t xszt_name = file_base_name(argv[1]);
    std::string xstr_path = std::string((x_cstring_t)argv[1], xszt_name) + std::string("subimage/");

    if (0 != X_mkdir(xstr_path.c_str(), 0755))
    {
        printf("X_mkdir([%s], 07) failed!", xstr_path.c_str());
        return -1;
    }

    for (x_int32_t xit_iter = 0; xit_iter < xit_count; ++xit_iter)
    {
        snprintf(xct_argv[xit_iter][2], ARGS_TEXT_LEN, "%s", argv[1]);
        snprintf(xct_argv[xit_iter][4], ARGS_TEXT_LEN, "%s[%02d]%s_%s_%s_%s",
                 xstr_path.c_str(),
                 xit_iter + 1,
                 xct_argv[xit_iter][1],
                 xct_argv[xit_iter][3],
                 xct_argv[xit_iter][5],
                 xszt_name);
    }

    //======================================

    for (x_int32_t xit_iter = 0; xit_iter < xit_count; ++xit_iter)
    {
        subimage obj;
        obj.main(6, xct_argv[xit_iter]);
    }

    //======================================

    return 0;
}

#endif // TEST_ONLY_ONE

////////////////////////////////////////////////////////////////////////////////
