#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include "con.h"

void flush_input(void)
{
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF)
        ;
}

void ask_size(int *n, int *m)
{
    int a, b;

    while (1) {
        printf("Введите N и M (0 0 - поле 3x3): ");
        if (scanf("%d %d", &a, &b) != 2) {
            if (feof(stdin))
                exit(0);
            flush_input();
            printf("Нужны два числа\n");
            continue;
        }
        flush_input();
        if (a == 0 && b == 0) {
            *n = 3;
            *m = 3;
            return;
        }
        if (a < 1 || b < 1 || a > MAXS || b > MAXS) {
            printf("Можно от 1 до %d\n", MAXS);
            continue;
        }
        *n = a;
        *m = b;
        return;
    }
}

void ask_symbol(char *sym, char busy, int who)
{
    char c;

    while (1) {
        printf("Игрок %d, символ: ", who);
        if (scanf(" %c", &c) != 1) {
            if (feof(stdin))
                exit(0);
            flush_input();
            continue;
        }
        flush_input();
        if (busy && c == busy) {
            printf("Этот символ уже занят\n");
            continue;
        }
        *sym = c;
        return;
    }
}

void ask_move(char f[][MAXS], int n, int m, char sym, int *row, int *col)
{
    int r, c;

    while (1) {
        printf("Ход %c. Строка и столбец: ", sym);
        if (scanf("%d %d", &r, &c) != 2) {
            if (feof(stdin))
                exit(0);
            flush_input();
            printf("Нужны два числа\n");
            continue;
        }
        flush_input();
        if (r < 1 || c < 1 || r > n || c > m) {
            printf("Мимо поля\n");
            continue;
        }
        if (f[r - 1][c - 1] != ' ') {
            printf("Тут уже стоит\n");
            continue;
        }
        *row = r - 1;
        *col = c - 1;
        return;
    }
}
