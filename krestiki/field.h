#ifndef FIELD_H
#define FIELD_H

#define MAXS 40

void clear_field(char f[][MAXS], int n, int m);
int field_line(char f[][MAXS], int n, int m, int k, char *dst, int cap);
void print_field(char f[][MAXS], int n, int m);
void put_mark(char f[][MAXS], int row, int col, char sym);
int is_full(char f[][MAXS], int n, int m);
int check_win(char f[][MAXS], int n, int m, char sym);

#endif
