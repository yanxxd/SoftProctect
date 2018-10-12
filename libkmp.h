/*
 * kmp.h
 *
 *  Created on: May 8, 2018
 *      Author: yan
 */

#ifndef KMP_H_143ef87b_3aec_47f6_a33a_ed51e3e965ac
#define KMP_H_143ef87b_3aec_47f6_a33a_ed51e3e965ac

#include <time.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <stdbool.h>

#ifdef DBG_PRINT
#define dbg_fprintf fprintf
#define dbg_printf printf
#else
#define dbg_fprintf
#define dbg_printf
#endif

extern SSL				*g_ssl;
extern bool            g_is_server;		//服务器端为true 客户端为false
extern unsigned int   g_checksum;      //没有作为端口用了，两端用来同步的一个参数，kmp1中用这个作为新的端口，kmp2以后只是做比较用
extern char            g_cha_input[256];	//x作为种子, update x when client recv data.
extern struct timespec g_offset;			//客户端和服务器的时间差 慢多少
extern struct timespec g_delay;			//线路延迟
extern time_t    		 g_timeslice; 		//200ms一个时间片,变为ns表示

#ifdef __cplusplus
extern "C" {
#endif
void timeval_us_add(struct timeval *start, struct timeval *end, struct timeval *result);
void timeval_us_sub(struct timeval *start, struct timeval *end, struct timeval *result);
void timespec_add(struct timespec *t1, struct timespec *t2, struct timespec *ret);
void timespec_dec(struct timespec *t1, struct timespec *t2, struct timespec *ret);
unsigned int get_rand_by_time(int);//(int delay_timeslice_num=0);
unsigned int easy_hash(unsigned char *buf, unsigned int len);
int recv_seed_delta(SSL *ssl, int *is_check, int *delta);
#ifdef __cplusplus
}
#endif

#endif /* KMP_H_ */
