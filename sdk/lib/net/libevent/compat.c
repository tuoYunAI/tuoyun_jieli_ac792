#include <stdarg.h> // 引入 va_list 相关宏

    
// 声明一个函数，名字可以叫做 fprintf，但实际上是打印到 stdout
int fprintf(const char *format, ...) 
{
    va_list args; // 声明 va_list 变量
    va_start(args, format); // 初始化 va_list，format 是最后一个固定参数
    int result = vprintf(format, args); // 使用 vprintf 执行格式化输出到 stdout
    va_end(args); // 清理 va_list
    return result; // 返回 printf 的返回值
}

int getpid(void)
{
    return get_cur_thread_pid();
}



