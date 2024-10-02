/**
 * @file jclip.cpp
 * Copyright (c) 2024 Gaaagaa. All rights reserved.
 * 
 * @author  : Gaaagaa
 * @date    : 2024-10-02
 * @version : 1.0.0.0
 * @brief   : JPEG 图片剪切器 程序。
 */

#include "jencoder.h"
#include "jdecoder.h"

#include <stdlib.h>
#include <string.h>

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

j_cstring_t JSZ_iput = J_NULL;
jctl_cs_t   JCS_iput = JCTL_CS_UNKNOWN;
j_char_t    JSZ_oput[256] = { 0 };
jpeg_cs_t   JCS_oput = JPEG_CS_UNKNOWN;

jrect_t     JRC_area = { 0, 0, 0, 0 };
j_uint_t    JUT_qual = 75;

/**********************************************************/
/**
 * @brief 解析命令行的输入参数，初始化工作参数。
 * 
 * @param [in ] jit_argc : 输入的命令行参数数量。
 * @param [in ] jsz_argv : 输入的命令行参数字符串数组。
 * 
 * @return j_int_t : 成功，返回 0；失败，返回 -1 。
 */
j_int_t jclip_getopt(j_int_t jit_argc, j_char_t * jsz_argv[]);

/**********************************************************/
/**
 * @brief 执行区域剪切工作。
 */
j_int_t jclip_proc(void);

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 输出程序帮助信息。
 */
void usage(const char * xsz_name)
{
    printf(
        "usage: %s -i input [-ics colorspace] [-o output] "
        "[-x xpos] [-y ypos] [-w width] [-h height] [-ocs colorspace] [-q quality]\n"
        "       imgW = the input image width;\n"
        "       imgH = the input image height;\n"
        "       -i   : input jpeg image file;\n"
        "       -ics : the input colorspace, which defaults to the same value as the input image.\n"
        "       -o   : output jpeg image file, the default is basename(input)_clip_[colorspace].jpg;\n"
        "       -x   : the x position of the clip area, the default is 0;\n"
        "              if the x is negative, x = imgW - (abs(x) %% imgW);\n"
        "       -y   : the y position of the clip area, the default is 0;\n"
        "              if the y is negative, y = imgH - (abs(y) %% imgH);\n"
        "       -w   : the width of the clip area, the default is imgW;\n"
        "              if the width is negative, width = max(0, imgW - (abs(width) %% imgW) - x);\n"
        "       -h   : the height of the clip area, the default is imgH;\n"
        "              if the height is negative, height = max(0, imgH - (abs(height) %% imgH) - y);\n"
        "       -ocs : the output colorspace, default value is -ics.\n"
        "       -q   : the output image quality[ 1 - 100 ], default value is 75.\n\n",
        xsz_name);

    printf("jpeg colorspace : GRAY, RGB, YCC, CMYK, YCCK, BGRGB, BGYCC.\n\n");

    printf(
        "decode jpeg, provides color space conversion:\n"
        "       GRAY  => GRAY\n"
        "       RGB   => GRAY\n"
        "       YCC   => GRAY\n"
        "       BGYCC => GRAY\n"
        "       RGB   => RGB\n"
        "       GRAY  => RGB\n"
        "       YCC   => RGB\n"
        "       BGYCC => RGB\n"
        "       YCC   => YCC\n"
        "       BGRGB => BGRGB\n"
        "       BGYCC => BGYCC\n"
        "       CMYK  => CMYK\n"
        "       YCCK  => CMYK\n"
        "       YCCK  => YCCK\n\n");

    printf(
        "encode jpeg, provides color space conversion:\n"
        "       GRAY  => GRAY\n"
        "       RGB   => GRAY\n"
        "       YCC   => GRAY\n"
        "       BGYCC => GRAY\n"
        "       RGB   => RGB\n"
        "       BGRGB => BGRGB\n"
        "       YCC   => YCC\n"
        "       RGB   => YCC\n"
        "       BGYCC => BGYCC\n"
        "       RGB   => BGYCC\n"
        "       YCC   => BGYCC\n"
        "       CMYK  => CMYK\n"
        "       YCCK  => YCCK\n"
        "       CMYK  => YCCK\n\n");
}

/**********************************************************/
/**
 * @brief main function.
 */
