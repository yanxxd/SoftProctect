#ifndef PTP_H_29e8a09e_7a69_45fb_8a44_5934a9b8f295
#define PTP_H_29e8a09e_7a69_45fb_8a44_5934a9b8f295

#include <sys/time.h>

int ptp_client(char *if_name, char *ip_server, timespec &offset, timespec &delay);

int ptp_server_bind(char *if_name, int &sock);

int ptp_server(int sock, timespec &offset, timespec &delay);

#endif
