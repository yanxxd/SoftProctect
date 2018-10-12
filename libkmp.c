#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h> //for gethostbyname
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>//pid_t fork(void);
#include <signal.h>
#include <assert.h>
#include <sys/time.h>

#include "libkmp.h"
//#include "md5.h"

#define MAX_BUF 4096

SSL				*g_ssl = NULL;
bool            g_is_server = false;		//服务器端为true 客户端为false
unsigned int   g_checksum = 8443;       //没有作为端口用了，两端用来同步的一个参数，kmp1中用这个作为新的端口，kmp2以后只是做比较用
char            g_cha_input[256];			//x作为种子, update x when client recv data.
struct timespec g_offset;					//客户端和服务器的时间差 慢多少
struct timespec g_delay;					//线路延迟
time_t    		 g_timeslice = 200 * 1000 * 1000; //200ms一个时间片,变为ns表示

//char (*prand_str)(char *, const int);
//int (*pgetIndexOf)(string, string);

void timeval_us_add(struct timeval *start, struct timeval *end, struct timeval *result){
	result->tv_sec = end->tv_sec + start->tv_sec;
	result->tv_usec = end->tv_usec + start->tv_usec;
	if(result->tv_usec >= 1000000){
		result->tv_sec += 1;
		result->tv_usec -= 1000000;
	}
}
void timeval_us_sub(struct timeval *start, struct timeval *end, struct timeval *result){
	if(start->tv_usec <= end->tv_usec){
		result->tv_sec = end->tv_sec - start->tv_sec;
		result->tv_usec = end->tv_usec - start->tv_usec;
	} else {
		result->tv_sec = end->tv_sec - start->tv_sec - 1;
		result->tv_usec = 1000000 + end->tv_usec - start->tv_usec;
	}
}

void timespec_add(struct timespec *t1, struct timespec *t2, struct timespec *ret) {
	ret->tv_sec = t1->tv_sec + t2->tv_sec;
	ret->tv_nsec = t1->tv_nsec + t2->tv_nsec;
	if (ret->tv_nsec >= 1000000000) {
		ret->tv_nsec -= 1000000000;
		++ret->tv_sec;
	}
}

//tv_sec表示正负,结果中tv_nsec部分永远为正  {-1,500000000} 对应的数为 -1 + 0.5 = -0.5
void timespec_dec(struct timespec *t1, struct timespec *t2, struct timespec *ret) {
	ret->tv_sec = t1->tv_sec - t2->tv_sec;
	ret->tv_nsec = t1->tv_nsec - t2->tv_nsec;
	if (ret->tv_nsec < 0) {
		ret->tv_nsec += 1000000000;
		--ret->tv_sec;
	}
}

unsigned int get_rand_by_time(int delay_timeslice_num){
	struct timespec now ={0, 0};
	struct timespec slice = {0, g_timeslice};
	clock_gettime(CLOCK_REALTIME, &now);
	//dbg_fprintf(stdout, "\ncurrent time: %ld.%09ld", (long) now.tv_sec, (long) now.tv_nsec);
	if(!g_is_server){ //服务器发送x后立即计算，客户端收到x后才开始计算
		timespec_add(&now, &g_offset, &now);
		timespec_dec(&now, &g_delay, &now);
	}
	now.tv_nsec -= now.tv_nsec % g_timeslice; //按时间片对齐
	for(int i=0; i< delay_timeslice_num; ++i){
		timespec_add(&now, &slice, &now);
	}
	//dbg_fprintf(stdout, "\naligned time: %ld.%09ld\n", (long) now.tv_sec, (long) now.tv_nsec);
	unsigned int hash1 = easy_hash((unsigned char*)&get_rand_by_time, 256);
	unsigned int hash2 = easy_hash((unsigned char*)&now, sizeof(struct timespec));
	return hash1 ^ hash2;
}

unsigned int easy_hash(unsigned char *buf, unsigned int len)
{
	unsigned int hash = 0;
	unsigned short *p = (unsigned short*)buf;
	for(unsigned int i=0; i<len/2; ++i){
		//hash = (hash << 5 | hash >> 27); //循环左移5位
		hash += p[i%len];
	}
	if(len & 1)
		hash += buf[len-1];
	return hash;
}

int recv_seed_delta(SSL *ssl, int *is_check, int *delta)
{
	char recvBuf[MAX_BUF];
	int len;
	if ( (len = SSL_read(ssl, recvBuf, MAX_BUF)) <= 8) {
		fprintf(stderr, "Server maybe shutdown!\n");
		return -1;
	}
	recvBuf[len] = 0;
	*is_check = *(int*)recvBuf;
	*delta = *(int*)(recvBuf+4);
	strcpy(g_cha_input, recvBuf+8);
	printf("\nrecv seed:%s", g_cha_input);
	return 0;
}

#ifndef KMP_SO
int main(int argc,char *argv[]) {

	return 0;
}
#endif
