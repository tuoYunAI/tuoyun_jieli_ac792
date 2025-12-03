#include <stdarg.h>

typedef unsigned long size_t ;
extern "C" {
    int fseek(void *file, int offset, int orig);
    int ftell(void *file);
    void *malloc(size_t);
    void free(void *);
    int printf(char const *, ...);

}
__attribute__((__weak__, __visibility__("default")))
void *
operator new (size_t size)
{
    if (size == 0) {
        size = 1;
    }
    void *p = malloc(size);
    if (p == (void *)0) {
        // puts("malloc failed");
        while (1);
    }
    return p;
}

__attribute__((__weak__, __visibility__("default")))
void *
operator new[](size_t size)
{
    return ::operator new (size);
}

__attribute__((__weak__, __visibility__("default")))
void
operator delete (void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

__attribute__((__weak__, __visibility__("default")))
void
operator delete[](void *ptr)
{
    ::operator delete (ptr);
}

//fix me
extern "C" {
    int fseeko(void *stream, int offset, int fromwhere)
    {
        return fseek(stream, offset, fromwhere);
    }

    int ftello(void *stream)
    {
        return ftell(stream);
    }

    int strerror_r(int errnum, char *buf, unsigned int n)
    {
        printf("[%s, %d]is not supported yet\n", __FUNCTION__, __LINE__);
        return -1;
    }

    int fputs(const char *s, void *stream)// should be FILE *stream
    {
        printf("Need to make sure \"fputs\" runs OK!");
        // puts(s);
        return 0;
    }

    void *fdopen(int fd, const char *mode)
    {
        printf("[%s, %d]is not supported yet\n", __FUNCTION__, __LINE__);
        return (void *)0;
    }
}
