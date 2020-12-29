# 封装 libjpeg 库

&emsp;&emsp;直接使用 libjpeg 库提供的 API 进行图像的**编码或解码**，总有诸多的不便。这几天把以前使用的代码，好好整理了一翻，封装成一套简单易用的 C/C++ API 公布出来，便于自己以后使用的同时，也希望能帮到他人。

&emsp;&emsp;当前封装的 API 代码，只有 **XJPEG_wrapper.h** 和 **XJPEG_wrapper.c** 两个文件，其主要实现 RGB 格式的图像进行 编码 或 压缩。以后可能会增加 灰度、YCbCr、CMYK、YCCK 等格式的支持。下面介绍如何使用这套 API。

## 1. 主要数据类型与枚举常量

- JPEG 编码/解码 所操作的图像像素格式。
    >  ```
    >  /**
    >   * @enum  jctrl_color_space_t
    >   * @brief JPEG 编码/解码 所操作的图像像素格式。
    >   * @note
    >   * 枚举值的 比特位 功能，定义如下：
    >   * - [  0 ~  7 ] : 用于表示每个像素所占的比特数，如 24，32 等值；
    >   * - [  8 ~ 15 ] : 用于标识小分类的类型，如区别 RGB24 与 BGR24；
    >   * - [ 16 ~ 23 ] : 用于区分颜色种类，如 RGB，YUV 等。
    >   * - [ 24 ~ 31 ] : 暂未使用。
    >   */
    >  typedef enum jctrl_color_space_t
    >  {
    >      JCTRL_CS_UNKNOW = 0x00000000, ///< 未知格式
    >      JCTRL_CS_RGB    = 0x00010118, ///< RGB 24位 格式，顺序为 RGB
    >      JCTRL_CS_BGR    = 0x00010218, ///< RGB 24位 格式，顺序为 BGR
    >      JCTRL_CS_RGBA   = 0x00010120, ///< RGB 32位 格式，顺序为 RGBA，编码时忽略 ALPHA 通道值
    >      JCTRL_CS_BGRA   = 0x00010220, ///< RGB 32位 格式，顺序为 BGRA，编码时忽略 ALPHA 通道值
    >      JCTRL_CS_ARGB   = 0x00010320, ///< RGB 32位 格式，顺序为 ARGB，编码时忽略 ALPHA 通道值
    >      JCTRL_CS_ABGR   = 0x00010420, ///< RGB 32位 格式，顺序为 ABGR，编码时忽略 ALPHA 通道值
    >  } jctrl_color_space_t;
    >  ```

- JPEG 编码/解码 的操作模式。
    >  ```
    >  /**
    >   * @enum  jctrl_mode_t
    >   * @brief JPEG 编码/解码 的操作模式。
    >   */
    >  typedef enum jctrl_mode_t
    >  {
    >      JCTRL_MODE_UNKNOW,   ///< 未知模式
    >      JCTRL_MODE_MEM   ,   ///< 内存模式
    >      JCTRL_MODE_FIO   ,   ///< 文件流模式
    >      JCTRL_MODE_FILE  ,   ///< 文件模式
    >  } jctrl_mode_t;
    >  ```

- JPEG 图像基本信息结构体，以及重定义 libjpeg 内部支持的色彩空间枚举值。
    >  ```
    >  /**
    >   * @struct jpeg_info_t
    >   * @brief  JPEG 图像基本信息。
    >   */
    >  typedef struct jpeg_info_t
    >  {
    >      j_int_t jit_width;    ///< nominal image width (from SOF marker)
    >      j_int_t jit_height;   ///< nominal image height
    >      j_int_t jit_channels; ///< # of color components in JPEG image
    >      j_int_t jit_cstype;   ///< colorspace of JPEG image ( @see jpeg_color_space_t )
    >  } jpeg_info_t, * jinfo_ptr_t;
    >  
    >  /**
    >   * @enum  jpeg_color_space_t
    >   * @brief JPEG 图像所支持的像素格式。
    >   */
    >  typedef enum jpeg_color_space_t
    >  {
    >      JPEG_CS_UNKNOWN  ,  ///< error/unspecified
    >      JPEG_CS_GRAYSCALE,  ///< monochrome
    >      JPEG_CS_RGB      ,  ///< red/green/blue, standard RGB (sRGB)
    >      JPEG_CS_YCbCr    ,  ///< Y/Cb/Cr (also known as YUV), standard YCC
    >      JPEG_CS_CMYK     ,  ///< C/M/Y/K
    >      JPEG_CS_YCCK     ,  ///< Y/Cb/Cr/K
    >      JPEG_CS_BG_RGB   ,  ///< big gamut red/green/blue, bg-sRGB
    >      JPEG_CS_BG_YCC   ,  ///< big gamut Y/Cb/Cr, bg-sYCC
    >  } jpeg_color_space_t;
    >  ```

