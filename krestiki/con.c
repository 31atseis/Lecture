#define _CRT_SECURE_NO_WARNINGS
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "con.h"
#undef printf

void con_setup(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
}

int con_printf(const char *fmt, ...)
{
    char buf[4096];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0)
        return -1;
    if (n >= (int)sizeof buf)
        n = (int)sizeof buf - 1;
    if (n == 0)
        return 0;

#ifdef _WIN32
    {
        HANDLE h;
        DWORD mode, written;
        wchar_t wide[4096];
        wchar_t out[8192];
        int wn, i, j, cp;

        h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
            cp = CP_UTF8;
            wn = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, buf, n, wide, 4096);
            if (wn <= 0) {
                cp = 1251;
                wn = MultiByteToWideChar(cp, 0, buf, n, wide, 4096);
            }
            if (wn > 0) {
                j = 0;
                for (i = 0; i < wn && j < 8190; i++) {
                    if (wide[i] == L'\n')
                        out[j++] = L'\r';
                    out[j++] = wide[i];
                }
                WriteConsoleW(h, out, j, &written, NULL);
                return n;
            }
        }
    }
#endif
    fwrite(buf, 1, (size_t)n, stdout);
    fflush(stdout);
    return n;
}