int main(int argc, char * argv[])
{
    j_int_t jit_err;

    //======================================

    if (argc < 3)
    {
        usage(argv[0]);
        return -1;
    }

    //======================================

    if (0 != jclip_getopt(argc, argv))
    {
        return -1;
    }

    jit_err = jclip_proc();

    //======================================

    return jit_err;
}

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 字符串忽略大小写的比对操作。
 *
 * @param [in ] xsz_lcmp : 比较操作的左值字符串。
 * @param [in ] xsz_rcmp : 比较操作的右值字符串。
 *
 * @return int
 *         - xsz_lcmp <  xsz_rcmp，返回 <= -1；
 *         - xsz_lcmp == xsz_rcmp，返回 ==  0；
 *         - xsz_lcmp >  xsz_rcmp，返回 >=  1；
 */
static int X_stricmp(const char * xsz_lcmp, const char * xsz_rcmp)
{
    register int xdi_lvalue = 0;
    register int xdi_rvalue = 0;

    if (xsz_lcmp == xsz_rcmp)
        return 0;
    if (NULL == xsz_lcmp)
        return -1;
    if (NULL == xsz_rcmp)
        return 1;

    do
    {
        if (((xdi_lvalue = (*(xsz_lcmp++))) >= 'A') && (xdi_lvalue <= 'Z'))
            xdi_lvalue -= ('A' - 'a');

        if (((xdi_rvalue = (*(xsz_rcmp++))) >= 'A') && (xdi_rvalue <= 'Z'))
            xdi_rvalue -= ('A' - 'a');

    } while (xdi_lvalue && (xdi_lvalue == xdi_rvalue));

    return (xdi_lvalue - xdi_rvalue);
}

/**********************************************************/
/**
 * @brief 
 * 从文件全路径名中，获取其文件名。
 * 例如：D:\Folder\FileName.txt 返回 FileName.txt。
 * 
 * @param [in ] xsz_path : 文件路径 字符串。
 * 
 * @return const char * : 返回对应的 文件名。
 */
static const char * X_basename(const char * xsz_path)
{
	register const char * xsz_iter = xsz_path;
	register const char * xsz_ipos = xsz_path;

	if (NULL == xsz_ipos)
	{
		return NULL;
	}

	while (*xsz_iter)
	{
		if (('\\' == *xsz_iter) || ('/' == *xsz_iter))
			xsz_ipos = xsz_iter + 1;
		xsz_iter += 1;
	}

	return xsz_ipos;
}

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 更新 输出图片文件路径 的字符串内容。
 */
j_void_t jclip_ofile_path(j_cstring_t jsz_path)
{
    j_char_t * jsz_iter = J_NULL;

    //======================================

    if (J_NULL != jsz_path)
    {
        strcpy(JSZ_oput, jsz_path);
        return;
    }

    //======================================
    
    strcpy(JSZ_oput, X_basename(JSZ_iput));

    jsz_iter = (j_char_t *)strrchr(JSZ_oput, '.');
    if (J_NULL != jsz_iter)
        *jsz_iter = '\0';

    strcat(JSZ_oput, "_clip");

    jpeg_cs_t jcs_type = JPEG_CS_UNKNOWN;

    switch (JCS_oput)
    {
    case JPEG_CS_GRAY   : strcat(JSZ_oput, "_gray.jpg" ); break;
    case JPEG_CS_RGB    : strcat(JSZ_oput, "_rgb.jpg"  ); break;
    case JPEG_CS_YCC    : strcat(JSZ_oput, "_ycc.jpg"  ); break;
    case JPEG_CS_CMYK   : strcat(JSZ_oput, "_cmyk.jpg" ); break;
    case JPEG_CS_YCCK   : strcat(JSZ_oput, "_ycck.jpg" ); break;
    case JPEG_CS_BG_RGB : strcat(JSZ_oput, "_bgrgb.jpg"); break;
    case JPEG_CS_BG_YCC : strcat(JSZ_oput, "_bgycc.jpg"); break;
    default             : strcat(JSZ_oput, ".jpg"      ); break;
    }

    //======================================
}

/**********************************************************/
/**
 * @brief 由 色彩空间 的字符串名称，转换为 jpeg_cs_t 类型值。
 */