- 其他的，还有 编码器对象指针 **jenc_ctxptr_t**，解码器对象指针 **jdec_ctxptr_t**，以及重定义了一部分基本数据类型，如下：
    >  ```
    >  #define J_NULL          0
    >  #define J_FALSE         0
    >  #define J_TRUE          1
    >  
    >  typedef void            j_void_t;
    >  typedef int             j_int_t;
    >  typedef long            j_long_t;
    >  typedef unsigned int    j_uint_t;
    >  typedef unsigned long   j_ulong_t;
    >  typedef unsigned char   j_uchar_t;
    >  typedef unsigned int    j_bool_t;
    >  
    >  typedef size_t          j_size_t;
    >  typedef fpos_t          j_fpos_t;
    >  
    >  typedef void *          j_handle_t;
    >  typedef const char *    j_cstring_t;
    >  typedef unsigned char * j_mem_t;
    >  typedef FILE *          j_fio_t;
    >  typedef const char *    j_path_t;
    >  ```

## 2. 编码操作

&emsp;&emsp;这里说的 JPEG 编码，是指将原始的 RGB 像素数据，压缩成 JPEG 数据流。这过程，是以 RGB 数据作为数据 **输入源**，JPEG 数据流则是 **输出源**。

&emsp;&emsp;RGB数据输入源，当下支持有 6 种，即 **jctrl_color_space_t** 中提到的 JCTRL_CS_RGB、JCTRL_CS_BGR、JCTRL_CS_RGBA、JCTRL_CS_BGRA、JCTRL_CS_ARGB、JCTRL_CS_ABGR 。

&emsp;&emsp;而输出的 JPEG 数据，当下只支持 JPEG_CS_RGB。也就是说，不支持编码成其他的色彩空间（libjpeg内是支持的），在以后的代码中，会考虑增加色彩转换的功能，如 RGB 转 灰度、RGB 转 YCbCr 等。

&emsp;&emsp;JPEG 编码操作，必要的步骤如下：

1. 使用 jenc_alloc() 申请编码器对象。

    >  ```
    >  jenc_ctxptr_t jenc_cptr = jenc_alloc(J_NULL);
    >  ```

2. 使用 jenc_config_dst() 设置解码输出源，即编码后的 JPEG 数据存放位置。

    >  ```
    >  /**********************************************************/
    >  /**
    >   * @brief 配置 JPEG 编码操作的目标输出源信息。
    >   * 
    >   * @param [in ] jenc_cptr : JPEG 编码操作的上下文对象。
    >   * @param [in ] jit_mode  : JPEG 编码输出模式（参看 jctrl_mode_t ）。
    >   * 
    >   * @param [in ] jht_optr  : 指向输出源的操作信息。
    >   * - 内存模式，jht_optr 的类型为 j_mem_t ，其为输出 JPEG 数据的缓存地址，
    >   *   若 ((J_NULL == jht_optr) || (0 == jut_mlen)) 时，则取内部缓存地址；
    >   * - 文件流模式，jht_optr 的类型为 j_fio_t ，其为输出 JPEG 数据的文件流；
    >   * - 文件模式，jht_optr 的类型为 j_path_t ，其为输出 JPEG 数据的文件路径。
    >   * 
    >   * @param [in ] jut_mlen  : 只针对于 内存模式，表示缓存容量。
    >   * 
    >   * @return j_int_t : 错误码，请参看 jenc_errno_table_t 相关枚举值。
    >   */
    >  j_int_t jenc_config_dst(
    >                  jenc_ctxptr_t jenc_cptr,
    >                  j_int_t       jit_mode,
    >                  j_handle_t    jht_optr,
    >                  j_uint_t      jut_mlen);
    >  ```

