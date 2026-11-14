#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include <stdio.h>
#include "field.h"
#include "con.h"

void clear_field(char f[][MAXS], int n, int m)
{
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++)
            f[i][j] = ' ';
    }
}

static int put_s(char *dst, int cap, int pos, const char *s)
{
    int i;

    for (i = 0; s[i] != '\0' && pos < cap - 1; i++)
        dst[pos++] = s[i];
    dst[pos] = '\0';
    return pos;
}

int field_line(char f[][MAXS], int n, int m, int k, char *dst, int cap)
{
    int j, row, pos;
    char tmp[8];

    if (cap < 8 || k < 0 || k > n + 2)
        return 0;

    pos = 0;
    dst[0] = '\0';

    if (k == 0) {
        pos = put_s(dst, cap, pos, "   ");
        for (j = 0; j < m; j++) {
            sprintf(tmp, "%2d ", j + 1);
            pos = put_s(dst, cap, pos, tmp);
        }
        return 1;
    }

    if (k == 1 || k == n + 2) {
        pos = put_s(dst, cap, pos, "   ");
        for (j = 0; j < m; j++)
            pos = put_s(dst, cap, pos, "+--");
        put_s(dst, cap, pos, "+");
        return 1;
    }

    row = k - 2;
    sprintf(tmp, "%2d ", row + 1);
    pos = put_s(dst, cap, pos, tmp);
    for (j = 0; j < m; j++) {
        sprintf(tmp, "|%c ", f[row][j]);
        pos = put_s(dst, cap, pos, tmp);
    }
    put_s(dst, cap, pos, "|");
    return 1;
}

void print_field(char f[][MAXS], int n, int m)
{
    char line[512];
    int k;

    for (k = 0; field_line(f, n, m, k, line, 512); k++)
        printf("%s\n", line);
}

void put_mark(char f[][MAXS], int row, int col, char sym)
{
    f[row][col] = sym;
}

int is_full(char f[][MAXS], int n, int m)
{
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            if (f[i][j] == ' ')
                return 0;
        }
    }
    return 1;
}

static int same_line(char f[][MAXS], int n, int m, char sym,
                     int r, int c, int dr, int dc, int len)
{
    int k;

    for (k = 0; k < len; k++) {
        if (r < 0 || c < 0 || r >= n || c >= m)
            return 0;
        if (f[r][c] != sym)
            return 0;
        r = r + dr;
        c = c + dc;
    }
    return 1;
}

int check_win(char f[][MAXS], int n, int m, char sym)
{
    int i;

    if (m > 1) {
        for (i = 0; i < n; i++) {
            if (same_line(f, n, m, sym, i, 0, 0, 1, m))
                return 1;
        }
    }

    if (n > 1) {
        for (i = 0; i < m; i++) {
            if (same_line(f, n, m, sym, 0, i, 1, 0, n))
                return 1;
        }
    }

    if (n > 1) {
        for (i = 0; i + n - 1 < m; i++) {
            if (same_line(f, n, m, sym, 0, i, 1, 1, n))
                return 1;
        }
        for (i = n - 1; i < m; i++) {
            if (same_line(f, n, m, sym, 0, i, 1, -1, n))
                return 1;
        }
    }

    if (m > 1) {
        for (i = 0; i + m - 1 < n; i++) {
            if (same_line(f, n, m, sym, i, 0, 1, 1, m))
                return 1;
            if (same_line(f, n, m, sym, i, m - 1, 1, -1, m))
                return 1;
        }
    }

    return 0;
}
