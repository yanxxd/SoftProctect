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
#include <string>
#include <assert.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "libkmp.h"
#include "md5.h"
#include "ptp.h"

using namespace std;

#define MAX_BUF 4096

char *g_ip = NULL;

int recv_data(SSL *ssl);
char kmp_rand_str(char str[], const int len, SSL *ssl, bool protect=true);
int  kmp_getIndexOf(std::string s, std::string m, SSL *ssl, bool protect=true);
void compute_time(struct timeval *start, struct timeval *end, struct timeval *result);
SSL_CTX* InitCTX(void);

int main(int argc,char *argv[]) {
	int sockfd;          //socket
	char sendBuf[MAX_BUF];
	//char recvBuf[MAX_BUF];
	int sendSize, recvSize;          //用于记录记录发送和接收到数据的大小
	int pid;

	if(3 != argc){
		printf("\nUsage:\tkmp interface ip\n\n");
		return 0;
	}

	g_ip = argv[2];
	bool is_loop = true;

	FILE *flog = fopen("time.log", "w");
	SSL_CTX *ctx = InitCTX();
    SSL *ssl = SSL_new(ctx);      /* create new SSL connection state */

	while (is_loop) {
		//每次都要重连新端口
		sockfd = connect_serv(g_ip, g_server_port);
		if (sockfd <= 0) {
			fprintf(stderr, "connect server port %d error\n", g_server_port);
			break;
		}
		//连接成功后先做时间同步
		if( ptp_client(argv[1], g_ip, g_offset, g_delay) ){
			printf("\n[!] Time Sync failed!\n\n");
			break;
		}
		SSL_set_fd(ssl, sockfd);	  /* attach the socket descriptor */
		if (1 != SSL_connect(ssl)) /* perform the connection */
		{
			ERR_print_errors_fp(stderr);
			break;
		}
		int len;
		while (fgets(sendBuf, 32, stdin)) { //一个指令,每个指令代表一个具体的功能
			len = strlen(sendBuf);
			sendBuf[len - 1] = 0; //去掉尾部的'\n'
			if (SSL_write(ssl, sendBuf, 32) == -1) {
				fprintf(stderr, "fail to send datas.");
				break;
			}
			if (strncmp(sendBuf, "quit", 4) == 0) {
				is_loop = false;
				break;
			} else	if (strncmp(sendBuf, "@", 1) == 0) {
at_begin:
				printf("\n********************************************************************\n");
				//把时间重置为0
				memset(&g_time_consume_org, 0, sizeof(g_time_consume_org));		//原函数花费时间
				memset(&g_time_consume, 0, sizeof(g_time_consume));				//加保护后花费时间

				//1
				int n = 48, m = 6;
				char *str = new char[n];
				memset(str, 0, n);
				if(-1 == kmp_rand_str(str, n, ssl)){
					fprintf(stderr, "fail to call rand_str().");
					delete str;
					break;
				}
				//rand_str(str, n);
				printf("\n1 g_server_port=%u\n", g_server_port);
				printf("====str=%s====\n", str);
				printf("original  program time_use is %ds %dus\n", (int)g_time_consume_org.tv_sec, (int)g_time_consume_org.tv_usec);
				printf("protected program time_use is %ds %dus\n", (int)g_time_consume.tv_sec, (int)g_time_consume.tv_usec);
				fprintf(flog, "rand_str %d %d %d\n", (int)g_time_consume_org.tv_sec * 1000000 + (int)g_time_consume_org.tv_usec,
						(int)g_time_consume_sock.tv_sec * 1000000 + (int)g_time_consume_sock.tv_usec,
						(int)g_time_consume.tv_sec * 1000000 + (int)g_time_consume.tv_usec);

				//2
				char *mstr = new char[m];
				memset(mstr, 0, m);
				if(-1 == kmp_rand_str(mstr, m, ssl)){
					fprintf(stderr, "fail to call rand_str().\n");
					delete str;
					delete mstr;
					break;
				}
				printf("\n2 g_server_port=%u\n", g_server_port);
				printf("====mstr=%s====\n", mstr);
				printf("original  program time_use is %ds %dus\n", (int)g_time_consume_org.tv_sec, (int)g_time_consume_org.tv_usec);
				printf("protected program time_use is %ds %dus\n", (int)g_time_consume.tv_sec, (int)g_time_consume.tv_usec);
				fprintf(flog, "rand_str %d %d %d\n", (int)g_time_consume_org.tv_sec * 1000000 + (int)g_time_consume_org.tv_usec,
						(int)g_time_consume_sock.tv_sec * 1000000 + (int)g_time_consume_sock.tv_usec,
						(int)g_time_consume.tv_sec * 1000000 + (int)g_time_consume.tv_usec);

				//3
				int mi = kmp_getIndexOf(str, mstr, ssl);
				if(-1 == mi){
					fprintf(stderr, "getIndexOf(): can't find \n");
				}
				printf("\n3 g_server_port=%u\n", g_server_port);
				printf("The index of %s in %s is %d\n", mstr, str, mi);
				printf("original  program time_use is %ds %dus\n", (int)g_time_consume_org.tv_sec, (int)g_time_consume_org.tv_usec);
				printf("protected program time_use is %ds %dus\n", (int)g_time_consume.tv_sec, (int)g_time_consume.tv_usec);
				fprintf(flog, "getIndexOf %d %d %d\n", (int)g_time_consume_org.tv_sec * 1000000 + (int)g_time_consume_org.tv_usec,
						(int)g_time_consume_sock.tv_sec * 1000000 + (int)g_time_consume_sock.tv_usec,
						(int)g_time_consume.tv_sec * 1000000 + (int)g_time_consume.tv_usec);
				delete str;
				delete mstr;

				fflush(flog);

				sleep(1);
				sendBuf[0] = '@';
				sendBuf[1] = '\0';
				if (SSL_write(ssl, sendBuf, 32) == -1) {
					fprintf(stderr, "fail to send datas.\n");
					break;
				}
				goto at_begin;

				/*gettimeofday(&end, NULL);
				printf("end.tv_sec:%d\n", (int) end.tv_sec);
				printf("end.tv_usec:%d\n", (int) end.tv_usec);
				time_use = (end.tv_sec - start.tv_sec) * 1000000
						+ (end.tv_usec - start.tv_usec);          //微秒
				printf("time_use is %f\n", time_use);*/

				//printf("original  program time_use is %ds %dus\n", (int)g_time_consume_org.tv_sec, (int)g_time_consume_org.tv_usec);
				//printf("protected program time_use is %ds %dus\n", (int)g_time_consume.tv_sec, (int)g_time_consume.tv_usec);
			} else { //非"@"或"quit",接收服务器端发回的
				if(recv_data(ssl)) {
					fprintf(stderr, "fail to recv data.\n");
					break;
				}
			}
			memset(sendBuf, 0, sizeof(sendBuf));
		} // end of while (fgets(sendBuf, MAX_BUF, stdin))
		close(sockfd); //需要重新连接
	} //end of 大循环

	fclose(flog);
	close(sockfd);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}