3. 使用 jenc_rgb_to_dst() 执行 RGB 数据的编码操作。

    >  ```
    >  /**********************************************************/
    >  /**
    >   * @brief 将 RGB 图像数据编码成 JPEG 数据流，输出至所配置的输出源中。
    >   * @note 调用该接口前，必须使用 jenc_config_dst() 配置好输出源信息。
    >   * 
    >   * @param [in ] jenc_cptr  : JPEG 编码操作的上下文对象。
    >   * @param [in ] jmem_iptr  : RGB 图像数据 缓存。
    >   * @param [in ] jit_stride : RGB 图像数据 像素行 步进大小。
    >   * @param [in ] jit_width  : RGB 图像数据 宽度。
    >   * @param [in ] jit_height : RGB 图像数据 高度。
    >   * @param [in ] jit_ctrlcs : RGB 图像数据 像素格式（参看 jctrl_color_space_t）。
    >   * 
    >   * @return j_int_t :
    >   * - 操作失败时，返回值 < 0，表示 错误码，参看 jenc_errno_table_t 相关枚举值。
    >   * - 操作成功时：
    >   *   1. 使用 文件流模式 或 文件模式 时，返回值 == JENC_ERR_OK；
    >   *   2. 使用 内存模式 时，返回值 > 0，表示输出源中存储的 JPEG 数据的有效字节数；
    >   *   3. 使用 内存模式 时，且 返回值 == JENC_ERR_OK，表示输出源的缓存容量不足，
    >   *      而输出的 JPEG 编码数据存储在 上下文对象 jenc_cptr 的缓存中，后续可通过
    >   *      jenc_cached_data() 和 jenc_cached_size() 获取此次编码得到的 JPEG 数据。
    >   */
    >  j_int_t jenc_rgb_to_dst(
    >                  jenc_ctxptr_t jenc_cptr,
    >                  j_mem_t       jmem_iptr,
    >                  j_int_t       jit_stride,
    >                  j_int_t       jit_width,
    >                  j_int_t       jit_height,
    >                  j_int_t       jit_ctrlcs);
    >  ```

    JPEG 编码操作，还可通过事先调用 **jenc_set_quality()** 设置 JPEG 的压缩质量。

4. 得到 JPEG 压缩后的数据。

    &emsp;在 步骤 2 中，使用 **内存模式** 时，且设置接收 JPEG 编码后的数据流缓存大小 不足，此时，步骤 4 编码操作后的结果（JPEG数据）存放在**编码器内部的缓存**中，可通过如下方式得到数据：
    >  ```
    >  j_mem_t  jmem_optr = jenc_cached_data(jenc_cptr);
    >  j_uint_t jut_msize = jenc_cached_size(jenc_cptr);
    >  ```

    &emsp;而若 步骤 2 配置的输出源为 **文件流模式（FILE * 指针）** 或 **文件模式(文件存储路径名)** ，则不会出现输出数据缓存不足的情况。

5. 使用 jenc_release() 释放编码器对象。

    &emsp;可重复 2 -> 3 -> 4 步骤继续进行其他图像的编码操作，直至退出（或 不再需要编码数据）时，必须使用 jenc_release() 释放编码器对象。
    >  ```
    >  jenc_release(jenc_cptr);
    >  jenc_cptr = J_NULL;
    >  ```

&emsp;以上所提及的 5 个步骤，在测试程序 test.cpp 代码中，**enc_mode_mem()**、**enc_mode_fio()**、**enc_mode_file()** 三个函数的流程，全部体现出来。

## 3. 解码操作

