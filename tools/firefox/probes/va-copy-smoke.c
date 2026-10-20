#include <stdarg.h>
#include <stdio.h>

static int check(const char* format, ...)
{
    va_list original, copy;
    va_start(original, format);
    va_copy(copy, original);
    int result = 0;
    for (int i = 1; i <= 9; i++)
        if (va_arg(original, int) != i)
            result = 1;
    for (int i = 1; i <= 9; i++)
        if (va_arg(copy, int) != i)
            result = 1;
    va_end(copy);
    va_end(original);
    return result;
}

int main(void)
{
    if (check("test", 1, 2, 3, 4, 5, 6, 7, 8, 9))
        return 1;
    puts("VA_COPY_C99_PASS");
    return 0;
}
