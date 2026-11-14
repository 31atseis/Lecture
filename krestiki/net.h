#ifndef NET_H
#define NET_H

void net_show_me(int port);
int net_host(int port);
int net_join(const char *ip, int port);
int net_send_line(int sock, const char *line);
int net_recv_line(int sock, char *buf, int cap);
void net_close(int sock);
void net_stop(void);

#endif