&emsp;&emsp;解码操作，指的是将 JPEG 编码的数据流解码到 RGB 图像缓存中。而现在的 API 版本，只支持 RGB 色彩空间的 JPEG 数据解码操作，以后的版本中，会增加其他 色彩空间 的解码功能。

&emsp;&emsp;解码输出的 RGB 数据，同样支持 6 种色彩空间，请参看 **jctrl_color_space_t** 中的相关枚举值。

&emsp;&emsp;JPEG 的解码操作，则是以 JPEG 数据源作为 **输入源**，而 RGB 缓存成了数据 **输出源** ，必要的操作步骤如下：

1. 使用 jdec_alloc() 申请解码器对象。

    >  ```
    >  jdec_ctxptr_t jdec_cptr = jdec_alloc(J_NULL);
    >  ```

2. 使用 jdec_config_src() 配置待解码的 JPEG 图像源。

    >  ```
    >  /**********************************************************/
    >  /**
    >   * @brief 配置 JPEG 编码操作的数据输入源信息。
    >   * 
    >   * @param [in ] jenc_cptr : JPEG 编码操作的上下文对象。
    >   * @param [in ] jit_mode  : JPEG 编码输入模式（参看 jctrl_mode_t ）。
    >   * 
    >   * @param [in ] jht_optr  : 指向输入源的操作信息。
    >   * - 内存模式，jht_dst 的类型为 j_mem_t ，其为输入 JPEG 数据的缓存地址；
    >   * - 文件流模式，jht_dst 的类型为 j_fio_t ，其为输入 JPEG 数据的文件流；
    >   * - 文件模式，jht_dst 的类型为 j_path_t ，其为输入 JPEG 数据的文件路径。
    >   * 
    >   * @param [in ] jut_size  : 只针对于 内存模式，表示缓存中的有效字节数。
    >   * 
    >   * @return j_int_t : 错误码，请参看 jdec_errno_table_t 相关枚举值。
    >   */
    >  j_int_t jdec_config_src(
    >                  jdec_ctxptr_t jdec_cptr,
    >                  j_int_t       jit_mode,
    >                  j_handle_t    jht_iptr,
    >                  j_uint_t      jut_size);
    >  ```

3. 可先使用 jdec_src_info() 获取 JPEG 图像源的基本信息，得知图像的宽高，并判断是否为 RGB 色彩空间。

    >  ```
    >  /**********************************************************/
    >  /**
    >   * @brief 获取 JPEG 输入源中的图像基本信息。
    >   * @note 调用该接口前，必须使用 jdec_config_src() 配置好输入源。
    >   * 
    >   * @param [in ] jdec_cptr : JPEG 解码操作的上下文对象。
    >   * @param [out] jinfo_ptr : 操作成功返回的图像基本信息。
    >   * 
    >   * @return j_int_t : 错误码（参看 jdec_errno_table_t 相关枚举值）。
    >   */
    >  j_int_t jdec_src_info(
    >                  jdec_ctxptr_t jdec_cptr,
    >                  jinfo_ptr_t   jinfo_ptr);
    >  ```

