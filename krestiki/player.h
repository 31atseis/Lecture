#ifndef PLAYER_H
#define PLAYER_H

#include "field.h"

void flush_input(void);
void ask_size(int *n, int *m);
void ask_symbol(char *sym, char busy, int who);
void ask_move(char f[][MAXS], int n, int m, char sym, int *row, int *col);

#endif