jpeg_cs_t jclip_to_cs(j_cstring_t jsz_type)
{
    jpeg_cs_t jcs_type = JPEG_CS_UNKNOWN;

    if (0 == X_stricmp("gray", jsz_type))
        jcs_type = JPEG_CS_GRAY;
    else if (0 == X_stricmp("rgb", jsz_type))
        jcs_type = JPEG_CS_RGB;
    else if (0 == X_stricmp("ycc", jsz_type))
        jcs_type = JPEG_CS_YCC;
    else if (0 == X_stricmp("cmyk", jsz_type))
        jcs_type = JPEG_CS_CMYK;
    else if (0 == X_stricmp("ycck", jsz_type))
        jcs_type = JPEG_CS_YCCK;
    else if (0 == X_stricmp("bgrgb", jsz_type))
        jcs_type = JPEG_CS_BG_RGB;
    else if (0 == X_stricmp("bgycc", jsz_type))
        jcs_type = JPEG_CS_BG_YCC;

    return jcs_type;
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
j_int_t jclip_getopt(j_int_t jit_argc, j_char_t * jsz_argv[])
{
    j_int_t jit_iter;

    //======================================
    // 解析参数

    for (jit_iter = 1; jit_iter < jit_argc; )
    {
        if ((0 == X_stricmp("-i", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            JSZ_iput = jsz_argv[++jit_iter];
            continue;
        }

        if ((0 == X_stricmp("-ics", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            JCS_iput = jcs_mapto_ctl(jclip_to_cs(jsz_argv[++jit_iter]));
            continue;
        }

        if ((0 == X_stricmp("-o", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            jclip_ofile_path(jsz_argv[++jit_iter]);
            continue;
        }

        if ((0 == X_stricmp("-x", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            JRC_area.jit_x = (j_int_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        if ((0 == X_stricmp("-y", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            JRC_area.jit_y = (j_int_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        if ((0 == X_stricmp("-w", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            JRC_area.jit_w = (j_int_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        if ((0 == X_stricmp("-h", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            JRC_area.jit_h = (j_int_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        if ((0 == X_stricmp("-ocs", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            JCS_oput = jclip_to_cs(jsz_argv[++jit_iter]);
            continue;
        }

        if ((0 == X_stricmp("-q", jsz_argv[jit_iter])) &&
            ((jit_iter + 1) < jit_argc))
        {
            JUT_qual = (j_uint_t)strtol(jsz_argv[++jit_iter], J_NULL, 0);
            continue;
        }

        ++jit_iter;
    }

    //======================================
    // 验证工作参数的有效性

    if (J_NULL == JSZ_iput)
    {
        usage(jsz_argv[0]);
        printf("Error: no input file was specified!\n");
        return -1;
    }

    //======================================

    return 0;
}

/**********************************************************/
/**
 * @brief 图片解码操作。
 * 
 * @param [in ] jsz_file  : 图片文件路径。
 * @param [out] jmt_pxls  : 操作返回解码得到的像素缓存。
 * @param [out] jinfo_ctx : 操作返回的图片基本信息。
 * @param [out] jenc_ccs  : 返回目标剪切图像使用的编码 色彩空间 转换操作值。
 * 
 * @return j_int_t : 解码操作返回的 错误码（参看 jdec_errno_t）。
 */
j_int_t jclip_decode(
            j_cstring_t   jsz_file,
            j_mptr_t    & jmt_pxls,
            jpeg_info_t & jinfo_ctx,
            jenc_ccs_t  & jenc_ccs)
{
    j_int_t    jit_err = JDEC_ERR_UNKNOWN;
    jdecoder_t jdecoder;

    do
    {
        //======================================
        // 配置图像输入源

        jit_err = jdecoder.config(JCTL_MODE_FSZPATH, (j_fhandle_t)jsz_file, 0);
        if (JDEC_ERR_OK != jit_err)
        {
            printf(
                "jdecoder.config(, [%s],) return error: %s\n",
                JSZ_iput,
                jdec_errno_name(jit_err));
            break;
        }

        //======================================
        // 获取图像基本信息

        jit_err = jdecoder.info(&jinfo_ctx);
        if (JDEC_ERR_OK != jit_err)
        {
            printf(
                "jdecoder.config(, [%s],) return error: %s\n",
                JSZ_iput,
                jdec_errno_name(jit_err));
            break;
        }

        printf(
            "image[%s] info: [w: %d, h: %d, nc: %d, cs: %s]\n",
            JSZ_iput,
            jinfo_ctx.jit_imgw,
            jinfo_ctx.jit_imgh,
            jinfo_ctx.jit_nchs,
            jpeg_cs_name(jinfo_ctx.jcs_type));

        //======================================
        // 为后续编码操作，确认像素的 色彩空间 转换是否有效

        if (JCTL_CS_UNKNOWN == JCS_iput)
        {
            JCS_iput = jcs_mapto_ctl(jinfo_ctx.jcs_type);
        }

        if (JPEG_CS_UNKNOWN == JCS_oput)
        {
            JCS_oput = JCTL_CS_TYPE(JCS_iput);
        }

        jenc_ccs = (jenc_ccs_t)JENC_CCS_MAKE(JCS_iput, JCS_oput);
        if (!jenc_ccs_valid(jenc_ccs))
        {
            printf("clip to colorspace [0x%08X] is invalid!\n", jenc_ccs);
            jit_err = JENC_ERR_CCS_VALUE;
            break;
        }

        //======================================
        // 分配解码输出像素缓存

        jmt_pxls = (j_mptr_t)malloc(
            JENC_CCS_NUMC(jenc_ccs) * jinfo_ctx.jit_imgw * jinfo_ctx.jit_imgh);
        if (J_NULL == jmt_pxls)
        {
            printf("malloc() return J_NULL!\n");
            jit_err = JDEC_ERR_MALLOC;
            break;
        }

        //======================================
        // 解码操作

        jit_err = jdecoder.decode_image(
                                JCS_iput,
                                jmt_pxls,
                                JENC_CCS_NUMC(jenc_ccs) * jinfo_ctx.jit_imgw);
        if (jit_err < 0)
        {
            printf(
                "jdecoder.decode_image() return error: %s\n",
                jdec_errno_name(jit_err));
            break;
        }

        if (0 == jit_err)
        {
            printf("input image [%s] is empty!\n", JSZ_iput);
            jit_err = JDEC_ERR_OUT_EMPTY;
        }

        jit_err = JDEC_ERR_OK;

        //======================================
    } while (0);

    if (JDEC_ERR_OK != jit_err)
    {
        if (J_NULL != jmt_pxls)
        {
            free(jmt_pxls);
            jmt_pxls = J_NULL;
        }
    }

    return jit_err;
}

/**********************************************************/
/**
 * @brief 对图片裁剪区域进行编码输出操作。
 * 
 * @param [in ] jsz_file  : 输出图片的文件路径。
 * @param [in ] jmt_pxls  : 待编码的图像像素缓存。
 * @param [in ] jinfo_ctx : 图象基本信息。
 * @param [in ] jrc_area  : 编码输出的图片裁剪区域。
 * @param [in ] jenc_ccs  : 编码 色彩空间 转换操作值。
 * 
 * @return j_int_t : 解码操作返回的 错误码（参看 jdec_errno_t）。
 */
j_int_t jclip_encode(
            j_cstring_t         jsz_file,
            j_mptr_t            jmt_pxls,
            const jpeg_info_t & jinfo_ctx,
            const jrect_t     & jrc_area,
            jenc_ccs_t          jenc_ccs)
{
    j_int_t    jit_err = JENC_ERR_UNKNOWN;
    jencoder_t jencoder;

    j_int_t    jit_step;
    j_int_t    jit_offs;

    //======================================

    jit_err = jencoder.config(JCTL_MODE_FSZPATH, (j_fhandle_t)jsz_file, 0, JUT_qual);
    if (JENC_ERR_OK != jit_err)
    {
        printf(
            "jencoder.config(, [%s], ) return error: %s\n",
            jsz_file,
            jenc_errno_name(jit_err));
        return jit_err;
    }

    jit_step = JENC_CCS_NUMC(jenc_ccs) * jinfo_ctx.jit_imgw;
    jit_offs = jit_step * jrc_area.jit_y + JENC_CCS_NUMC(jenc_ccs) * jrc_area.jit_x;

    jit_err = jencoder.encode_image(
                    jenc_ccs,
                    jmt_pxls + jit_offs,
                    jit_step,
                    jrc_area.jit_w,
                    jrc_area.jit_h);
    if (JENC_ERR_OK != jit_err)
    {
        printf(
            "jencoder.encode_image([%s], ...) return error: %s\n",
            jenc_ccs_name(jenc_ccs),
            jenc_errno_name(jit_err));
        return jit_err;
    }

    printf("output image file: %s\n", jsz_file);

    //======================================

    return jit_err;
}

/**********************************************************/
/**
 * @brief 确认 剪裁区域 的矩形参数。
 */
j_bool_t jclip_area(j_int_t jit_imgw, j_int_t jit_imgh)
{
    //======================================

    if ((0 == JRC_area.jit_x) && (0 == JRC_area.jit_y) &&
        (0 == JRC_area.jit_w) && (0 == JRC_area.jit_h))
    {
        JRC_area.jit_x = 0;
        JRC_area.jit_y = 0;
        JRC_area.jit_w = jit_imgw;
        JRC_area.jit_h = jit_imgh;
    }
    else
    {
#define JRC_MOD(jval, jmod) ((jmod) - ((-(jval)) % jmod))
        if (JRC_area.jit_x < 0)
        {
            JRC_area.jit_x = JRC_MOD(JRC_area.jit_x, jit_imgw);
        }
        if (JRC_area.jit_y < 0)
        {
            JRC_area.jit_y = JRC_MOD(JRC_area.jit_y, jit_imgh);
        }
        if (JRC_area.jit_w < 0)
        {
            JRC_area.jit_w = JRC_MOD(JRC_area.jit_w, jit_imgw) - JRC_area.jit_x;
            if (JRC_area.jit_w < 0)
                JRC_area.jit_w = 0;
        }
        if (JRC_area.jit_h < 0)
        {
            JRC_area.jit_h = JRC_MOD(JRC_area.jit_h, jit_imgh) - JRC_area.jit_y;
            if (JRC_area.jit_h < 0)
                JRC_area.jit_h = 0;
        }
#undef JRC_MOD

        if ((JRC_area.jit_x + JRC_area.jit_w) > jit_imgw)
            JRC_area.jit_w = jit_imgw - JRC_area.jit_x;
        if ((JRC_area.jit_y + JRC_area.jit_h) > jit_imgh)
            JRC_area.jit_h = jit_imgh - JRC_area.jit_y;
    }

    if ((JRC_area.jit_w <= 0) || (JRC_area.jit_h <= 0))
    {
        printf(
            "Error: the clip area[%d, %d, %d, %d] is empty!\n",
            JRC_area.jit_x,
            JRC_area.jit_y,
            JRC_area.jit_w,
            JRC_area.jit_h);
        return J_FALSE;
    }

    printf(
        "the clip area is [%d, %d, %d, %d]\n",
        JRC_area.jit_x,
        JRC_area.jit_y,
        JRC_area.jit_w,
        JRC_area.jit_h);

    //======================================

    return J_TRUE;
}

/**********************************************************/
/**
 * @brief 执行区域剪切工作。
 */
j_int_t jclip_proc(void)
{
    j_int_t     jit_err = 0;

    j_mptr_t    jmt_pxls = J_NULL;
    jpeg_info_t jinfo_ctx;
    jenc_ccs_t  jenc_ccs = JENC_CCS_UNKNOWN;

    //======================================
    // 解码输入图像

    jit_err = jclip_decode(JSZ_iput, jmt_pxls, jinfo_ctx, jenc_ccs);
    if (JDEC_ERR_OK != jit_err)
    {
        goto __EXIT_FUNC;
    }

    //======================================
    // 裁剪区域 和 编码输出图片的文件路径

    if (!jclip_area(jinfo_ctx.jit_imgw, jinfo_ctx.jit_imgh))
    {
        jit_err = -1;
        goto __EXIT_FUNC;
    }

    if ('\0' == JSZ_oput[0])
    {
        // 使用 J_NULL 进行更新，让 JSZ_oput 生成默认值
        jclip_ofile_path(J_NULL);
    }

    //======================================
    // 执行编码输出

    jit_err = jclip_encode(JSZ_oput, jmt_pxls, jinfo_ctx, JRC_area, jenc_ccs);
    if (JDEC_ERR_OK != jit_err)
    {
        goto __EXIT_FUNC;
    }

    //======================================

__EXIT_FUNC:
    if (J_NULL != jmt_pxls)
    {
        free(jmt_pxls);
        jmt_pxls = J_NULL;
    }

    //======================================

    return jit_err;
}

////////////////////////////////////////////////////////////////////////////////
