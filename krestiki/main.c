#define _CRT_SECURE_NO_WARNINGS
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "field.h"
#include "player.h"
#include "net.h"
#include "con.h"

static int ask_num(const char *text, int *out)
{
    printf("%s", text);
    if (scanf("%d", out) != 1) {
        if (feof(stdin))
            exit(0);
        flush_input();
        return 0;
    }
    flush_input();
    return 1;
}

static int ask_mode(void)
{
    int a;

    while (1) {
        printf("1 - вдвоем за этим компьютером\n");
        printf("2 - по сети\n");
        if (!ask_num("Выбор: ", &a))
            continue;
        if (a == 1 || a == 2)
            return a;
    }
}

static int ask_port(void)
{
    int p;

    while (1) {
        if (!ask_num("Порт (0 - это 5555): ", &p))
            continue;
        if (p == 0)
            return 5555;
        if (p > 0 && p <= 65535)
            return p;
        printf("Порт от 1 до 65535\n");
    }
}

static int tell(int sock, const char *s)
{
    if (net_send_line(sock, s) < 0) {
        printf("Связь оборвалась\n");
        return -1;
    }
    return 0;
}

static int show(char f[][MAXS], int n, int m, int sock)
{
    char line[512];
    char msg[520];
    int k;

    for (k = 0; field_line(f, n, m, k, line, 512); k++) {
        printf("%s\n", line);
        if (sock >= 0) {
            snprintf(msg, sizeof msg, "T %s", line);
            if (tell(sock, msg) < 0)
                return -1;
        }
    }
    return 0;
}

static int parse_move(char f[][MAXS], int n, int m, const char *text,
                      int *row, int *col, const char **err)
{
    int r, c;

    if (sscanf(text, "%d %d", &r, &c) != 2) {
        *err = "Нужны два числа";
        return 0;
    }
    if (r < 1 || c < 1 || r > n || c > m) {
        *err = "Мимо поля";
        return 0;
    }
    if (f[r - 1][c - 1] != ' ') {
        *err = "Тут уже стоит";
        return 0;
    }
    *row = r - 1;
    *col = c - 1;
    return 1;
}

static void play_local(void)
{
    char field[MAXS][MAXS];
    char s1, s2, cur;
    int n, m, row, col;

    ask_size(&n, &m);
    ask_symbol(&s1, 0, 1);
    ask_symbol(&s2, s1, 2);
    clear_field(field, n, m);

    cur = s1;
    while (1) {
        print_field(field, n, m);
        ask_move(field, n, m, cur, &row, &col);
        put_mark(field, row, col, cur);
        if (check_win(field, n, m, cur)) {
            print_field(field, n, m);
            printf("Победа: %c\n", cur);
            break;
        }
        if (is_full(field, n, m)) {
            print_field(field, n, m);
            printf("Ничья\n");
            break;
        }
        if (cur == s1)
            cur = s2;
        else
            cur = s1;
    }
}

static int take_guest(int sock, char mine, char *guest)
{
    char line[128];
    char msg[128];
    char c;

    if (tell(sock, "NEEDSYM") < 0)
        return -1;
    while (1) {
        if (net_recv_line(sock, line, (int)sizeof line) < 0) {
            printf("Связь оборвалась\n");
            return -1;
        }
        if (sscanf(line, "S %c", &c) != 1) {
            if (tell(sock, "ERR Напиши символ") < 0)
                return -1;
            if (tell(sock, "NEEDSYM") < 0)
                return -1;
            continue;
        }
        if (c == mine) {
            if (tell(sock, "ERR Этот символ уже занят") < 0)
                return -1;
            if (tell(sock, "NEEDSYM") < 0)
                return -1;
            continue;
        }
        *guest = c;
        sprintf(msg, "T Ты %c, соперник %c. Первым ходит он", c, mine);
        if (tell(sock, msg) < 0)
            return -1;
        return 0;
    }
}