4. 使用 jdec_src_to_rgb() 进行解码操作。

    >  ```
    >  /**********************************************************/
    >  /**
    >   * @brief 将 JPEG 输入源图像 解码成 RGB 图像像素数据。
    >   * @note 调用该接口前，必须使用 jdec_config_src() 配置好输入源。
    >   * 
    >   * @param [in ] jdec_cptr : JPEG 解码操作的上下文对象。
    >   * @param [out] jmem_optr : 输出的 RGB 图像像素数据 缓存。
    >   * 
    >   * @param [in ] jit_stride : 
    >   * 输出的 RGB 图像像素行 步进大小；若为 0 时，
    >   * 则取 ((3 or 4) * jit_width) 为值（可能会 按 4 字节对齐）。
    >   * 
    >   * @param [in ] jut_mlen : 
    >   * 输出 RGB 缓存容量，其值应 >= abs(jit_stride * jit_height) ，
    >   * 所指定覆盖的内存区域为 [jmem_optr, jmem_optr + jit_stride * jit_height]。
    >   * 
    >   * @param [out] jit_width  : 操作成功返回的图像宽度（像素为单位）。
    >   * @param [out] jit_height : 操作成功返回的图像高度（像素为单位）。
    >   * @param [in ] jit_ctrlcs : 要求输出的 RGB 像素格式（参看 jctrl_color_space_t ）。
    >   * 
    >   * @return j_int_t : 错误码（参看 jdec_errno_table_t 相关枚举值）。
    >   * @retval JDEC_ERR_STRIDE : 
    >   * 可通过返回的 jit_width 值，重新调整 jit_stride 等参数，再进行尝试。
    >   * @retval JDEC_ERR_CAPACITY :
    >   * 可通过返回的 jit_height 值，重新调整 jut_mlen 等参数，再进行尝试。
    >   */
    >  j_int_t jdec_src_to_rgb(
    >                  jdec_ctxptr_t jdec_cptr,
    >                  j_mem_t       jmem_optr,
    >                  j_int_t       jit_stride,
    >                  j_uint_t      jut_mlen,
    >                  j_int_t     * jit_width,
    >                  j_int_t     * jit_height,
    >                  j_int_t       jit_ctrlcs);
    >  ```

    有以下几点需要特别注意：
    - 要想事先得知输出的 RGB 图像数据需要多大的缓存空间，在 步骤 3 得到的图像基本信息上，通过计算 **4 x 宽 x 高** 的值，可得到 RGBA 格式这样的图像所需要的缓存大小。
    - 若要解码输出的 **RGB 32位** 的像素数据带上 ALPHA 通道值，只需在申请解码器对象后（步骤 1），调用 **jdec_set_vpad()** 设置解码时填充的 ALPHA 通道值即可。
    - 对于解码输出 **RGB 24位** 的像素数据，则有可能需要考虑图像每行所占的字节数是否要 **4字节对齐** 的问题。这可以通过调用 jdec_set_align() 接口配置操作方式。

5. 使用 jdec_release() 释放解码器对象。

    &emsp;可重复 2 -> 3 -> 4 步骤继续进行其他图像的解码操作，直至退出（或 不再需要解码数据）时，必须使用 jdec_release() 释放解码器对象。
    >  ```
    >  jdec_release(jdec_cptr);
    >  jdec_cptr = J_NULL;
    >  ```

&emsp;&emsp;以上所提及的 5 个步骤，在测试程序 test.cpp 代码中，**dec_mode_mem()**、**dec_mode_fio()**、**dec_mode_file()** 三个函数的流程，全部体现出来。

## 4. C++ 的类封装

&emsp;&emsp;所有的 API 都是按面向对象的 C 接口封装的代码，但在使用 C++ 方面，还是 **class** 对象来得直接，故在 XJPEG_wrapper.h 代码下方，封装了 **jenc_handle_t** 编码器类 和 **jdec_handle_t** 解码器类，所有成员函数调用均是内联的，效率上并不会降低。

## 5. 关于测试代码

&emsp;&emsp;测试程序的代码，是用 C++ 写的，在 **test.cpp** 文件中，其功能是对 JPEG 图片裁剪出 中部 子图片来。各个 **[ JPEG 编码/解码 的操作模式 ]** 和 **[ RGB色彩空间 ]** 的组合方式，都有被测试通过。


## 6. 源码下载
- github 下载地址: [https://github.com/Gaaagaa/jpeg_wrapper](https://github.com/Gaaagaa/jpeg_wrapper)
- gitee 下载地址: [https://gitee.com/Gaaagaa/jpeg_wrapper](https://gitee.com/Gaaagaa/jpeg_wrapper)
- 另外，libgjpeg 的官方地址在这：http://www.ijg.org/ ，我所使用的版本是 **jpegsr9d** 。

&emsp;&emsp;以后，我仍会继续补充其他的功能，如支持其他色彩空间的编码/解码操作。现在，暂且先只支持 RGB 图像格式吧！！！