int recv_data(SSL *ssl)
{
	char recvBuf[MAX_BUF];
	if (SSL_read(ssl, recvBuf, MAX_BUF) <= 0) {
		fprintf(stderr, "Server maybe shutdown!\n");
		return -1;
	}
	printf("Server:%s\n", recvBuf);
	if (strncmp(recvBuf, "OK", 2) == 0) {
		strtok(recvBuf, " ");
		strcpy(g_cha_input, strtok(NULL, " "));
		printf("x:%s\n", g_cha_input);
	}
	return 0;
}

char kmp_rand_str(char str[], const int len, SSL *ssl, bool protect){
	char sendBuf[MAX_BUF];
	//接收参数x
	if (recv_data (ssl)) {
		fprintf(stderr, "rand_str():fail to recv x.\n");
		return -1;
	}

	//参数发给服务端
	*(unsigned int*)sendBuf = g_server_port;
	int sendSize = 4 + sprintf(sendBuf+4, "rand_str %d", len);
	if (SSL_write(ssl, sendBuf, sendSize+4) <= 0) {
		fprintf(stderr, "rand_str():fail to send param.");
		return -1;
	}

	struct timeval start, end;
	char r;

	//调用函数,让这两个函数所在的代码块加入CPU缓存，不加这个，下面两次调用，会出现后面的rand_str比前面的rand_str_org快
	gettimeofday(&start, NULL);
	r = rand_str_org(str, len);
	gettimeofday(&end, NULL);
	//计算原函数花费时间
	compute_time(&start, &end, &g_time_consume_org);//*/

	//调用加保护后的函数
	gettimeofday(&start, NULL);
	r = rand_str(str, len);
	gettimeofday(&end, NULL);
	//计算加保护后花费时间
	compute_time(&start, &end, &g_time_consume);

	//调用原函数
	gettimeofday(&start, NULL);
	r = rand_str_org(str, len);
	gettimeofday(&end, NULL);
	//计算原函数花费时间
	compute_time(&start, &end, &g_time_consume_org);

	/*/
	if (protect) {
		//调用加保护后的函数
		r = rand_str(str, len);
	} else {
		//调用原函数
		r = rand_str_org(str, len);
	}//*/

	return r;
}

