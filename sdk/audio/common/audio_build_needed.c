#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_build_needed.data.bss")
#pragma data_seg(".audio_build_needed.data")
#pragma const_seg(".audio_build_needed.text.const")
#pragma code_seg(".audio_build_needed.text")
#endif
/*
 ****************************************************************
 *							Audio Build Needed
 * File  : audio_build_needed.c
 * By    :
 * Notes : 定义一些编译依赖的函数（一般是第三方算法会用到）
 ****************************************************************
 */

__attribute__((weak))void __assert_func()
{
    return;
}

__attribute__((weak))void exit(int status)
{
    return;
}