static int play_host(int sock, int n, int m, char s1)
{
    char field[MAXS][MAXS];
    char line[128];
    char msg[160];
    char s2, cur;
    int row, col;
    const char *err;

    if (take_guest(sock, s1, &s2) < 0)
        return -1;
    printf("Соперник выбрал %c. Ты ходишь первым\n", s2);
    clear_field(field, n, m);
    cur = s1;

    while (1) {
        if (show(field, n, m, sock) < 0)
            return -1;
        if (cur == s1) {
            if (tell(sock, "T Ход соперника") < 0)
                return -1;
            ask_move(field, n, m, cur, &row, &col);
        } else {
            printf("Жду ход соперника\n");
            while (1) {
                if (tell(sock, "ASK") < 0)
                    return -1;
                if (net_recv_line(sock, line, (int)sizeof line) < 0) {
                    printf("Связь оборвалась\n");
                    return -1;
                }
                if (parse_move(field, n, m, line, &row, &col, &err))
                    break;
                sprintf(msg, "ERR %s", err);
                if (tell(sock, msg) < 0)
                    return -1;
            }
        }
        put_mark(field, row, col, cur);
        if (check_win(field, n, m, cur)) {
            if (show(field, n, m, sock) < 0)
                return -1;
            printf("Победа: %c\n", cur);
            sprintf(msg, "WIN %c", cur);
            tell(sock, msg);
            break;
        }
        if (is_full(field, n, m)) {
            if (show(field, n, m, sock) < 0)
                return -1;
            printf("Ничья\n");
            tell(sock, "DRAW");
            break;
        }
        if (cur == s1)
            cur = s2;
        else
            cur = s1;
    }
    return 0;
}

static void cut_line(char *s)
{
    int i;

    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\n' || s[i] == '\r') {
            s[i] = '\0';
            return;
        }
    }
}

static void play_client(int sock)
{
    char line[512];
    char move[64];
    char sym;

    printf("Подключился. Если тишина, хост ещё выбирает поле\n");
    while (1) {
        if (net_recv_line(sock, line, (int)sizeof line) != 0) {
            printf("Связь оборвалась\n");
            return;
        }
        if (strcmp(line, "NEEDSYM") == 0) {
            ask_symbol(&sym, 0, 2);
            sprintf(move, "S %c", sym);
            if (net_send_line(sock, move) < 0)
                break;
        } else if (strcmp(line, "ASK") == 0) {
            printf("Твой ход. Строка и столбец: ");
            if (!fgets(move, (int)sizeof move, stdin))
                break;
            cut_line(move);
            if (net_send_line(sock, move) < 0)
                break;
        } else if (strncmp(line, "ERR ", 4) == 0) {
            printf("%s\n", line + 4);
        } else if (strncmp(line, "T ", 2) == 0) {
            printf("%s\n", line + 2);
        } else if (strncmp(line, "WIN ", 4) == 0) {
            printf("Победа: %c\n", line[4]);
            break;
        } else if (strcmp(line, "DRAW") == 0) {
            printf("Ничья\n");
            break;
        }
    }
}

static void play_net(void)
{
    char ip[64];
    char s1;
    int kind, port, n, m, sock;

    while (1) {
        printf("1 - ждать друга\n");
        printf("2 - подключиться к другу\n");
        if (!ask_num("Выбор: ", &kind))
            continue;
        if (kind == 1 || kind == 2)
            break;
    }

    if (kind == 1) {
        port = ask_port();
        net_show_me(port);
        ask_size(&n, &m);
        ask_symbol(&s1, 0, 1);
        sock = net_host(port);
        if (sock < 0) {
            printf("Не открылся порт\n");
            net_stop();
            return;
        }
        play_host(sock, n, m, s1);
        net_close(sock);
        net_stop();
        return;
    }

    printf("IP друга (или localhost): ");
    if (scanf("%63s", ip) != 1) {
        if (feof(stdin))
            exit(0);
        flush_input();
        printf("Не понял IP\n");
        return;
    }
    flush_input();
    port = ask_port();
    sock = net_join(ip, port);
    if (sock < 0) {
        printf("Не подключился. Хост уже запущен? IP и порт те же?\n");
        net_stop();
        return;
    }
    play_client(sock);
    net_close(sock);
    net_stop();
}

int main(void)
{
    con_setup();
    if (ask_mode() == 1)
        play_local();
    else
        play_net();
    return 0;
}
