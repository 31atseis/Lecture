#ifndef CON_H
#define CON_H

void con_setup(void);
int con_printf(const char *fmt, ...);

#define printf con_printf

#endif
