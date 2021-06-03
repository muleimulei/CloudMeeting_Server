
#include "unp.h"


void * Calloc(size_t n, size_t size)
{
    void *ptr;
    if( (ptr = calloc(n, size)) == NULL)
    {
        errno = ENOMEM;
        err_quit("Calloc error");
    }
    return ptr;
}