int kmp_getIndexOf(std::string s, std::string m, SSL *ssl, bool protect) {
	char sendBuf[MAX_BUF];
	//1. 接收参数x
	if (recv_data (ssl)) {
		fprintf(stderr, "getIndexOf():fail to recv x.\n");
		return -1;
	}

	//2. 参数发给服务端
	*(unsigned int*)sendBuf = g_server_port;
	int sendSize = 4 + sprintf(sendBuf + 4, "getIndexOf %s %s", s.c_str(), m.c_str());
	if (SSL_write(ssl, sendBuf, sendSize) <= 0) {
		fprintf(stderr, "getIndexOf():fail to send param.");
		return -1;
	}

	struct timeval start, end;
	int mi;

	//调用函数,让这两个函数所在的代码块加入CPU缓存
	gettimeofday(&start, NULL);
	mi = getIndexOf_org(s, m);
	gettimeofday(&end, NULL);
	compute_time(&start, &end, &g_time_consume_org);

	//3. 调用加保护后函数
	gettimeofday(&start, NULL);
	mi = getIndexOf(s, m);
	gettimeofday(&end, NULL);
	//计算加保护后花费时间
	compute_time(&start, &end, &g_time_consume);

	//4. 调用原函数
	gettimeofday(&start, NULL);
	mi = getIndexOf_org(s, m);
	gettimeofday(&end, NULL);
	//计算原函数花费时间
	compute_time(&start, &end, &g_time_consume_org);

	/*/
	if (protect) {
		//4. 调用加保护后函数
		mi = getIndexOf(s, m);
	} else {
		//3. 调用原函数
		mi = getIndexOf_org(s, m);
	}//*/

	return mi;
}

void compute_time(struct timeval *start, struct timeval *end, struct timeval *result){
	if(start->tv_usec < end->tv_usec){
		result->tv_sec = end->tv_sec - start->tv_sec;
		result->tv_usec = end->tv_usec - start->tv_usec;
	} else {
		result->tv_sec = end->tv_sec - start->tv_sec - 1;
		result->tv_usec = 1000000 + end->tv_usec - start->tv_usec;
	}
}


SSL_CTX* InitCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
	SSL_library_init();
    SSL_load_error_strings();   /* Bring in and register error messages */
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    method = TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
